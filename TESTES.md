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
| TST-SFNT-SAN | Sanitizer (ASan/UBSan) via harness HOSTED-companion | Só a UNIDADE DE TRADUÇÃO `src/sfnt.c` (parser de input **hostil por natureza** — o gatilho de "Fora de escopo" abaixo virou aplicável) sob `-fsanitize=address,undefined` — achou em produção o CRÍTICO de UB float→int16 (review adversarial do SOV-SFNT) que `make test`'s freestanding, não-sanitizado, não conseguia ver | `make sanitize-sfnt`: compila o MESMO `src/sfnt.c` (sem `#ifdef`, sem cópia) contra libc normal via um programa `tests/sanitize_sfnt.c` **hosted** dedicado, fora de `$(PROGRAMS)`/`make test` — ver "TST-SFNT-SAN" abaixo |

### Comandos (W11)

```sh
make test           # TST-INT: build + roda toda tests/*.c, exit code + stdout/stderr golden
make test-negative   # AUD-C0-5.3: verifica o caminho de FALHA do harness (TEST_ASSERT aborta com
                      # a mensagem "FAIL: <file>:<line>: <expr>" correta) -- já coberto também
                      # dentro de `make test` via tests/expected_stderr.txt
make check-static    # TST-STATIC: cppcheck (src/) + clang --analyze (src/) + check_spdx.sh +
                      # check_syscall_nums_sync.sh (drift entre syscall_nums.h/.inc)
make test-mem        # TST-MEM: valgrind memcheck em test_alloc + test_mem
make sanitize-sfnt   # TST-SFNT-SAN: ASan+UBSan sobre src/sfnt.c via harness hosted-companion
```

**TST-INT — golden files.** Três manifestos irmãos em `tests/` (`expected_exit.txt`,
`expected_stdout.txt`, `expected_stderr.txt`), todos no formato `<nome> <valor-ou-caminho>`. Um
programa só é comparado byte-a-byte no fd que tiver entrada registrada; ausência mantém o
contrato original (só exit code). `tests/printf_e2e.c` força ≥2 flushes do `mini_printf` (saída
>256B, AUD-C0-3); `tests/negative_probe.c` prova o caminho de falha do `TEST_ASSERT` (AUD-C0-5.3).

**TST-STATIC — escopo do cppcheck restrito a `src/`.** `tests/*.c` fica FORA do cppcheck de
propósito: os testes exercitam nossas PRÓPRIAS `strcmp`/`memcmp`/`memset` com padrões
(comparação de strings estáticas, `memset` com valor fora de `unsigned char` testando
truncamento) que as heurísticas do cppcheck, moldadas pra código hospedado em libc, leem como
ruído — não são achados reais sobre a runtime que estamos validando. Achados reais em `src/` são
corrigidos; falso-positivo inevitável (ex.: `sys_mmap`/`sys_read` cujo parâmetro cppcheck sugere
`const` mas o contrato documentado no header é o kernel escrevendo através do ponteiro) é
suprimido cirurgicamente com `// cppcheck-suppress <id>` + rationale, nunca desligando a
checagem inteira.

**TST-MEM — limitação documentada do valgrind em freestanding.** `valgrind` **consegue**
instrumentar o binário estático/no-PIE/sem-libc (verificado empiricamente: roda limpo, e detecta
corretamente "Invalid write" numa sonda que escreve fora da arena mmap'ada inteira, antes do
SIGSEGV). O que ele **não** consegue: o "HEAP SUMMARY" sempre reporta `0 allocs, 0 frees` porque
`src/alloc.c` nunca chama `malloc`/`free` da libc — bump-aloca direto sobre `mmap` cru — então
não há leak-tracking nem redzone por-bloco (uma escrita fora do bloco mas dentro da arena de 1
MiB é invisível ao valgrind aqui). O fallback real de invariante é `tests/test_alloc.c`:
alinhamento de 16B e não-sobreposição entre blocos (padrão de byte único por bloco, reconferido),
já exercitado por `make test` e agora também sob `make test-mem`. Detalhe completo no comentário
do alvo `test-mem` no `Makefile`.

