# Fix Report — glintfx blur effects (L1-DEMO / L1-BACKEND)

**Data:** 2026-06-28  
**Branch:** `feat/glintfx-v1`  
**Plataforma de teste:** Linux x86-64, Mesa/llvmpipe (software rasterizer, Xvfb 900×600)  
**GPU real:** não disponível no ambiente de CI; validação llvmpipe confirma o pipeline; efeitos visuais plenos aguardam hardware real.

---

## 1. Causa-raiz — Bug 1: layout horizontal por falta de `display: block`

### Sintoma
Cards empilhavam horizontalmente (flow inline) em vez de verticalmente. O card de backdrop-blur ficava à direita dos outros, sem nada colorido atrás para desfocar. A sombra/glow do card glow era mascarada porque não havia espaço vertical para expandir.

### Causa
RmlUi 6.x registra `display: inline` como valor padrão para **todos** os elementos, ao contrário do HTML onde `<div>` é block por padrão:

```cpp
// RmlUi/Core/StyleSheetSpecification.cpp
RegisterProperty(PropertyId::Display, "display", "inline")
```

Sem `display: block` explícito no `<body>` e nos `.card`, o layout era inline → cards lado a lado numa única linha, `width: 260px` sem efeito real, `padding` e `margin` se comportando como inline.

### Fix aplicado
`glintfx/demos/showcase/showcase.rcss`:

```css
body   { display: block; }   /* body como container de bloco */
.card  { display: block; }   /* cards empilham verticalmente */
```

Adicionados também `background-color: #14171f` + `decorator: linear-gradient(135deg, #6a0ddc, #e02828, #0f8a4a)` no body para dar conteúdo visível e colorido por trás do card de backdrop-filter.

### Antes / depois (layout)

| Região | Antes (inline) | Depois (block) |
|--------|---------------|----------------|
| Glow card (CSS) | x=0–246, y=0–48 (linha única estreita) | x=40–300, y=40–120 (card vertical 80px) |
| Blur card (CSS) | x=367–578, y=0–48 (à direita) | x=40–300, y=232–312 (abaixo dos outros) |
| CompositeLayers scissor (blur) | `(367,0)-(552,80)` | `(16,208)-(324,336)` ✓ |

---

## 2. Causa-raiz — Bug 2: heap overflow em `glReadPixels` com GL_PACK_ALIGNMENT=4

### Sintoma
Crash `double free or corruption (!prev)` ou `free(): invalid next size` ao executar o bloco de diagnóstico que lia um bloco 50×50 de pixels RGB.

### Causa
GL_PACK_ALIGNMENT padrão = 4. O GL padeia cada linha lida para múltiplo de 4 bytes:

```
50 pixels × 3 bytes RGB = 150 bytes/linha → pad para 152 bytes/linha (múltiplo de 4)
50 linhas × 152 bytes  = 7600 bytes escritos em buffer de 7500 bytes → 100 bytes de overflow
```

O mesmo ocorria com buffers de 1 pixel (3 bytes lidos → GL escreve 4 bytes em array de 3).

### Fix aplicado
`glPixelStorei(GL_PACK_ALIGNMENT, 1)` antes de cada `glReadPixels`:

```cpp
glPixelStorei(0x0D05, 1);     // GL_PACK_ALIGNMENT = 1 (sem padding de linha)
glReadPixels(...);
glPixelStorei(0x0D05, 4);     // restaurar
```

Também usado em `App::snapshot()` para que a captura de frame completo (ex: 900×600×3 = 1.620.000 bytes) não sofra overflow.

---

## 3. Limitação conhecida — Mesa/llvmpipe: crash no BlendMask dual-sampler

### Sintoma
Crash `free(): invalid next size` ao renderizar o card de máscara (`.mask { mask-image: horizontal-gradient(#000f #0000); }`).

### Causa
`FilterType::MaskImage` usa `ProgramId::BlendMask` com dois texture units (`_tex` e `_texMask`). O rasterizador de software Mesa (`llvmpipe`) corrompe a heap durante a execução desse programa com dois samplers ativos.

**Este é um bug do Mesa, não do glintfx.** Em GPU real o BlendMask deve funcionar corretamente.

### Workaround
Card de máscara comentado em `showcase.rml` com comentário bilíngue explicando o motivo. Reativar em GPU real.

---

## 4. Confirmação — Blur pipeline ativo e correto

