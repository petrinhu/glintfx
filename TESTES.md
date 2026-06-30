# TESTES — loucura_c_asm

Manual de testes do projeto. Stack: C + Assembly puros, freestanding, sem libc, Linux x86-64. Como não há framework externo, **o harness de teste é nosso** (item `C1`). Testes unitários seguem TDD e ridem junto da implementação (não são itens da tabela); este manual cobre os testes **não-unitários** (itens `TST-*`).

## Princípios

- Sem libc → sem framework de teste pronto. O runner (`C1`) reporta via `sys_write` e sinaliza resultado pelo **exit code** (0 = passou, ≠0 = falhou).
- Todo binário/demo deve ser determinístico e validável por exit code e/ou stdout.
- Testes rodam no CI freestanding (`F1`) e localmente via alvo `make test`.

## Catálogo aplicável (projeto base C/ASM)

| ID | Tipo | O que cobre | Como |
| :--- | :--- | :--- | :--- |
| TST-INT | Integração / E2E | Cada binário/demo builda, roda e produz exit code + stdout esperados | Script que compila, executa e compara saída/`$?` (golden files) |
| TST-STATIC | Análise estática | Bugs sem executar: UB, uso indevido, dead code | `clang --analyze` / `scan-build`; cppcheck em modo freestanding |
| TST-MEM | Memória (alocador) | Leak, double-free, alinhamento, escrita fora de bloco no nosso `malloc` | Testes dedicados do alocador + `valgrind` no binário estático quando viável; checagem de invariantes do bump/free-list |

## Fora de escopo (projeto base — por enquanto)

- Sanitizers (ASan/UBSan) exigem runtime próprio em freestanding — reavaliar se virar necessário.
- Fuzzing — considerar quando houver parser de input não-confiável (gatilho de re-roteamento do porte).

## Definition of Done de teste

Um item de implementação só vira `✅ Concluído` depois que os `TST-*` da sua onda passam. Implementação entregue mas sem teste verde fica `🔍 Pendente verificação`.

---

## glintfx — sub-projeto: biblioteca de efeitos visuais Qt/RmlUi

A glintfx é um sub-projeto C++/OpenGL (pasta `glintfx/`) com stack completamente diferente: usa RmlUi, OpenGL 3, GLFW e FreeType. O harness dela é **ctest** (CMake CTest), e os testes rodam sob **Xvfb** (display virtual) via `xvfb-run -a`.

### Suíte de CI automatizada (default — `ctest` sem flags extras)

Acionada com:
```sh
cmake -S glintfx -B glintfx/build -DGLINTFX_BUILD_TESTS=ON
cmake --build glintfx/build -j$(nproc)
xvfb-run -a ctest --test-dir glintfx/build --output-on-failure
```

| Teste | Tipo | O que cobre |
| :--- | :--- | :--- |
| `window_smoke` | Smoke | Criação de janela GLFW + contexto OpenGL |
| `render_smoke` | Smoke | Init do `RenderGl3` + um frame sob contexto GL ativo |
| `bootstrap_smoke` | Smoke | Init do RmlUi + criação de `Rml::Context` + carregamento de documento |
| `app_smoke` | Smoke | Fachada pública `App`: abre, carrega RML, 1 frame headless |
| `render_sanity` | Structural / visual | Renderiza `showcase_test.rml` completo e verifica estatísticas de pixels |

**`render_sanity` — detalhes**: captura o showcase CI via `App::snapshot`, lê o PPM e valida 3 propriedades estruturais:
1. `mean_brightness > 5` — não é imagem toda-preta (cena renderizou algo)
2. `dark_count > 10%` — fundo `#0d1020` presente (body/section-dark renderizou)
3. `bright_count > 1%` — seção gradiente + efeitos glow presentes (canal > 150)

Os limites são propositalmente largos (tolerância ~10× o nível mínimo esperado) para ser imune à variação per-pixel causada pelo rendering de tiles paralelos do Mesa llvmpipe — a causa raiz do flakiness do `golden_test` (ver seção abaixo).

### Validação visual manual / GPU real