**TST-SFNT-SAN — o gatilho da linha "Fora de escopo" abaixo disparou; padrão hosted-companion.**
`SOV-SFNT` (`src/sfnt.c`) é o primeiro **parser de input hostil por natureza** (formato de arquivo
de fonte, superfície de ataque clássica) da Camada 0 — exatamente o gatilho que este manual já
previa pra tirar sanitizers de "fora de escopo". Mas ASan/UBSan continuam impossíveis de rodar
sobre um binário `-ffreestanding -nostdlib` de verdade (exigem runtime próprio, ver a linha
abaixo). A saída **não foi** dar `-ffreestanding` uma exceção nem inventar um runtime de
sanitizer do zero — foi notar que `src/sfnt.c` **não usa nenhum símbolo freestanding-interno**
(só `<stdint.h>`/`<stddef.h>`, ver `include/core/sfnt.h`, "NO DYNAMIC ALLOCATION"), então a MESMA
unidade de tradução compila igualzinha num programa **HOSTED comum** (libc normal). Esse é o
padrão **hosted-companion**: `tests/sanitize_sfnt.c` (fora de `$(PROGRAMS)`/`make test`, mesmo
precedente de `tests/libcore_consumer.cpp`) linka `src/sfnt.c` sem `#ifdef`/sem cópia contra libc,
compilado com `-fsanitize=address,undefined -fno-sanitize-recover=all`. Achou o CRÍTICO real
(float→int16 UB no `apply_component_transform`, review adversarial do SOV-SFNT) que `make test`
nunca teria pego (o cast trunca em silêncio sob build normal, sem sinal/crash). Prova negativa
verificada manualmente: revertendo o fix, `sanitize-sfnt` aborta com
`runtime error: 40000.5 is outside the range of representable values of type 'short'` no exato
`src/sfnt.c:apply_component_transform` — com o fix, roda limpo. **Reuso futuro**: `SOV-RAST` (o
próximo parser/consumidor não-confiável, o rasterizador) deve crescer um `sanitize-rast` irmão na
mesma forma, per o brief que criou este alvo.

## Fora de escopo (projeto base — por enquanto)

- Sanitizers (ASan/UBSan) sobre o BINÁRIO freestanding-de-verdade (`-ffreestanding -nostdlib`,
  `$(PROGRAMS)`/`make test`) continuam fora de escopo — exigem runtime próprio que não temos.
  Onde o gatilho abaixo dispara (parser de input não-confiável) e a unidade de tradução em
  questão não usa símbolo freestanding-interno nenhum, o padrão **hosted-companion** (ver
  TST-SFNT-SAN acima) contorna a limitação sem exigir esse runtime — não é uma exceção à regra,
  é uma segunda rota que só existe quando a TU é hosted-compilável como está.
- Fuzzing — considerar quando houver parser de input não-confiável (gatilho de re-roteamento do porte). `SOV-SFNT` já dispara o gatilho; ainda não fuzzado (INBOX).

## Definition of Done de teste

Um item de implementação só vira `✅ Concluído` depois que os `TST-*` da sua onda passam. Implementação entregue mas sem teste verde fica `🔍 Pendente verificação`.

---

## glintfx — sub-projeto: biblioteca de efeitos visuais Qt/RmlUi

A glintfx é um sub-projeto C++/OpenGL (pasta `glintfx/`) com stack completamente diferente: usa RmlUi, OpenGL 3, GLFW e FreeType. O harness dela é **ctest** (CMake CTest), e os testes rodam sob **Xvfb** (display virtual) via `xvfb-run -a`.

**Pipeline de CI automatizado (item L1.2-CI):**
- **GitHub Actions:** `.github/workflows/ci.yml` — runners gratuitos `ubuntu-latest`; gate principal.
- **Codeberg (Forgejo Actions):** `.forgejo/workflows/ci.yml` — soberania; mesma sintaxe, requer runners habilitados em Settings → Actions.

Ambos disparam em todo push/PR que toca `glintfx/**`. A validação real ocorre no primeiro push ao remote correspondente.

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

---

## Camada 1 (glintfx) — catálogo não-unitário (`TST-L1-*`)

Esta seção é o par da **Camada 1** para o "Catálogo aplicável (projeto base C/ASM)" do topo do arquivo: o conjunto **não-unitário** (`TST-L1-*`, no espírito do catálogo canônico T2–T15 da constelação bigtech), paralelo aos `TST-INT`/`TST-STATIC`/`TST-MEM` da Camada 0. Prefixo `TST-L1-*` para não colidir com aqueles.