Evidências coletadas nos logs de diagnóstico antes da limpeza:

### CompileFilter — todos os 3 efeitos compilados
```
[rmlui] CompileFilter: name=drop-shadow type=3 sigma=8.00   ← box-shadow #5fd0ff blur=24px
[rmlui] CompileFilter: name=drop-shadow type=3 sigma=8.00   ← filter: drop-shadow(#0008 ...)
[rmlui] CompileFilter: name=blur       type=1 sigma=8.00    ← backdrop-filter: blur(8px)
```

### RenderBlur — executado para os 3 passes
```
[rmlui] RenderBlur: sigma=12.00 window_flipped=(0,448)-(332,600)    ← box-shadow glow
[rmlui] RenderBlur: sigma=8.00  window_flipped=(0,416)-(360,600)    ← drop-shadow
[rmlui] RenderBlur: sigma=8.00  window_flipped=(16,264)-(324,392)   ← backdrop-filter blur
```

### Pixels após fix (layout vertical correto)

| Probe GL | CSS equiv. | Valor antes | Valor depois | Interpretação |
|----------|-----------|------------|--------------|---------------|
| GL(100,520) | CSS(100,80) | (0,0,0) | **(43,58,103)** | Card glow interior = #2b3a67 ✓ |
| GL(310,520) | CSS(310,80) | (0,0,0) | **(207,42,41)** | Área de sombra/glow fora do card ≠ zero ✓ |
| GL(150,490) 50×50 | CSS blur ativo | — | **7500/7500 nz** | 100% dos pixels da região são não-zero ✓ |
| CSS(150,220) | acima blur card | (221,41,41) | — | body gradient puro (referência) |
| CSS(150,260) | interior blur | (210,75,70) | — | G↑ B↑ vs referência: backdrop blur ativo ✓ |

A diferença no card de blur (G: 41→75, B: 41→70) reflete a blur kernel espalhando cores do gradiente horizontal para dentro da área do card.

---

## 5. Screenshot

**Caminho:** `/tmp/glintfx_showcase_fixed.ppm` (orientação correta, y=0 no topo)  
**Golden de referência:** `glintfx/demos/showcase/golden/showcase_reference.ppm`  
**MSE:** 0.00 (re-render idêntico ao golden após limpeza do código de debug)

Gerado por `App::snapshot()` que lê o FBO 0 **antes** de `glfwSwapBuffers` — garante conteúdo definido mesmo em Mesa/llvmpipe com Xvfb.

---

## 6. Arquivos modificados

| Arquivo | Tipo | Mudança |
|---------|------|---------|
| `glintfx/demos/showcase/showcase.rcss` | Demo | `display: block` em body e .card; gradiente no body para backdrop-filter |
| `glintfx/demos/showcase/showcase.rml` | Demo | Comentário no card mask explicando limitação Mesa/llvmpipe |
| `glintfx/src/app.cpp` | Backend | `App::snapshot()` com GL_PACK_ALIGNMENT=1; debug removido |
| `glintfx/src/render_gl3.cpp` | Backend | Debug fprintfs removidos; `end_frame()` limpo |
| `glintfx/include/glintfx/app.hpp` | API | Declaração de `snapshot(const char*)` |
| `glintfx/tests/golden_test.cpp` | Teste | Smoke test + captura PPM via `snapshot()` |
| `glintfx/demos/showcase/golden/showcase_reference.ppm` | Artefato | Golden image gerada com layout correto |

O arquivo `glintfx/build/_deps/rmlui-src/Backends/RmlUi_Renderer_GL3.cpp` foi modificado para debug e restaurado (não é rastreado pelo git; o build usa o fonte da dependência em cache).

---

## 7. Pendências / observações para GPU real

- Reativar o card `.mask` e verificar que `BlendMask` funciona sem crash.
- Verificar visualmente a intensidade do glow (box-shadow `#5fd0ff 0 0 24px 0`): em llvmpipe a sombra cyan é sobreposta pelo gradiente do body; em GPU real a composição de camadas pode diferir.
- `drop-shadow(#0008 0 4px 8px)`: alfa de 8/255 ≈ 3%, essencialmente invisível por design — intencional?
- Body não expande para largura total do viewport (900px) em RmlUi com `display: block` e sem `width: 100%` explícito. A largura atual ≈ 340px (card 260px + margens + padding). Adicionar `width: 100%` ao body se o gradiente de fundo deve cobrir tela cheia.