Para validar os efeitos pixel-a-pixel em hardware real (o que o CI headless não faz), rode os demos **diretamente no desktop** com `DISPLAY` ativo -- sem `xvfb-run`, que usa **llvmpipe (software render)** e não equivale à GPU real:

```sh
# Run on desktop with real GPU (DISPLAY active, no xvfb-run)
# Rodar no desktop com GPU real (DISPLAY ativo, sem xvfb-run)
./glintfx/build/demos/showcase/glintfx_showcase

# GPU real screenshot capture: generates PPM of the showcase
# Captura em GPU real: gera screenshot PPM do showcase
./glintfx/build/demos/showcase/glintfx_capture showcase_gpu.ppm
```

> **Nota (headless/CI vs GPU real):** `xvfb-run` cria um display virtual e usa **llvmpipe**, o software renderer do Mesa -- adequado para a suíte `ctest` (seção acima), mas não valida efeitos visuais em hardware real. Para validação autentica, rodar sem `xvfb-run` com o desktop ativo.

Os efeitos visuais (glow/drop-shadow ciano, degradê laranja→pink, backdrop-blur, mask) foram **validados visualmente pelo líder em GPU real** na release v1. Esse estado é o "golden" humano; o `render_sanity` automatizado detecta regressões grosseiras (output preto, efeitos sumindo completamente, fundo não renderizando).

### Pixel-golden opt-in (`GLINTFX_GOLDEN_TEST=ON`)

O teste `golden_test` faz comparação pixel-exata (MSE < 50) de um screenshot contra uma referência PPM armazenada. Ele está **desabilitado por padrão** porque é flaky sob Mesa llvmpipe/Xvfb.

**Causa raiz do flakiness**: o llvmpipe processa tiles do framebuffer em threads paralelas. As passagens de blur (`backdrop-filter: blur`) e glow (`box-shadow` + `filter: drop-shadow`) envolvem convolução multi-passo com acumulação de ponto flutuante — não-associativa em threads. O agendamento de threads varia entre execuções, produzindo MSE ~3 000 entre duas renders da **mesma cena** (tolerância era 50).

Para usar o `golden_test`, compile com a flag antes:

```sh
cmake -S glintfx -B glintfx/build -DGLINTFX_BUILD_TESTS=ON -DGLINTFX_GOLDEN_TEST=ON
cmake --build glintfx/build -j$(nproc)
```

**Em GPU real (estável -- desktop com `DISPLAY` ativo, sem `xvfb-run`):**
```sh
# First run: generates reference golden/showcase_reference.ppm and exits 0
# 1ª execução: gera referência golden/showcase_reference.ppm e sai 0
ctest --test-dir glintfx/build -R golden_test --output-on-failure
# Subsequent runs: compare via MSE (tolerance 50) -- stable on real GPU
# Execuções seguintes: compara via MSE (tolerância 50) -- estável em GPU real
ctest --test-dir glintfx/build -R golden_test --output-on-failure
```

**Em headless/llvmpipe (flaky -- só para verificar que o teste executa, não para validação pixel):**
```sh
# MSE ~3 000 between runs on llvmpipe (tolerance 50) -- pixel comparison is unreliable
# MSE ~3 000 entre corridas no llvmpipe (tolerância 50) -- comparação pixel não é confiável
xvfb-run -a ctest --test-dir glintfx/build -R golden_test --output-on-failure
```

### O que a suíte **não cobre** (declarado explicitamente)

- **Exatidão pixel-a-pixel dos efeitos sob software render** — o `render_sanity` detecta ausência dos efeitos, não sua qualidade visual. Essa validação é manual/GPU-real.
- **Efeito `mask-image`** — exclui o card mask por crash do Mesa llvmpipe (bug BlendMask dual-sampler). Validado somente em GPU real com `showcase.rml`.
- **Performance** — sem baseline de FPS automatizado. Avaliar manualmente com `glintfx_showcase` e `glintfx_capture`.
- **Acessibilidade** — glintfx é biblioteca de efeitos GL, sem DOM HTML acessível.