Relação com a suíte existente:

- Os testes **smoke/sanity** documentados acima (`window_smoke`, `render_sanity`, `ui_layer_*`, `data_model_*`, `texture_png_alpha`, etc.) são a **camada por-feature**: nascem junto da implementação (TDD via SDD), rodam no CI a cada push, e cobrem T1 (unit/component) + a regressão **visual/estrutural** (via `render_sanity`/`ui_layer_sanity`, e `golden_test` opt-in). **Não** viram itens `TST-L1-*` — já existem e já são gate.
- Os `TST-L1-*` abaixo são a **rede larga**: os gates transversais (estáticos, dinâmicos, de supply-chain e de contrato) que hoje **não** estão no CI (o CI atual roda só a matriz de 26/13 testes) e que fecham classes inteiras de bug que a suíte por-feature não pega.

**Porte e cerimônia (anti over-engineering).** early / eixo técnico elevado, variante **Pipeline-Lean**, Kanban leve **pull-based**: o catálogo é **podado** ao perfil real (lib C++ gráfica; **sem rede, sem PII, sem DB, sem auth**; superfície real = arquivos de asset + parsers de terceiros vendorizados + estado/recurso GPU/GL + um contrato público de embed). Nenhum item é big-bang: os baratos e sempre-verdes entram no CI já; os mais caros entram **por demanda/pull** conforme a superfície que tocam amadurece.

### Catálogo aplicável (Camada 1)

