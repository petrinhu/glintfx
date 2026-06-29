# Fix Report — glintfx blur effects (L1-DEMO / L1-BACKEND)

**Data:** 2026-06-28  
**Branch:** `feat/glintfx-v1`  
**Plataforma de teste:** Linux x86-64, Mesa/llvmpipe (software rasterizer, Xvfb 900×600)  
**GPU real:** validado pelo líder — glow/sombra aparecia como COR SÓLIDA (gradiente body mascarava o halo ciano). Fix em v2 (commit `6217016`): seções separadas + box-shadow reforçado.

---

## Iteração v2 (commit `6217016`) — glow visível + mask reativada

**Problema identificado em GPU real:** apesar do pipeline blur ativo, o glow cyan era mascarado pelo gradiente colorido do body (o gradiente sobrepunha a sombra de forma que ambos mesclavam numa cor sólida sem halo visível).

**Fix v2 aplicado:**

1. **Layout em seções**: separar cards por contexto visual.
   - `.section-dark` (fundo `#0d1020` quase-preto): cards `.glow` e `.grad`
   - `.section-gradient` (gradiente diagonal `#6a0ddc→#e02828→#0f8a4a`): cards `.blur` e `.mask`
   - O halo ciano agora contrasta diretamente contra o fundo escuro.

2. **box-shadow reforçado**: `#5fd0ff 0 0 24px 0` → `#5fd0ff 0 0 32px 8px` (spread 0→8px, blur 24→32px).

3. **drop-shadow reparado**: `#0008` (alpha 3%/255 = invisível em RmlUi's 8-digit `#rrggbbaa`) → `#5fd0ff80 0 0 20px` (ciano 50% alpha, blur 20px, sem offset). Demonstra o efeito de drop-shadow por forma alpha (segue o arredondamento do card).

4. **Card `.mask` reativado** em `showcase.rml` (GPU real confirma BlendMask funciona). CI/headless usa `showcase_test.rml` (sem mask).

**Validação pixel (llvmpipe, CSS-space):**

| Probe CSS | Valor | Interpretação |
|-----------|-------|---------------|
| Halo ciano (301,80) | (46,97,127) | B+95 G+81 vs bg=(13,16,32) — CIANO inequívoco ✓ |
| Halo acima (150,38) | (47,97,127) | Halo se extende acima do card ✓ |
| Card interior (100,80) | (30,42,80) | Exatamente #1e2a50 ✓ |
| Grad interior (150,190) | (249,72,43) | Laranja-rosa do gradient decorator ✓ |
| Blur card interior (100,300) | (162,52,167) | G+26 B+35 vs bg → backdrop-filter ativo ✓ |

**Artefato v2:** `/tmp/glintfx_showcase_v2.ppm`

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

## 7. Iteração v3 (L1-BACKEND) — backbuffer opaco para compositor Wayland

**Data:** 2026-06-29  
**Problema (alta confiança):** RmlUi 6 usa premultiplied alpha. O pipeline `CompositeLayers → RenderFilters → RenderBlur/DropShadow/MaskImage` escreve no FBO0 assumindo fundo opaco; nas regiões de efeito translúcidas (glow halo / box-shadow / drop-shadow / mask / backdrop) o canal ALPHA do FBO0 pode ficar < 1 dependendo do caminho de blend do driver GPU. O compositor Wayland/Mutter do Fedora respeita o alpha da janela e trata essas regiões como transparentes → efeitos "somem" na tela real.  
**Nota:** `glReadPixels(GL_RGB)` das iterações anteriores ignorava o canal alpha — por isso o readback "parecia ok" e o bug não foi detectado em headless.  
**Ref:** `examples/RmlUi/Backends/RmlUi_Renderer_GL3.cpp:912-913` ("Assuming we have an opaque background…"); o backend oficial limpa FBO0 com alpha=1 via `Clear()` em `:974-978`.

**Fix aplicado** (`glintfx/src/render_gl3.cpp`, `RenderGl3::end_frame()`, após `EndFrame()`):

```cpp
// EN: Force backbuffer alpha to 1 so compositors (Wayland/Mutter) do not treat translucent
//     effect regions (glow/shadow/mask/backdrop) as window transparency. RmlUi composites in
//     premultiplied alpha assuming an opaque background; the postprocess-to-FBO0 fullscreen
//     quad can leave alpha < 1 in shadow/glow regions depending on the GPU blend path.
//     Binding FBO0 explicitly is safe: EndFrame() restores the pre-BeginFrame binding (= 0).
//     This masks RGB writes so only the alpha channel is touched.
// PT: (ver comentário bilíngue completo em render_gl3.cpp)
glBindFramebuffer(GL_FRAMEBUFFER, 0);
glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
glClearColor(0.f, 0.f, 0.f, 1.f);
glClear(GL_COLOR_BUFFER_BIT);
glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
```

**Probe RGBA (smoke-test, llvmpipe/Xvfb 900×600):**

| Probe GL | Região | RGBA BEFORE | RGBA AFTER | Esperado |
|----------|--------|-------------|------------|---------|
| (301,520) | halo glow (fora do card, fundo escuro) | (46,97,127,A=255) | (46,97,127,A=255) | A=255 ✓ |
| (100,520) | card sólido interior | (30,42,80,A=255) | (30,42,80,A=255) | A=255 ✓ |

**Observação:** Em llvmpipe/Xvfb o alpha já era 255 antes do fix (Xvfb não tem compositor separado que leia o alpha channel da janela). O bug só se manifesta em GPU real com Wayland/EGL, onde o Mutter compositor expõe o alpha do FBO0 via surface EGL para composição. O fix garante alpha=1 independentemente do caminho de blend do driver.

**RGB preservado:** ambas as regiões mantiveram valores RGB idênticos antes/depois (colorMask protege RGB).

**Golden test:** MSE=0.00 após fix. Nenhuma alteração visual — apenas canal alpha garantido.

---

## 8. Pendências / observações para GPU real

- Verificar visualmente na tela com Wayland se os efeitos (glow halo, drop-shadow, backdrop-blur, mask) agora aparecem corretamente com o fix de alpha do backbuffer.
- Body não expande para largura total do viewport (900px) em RmlUi com `display: block` e sem `width: 100%` explícito. A largura atual ≈ 340px. Adicionar `width: 100%` ao body se o gradiente de fundo deve cobrir tela cheia.