| ID | Tipo | O que cobre | Como |
| :--- | :--- | :--- | :--- |
| TST-L1-STATIC | Análise estática (T2) | Bugs/UB e má prática em C++17→23 sem executar; **+ gate de encapsulamento**: nenhum tipo de terceiro cruza o header público (`glintfx/include/glintfx/`) | `clang-tidy` (`modernize`, `cppcoreguidelines`, `clang-analyzer-*`) + `cppcheck`; grep include-based `(Rml\|glfw\|gl_loader\|GL/\|stb\|SDL)` nos headers públicos como gate (a substring "GLFW" da macro `GLINTFX_BACKEND_GLFW` não conta — casar include, não nome cru) |
| TST-L1-MEM | Memória dinâmica / runtime (T4) | Leaks e UB de heap na suíte inteira sob GL real (llvmpipe). **Ancora o bug histórico**: buffer do stb_image não liberado no `LoadTexture` (`render_gl3.cpp`) se `GenerateTexture` lançasse — achado **só em review manual**, hoje corrigido via RAII (`unique_ptr` + `stbi_image_free`). Um teste de memória automatizado o pegaria de graça | Rebuild com `-fsanitize=address,undefined` + LSan, rodar `ctest` sob Xvfb; LSan reporta o buffer vazado, UBSan pega overflow no loop de premultiply. Suppression-file para ruído de GLFW/Mesa/FreeType. (glintfx é C++ **hosted** → sanitizers funcionam direto, ≠ Camada 0 freestanding onde estão fora de escopo) |
| TST-L1-DECODE | Robustez/fuzz de input (T3) | Decode de asset **não-confiável**: PNG/JPG/TGA malformado/truncado via **stb_image vendorizado** + o loop **próprio** de premultiply (bounds `w*h*4`) não corrompe memória nem crasha o processo — retorna fallback | Corpus de imagens malformadas/truncadas → `LoadTexture` retorna fallback sem crash; graduável a `libFuzzer`/AFL no decode path. Rodar sob a build ASan de `TST-L1-MEM`: um OOB read do decoder vira falha dura, não silêncio |
| TST-L1-DEPS | Scanning de deps + CVEs (T5+T12) | Deps **vendorizadas** (stb_image, Khronos `glcorearb.h`/`khrplatform.h`/`gl.xml`), **fetchadas** (RmlUi 6.3) e **linkadas** do sistema (GLFW, FreeType, libGL) — versão pinada conferida contra CVE conhecido; vendorizadas **não** recebem update automático, então precisam rastreio | `trivy fs` / `grype` na árvore + auditoria manual das versões vendorizadas vs advisories (stb_image e FreeType têm histórico de CVE). T5 e T12 **fundidos** (anti-OE: um item, não dois) |
| TST-L1-SECRETS | Verificação de secrets (T8) | Credencial/segredo commitado — guardrail barato para o repo **público dual** (Codeberg + GitHub), mesmo sem auth por design | `gitleaks` / `trufflehog` no histórico + no pre-CI. Custo ~zero, sempre-verde |
| TST-L1-CONTRACT | Contrato de consumidor / integração drop-in (espírito T6 + T14) | O contrato público de embed (`docs/embed-integration.md`) exercido como um host **real** faria: **só via API pública** (FetchContent + `find_package`), **sem** includes internos — lifecycle de frame, ordem obrigatória do data-model, invariantes do `GlStateGuard`, resolução de asset. Fecha o `consumer-example-embed/` da INBOX | Mini-consumidor drop-in que builda contra o pacote público nos **2 modos** (GLFW=ON e embed-only OFF), dá `load` + `render()` num FBO e lê pixel; falha se a superfície pública quebrar ou vazar tipo de terceiro. (T6-HTTP não existe aqui; adaptado como contrato de **lib**) |
| TST-L1-GLLEAK | Vazamento de recurso GPU/GL (domínio; além do T-catálogo) | Handles GL (textura/FBO/buffer/VAO/programa) vazados no ciclo de vida de `App`/`UiLayer` e em load/unload repetido de documento — o que ASan **não** enxerga (handle é do driver, não heap do processo). Risco real no host de longa duração (GusWorld roda horas) | Contexto GL de debug (`GL_KHR_debug`, suportado no Mesa/llvmpipe) ou contagem de objetos live (`glIsTexture` / accounting `glGen`↔`glDelete`) antes/depois de N ciclos create→destroy; assert baseline estável. **Endurance-lite direcionado**, não soak completo |
| TST-L1-PRECI | Pré-CI local (T15) | Espelhar o gate de CI **antes** do push: os **2 configs** (GLFW ON/OFF) + build + `ctest` sob Xvfb, MAIS os gates hoje ausentes do CI (static, sanitizers, secret scan) | `tools/preci.sh` que roda configure+build+ctest nas 2 matrizes e invoca `TST-L1-STATIC`/`MEM`/`SECRETS`; alinhado ao `forgejo-runner exec` da memória de CI. **Local-first** (soberania; offline exceto o FetchContent do RmlUi) |
| TST-L1-COV | Cobertura de código (L1.18-COVCI) | % de linha/região/função exercitada pela suíte, restrita ao código **autoral** da glintfx (`src/` + headers públicos) — RmlUi (`_deps`), `third_party/`, `tests/` e `demos/` ficam **fora** do relatório de propósito (não é nosso código sob teste) | `tools/coverage_report.sh` (self-hosted, **não** Codecov — decisão do líder 2026-07-04): rebuild `-DGLINTFX_COVERAGE=ON` (só Clang, `-fprofile-instr-generate -fcoverage-mapping`, instrumentação **por-alvo**, nunca no RmlUi) → `ctest` sob Xvfb com `LLVM_PROFILE_FILE` → `llvm-profdata merge` → `llvm-cov show`/`export`. Relatório HTML como artifact do CI (2 remotos); badge `shields.io` lendo `glintfx/coverage-badge.json` commitado (zero dado a terceiro) |

### Ordem de adoção (lean, pull-based)

Nenhum item é big-bang. Sequência recomendada, do maior valor/menor custo ao mais caro:

1. **Sempre-verdes e baratos → entram no CI/pre-CI já:** `TST-L1-STATIC`, `TST-L1-SECRETS`, `TST-L1-DEPS`, `TST-L1-COV`. Guardrails contínuos, ~zero manutenção.
2. **Maior valor único → agendar (nightly / job separado):** `TST-L1-MEM`. Fecha a **classe** do bug histórico do stb_image; a build de sanitizer é mais lenta, então roda fora do gate quente de PR.
3. **Pull por demanda (Kanban leve):** `TST-L1-DECODE`, `TST-L1-GLLEAK`, `TST-L1-CONTRACT` — puxados quando a superfície que tocam é modificada, quando surge um 2º consumidor, ou quando o GusWorld reportar um achado. `TST-L1-PRECI` consolida os anteriores num só comando local quando ≥2 gates existirem.

#### TST-L1-COV

Gate de cobertura no CI (job `coverage`, `.github/workflows/ci.yml` + `.forgejo/workflows/ci.yml`), config única GLFW=ON (superconjunto da suíte embed-only), demos OFF (biblioteca instrumentada não linka com um demo não-instrumentado — ver comentário `GLINTFX_COVERAGE` em `glintfx/CMakeLists.txt`).

- **Piso, não meta aspiracional.** `--floor N` falha duro o job se a % de cobertura de linha cair abaixo de `N`. Piso atual: **75%** — medido localmente em 2026-07-10 (83.8% de linha), arredondado **para baixo** ao múltiplo de 5 mais próximo (80), menos 5pp de folga (75). A folga existe para que a flutuação normal de código novo pousando entre recalibrações não vire o gate vermelho sozinha; não é licença para deixar a cobertura cair sem justificativa.
- **Recalibração.** Quando a cobertura medida subir de forma sustentada, o piso deve subir junto (mesma fórmula: arredondar para baixo ao múltiplo de 5, menos 5pp) — um piso desatualizado que fica muito abaixo da realidade não protege nada. Recalibrar via `tools/coverage_report.sh` local + atualizar o valor em ambos os workflows no mesmo commit.
- **Badge vs. gate são desacoplados.** `glintfx/coverage-badge.json` (o número mostrado no README) só é atualizado **localmente/por humano** via `tools/coverage_report.sh --update-badge` — o job de CI só **valida** a % contra `--floor` e sobe o relatório HTML como artifact; nunca escreve/commita/pusha o JSON do badge de volta ao repo.
- **Fallback aprovado se o job estourar o teto do runner (codeberg-medium, ~10 min):** cobertura GitHub-only (job removido do lado Forgejo, comentário no cabeçalho do workflow explicando, achado registrado na INBOX do `TODO.md`) — não inventar alternativa (matrix, suíte reduzida, retry silencioso) sem aval do líder.

### Fora de escopo (Camada 1 — glintfx)

Além do que a subseção "O que a suíte **não cobre**" já declara (pixel-exato sob SW render, `mask-image`):

- **Teste de APIs HTTP (T6), Teste de rede (T9), SQL injection (T10), Fuzzing de protocolo (T11)** — sem rede, servidor, DB ou protocolo próprio. (O *espírito* de contrato do T6 é coberto como contrato de **lib** em `TST-L1-CONTRACT`; o T6-HTTP em si não existe.)
- **Scanning de binário / hardening (T7, `checksec`)** — glintfx é **biblioteca** linkada por apps, não binário privilegiado/deployado; flags de hardening são responsabilidade do app consumidor. Reavaliar só se algum dia embarcar um executável standalone com superfície de rede.
- **Performance / Load (k6/Locust/wrk)** — não é serviço com SLO de RPS; performance de render é FPS local, avaliável manualmente com `glintfx_showcase`/`glintfx_capture` (já declarado acima). Reavaliar se a v2/componentes trouxer orçamento de frame.
- **Acessibilidade (axe/WCAG)** — o DOM RmlUi não é HTML acessível (sem bridge de AT / leitor de tela); a superfície é efeito GL (já declarado acima). Reavaliar só se a v2 expuser navegação/foco como produto (INBOX `GAP-4` `set_focus`).
- **Soak/Endurance completo + Chaos** — sem infra distribuída nem loop próprio de longa duração (o host é dono do loop); o risco de vazamento em execução longa é coberto **direcionado** por `TST-L1-GLLEAK` (endurance-lite), sem soak/chaos pesados.
- **Mutation testing** — vaidade cara neste porte: a suíte é majoritariamente render-statistics (difícil mutar com sinal real). Reavaliar em módulos de **lógica pura** (ex.: `DataBinder`) se crescerem.

### Definition of Done de teste (Camada 1)

Um item de feature da Camada 1 só passa de `🔍 Pendente verificação` para `✅ Concluído` depois que os `TST-L1-*` **relevantes à superfície que ele toca** passam (não a suíte inteira — Kanban leve). Coerente com o mandato de QA e a convenção de frescor do `TODO.md`: implementação entregue → `🔍`, **nunca** `✅` direto; `✅` só após a onda de teste/auditoria correspondente.
