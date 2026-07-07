# AUDIT_FIND.md — Auditoria de distribuição pública + memória (2026-07-07)

> Auditoria READ-ONLY conduzida por Caetano (CTO). **Toda claim arquivo:linha abaixo foi
> verificada pessoalmente contra o código real desta árvore** (`main` @ `7e8eff8`, v0.4.0;
> refs do `TODO.md` re-verificadas após o commit externo `cfb68e9`, que só adicionou o item
> de INBOX referenciando este documento — os achados AUD-TEC-10/AUD-C0-6 continuam válidos).
> Nenhum arquivo existente foi tocado por esta auditoria — este é o único arquivo criado.
> A seção 5 (Plano de Transição, `AUD-TRANS-*`) é um PLANO (documento de decisão para o
> líder), não uma execução — nenhum código muda agora. Documento de trabalho interno, pt-br.

---

## 1. Sumário executivo

**Veredito sobre a preocupação do líder ("API moldada demais no GusWorld"): confirma-se PARCIALMENTE, num padrão específico.** A fronteira pública está exemplar (zero tipos de terceiros vazando nos headers; pImpl consistente), o `App` standalone é cidadão de primeira classe nas docs (headline do Quick Start e do getting-started), e o embed mode é genérico o bastante no seu núcleo. **O problema não é o que o GusWorld pediu — é o que ele nunca exercitou.** As áreas que o consumidor único não usa estão subdesenvolvidas ou quebradas, e é exatamente onde um segundo consumidor tropeçaria primeiro:

1. **`App` standalone tem um bug real de resize** (`AUD-PUB-1`): a janela redimensiona, mas o contexto RmlUi nunca recebe `SetDimensions` — o layout não reflui. O GusWorld nunca notou porque usa `UiLayer` e chama `set_viewport` explicitamente. Um app desktop puro está quebrado no caso de uso mais básico.
2. **A superfície de input é gamepad/cockpit-shaped** (`AUD-PUB-3`/`AUD-PUB-4`): `enum Key` só tem navegação (sem Delete/Home/End/PageUp/letras), o click callback só reporta id (sem botão/coords/double-click), e o único evento RmlUi exposto é `click`.
3. **A próxima classe de bug dominó existe e foi achada** (`AUD-TEC-1`/`AUD-TEC-2`): `App::snapshot(nullptr)` é o gêmeo exato do `load(nullptr)` já caçado (mesmo padrão `const char*` sem guard → `fopen(NULL)` UB), e as coordenadas de mouse (`MouseMove`/`MouseWheel`) aceitam NaN/inf que `set_element_scroll_top` já rejeita — o guard de finitude não foi propagado aos irmãos.
4. **Deriva de versionamento em docs públicas** (`AUD-TEC-8`): README PT diz v0.3.0 enquanto o EN diz v0.4.0; os snippets copy-paste pinam `GIT_TAG v0.3.0`; headers/supressões citam uma tag `v0.3.2` que nunca existiu; `SECURITY.md` não lista 0.4.x. Mais: **critério de SemVer inconsistente e não documentado** (`AUD-PUB-9` — houve mudança de assinatura/ABI declarada em release *patch*), e **GLFW está ausente do `NOTICE`** (`AUD-PUB-10` — gap legal de atribuição).
5. **Sistema de memória atrasado e auto-contraditório** (`AUD-MEM-*`): os dois snapshots de estado param em v0.2.4/v0.3.0 e se contradizem entre si; o índice `MEMORY.md` ecoa estado duplamente velho; e há um conflito latente sério — duas memórias tratam *push* como rotina do orquestrador, quando a disciplina real da sessão é nunca pushar sem pedido explícito.

Camada 0: **nenhum achado crítico** — a implementação está correta nos pontos delicados (INT_MIN, saturação, overlap, promoções variádicas, guarda de overflow do alocador, ABI dos wrappers). Os achados são UB pedante (`memmove` compara ponteiros relacionalmente), código morto não testado (`syscall0/2/4/5`), o caminho de I/O do `mini_printf` sem gate automatizado, e um ADR que promete helpers que não existem.

Adicionalmente (pedido do líder), a **seção 5** traz o **Plano de Transição** (`AUD-TRANS-*`): o caminho técnico pra Camada 1 (glintfx) deixar de chamar RmlUi/gl3w/FreeType/GLFW como acessório e rodar autossuficiente sobre a Camada 0 — detalhamento do épico já existente `L1-INTERNALIZE` (o item `L1-INTERNALIZE` do TODO.md, hoje ~l.92 @ `cfb68e9`). Resumo: a primeira coisa NÃO é código, é a **decisão de fronteira** (o modelo "camadas não se linkam" do ADR-0006 precisa evoluir pra internalização fazer sentido — pergunta aberta pro líder, 3 opções); ordem recomendada gl3w (P, quick win que prova o processo clean-room) → FreeType via `FontEngineInterface` plugável (G) → RmlUi por subsistema (GG, endgame); GLFW com recomendação de **não** internalizar (o build embed-only já vive sem ele); fronteira irredutível = Mesa/libGL/driver/kernel.

**Contagem: 10 AUD-PUB · 10 AUD-TEC · 6 AUD-C0 · 8 AUD-MEM · 6 AUD-TRANS = 40 achados.**

---

## 2. Tabela de achados técnicos/produto

Ondas propostas (ordem de execução): **AW1** dominó de robustez (1 sessão, sem decisão de design) → **AW2** higiene de versão/licença/docs → **AW3** frescor do TODO.md → **AW4** generalização de API (exige decisão do líder via AskUserQuestion) → **AW5** Camada 0 (anexar às ondas `TST-*`/`AUD-*` já existentes). Memória (`AUD-MEM-*`) na seção 4, própria. Os itens `AUD-TRANS-*` pertencem à onda **LW8** (o épico `L1-INTERNALIZE` já existente no TODO.md, hoje ~l.92 @ `cfb68e9`) e são PLANO, não execução — detalhados na seção 5.

| ID | Onda | Grupo | Descrição Técnica | Prioridade | Pré-requisito | Dificuldade | Status | Estado Auditado |
|---|---|---|---|---|---|---|---|---|
| AUD-TEC-1 | AW1 | glintfx/Robustez | `App::snapshot(nullptr)` → `fopen(NULL)` UB/crash — gêmeo do `load(nullptr)` já caçado (`app.cpp:183`) | Alta | — | Trivial | ⏳ Pendente | achado novo |
| AUD-TEC-2 | AW1 | glintfx/Robustez | NaN/inf em `UiEvent.x/y` não validados em `MouseMove` (cast float→int UB, `ui_layer.cpp:320`) e `MouseWheel` (`ui_layer.cpp:376`) — guard de finitude existe em `set_element_scroll_top` mas não foi propagado | Alta | — | Fácil | ⏳ Pendente | achado novo |
| AUD-TEC-5 | AW1 | glintfx/Robustez | `id=""` aceito nos 6 métodos por-id (`bootstrap.cpp:330-372`) — **confirma o item já na INBOX do TODO.md (o item "`id=""` (string vazia) aceito onde deveria falhar, em todos os métodos por-id", hoje ~l.122 @ `cfb68e9`), ainda aberto**; fechar junto de AUD-TEC-1/2 (mesma varredura dominó) | Média | — | Fácil | ⏳ Pendente | já na INBOX |
| AUD-TEC-3 | AW1 | glintfx/Robustez | Reentrância: `set_click_callback` chamado de dentro do próprio callback → UAF do `std::function` em execução (`bootstrap.cpp:325-327`) | Média | — | Média | ⏳ Pendente | achado novo |
| AUD-TEC-4 | AW1 | glintfx/Robustez | Overflow de int com sinal em `gl_offset_y = target_h - y - h` (`ui_layer.cpp:167,401`); dims sem teto são | Média | — | Fácil | ⏳ Pendente | achado novo |
| AUD-TEC-8 | AW2 | glintfx/Docs | Deriva de versão: README PT diz v0.3.0 (l.457) vs EN v0.4.0 (l.225); `GIT_TAG v0.3.0` em 4 snippets; tag fantasma "v0.3.2" em headers+`ubsan_suppressions.txt`; `SECURITY.md` sem 0.4.x; `embed-integration.md` ancorado "as of v0.2.1" | Alta | — | Fácil | ⏳ Pendente | achado novo |
| AUD-PUB-10 | AW2 | glintfx/Legal | GLFW ausente do `NOTICE` (linkado por default com `GLINTFX_BACKEND_GLFW=ON`; licença zlib/libpng exige aviso); título do NOTICE é "loucura_c_asm", não "glintfx" | Alta | — | Trivial | ⏳ Pendente | achado novo |
| AUD-PUB-9 | AW2 | glintfx/Release | Critério SemVer inconsistente e não documentado: API nova em patch (0.2.2/0.2.3/0.2.5/0.3.1), mudança de assinatura/ABI de `load()` declarada em *patch* (0.2.3); nenhuma política escrita | Alta | — | Fácil | ⏳ Pendente | achado novo |
| AUD-PUB-7 | AW2 | glintfx/Docs | Jargão GusWorld em doc pública: roadmap do README com rationale nominal por item (l.234-237/466-469), "brass screw-head" como exemplo canônico (l.49, `effects.md:87`), `embed-integration.md` com exemplo único "cockpit" | Média | — | Fácil | ⏳ Pendente | achado novo |
| AUD-PUB-8 | AW2 | glintfx/Docs | Gaps de doc pública: sem doc de packaging/`cmake --install`, sem tabela de compatibilidade (RmlUi pinada? gcc?), sem troubleshooting, seção Documentation não aponta os headers como API reference | Média | — | Média | ⏳ Pendente | achado novo |
| AUD-TEC-9 | AW2 | glintfx/Build | `app.hpp` incluído avulso num build embed-only (`GLINTFX_BACKEND_GLFW=0`) → link error críptico; falta guard no próprio header | Baixa | — | Trivial | ⏳ Pendente | achado novo |
| AUD-TEC-10 | AW3 | TODO.md/Frescor | 3 defeitos no TODO.md: a frase "LW9 entregue e testada, pendente onda de verificação." (parágrafo "Ordem recomendada" da seção Camada 1, hoje ~l.55 @ `cfb68e9`) contradiz "LW9 ✅ FECHADA" (parágrafo "Estado" da mesma seção, hoje ~l.53 @ `cfb68e9`); o item `AUD-L1-PARSE` (hoje ~l.86 @ `cfb68e9`) lista 2 achados JÁ remediados no código como "PRIORIDADE ALTA -- 2 achados reais"; o item `L2-COMPONENTS` (hoje ~l.106 @ `cfb68e9`) — link do plano F2 só existe no branch, 404 no `main` | Alta | — | Trivial | ⏳ Pendente | achado novo |
| AUD-PUB-1 | AW4 | glintfx/API | `App` standalone não refaz layout em resize: `render()` nunca chama `SetDimensions`, `App` não tem `set_viewport` — UI congela nas dims do `AppConfig` | Alta | decisão do líder | Média | ⏳ Pendente | achado novo |
| AUD-PUB-4 | AW4 | glintfx/API | Click callback insuficiente p/ consumidor genérico: só `const char* id` — sem botão (right-click), sem coords, sem double-click; elementos sem id indistinguíveis (`""`) | Média | decisão do líder | Média | ⏳ Pendente | achado novo |
| AUD-PUB-3 | AW4 | glintfx/API | `enum Key` é subconjunto gamepad-nav (`ui_event.hpp:19-32`); faltam Delete/Home/End/PageUp/PageDown/letras/dígitos p/ app desktop (RmlUi já tem os `KI_*` prontos) | Média | decisão do líder | Fácil | ⏳ Pendente | achado novo |
| AUD-PUB-2 | AW4 | glintfx/API | `App` sem injeção de input (`process_event` só no `UiLayer`) — runner opaco não documentado como tal | Média | decisão do líder | Média | ⏳ Pendente | achado novo |
| AUD-PUB-6 | AW4 | glintfx/API | Gaps de API p/ consumidor genérico (registrar como sementes INBOX, modo pull — NÃO implementar sem demanda): unload/hide, múltiplos documentos, eventos RmlUi genéricos (change/submit/focus), setters imperativos por id, `get_*` do data-model, `load_font` runtime, cursor callback, clipboard | Média | — (só registrar) | Fácil (registro) | ⏳ Pendente | relacionado a GAP-4, GAP-2b |
| AUD-PUB-5 | AW4 | glintfx/API | Contrato amarrado a FBO 0 + `target_h` bottom-up na assinatura pública de `set_viewport` — documentar limitação como decisão; feature real já adiada como GAP-2b na INBOX | Baixa | — | Fácil (doc) | ⏳ Pendente | já na INBOX (GAP-2b) |
| AUD-TEC-6 | AW4 | glintfx/API | Dívida de forma das 10 releases: 4 contratos de erro distintos (log/silêncio/bool/void), `bind_*` bool vs `set_*` void, mutadores marcados `const`, `enum class Key` vs `enum Mod` unscoped, `button` int vs `key` enum | Média | decisão do líder (breaking p/ v0.5 ou v1.0) | Média | ⏳ Pendente | achado novo |
| AUD-TEC-7 | AW4 | glintfx/Testes | Cobertura: data-model 100% sem teste no caminho `UiLayer`/embed (todos os 4 testes usam `App`, dentro do bloco GLFW=ON); `set_bool` zero testes; `App::run()` zero testes; 2º `create_data_model` falha silenciosa não testada/documentada | Média | — | Média | ⏳ Pendente | achado novo |
| AUD-C0-1 | AW5 | Camada0/UB | `memmove` compara ponteiros de objetos distintos com `<`/`>` (`mem.c:86,90`) — UB estrito C23, contradiz o mandato "ausência de UB" da `AUD-SEC` | Média | anexar a AUD-SEC (W12) | Trivial | ⏳ Pendente | achado novo |
| AUD-C0-2 | AW5 | Camada0/YAGNI | `syscall0/2/4/5` são código morto sem nenhum call-site nem teste (`syscall.asm`) — podar ou testar antes da tag | Média | anexar a AUD-ABI (W12) | Fácil | ⏳ Pendente | achado novo |
| AUD-C0-3 | AW5 | Camada0/Testes | `mini_printf` (wrapper stdout + laço de flush >256B, `printf.c:265-309`) com ZERO teste automatizado — `test_printf.c` só cobre `mini_vsnprintf` (admitido no próprio cabeçalho) | Média | anexar a TST-INT (W11) | Fácil | ⏳ Pendente | achado novo |
| AUD-C0-4 | AW5 | Camada0/ADR | ADR-0002 promete `IS_ERR(x)`/`ERR_CODE(x)` + header de códigos de erro que NÃO existem (grep vazio); call-sites aplicam o contrato `-errno` ad-hoc | Média | antes de REL-TAG | Fácil | ⏳ Pendente | achado novo |
| AUD-C0-5 | AW5 | Camada0/Testes | Testes faltantes menores: guarda de overflow do `malloc` (`alloc.c:167`) sem regressão; `realloc` shrink não testado; prova do caminho de FALHA do harness não versionada (sonda descartável) | Baixa | anexar a TST-MEM/TST-INT | Fácil | ⏳ Pendente | achado novo |
| AUD-C0-6 | AW5 | Housekeeping | o item B1 do TODO.md (hoje ~l.15 @ `cfb68e9`) lista `bool` como entregável de `types.h` mas ele foi deliberadamente omitido (keyword C23, ver doc-comment em `include/types.h:4-14`); `out.ppm` (1.6 MB) commitado na raiz — artefato de render stray | Baixa | — | Trivial | ⏳ Pendente | achado novo |
| AUD-TRANS-5 | LW8 | Transição/ADR | **Decisão de fronteira** (bloqueia todo o resto da trilha): o modelo ADR-0006 "camadas não se linkam" precisa evoluir pra internalização acontecer — 3 opções (link estático incremental / process-boundary com IPC / clean-room dentro da Camada 1 com soberania em fase 2) — **decisão do líder, propor ADR-0009** | Alta (é o gate) | L1-INTERNALIZE | Média (decisão+ADR) | 💡 Decisão pendente | plano |
| AUD-TRANS-0 | LW8 | Transição/Camada 0 | Pré-requisitos na Camada 0 (onda nova `SOV-*`): alocador geral com `free` real (E1 bump não sustenta UI de vida longa), float↔string, libm-núcleo, syscalls novos (`clock_gettime`, `munmap`, `socket`/`connect`/`sendmsg`/`recvmsg`, futex), runtime C++ freestanding (condicional a AUD-TRANS-5) | Média | AUD-TRANS-5; REL-TAG core-v0.1.0 | GG (decomponível) | ⏳ Pendente | plano |
| AUD-TRANS-1 | LW8 | Transição/gl3w | Internalizar gl3w: loader GL próprio gerado clean-room do registro Khronos (`gl.xml`) — substitui `third_party/gl3w` mantendo suíte verde; **1º marco recomendado** (prova o processo clean-room ponta a ponta com risco mínimo) | Média | AUD-TRANS-5 | P | ⏳ Pendente | plano |
| AUD-TRANS-2 | LW8 | Transição/FreeType | Internalizar FreeType via `Rml::FontEngineInterface` própria (parser SFNT/TTF + rasterizador de outline clean-room, sem hinting na v1) — a única dep com **interface de plugin pronta** no RmlUi (`RMLUI_FONT_ENGINE` já é opção do nosso CMake) | Média | AUD-TRANS-1; SOV-ALLOC/SOV-MATH | G (GG c/ hinting) | ⏳ Pendente | plano |
| AUD-TRANS-3 | LW8 | Transição/GLFW | GLFW: **recomendação de NÃO internalizar** (adiar indefinidamente) — o build embed-only (`GLINTFX_BACKEND_GLFW=OFF`) já entrega o caminho sem GLFW; backend janela/input próprio (X11/Wayland wire por socket + EGL) é G com valor baixo e risco alto de regressão | Baixa | — (recomendação) | G | 💡 Decisão pendente | plano |
| AUD-TRANS-4 | LW8 | Transição/RmlUi | Internalizar RmlUi por subsistema, de fora pra dentro: (1º) `RenderInterface` de efeitos próprio (hoje wrapper do `RenderInterface_GL3` deles), (2º) font engine (=TRANS-2), (por último) core layout/RCSS/RML/data-binding — **endgame**, anos, só com Camada 1 estável e API consolidada (pós AUD-TEC-6) | Baixa (longo prazo) | AUD-TRANS-1, AUD-TRANS-2 | GG | ⏳ Pendente | plano |

---

## 3. Instruções técnicas exaustivas por achado

### AUD-TEC-1 — `App::snapshot(nullptr)` → `fopen(NULL)` UB (a próxima cabeça do dominó)

**Problema exato.** `glintfx/src/app.cpp:159` — `bool App::snapshot(const char* ppm_path)` valida `impl_->ok` mas nunca `ppm_path`. Em `app.cpp:183`:
```cpp
FILE* f = fopen(ppm_path, "wb");
if (!f) return false;
```
`fopen(NULL, "wb")` é UB (crash na glibc). É **exatamente o mesmo padrão** do CRITICAL `load(nullptr)` corrigido na v0.3.0 (guard hoje em `bootstrap.cpp:243`, dentro de `Bootstrap::load`): único outro método público que recebe `const char*` de caminho e ficou fora da varredura dominó daquela release.

**Por que importa.** O GusWorld não usa `snapshot()` (é `App`-only). Um consumidor público que capture screenshots (ferramenta de CI própria, gerador de thumbnails) e passe um caminho computado que pode ser nulo derruba o processo host — a mesma classe de severidade que motivou o hardening da v0.3.0.

**Correção.** Adicionar como primeira linha do corpo (antes de `if (!impl_->ok)`):
```cpp
if (!ppm_path) return false;  // EN: null path is a caller error, not a crash. PT: caminho nulo é erro do caller, não crash.
```
**Teste que prova:** em `app_input_hardening_smoke.cpp` (já existe e cobre `load(nullptr)`), adicionar `assert(!app.snapshot(nullptr));` no mesmo bloco. Convém também `snapshot("")` → `fopen` retorna NULL → já coberto pelo `if (!f)`, mas o assert documenta.

**Não fazer:** não validar existência de diretório/permissões — `fopen` já reporta isso via `!f`.

### AUD-TEC-2 — NaN/inf em coordenadas de mouse (guard de finitude não propagado)

**Problema exato.** `glintfx/src/ui_layer.cpp:320` (case `T::MouseMove` dentro de `UiLayer::process_event`):
```cpp
c->ProcessMouseMove(static_cast<int>(ev.x) - impl_->x, static_cast<int>(ev.y) - impl_->y,
                    to_rml_mods(ev.modifiers));
```
`static_cast<int>` de float NaN/inf é UB ([conv.fpint]). E `ui_layer.cpp:376` (case `T::MouseWheel`, mesma função `UiLayer::process_event`):
```cpp
c->ProcessMouseWheel(Rml::Vector2f(ev.x, ev.y), to_rml_mods(ev.modifiers));
```
delta não-finito flui direto pro RmlUi e pode envenenar o offset de scroll com NaN. O projeto **já tem** essa regra: `set_element_scroll_top` rejeita não-finito (`Bootstrap::set_element_scroll_top`, `bootstrap.cpp:377`), e a memória do projeto registra "validar float antes do cast" como lição da sessão (feedback_auditoria_domino). Faltou aplicar aos irmãos de `process_event` — `MouseButton` (se usa `ev.x/y`) deve ser varrido junto.

**Por que importa.** `UiEvent.x/y` são a superfície de entrada mais quente do embed mode — o host repassa valores de terceiros (driver, replay de input, rede). O GusWorld manda coords SDL sempre finitas; outro host pode não mandar.

**Correção.** No topo do dispatch de `process_event` (ou por-case nos que consomem `ev.x/ev.y`):
```cpp
if (!std::isfinite(ev.x) || !std::isfinite(ev.y)) return;  // ou break no case
```
(incluir `<cmath>`; seguir o estilo do guard de `set_dp_ratio` em `engine.cpp:90-95`).
**Teste:** ampliar `input_hardening_sanity.cpp` (roda em ambos os configs) com `MouseMove{NaN,inf}` e `MouseWheel{NaN}` + assert de que nada crasha e um `MouseWheel{0,-1}` subsequente ainda rola normal.

**Não fazer:** não clamp silencioso de valores finitos grandes aqui (isso é AUD-TEC-4); só rejeitar não-finito.

### AUD-TEC-5 — `id=""` nos 6 métodos por-id (fechar o item da INBOX junto da onda AW1)

**Problema exato.** Confirmado no código atual: `bootstrap.cpp:330,340,348,356,364,372` — todos os 6 métodos (`get_element_box`, `scroll_element_into_view`, `get_element_scroll_top/height`, `get_element_client_height`, `set_element_scroll_top`) usam `if (!impl_ || !impl_->doc || !id) return false;` sem checar `id[0]`. O item já está na INBOX do `TODO.md` (o item "`id=""` (string vazia) aceito onde deveria falhar, em todos os métodos por-id", hoje ~l.122 @ `cfb68e9`) e está **acurado** — só não foi executado.

**Correção.** Trocar o guard nos 6 pontos por `if (!impl_ || !impl_->doc || !id || !*id) return ...;` (nos que retornam `ElementBox`, retornar `{}` com `found=false` como já fazem pro id nulo). **Teste:** casos `""` em `element_box_sanity.cpp` e `scroll_sanity.cpp`.

**Não fazer:** não rejeitar ids com espaços/unicode — RmlUi decide o que é id válido; só a string vazia é inequivocamente inválida.

### AUD-TEC-3 — UAF por reentrância em `set_click_callback`

**Problema exato.** `bootstrap.cpp:325-327` (`Bootstrap::set_click_callback`): `set_click_callback` faz `impl_->click_cb = std::move(cb)`. O `ClickEventListener::ProcessEvent` (`bootstrap.cpp:92-112`) invoca via ponteiro `cb_ = &impl_->click_cb` (setado no construtor do listener, chamado em `bootstrap.cpp:318`). Se o handler do consumidor chamar `set_click_callback(...)` **de dentro do próprio handler** (padrão plausível: "clicou em 'Play' → troca o handler da tela seguinte"), o `std::function` em execução é destruído durante a sua própria invocação → use-after-free.

**Correção (preferida).** No `ProcessEvent`, copiar o functor para uma variável local antes de invocar:
```cpp
auto cb = *cb_;          // cópia barata; sobrevive a reatribuições reentrantes
if (cb) cb(id_str);
```
Alternativa mais leve: documentar a proibição no doc-comment de `set_click_callback` em `app.hpp:139` (declaração de `App::set_click_callback`) e `ui_layer.hpp:183` (declaração de `UiLayer::set_click_callback`). Recomendo a cópia — custo desprezível, elimina a classe inteira.
**Teste:** callback que troca o próprio callback e re-clica; rodar sob ASan (a suíte nightly já existe).

### AUD-TEC-4 — Overflow de int com sinal em `gl_offset_y`

**Problema exato.** `ui_layer.cpp:167` (dentro de `UiLayer::set_viewport(x,y,w,h,target_h)`, e uso análogo no handler `T::Resize` em `:401`): `impl_->gl_offset_y = target_h - y - h;` — `x/y/target_h` não têm validação nenhuma e `w/h` só `>0` sem teto. Com valores próximos de `INT_MAX`, a subtração transborda (UB de signed overflow). `Rml::Vector2i(w,h)` gigante em `SetDimensions` (`engine.cpp:66`, dentro de `Engine::set_viewport`) também tenta layout absurdo.

**Correção.** Definir um teto são (ex.: `constexpr int kMaxDim = 32768;`) e rejeitar (no-op, consistente com o guard atual de `w/h<=0` nas duas sobrecargas de `UiLayer::set_viewport`, `ui_layer.cpp:142,159`) qualquer `w/h/target_h` fora de `(0, kMaxDim]` e `x/y` fora de `[-kMaxDim, kMaxDim]`. Com o teto, `target_h - y - h` cabe folgado em int.
**Teste:** `set_viewport(0, INT_MAX, 100, INT_MAX, INT_MAX)` em `viewport_origin_sanity.cpp` → no-op, sem UBSan hit.

**Não fazer:** não mudar a assinatura pública aqui (isso é AUD-PUB-5/decisão de design).

### AUD-TEC-8 — Deriva de versão nas docs públicas (varredura única)

**Problemas exatos (todos confirmados):**
1. `README.md:457` (PT, seção "Roadmap e visão"): "Lançamento atual: **v0.3.0** ... 2026-07-04" — o EN em `README.md:225` (seção "Roadmap and vision") já diz v0.4.0/2026-07-07. Corrigir o PT para espelhar o EN.
2. `README.md:213` (EN, seção "Known limitations") e `:445` (PT, seção "Limitações conhecidas"): "glintfx v0.3.0 is honest about..." → v0.4.0.
3. `GIT_TAG v0.3.0` no snippet cmake de `README.md:82` (Quick start EN), `README.md:314` (Quick start PT), `docs/getting-started.md:45` e `docs/getting-started.md:188` → `v0.4.0`. (Alternativa melhor a prova de futuro: comentário `# pin to the latest tag` ao lado, e item de checklist de release "bump GIT_TAG nos snippets" — a causa-raiz é não estar no checklist.)
4. Tag fantasma **"v0.3.2"**: doc-comment de `scroll_element_into_view` em `app.hpp:157,161`, doc-comment do enum `MouseWheel` em `ui_event.hpp:69,96`, doc-comment de `scroll_element_into_view` em `ui_layer.hpp:201,214`, e os dois blocos de suppression `vptr:*WidgetScroll.cpp*`/`vptr:*RemoveEventListener*` em `glintfx/tests/ubsan_suppressions.txt:93,148,154,214` — a feature GLINTFX-SCROLL-1 saiu como **v0.4.0**; não existe tag v0.3.2. Trocar todas as 10 ocorrências por "v0.4.0".
5. `SECURITY.md:16-17` (tabela "Supported versions", EN) e bloco PT equivalente `SECURITY.md:53-54` ("Versões suportadas"): tabela diz "0.3.x Yes / <0.3.0 No" → "0.4.x Yes / <0.4.0 No". (Também citado dentro de AUD-L1-PARSE — ver AUD-TEC-10.)
6. `docs/embed-integration.md:3` e `:5` (parágrafo de abertura EN/PT do documento): "Line references point at the source as of glintfx **v0.2.1**" — o doc já documenta features até v0.4.0. Ou re-ancorar as refs de linha na v0.4.0 (trabalho maior), ou trocar a frase por "line references are approximate; verify against the current source" (honesto e barato). Recomendo a 2ª + re-ancorar apenas as refs realmente citadas nas seções novas.

**Por que importa.** É a cara pública da lib: um consumidor que segue o tutorial pina v0.3.0 e não recebe os fixes de memória da v0.3.1 nem o scroll da v0.4.0.

### AUD-PUB-10 — NOTICE sem GLFW + branding

**Problema exato.** `grep -i glfw NOTICE` = vazio. O NOTICE lista RmlUi, FreeType, OpenGL/libGL, gl3w, stb_image, glcorearb/khrplatform, Open Sans — mas não GLFW, que é linkado por default (`GLINTFX_BACKEND_GLFW=ON`) e tem licença zlib/libpng (atribuição requerida em distribuição binária). Além disso `NOTICE:1` = "NOTICE — loucura_c_asm" (nome do runtime interno, não do produto público).

**Correção.** Adicionar à seção de bibliotecas linkadas:
```
GLFW — https://www.glfw.org — zlib/libpng License.
Copyright (c) 2002-2006 Marcus Geelnard, (c) 2006-2019 Camilla Löwy.
Linked only when built with GLINTFX_BACKEND_GLFW=ON (the default).
```
E trocar o título para "NOTICE — glintfx" (mantendo menção ao repo se quiser). **Não fazer:** não copiar o texto integral da licença zlib pro NOTICE — a atribuição + nome da licença basta (zlib não exige reprodução em uso como lib, mas a atribuição é boa prática e o NOTICE já segue esse padrão pros demais).

### AUD-PUB-9 — Política de SemVer inexistente; histórico inconsistente

**Problema exato.** Tabela do histórico (fonte `CHANGELOG.md`): API pública nova entrou como **patch** em v0.2.2 (`set_dp_ratio`+`set_asset_base_url`), v0.2.3 (data-model inteiro **+ `load()` `void`→`bool`**, que o próprio changelog chama de "signature and ABI change", `CHANGELOG.md:132-133`), v0.2.5 (`set_click_callback`+`get_element_box`+header novo `element_box.hpp`), v0.3.1 (gradiente no polygon) — mas features de peso comparável foram **minor** em v0.3.0 e v0.4.0. Nenhum critério escrito: nem `CONTRIBUTING.md` nem README definem o que dispara minor vs patch; a única âncora é o footnote do badge (`README.md:17-18`).

**Por que importa.** Consumidor público decide bump de dependência pelo número. "Patch" que muda assinatura quebra a expectativa de drop-in mesmo em 0.x.

**Correção.** Adicionar seção "Versioning" ao `CONTRIBUTING.md` (e 1 linha no README apontando pra ela) com a regra 0.x explícita, ex.:
> Enquanto 0.x: **minor** = qualquer adição ou mudança de API/ABI pública (método novo, header novo, assinatura alterada); **patch** = só correção de bug, doc, interno. Breaking intencional documentado na seção "Changed" do CHANGELOG com nota de migração. A partir de 1.0: SemVer estrito.

Não reescrever tags passadas (históricas, imutáveis); a política vale daqui pra frente. Registro do passado fica no próprio AUDIT_FIND.

### AUD-PUB-7 — Jargão GusWorld em doc pública

**Problemas exatos.**
1. `README.md:49` (seção "Features"): feature list usa `"brass screw-head" volume` como exemplo canônico do polygon — trocar por exemplo neutro ("e.g. a hexagonal badge or gauge fill").
2. `README.md:234-237` (EN, bullets "Delivered" de v0.2.5 a v0.4.0) e `:466-469` (PT, mesmos 4 bullets): cada item do roadmap "Delivered" embute o rationale nominal ("driven by GusWorld's hexagonal slider node" em `:235`, "GusWorld's unscrollable menu list" em `:237`). Mover o "porquê consumer-driven" pro CHANGELOG (onde já está em detalhe) e descrever a capability em termos do que ela faz pro leitor. Manter UMA menção honesta "battle-tested by a real consumer (GusWorld, an SDL3 game)" na seção de status — isso é marketing legítimo, o problema é o rationale por-item.
3. `docs/embed-integration.md:192-217` (a subseção "### Cockpit example", dentro da seção "6. Data model", indo até a excerpt `cockpit.rml`): a seção "Cockpit example" + `cockpit.rml` + "a verb menu driven by WASD, arrows, or gamepad" (seção "5. Focus navigation", `:132-145`, quote exata em `:135`) — renomear para um exemplo neutro ("Overlay panel example", `panel.rml`) mantendo GusWorld citado como primeiro consumidor real na introdução (`:3-5`), que é apropriado.
4. `docs/effects.md:87` (título "### How-to: a polygon with a gradient fill (e.g. a brass screw-head)") — opcional, baixa prioridade (é um how-to real e funciona).

**Não fazer:** não apagar o GusWorld da história do projeto (CHANGELOG e ADRs devem continuar citando — são registros de rationale). O alvo é só a doc de *uso* público.

### AUD-PUB-8 — Gaps de doc pública

1. **Packaging**: criar `docs/packaging.md` (ou seção no getting-started) com o fluxo completo `cmake --install glintfx/build --prefix ~/opt/glintfx` → `CMAKE_PREFIX_PATH` → `find_package(glintfx)`, o que é instalado, e a nota sobre RmlUi co-instalada.
2. **Tabela de compatibilidade** no README: RmlUi 6.3 (pinada via FetchContent — dizer explicitamente se outras versões são suportadas), OpenGL 3.3 core, clang (versão mínima; declarar se gcc é suportado — hoje não declarado em lugar nenhum), CMake ≥3.16, Linux x86-64 only.
3. **Troubleshooting**: `docs/troubleshooting.md` com os 4-5 erros mais prováveis (GLFW ausente; rodar sob llvmpipe e o card `mask`; `find_package` não acha; janela translúcida/compositor — este último já tem a explicação na memória do projeto, migrar pra doc pública).
4. **API reference**: na seção "Documentation" do README (`:203-209`), adicionar linha "API reference: the public headers under `glintfx/include/glintfx/` (fully doc-commented, bilingual)". Os headers JÁ são a referência (doc-comments completos verificados) — só falta dizer isso ao leitor. Doxygen fica adiado (modo pull).

### AUD-TEC-9 — `app.hpp` avulso em build embed-only

**Problema exato.** `glintfx/include/glintfx/glintfx.hpp:13` protege com `#if GLINTFX_BACKEND_GLFW` (guarda o `#include <glintfx/app.hpp>` da linha seguinte), mas quem inclui `<glintfx/app.hpp>` direto num build embed-only compila e falha no LINK com erro críptico (símbolos de `app.cpp`, que não foi compilado — o bloco `if(GLINTFX_BACKEND_GLFW)...endif()` de `glintfx/CMakeLists.txt:137-146`).

**Correção.** No topo de `app.hpp` (após o SPDX):
```cpp
#include <glintfx/config.hpp>
#if !GLINTFX_BACKEND_GLFW
#error "glintfx::App requires GLINTFX_BACKEND_GLFW=ON (embed-only build: use glintfx::UiLayer)"
#endif
```
Erro em compile-time com mensagem acionável em vez de link error.

### AUD-TEC-10 — Frescor do TODO.md (3 correções cirúrgicas)

Executar num único commit `docs(todo)` — o TODO.md é o arquivo canônico e as 3 divergências confundem qualquer planejamento:

1. **O texto "LW9 entregue e testada, pendente onda de verificação."** (parágrafo "Ordem recomendada" da seção Camada 1 do TODO.md, hoje ~l.55 @ `cfb68e9`) — trocar por `LW9 ✅ fechada (entregue+testada+auditada, v0.2.5).` — o parágrafo "Estado" da mesma seção (hoje ~l.53 @ `cfb68e9`) e a tabela (itens `L1.11-CLICKCB`/`L1.12-ELEMBOX`/`L1.13-VPORIGIN`, hoje ~l.75-77 @ `cfb68e9`, todos ✅+✓) já registram o fechamento; o texto é resíduo de estado anterior.
2. **O item `AUD-L1-PARSE`** (hoje ~l.86 @ `cfb68e9`) — os "2 achados reais" JÁ FORAM remediados (confirmado: o laço de premultiply usa `size_t pixel_count`, dentro da função de decode/premultiply de textura em `render_gl3.cpp:132-133`; `SECURITY.md` já não diz "0.1.x"). Trocar `**PRIORIDADE ALTA -- 2 achados reais.** [...] Achados: (1) **overflow inteiro** no bound `i < w*h` (`int`) no premultiply; (2) `SECURITY.md` desatualizado ("0.1.x suportada").` por `**Os 2 achados originais já foram remediados adiantados** (overflow `w*h` → `size_t pixel_count` em `render_gl3.cpp`; `SECURITY.md` atualizado). Resta a auditoria formal: fuzz (libFuzzer/AFL++), cap de dimensão (`STBI_MAX_DIMENSIONS`), DoS por alocação.` e rebaixar a Prioridade de Alta para Média (o que a mantinha Alta eram os achados abertos). Nota: o `SECURITY.md` voltou a defasar (0.3.x vs v0.4.0) — coberto por AUD-TEC-8 item 5.
3. **O item `L2-COMPONENTS`** (hoje ~l.106 @ `cfb68e9`) — o caminho `docs/superpowers/plans/2026-06-30-glintfx-v2-f2-components.md` **não existe no `main`** (verificado: `docs/superpowers/plans/` só tem 6 arquivos, nenhum f2; o plano vive só no branch `feat/v2-f2-components`). Trocar por `Plano pronto **no branch `feat/v2-f2-components`** (arquivo `docs/superpowers/plans/2026-06-30-glintfx-v2-f2-components.md`, não mergeado no `main`), branch parada`.

Opcionais (baixa prioridade, mesma sessão se barato): (a) anotar na convenção de frescor (parágrafo inicial "Convenção de frescor: implementação entregue → 🔍 Pendente verificação...", hoje ~l.5 @ `cfb68e9`) que na Camada 1 o review final do `internal-auditor` na entrega equivale à onda de verificação (é a prática real; a regra escrita só menciona `TST-*`/`AUD-*`); (b) `L1.9`/`L1.10` rotulados LW5 mas com pré-req `L2-EMBED` (V2W2) — anotar "(entregues pós-embed)" pra desfazer a inversão aparente.

### AUD-PUB-1 — `App` standalone não refaz layout em resize (bug real, mas AW4: decisão de forma)

**Problema exato.** `app.cpp:149-157` (corpo de `App::render`): `render()` lê o tamanho atual da janela e chama `engine.render_standalone(w,h)` (`engine.cpp:115`, início de `Engine::render_standalone`), que faz `begin_frame → Render → end_frame` — **nunca** `Context::SetDimensions` (que só existe em `Engine::set_viewport`, `engine.cpp:66`, jamais chamado pelo `App`). `App` não expõe `set_viewport` (confirmado: zero ocorrências em `app.hpp` fora de comentários). Resultado: janela GLFW redimensionável (default), mas o layout RmlUi congela nas dims do `AppConfig` — a UI não reflui, esticando/cortando.

**Por que importa.** É O caso de uso do consumidor "app desktop" — o tipo de consumidor público que o `App` existe pra servir. O GusWorld nunca tocou nisso (usa `UiLayer` + `set_viewport` manual).

**Correção proposta (apresentar 2 opções ao líder antes de implementar):**
- **Opção A (recomendada, automática):** em `App::render()`, cachear `(w,h)` no `Impl` e, quando mudar, chamar `impl_->engine.set_viewport(w,h)` antes de `render_standalone`. Zero mudança de API; o comportamento certo por default; ~5 linhas.
- **Opção B (explícita, paridade):** expor `void App::set_viewport(int w, int h)` e deixar o consumidor ligar no callback de resize — mas o `App` é dono da janela GLFW e não expõe callbacks de janela, então B sozinha é incompleta; B faz sentido apenas *junto* de A.
**Teste:** smoke GLFW=ON que cria `App` 400x300, força `glfwSetWindowSize` pra 800x600, roda 2 frames e verifica via `get_element_box` que um elemento `width: 100%` acompanhou (box.w ≈ 800·dp).

### AUD-PUB-2 — `App` sem injeção de input

**Problema.** `process_event`/`UiEvent` existem só no `UiLayer`. O `App` é um runner opaco: input vai por wiring GLFW interno não exposto. Consumidor standalone não consegue filtrar/interceptar input, injetar eventos sintéticos (testes, automação, acessibilidade) nem mapear atalhos próprios.

**Correção (decisão do líder, modo pull):** curto prazo, **documentar** no doc-comment da classe `App` que o input é gerenciado internamente (GLFW) e que hosts que precisam de controle de input devem usar `UiLayer`. Médio prazo (se demanda real): expor `App::process_event(const UiEvent&)` (o `Engine` já é compartilhado; o custo é pequeno) — registrar como semente na INBOX junto de AUD-PUB-6. **Não fazer:** não implementar já sem demanda (viola o modo pull do líder).

### AUD-PUB-3 — `enum Key` gamepad-shaped

**Problema exato.** `ui_event.hpp:19-32` (`enum class Key` completo): `enum class Key { None, Up, Down, Left, Right, Enter, Escape, Tab, Space, Backspace }` — o próprio comentário admite "navigation-oriented subset. Extend for text editing as needed". Faltam p/ app desktop: `Delete`, `Home`, `End`, `PageUp`, `PageDown`, F1-F12, letras/dígitos (atalhos Ctrl+C/V/Z). O mapeamento `to_rml_key` (`ui_layer.cpp:31-44`, função completa) já traduz pro RmlUi — os `KI_DELETE/KI_HOME/KI_END/KI_PRIOR/KI_NEXT/KI_A..` upstream existem, é só ampliar o enum + switch.

**Correção (aditiva, não-breaking):** ampliar `Key` com `Delete, Home, End, PageUp, PageDown` (bloco 1, barato e inequívoco) e avaliar com o líder se letras/dígitos entram já ou ficam como semente INBOX (text input real exigiria também evento `TextInput` com codepoint — escopo maior, claramente pull-based). **Teste:** casos novos em `ui_layer_events.cpp`.

### AUD-PUB-4 — Click callback insuficiente

**Problema exato.** `app.hpp:139` (`App::set_click_callback`) / `ui_layer.hpp:183` (`UiLayer::set_click_callback`): `set_click_callback(std::function<void(const char* element_id)>)` — sem botão, sem coordenadas, sem double-click; elementos sem id colapsam em `""` (`bootstrap.cpp:111`, dentro de `ClickEventListener::ProcessEvent`). Suficiente pro menu do GusWorld (todo botão tem id); insuficiente pra menu de contexto (right-click), double-click-para-abrir, ou listas geradas onde dar id a cada item é oneroso.

**Correção (decisão do líder — é a mudança de API mais visível):**
- **Opção A (aditiva):** manter o callback atual e adicionar `set_click_info_callback(std::function<void(const ClickInfo&)>)` com `struct ClickInfo { const char* id; int button; bool double_click; float x, y; }` (novo header ou dentro de `ui_event.hpp`).
- **Opção B (breaking na v0.5):** evoluir a assinatura existente. Dado o critério SemVer novo (AUD-PUB-9), B exigiria minor + nota de migração.
Registrar como semente INBOX se não houver demanda — mas o right-click é o gap com maior probabilidade de ser o próximo pedido de QUALQUER consumidor.

### AUD-PUB-5 — Contrato FBO 0 + `target_h` (documentar como decisão; feature já adiada)

**Problema.** `ui_layer.hpp:112` (assinatura de `UiLayer::set_viewport(x,y,w,h,target_h)`) + `ui_layer.cpp:167` (corpo dessa sobrecarga): `target_h` existe só pra converter origem top-left → bottom-up do `glViewport`, e o render mira FBO 0 incondicionalmente (limitação F1 já documentada no doc-comment de `UiLayer::render` em `ui_layer.hpp:143-151` e ADR-0008). A feature real (compor num FBO do host) já está adiada como **GAP-2b na INBOX** — correto pelo modo pull.

**Correção (só doc):** garantir que `embed-integration.md` declare a limitação em seção própria "Scope and limitations" (hoje está espalhada em doc-comment + ADR) e que o doc-comment de `set_viewport` diga explicitamente "if you composite into your own FBO, this parameter model does not apply — see GAP-2b". Nada de código.

### AUD-PUB-6 — Gaps de API genérica (REGISTRAR como sementes; não implementar)

Um consumidor genérico pediria, e o GusWorld nunca pediu (logo não existem): **(a)** `unload()`/`hide()`/`show()` — hoje a única forma de "tirar a UI da tela" é carregar outro `.rml`; **(b)** múltiplos documentos/overlay modal — `Bootstrap::Impl` guarda 1 doc, 1 contexto (campos `doc`/`ctx`, `bootstrap.cpp:73,213`); **(c)** eventos RmlUi além de click — `ClickEventListener` só escuta `EventId::Click` (registro em `Bootstrap::load`, `bootstrap.cpp:317`); sem change/submit/focus/blur, formulários são inertes pro host; **(d)** setters imperativos por id (`set_text`, `add_class`, `set_property`); **(e)** leitura de volta do data-model (`get_number/string/bool` — hoje write-only, two-way binding é invisível ao host); **(f)** `load_font` em runtime; **(g)** cursor callback (`SystemInterface::SetMouseCursor` não repassado); **(h)** clipboard callbacks.

**Correção:** adicionar à INBOX do `TODO.md` como sementes individuais no padrão do GAP-4 existente ("semente, sem demanda, NÃO urgente"), com uma linha cada. **Por quê registrar se é pull:** o roadmap público fica honesto (consumidores veem o que está no radar), e o custo é 8 linhas de INBOX. **Não fazer:** implementar qualquer um sem demanda confirmada — decisão canônica do líder de 2026-07-01 (pull incremental).

### AUD-TEC-6 — Dívida de forma da API (consolidar antes da v1.0)

**Problemas exatos (5 subitens):**
1. Quatro contratos de erro coexistem: `set_dp_ratio` inválido **loga e ignora** (`Engine::set_dp_ratio`, `engine.cpp:90-95`); `set_viewport` inválido é **no-op silencioso** (as 2 sobrecargas de `UiLayer::set_viewport`, `ui_layer.cpp:142,159`); `set_element_scroll_top` não-finito **retorna false** (`Bootstrap::set_element_scroll_top`, `bootstrap.cpp:377`); `set_number/string/bool/list` com chave errada é **void silencioso** (os 4 setters de `DataBinder`, `data_binder.cpp:108-146`).
2. `bind_*` retorna `bool` mas `set_*` retorna `void` — typo de chave em `set_number("hp_", x)` é indetectável.
3. Mutadores marcados `const`: `set_element_scroll_top(...) const` (`ui_layer.hpp:319`, `app.hpp:194`) e `scroll_element_into_view(...) const` (`ui_layer.hpp:227`, `app.hpp:164` — números corrigidos nesta revisão de higiene: a citação original apontava `app.hpp:194` para os dois métodos, mas essa linha é `set_element_scroll_top` nas duas classes; `scroll_element_into_view` está em `ui_layer.hpp:227`/`app.hpp:164`).
4. `enum class Key` (scoped) vs `enum Mod` unscoped despejando `Mod_None/Mod_Shift/...` em `namespace glintfx` (`ui_event.hpp:21` vs `ui_event.hpp:36`).
5. `UiEvent.button` é `int` mágico documentado por comentário (`ui_event.hpp:155`, campo `button` da struct `UiEvent`) enquanto `key` é enum; e `UiLayerConfig::logical_width/height` vs `AppConfig::width/height` pro mesmo conceito.

**Correção:** é um pacote **breaking** — não corrigir aos poucos. Propor ao líder um único milestone "API consolidation" (candidato natural: v0.5.0 sob o critério novo do AUD-PUB-9, ou v1.0): política de erro única (`bool` + log em nível warning), `set_*` → `bool`, remover `const` dos mutadores, `enum class Mod` com operadores bitwise, `enum class MouseButton`. Documentar a migração no CHANGELOG. **Não fazer:** não misturar com features — release só de consolidação.

### AUD-TEC-7 — Gaps de cobertura de teste

**Problemas exatos:**
1. **Data-model 100% não testado no caminho embed:** os 4 testes (`data_model_smoke/scalar/list/rebind_uaf`) usam `glintfx::App` e vivem dentro do bloco `if(GLINTFX_BACKEND_GLFW)...endif()` inteiro de `glintfx/tests/CMakeLists.txt:92-494` (verificado: l.92 abre o bloco, l.494 é o `endif() # GLINTFX_BACKEND_GLFW` que o fecha). **Sub-claim não confirmada nesta revisão de higiene** (sinalizada ao líder, não corrigida por falta de certeza): "o comentário nas linhas 6-11 do próprio arquivo já admite o skip silencioso do rebind no embed" — `tests/CMakeLists.txt:6-11` comenta sobre `GLINTFX_GOLDEN_TEST`, não sobre o rebind; o cabeçalho de `data_model_rebind_uaf.cpp` também não contém essa admissão. O mais próximo é o comentário sobre bind duplicado ser um "silent no-op on collision" em `data_model_rebind_uaf.cpp:6-8` — mecanismo do `Bind()`, não sobre a suíte ser pulada no embed. A API é anunciada como paridade (seção "Data-model API (T1) — parity with App", `ui_layer.hpp:322` — corrigido nesta revisão de `:321`, que é só o separador `// ---` imediatamente acima) e é exatamente a que o consumidor embed real usa — mas o caminho `UiLayer` nunca é exercitado. **Correção:** `data_model_embed_sanity.cpp` com `UiLayer` offscreen (usar `glintfx/tests/offscreen.hpp` como os demais sanity), registrado FORA do bloco GLFW (roda nos 2 configs): create → bind_number/string/bool/list → load → set_* → render → capture e assert de pixel/box.
2. **`set_bool` zero testes** (grep vazio em `tests/*.cpp`); `bind_bool` só no path de rejeição do `rebind_uaf`. **Correção:** adicionar bind_bool+set_bool com `data-if` no `data_model_scalar` e no novo teste embed.
3. **`App::run()` zero testes.** Aceitável (loop bloqueante), mas documentar no doc-comment que é a composição testada de `poll/update/render` — ou teste com fechamento programado da janela.
4. **2º `create_data_model` retorna false silencioso** (`data_binder.cpp:41`, dentro de `DataBinder::create` — `impl_->created` limita a 1 modelo). Não testado nem documentado. **Correção mínima:** documentar o limite no doc-comment de `create_data_model` (App e UiLayer) + assert do false no teste. Suporte a múltiplos modelos = semente INBOX (AUD-PUB-6).

### AUD-C0-1 — `memmove`: comparação relacional de ponteiros é UB estrito

**Problema exato.** `src/mem.c:86` (`if (d < s)`) e `:90` (`else if (d > s)`) — `<`/`>` entre ponteiros que não apontam pro mesmo objeto é UB (C23 §6.5.9). Funciona no x86-64 plano, mas o mandato do `AUD-SEC` (W12) é "ausência de UB" e este é o único UB conhecido restante nos módulos.

**Correção (3 linhas):**
```c
uintptr_t du = (uintptr_t)d, su = (uintptr_t)s;
if (du < su) { /* forward */ } else if (du > su) { /* backward */ }
```
(`uintptr_t` já existe em `include/types.h`.) Atualizar o doc-comment bilíngue do racional. **Anexar ao escopo do `AUD-SEC` no TODO.md.** Aproveitar pro follow-up já anotado no D1 (casos `dst==src` e `n==0` em `test_mem.c`).

### AUD-C0-2 — `syscall0/2/4/5`: código morto não testado

**Problema exato.** `src/syscall.asm` define 7 aridades; só `syscall1` (exit), `syscall3` (write/read) e `syscall6` (mmap) têm call-sites (grep confirmado: `syscall0/2/4/5` só aparecem em declarações de `include/syscall.h:15-20` — bloco com as declarações `syscall0` a `syscall5`, `syscall6` fica em `:21` — e comentários). Os reshuffles de registrador de `syscall4`/`syscall5` nunca são exercitados.

**Correção (decisão, apresentar ao líder):** **(a) podar** os 4 wrappers sem uso (YAGNI; ADR-0001 diz que adicionar de volta é barato) — recomendada pro corte `core-v0.1.0`, zera a superfície não-testada; ou **(b) testar** cada aridade contra um syscall real (ex.: `getpid`=39 pra `syscall0` — exigiria adicionar a constante). **Anexar ao `AUD-ABI` no TODO.md.**

### AUD-C0-3 — `mini_printf` (stdout) sem gate automatizado

**Problema exato.** `src/printf.c:265-309` (`mini_flush` em `:265-274`, `mini_flush_sink` em `:276-293`, `mini_printf` em `:295-309` — o laço de flush pra runs >256B e a soma de `total`) não tem NENHUM teste: `tests/test_printf.c` cobre só `mini_vsnprintf` (o próprio cabeçalho do teste admite, linhas 4-11: o harness só checa exit code, não captura stdout; a validação foi um `od -c` manual fora do CI). É a única lógica de laço não-trivial do arquivo.

**Correção.** O item `TST-INT` (W11) já promete E2E com stdout golden — **nomear explicitamente** no seu escopo: "programa que emite via `mini_printf` uma saída >256 bytes (força ≥2 flushes) comparada byte a byte com golden file". Sem isso o TST-INT pode passar sem nunca tocar o flush loop.

### AUD-C0-4 — ADR-0002 promete `IS_ERR`/`ERR_CODE` inexistentes

**Problema exato.** `docs/adr/0002-error-contract.md:21` (seção "Consequences / Consequências"): "we'll provide small helpers/macros (e.g. `IS_ERR(x)`, `ERR_CODE(x)`) ... Error codes live in our own header". Grep em `include/ src/ tests/` = zero ocorrências. Cada call-site aplica o contrato `-errno` ad-hoc (`alloc.c:140-142`, função `mmap_failed`, checa faixa de ponteiro; `tests/echo_stdin.c:44` checa `n<0` implícito no `else` final). Sem bug funcional, mas o ADR ratificado ("one-way-door") diverge da realidade — ruído permanente se a tag sair assim.

**Correção (decisão, 2 opções):** **(a)** emendar o ADR-0002 com uma nota datada "helpers adiados por YAGNI; cada call-site checa inline até haver 3+ consumidores do padrão" (recomendada — coerente com o tom do projeto); ou **(b)** criar `include/errcode.h` com os macros + constantes usadas e refatorar os 2 call-sites. Resolver **antes do `REL-TAG`**.

### AUD-C0-5 — Testes menores faltantes (anexar às ondas TST-*)

1. Guarda de overflow do `malloc` (`alloc.c:167`, dentro de `malloc()` — verificada correta na leitura, mas o próprio doc-comment do arquivo pede re-verificação pro AUD-SEC em `alloc.c:82` PT / `:40` EN): adicionar `TEST_ASSERT(malloc((size_t)-1) == NULL);` em `test_alloc.c`.
2. `realloc` shrink (ramo `copy_size = (old_size < size) ? old_size : size` de `alloc.c:231`, dentro de `realloc()`): caso `realloc(p_64, 8)` preservando os 8 primeiros bytes.
3. Caminho de FALHA do harness não versionado: a prova de que `TEST_ASSERT` aborta com `FAIL: file:line` correto é uma "sonda descartável nunca commitada" (parágrafo PT do cabeçalho de `tests/selftest.c:28-31`; equivalente EN em `:12-16`). Criar alvo `make test-negative` que builda uma sonda que DEVE sair 1 com stderr casando golden — fecha a única parte do C1 sem regressão. Candidato a subitem do `TST-INT`.

### AUD-C0-6 — Housekeeping

1. O item B1 do TODO.md (hoje ~l.15 @ `cfb68e9`) lista "`bool`" como entregável de `types.h`, mas o doc-comment de `include/types.h:4-14` documenta a omissão deliberada (keyword C23). Corrigir o texto do B1 pra "(`bool` = keyword C23, não redefinido)".
2. `out.ppm` (1.6 MB) commitado na **raiz** — artefato de render (leftover de demo/snapshot). Propor ao líder: remover do tracking + adicionar `*.ppm` da raiz ao `.gitignore` (verificar antes se algo o referencia — grep rápido em docs). Não deletar sem confirmação (regra de reversibilidade).

---

## 4. Plano de reorganização de memória (Escopo 2)

Base factual usada no cross-check: repo em **v0.4.0** (10 tags), suíte **31 GLFW=ON / 16 embed**, Camada 0 completa. O `CLAUDE.md` do projeto e o `AGENTS.md` estão CORRETOS e atuais — todo o trabalho é na camada de memória tipada. Agrupado por arquivo-alvo; executável por um agente Sonnet sem redescobrir contexto. `LOCAL/` = `/home/petrus/.claude/projects/-home-petrus-IDrive-Documentos-projetos-claudebrain-Projects-loucura-c-asm/memory/`.

### AUD-MEM-1 (Alta) — Fundir os dois snapshots de estado num só, atualizado

**Arquivos:** `LOCAL/project_session_atual.md` + `LOCAL/project_v2_roadmap_state.md` → **novo** `LOCAL/project_estado.md` (deletar os dois antigos).

**Problema:** são dois retratos do mesmo objeto, ambos stale de formas diferentes e um auto-contraditório: `project_session_atual.md` parou em 2026-07-01 ("6 tags v0.1.0→v0.2.4", "suíte 18/18", "aguardando decisão entre 3 caminhos" — tudo superado); `project_v2_roadmap_state.md` tem 3 estratos conflitantes (frontmatter "v0.2.5"; topo 2026-07-04 "v0.3.0, 8 tags, Camada 0 COMPLETA"; corpo 2026-07-02 "7 tags, próximo passo B6, líder PAUSOU" — a mesma Camada 0 completa E pausada no mesmo arquivo).

**Execução:** criar `project_estado.md` com frontmatter (`name: project-estado`, `type: project`) e seções: **Estado atual** (checkpoint 2026-07-07: glintfx v0.4.0, 10 tags, 31/16 testes, Camada 0 implementação completa aguardando ondas TST-*/AUD-*/REL-TAG `core-v0.1.0`; F2/v2 em modo pull, branch `feat/v2-f2-components` parada); **Próximo passo / decisões abertas** (aguardar demanda GusWorld ou puxar `AUD-L1-MEM`; achados desta auditoria em `AUDIT_FIND.md`); **Armadilhas** (migrar as ainda-válidas do `project_session_atual.md`: git-em-IDrive com refs defasadas; e a regra "contagem de testes cita-se por tag, não como fato perene — fonte é o CHANGELOG"). Deletar `project_session_atual.md` e `project_v2_roadmap_state.md`.

### AUD-MEM-2 (Alta) — Corrigir o índice `MEMORY.md` local

**Arquivo:** `LOCAL/MEMORY.md`.
1. A linha do `project_v2_roadmap_state.md` ecoa estado duplamente velho ("checkpoint 2026-07-03: glintfx até **v0.2.5** (7 tags...); pausado antes do B6"). Substituir (junto com AUD-MEM-1) por: `- [Estado do projeto](project_estado.md) — checkpoint vivo: glintfx v0.4.0 (10 tags), Camada 0 completa aguardando ondas TST/AUD.`
2. A linha 3 manda "ler antes de agir" o `project_session_atual.md` — repontar para `project_estado.md`.
3. Ajustar as demais linhas do índice conforme AUD-MEM-4/5/6 (arquivos promovidos/enxugados).

### AUD-MEM-3 (Alta) — Disciplina de push: gap + 2 memórias que dizem o CONTRÁRIO

**Problema:** a disciplina real da sessão (nunca push/merge/tag sem pedido explícito do líder) não está registrada em lugar nenhum, e duas memórias a contradizem ativamente: `LOCAL/feedback_uso_agentes_bigtech.md` linha 17 ("O orquestrador só faz: coordenar ... commit/push (git) ...") e `LOCAL/feedback_cadencia_releases_consumidor.md` linha 20 ("merge + tag patch + push nos 2 remotes" como fluxo normal).

**Execução:** (1) adicionar bullet ao apêndice do `~/.claude/CLAUDE.md` global (é disciplina git universal): `- **Push só com pedido explícito (2026-07-07)**: commit local é livre; push, merge em main, tag e qualquer ação visível em remoto SÓ com autorização explícita do líder naquele contexto (aprovação anterior não vale pra sempre). Vale pra orquestradores e agents.` (2) Em `feedback_uso_agentes_bigtech.md`, trocar "commit/push (git)" por "commit local (git); push só com autorização explícita do líder". (3) Em `feedback_cadencia_releases_consumidor.md`, anotar na frase do fluxo: "merge + tag + push **mediante aprovação do líder no momento**".

### AUD-MEM-4 (Média) — Promover a global: auditoria-dominó + implementer≠reviewer + re-verificação do orquestrador

**Problema:** `LOCAL/feedback_auditoria_domino.md` descreve método universal (nada é específico de C++/glintfx); o implementer≠reviewer (em `feedback_uso_agentes_bigtech.md` linha 18) idem; e a prática do orquestrador re-verificar o relatório de agents antes de aceitar está só difusa.

**Execução:** (1) adicionar bullet ao apêndice do `~/.claude/CLAUDE.md`: `- **Auditoria-dominó + verificação em camadas (2026-07-07)**: achado de robustez/input nunca é assumido isolado — perguntar "isolado ou padrão?" e varrer a superfície inteira read-only antes de fechar (procurar o gêmeo: "esse guard existe em algum irmão e faltou aqui?"). Validar float antes de cast. Implementer e reviewer são agents DIFERENTES; review adversarial executa código ("com dente"). O orquestrador re-verifica o entregável do agent (build limpo, spot-check das claims) antes de aceitar — relatório de agent não é prova.` (2) Encolher `LOCAL/feedback_auditoria_domino.md` para a instância local (o CRITICAL `load(nullptr)` como exemplo validado + os padrões específicos do glintfx: expansão de escopo via AskUserQuestion, serializar agents no mesmo arquivo) com link `[[...]]` pro bullet global. (3) Atualizar a linha correspondente do `LOCAL/MEMORY.md`.

### AUD-MEM-5 (Média) — Promover o núcleo genérico da cadência consumer-driven

**Arquivo:** `LOCAL/feedback_cadencia_releases_consumidor.md`.
**Problema:** mistura princípios genéricos de lib-com-consumidor (achado do consumidor → patch taggeado imediato; feature confirmada pelo consumidor não é over-engineering; desconfiar de fórmula de conversão sem teste de delta) com specifics GusWorld (relay, tags).
**Execução:** bullet global no apêndice: `- **Cadência consumer-driven de lib (2026-07-07)**: achado de consumidor real vira patch/release taggeada imediata (anti-retrabalho, não acumular); feature confirmada pelo consumidor não é over-engineering; fórmula de conversão (coordenada/sinal/unidade) sem teste de delta que a exercite é a classe de bug silencioso que só aparece em produção.` Manter o arquivo local só com o protocolo de relay GusWorld + histórico. Atualizar índice.

### AUD-MEM-6 (Média) — Enxugar duplicações de regra-de-repo (memória deve guardar o não-versionado)

1. **`LOCAL/feedback_idioma_codigo_docs.md`**: duplica verbatim a seção "Convenções" do `CLAUDE.md` do projeto e o `AGENTS.md`. Reduzir a stub de 2 linhas ("Regra canônica na seção Convenções do CLAUDE.md do projeto e no AGENTS.md; esta memória só marca que a decisão foi do líder em [data]") ou deletar + remover do índice. Recomendo o stub (preserva a proveniência).
2. **`LOCAL/feedback_uso_agentes_bigtech.md`**: a regra-mãe vive no global (bullet 2026-06-10), no CLAUDE.md do projeto e no AGENTS.md. Manter APENAS o que é local e não-versionado: o roteamento por-agente validado no glintfx (implementer→`backend-engineer`, reviewer→`tech-lead`, não usar `feature-dev:code-reviewer`) e o histórico das derrapadas de 2026-06-29 — aplicando também o ajuste de push do AUD-MEM-3.
3. **`LOCAL/project_escopo_stack.md`**: a tabela de stack duplica (incompleta) a do `CLAUDE.md` do projeto. Manter só o não-versionado: governança bigtech "cenário B / porte early com eixo técnico elevado" + histórico de decisão; o resto vira referência de 1 linha ao CLAUDE.md.
4. **`LOCAL/reference_glintfx_ci.md`**: manter (agrega valor específico: deps `-dev` exatas, xvfb, limitação de logs da API Codeberg) — só adicionar no topo cross-ref explícita à memória global `codeberg-runners.md`/`forgejo` pra regra runner/container. **`LOCAL/reference_rmlui_gl3_integracao.md`**: manter como está (carrega o *porquê* de debugging que o AGENTS.md não tem); adicionar 1 linha "detalha os Gotchas críticos do AGENTS.md".

### AUD-MEM-7 (Média) — Gaps novos a criar

1. **Recuperação pós-crash (global, gap total):** bullet no apêndice do `~/.claude/CLAUDE.md`: `- **Crash ≠ descarte (2026-07-07)**: após crash de sessão/processo com trabalho in-flight, PRIMEIRO recuperar do disco o que já foi escrito (arquivos salvos, stash, reflog) e só então decidir refazer — nunca descartar/regenerar sem inventariar o que sobreviveu.`
2. **Disco/builds scratch (estender arquivo global existente):** `~/.claude/memory/feedback_hardware_resource_limits.md` cobre CPU/RAM/GPU mas não disco. Adicionar seção: builds scratch/experimentais vão pra `/tmp` (tmpfs — não persistir artefato que importa) ou pro scratchpad da sessão; atenção à cota (builds C++ com deps fetchadas passam de GB); limpar `build_*` experimentais ao fim da sessão. Nota local relacionada: este repo tem `glintfx/build_asan/` e `glintfx/build_embed/` untracked na árvore — candidatos a mover pra /tmp no futuro (não tocar agora).

### AUD-MEM-8 (Baixa) — Wikilinks hífen vs underscore

Nos corpos das memórias locais, wikilinks usam hífen (`[[project-v2-roadmap-state]]`) enquanto os arquivos usam underscore (`project_v2_roadmap_state.md`). O índice em si está íntegro (10 links → 10 arquivos existentes, zero órfãos — verificado). Ao executar AUD-MEM-1..6, padronizar os wikilinks tocados para o slug do frontmatter `name:` de cada arquivo (que é o alvo canônico de resolução).

---

## 5. Plano de Transição — Camada 1 rodando sozinha sobre a Camada 0 (`AUD-TRANS-*`)

> **Natureza deste capítulo: PLANO, não execução.** Nenhum código muda agora. É o detalhamento
> técnico do épico `L1-INTERNALIZE` (LW8, item `L1-INTERNALIZE` do TODO.md, hoje ~l.92 @ `cfb68e9` — "independência clean-room das libs
> userspace (RmlUi/gl3w/FreeType/GLFW); fronteira irredutível = libGL/driver/kernel") pra dar ao
> líder base de decisão sobre cronograma e sequência. Sem prazo, como o CLAUDE.md registra.

### 5.0 Fotografia do acoplamento hoje (verificada no código)

| Dependência | Como entra | Pontos de acoplamento reais | Já temos substituto parcial? |
|---|---|---|---|
| **RmlUi 6.3** | FetchContent pinada por hash (`glintfx/CMakeLists.txt:89-91`, tag `2cd28864...`) | Motor inteiro: parser RML/RCSS, cascade, layout, data-binding, decorators, eventos. Nossas implementações das interfaces dele: `FileInterface` (`base_url_file_interface.hpp:32`), `SystemInterface` (`system_clock.hpp:13` — só relógio, via `std::chrono`) | Parcial: 2 das 4 interfaces plugáveis já são nossas; o `RenderInterface` NÃO (ver abaixo) |
| **gl3w** | Vendorizado (`glintfx/third_party/gl3w`), usado em 6 TUs (`window_glfw`, `gl_state`, `engine`, `app`, `ui_layer`, `render_gl3`) | Loader de ponteiros de função OpenGL 3.3 core (tabela + `dlopen`/GetProcAddress) | Não — mas é código gerado, o menor alvo |
| **FreeType** | `find_package(Freetype REQUIRED)` (`CMakeLists.txt:80`), "always present" — **consumida só VIA RmlUi** (`RMLUI_FONT_ENGINE "freetype"` forçado em `CMakeLists.txt:88`) | Rasterização de fontes do font engine default do RmlUi. Nenhum `#include <ft2build.h>` nosso — o acoplamento é 100% indireto | Não — mas o RmlUi aceita font engine custom (`FontEngineInterface`), interface de plugin pronta |
| **GLFW** | Só com `GLINTFX_BACKEND_GLFW=ON` (default): janela+contexto+input do `App` standalone (`window_glfw.cpp`) | Janela, contexto GL, eventos de input, swap | **Sim**: o build embed-only (`OFF`) já elimina GLFW por completo — o host é dono de janela/input (ADR-0008) |
| **Renderer de efeitos GL3** | `render_gl3.hpp:2`: "Thin RAII wrapper around RmlUi's `RenderInterface_GL3`" | Os efeitos (glow/blur/mask/gradiente) são shaders DO RmlUi, não nossos | Não — nota importante: o "nosso renderer" hoje é o deles embrulhado |
| **libstdc++/libc/libm** | Toolchain (std::function/vector/chrono, `<cmath>`) | Todo o corpo C++ da Camada 1 | Camada 0 tem mem/str/conv(int)/printf-mini/alloc-bump — núcleo C, não C++ |
| **Mesa/libGL + driver + kernel** | `find_package(OpenGL)` | Execução GL de verdade | **Fronteira irredutível** — não é alvo (já registrado no item `L1-INTERNALIZE` do TODO.md, hoje ~l.92 @ `cfb68e9`) |

Duas consequências dessa fotografia que moldam o plano inteiro:
- **A Camada 1 não fala com FreeType nem com metade do RmlUi diretamente** — fala com *interfaces plugáveis* do RmlUi. Isso significa que a internalização pode avançar **por trás das interfaces, sem tocar a API pública do glintfx** (a suíte — 31 testes GLFW=ON / 16 embed — continua sendo o contrato).
- **O renderer de efeitos — a alma do produto — hoje É o do RmlUi.** Qualquer fantasia de "internalizar o RmlUi por último porque só usamos o layout" está errada: o `RenderInterface_GL3` deles é parte do que teria de ser reimplementado (clean-room, MIT→MPL ok, mas sem copiar).

### 5.1 AUD-TRANS-5 — A decisão de fronteira VEM PRIMEIRO (pergunta aberta pro líder)

**Problema.** O ADR-0006 fixa: Camada 0 e Camada 1 **não se linkam** — coexistem no repo, a fronteira é o processo (o CLAUDE.md do repo resume: "a fronteira é o processo, não o linker"). Esse modelo é ótimo pra hoje, mas é **incompatível com qualquer internalização real**: no momento em que um `memcpy` ou um alocador da Camada 0 servir a Camada 1, ou as camadas passam a se linkar, ou a Camada 0 vira serviço em outro processo. Decidir isso ANTES de escrever qualquer código de internalização é obrigatório (one-way-door; o ADR-0006 é `Accepted` e teria de ser sucedido por um ADR-0009).

**Opções que enxergo (NÃO decidir sem o líder — apresentar via AskUserQuestion quando a trilha ativar):**

- **Opção A — Link estático incremental (a Camada 0 vira lib).** A Camada 0 ganha um alvo `libcore.a` (símbolos com prefixo próprio, ex. `glx_memcpy`, pra não colidir com a libc que continuará no processo pelo lado do C++). O glintfx passa a consumir peças dela explicitamente (`#include <core/...>`), começando pelo alocador e utilitários. *Prós:* caminho incremental natural, cada peça migra sozinha, testável passo a passo, sem custo de IPC. *Contras:* o processo fica "misto" (freestanding + hosted convivendo); a pureza zero-libc da Camada 0 deixa de ser observável via `strace` do binário final; exige disciplina de prefixo pra evitar colisão de símbolo. **Minha recomendação técnica** (é a única que permite progresso composto sem big-bang) — mas é recomendação, não decisão.
- **Opção B — Process boundary mantida (Camada 0 vira serviços via IPC).** Peças internalizadas rodam como processos soberanos (ex.: rasterizador de fontes servindo glyphs por shm+pipe). *Prós:* ADR-0006 intacto, isolamento total, a pureza continua auditável por processo. *Contras:* latência e complexidade de IPC num caminho quente de render (glyph atlas por frame é inviável por pipe sem cache agressivo); serialização de contratos; overkill claro pra `memcpy`/alloc. Só faz sentido pra peças grandes e batch (fonte, talvez decode de imagem).
- **Opção C — Clean-room dentro da Camada 1, soberania em fase 2.** As reimplementações nascem como código novo C++ dentro do glintfx (sem libc-avoidance no início), substituindo as libs externas 1:1; a migração "pra cima da Camada 0" fica pra uma fase posterior, quando a Opção A/B for decidida. *Prós:* desacopla "eliminar a dep externa" (valor imediato: menos supply-chain, menos NOTICE, controle total) de "zero-libc" (valor filosófico de longo prazo); menor risco. *Contras:* a Camada 0 fica de fora da festa por mais tempo — a "loucura" vira duas etapas.

**Instrução pro executor (quando o líder ativar):** redigir `docs/adr/0009-internalization-boundary.md` no template dos ADRs existentes (bilíngue, Status/Context/Decision/Consequences/Options considered), apresentando exatamente estas 3 opções com os trade-offs acima, e colher a decisão do líder via AskUserQuestion (recomendada primeiro = A, com C como alternativa de menor risco). Nada de código antes desse ADR.

### 5.2 AUD-TRANS-0 — Pré-requisitos na Camada 0 (proposta de onda nova `SOV-*`)

O que a Camada 0 (ou a camada intermediária, conforme AUD-TRANS-5) precisa ter ANTES de cada internalização fazer sentido. Proposta de itens novos pro `TODO.md` (padrão dos existentes; pré-requisito comum: `REL-TAG core-v0.1.0` fechado — não empilhar escopo novo sobre implementação ainda não auditada):

| Item proposto | O quê | Desbloqueia | T-shirt |
|---|---|---|---|
| `SOV-ALLOC` | Alocador geral com `free` real (free-list/segregated fit sobre `mmap`; o E1 bump é por design sem free — ADR-0004). UI de vida longa aloca/libera milhões de vezes | TRANS-2, TRANS-4, qualquer workload persistente | M |
| `SOV-FCONV` | float↔string (`%f`/`%g` no mini-printf; parse de float pra RCSS futuro). Hoje `conv.c` é só int | TRANS-4 (parser RCSS); melhora E2 já | M |
| `SOV-MATH` | libm-núcleo: `sqrt`, `cos`/`sin`, `floor`/`ceil`, `fabs`, `pow` (rasterização de curvas de fonte, geometria de efeitos) | TRANS-2 (bezier), TRANS-4 | M |
| `SOV-SYS2` | Syscalls novos + wrappers: `clock_gettime` (substitui `std::chrono` do `SystemClock`), `munmap` (par do `mmap` já existente), `socket`/`connect`/`sendmsg`/`recvmsg` (X11/Wayland wire — só se TRANS-3 for ativado), `futex` (só se threads) | TRANS-3; relógio próprio é quick-win isolado | P (relógio) a M (sockets) |
| `SOV-CXXRT` | Runtime C++ freestanding mínimo: `operator new/delete` sobre `SOV-ALLOC`, guards de static init, política `-fno-exceptions -fno-rtti` | Só na Opção A do TRANS-5; na C, desnecessário | G |

**Instrução pro executor:** quando o líder ativar a trilha, adicionar estes itens à seção Camada 0 do `TODO.md` como onda pós-`REL-TAG` (não executar nenhum antes do ADR-0009). `SOV-ALLOC` e `SOV-FCONV` têm valor próprio pra Camada 0 mesmo que a transição nunca ande — podem ser puxados antes se o líder quiser.

### 5.3 AUD-TRANS-1 — gl3w primeiro (P): o marco que prova o processo

**Por que primeiro:** é a menor peça (loader de ~600 ponteiros de função GL, código *gerado*), o contrato é mecânico (carregar símbolos, zero lógica), o teste de aceite já existe (a suíte inteira roda em cima de GL), e o exercício estabelece o *processo* clean-room (estudar → especificar → reimplementar sem copiar) com risco quase nulo antes de apostá-lo em peças grandes.

**O que é tecnicamente:** gl3w = um script que lê o registro público Khronos (`gl.xml`, o espec das APIs GL) e gera (a) declarações dos ponteiros de função GL 3.3 core, (b) um `gl3wInit()` que resolve cada símbolo via `glXGetProcAddress`/`eglGetProcAddress`/`dlopen("libGL.so")`.

**Plano de execução (pro Sonnet, quando ativado):**
1. Clean-room: NÃO ler o código do gl3w vendorizado durante a escrita (ele fica como oráculo de teste apenas). Fonte de verdade = `gl.xml` do registro Khronos (Apache-2.0, uso pra geração é prática padrão da indústria) + a doc pública de `glXGetProcAddress`. Os headers `glcorearb.h`/`khrplatform.h` (MIT Khronos, já vendorizados e atribuídos no NOTICE) podem ser mantidos.
2. Escrever `tools/gen_glloader.py` (ou C) que emite `glintfx/src/gl_loader.{h,c}` com o subconjunto GL 3.3 core que o repo realmente usa (grep dos símbolos `gl[A-Z]` em `glintfx/src/` dá a lista mínima; gerar o core profile completo também é aceitável).
3. Trocar `#include <GL/gl3w.h>` nos 6 TUs pelo header novo; remover `third_party/gl3w` do `CMakeLists.txt` e do `NOTICE` (a entrada gl3w sai; a dos headers Khronos fica).
4. Aceite: suíte completa verde nos 2 configs (GLFW=ON e OFF) + `ldd`/`nm` confirmando que nada mudou no perfil de link além do esperado. Registrar no CHANGELOG como minor (critério AUD-PUB-9).
**O que NÃO fazer:** não tentar eliminar `libGL` — o loader continua resolvendo símbolos da Mesa; a soberania aqui é sobre o *loader*, não sobre o driver (fronteira irredutível).

### 5.4 AUD-TRANS-2 — FreeType via `FontEngineInterface` (G): a peça com porta pronta

**Por que segundo:** é a única dependência com **interface de plugin oficial** no RmlUi — nosso CMake já força `RMLUI_FONT_ENGINE "freetype"` (`CMakeLists.txt:88`); trocar por `"none"` + registrar `Rml::FontEngineInterface` própria é um seam desenhado pra isso. A API pública do glintfx não muda em nada; os testes visuais existentes (`render_sanity`, capturas) são o aceite.

**Escopo técnico da reimplementação clean-room (specs públicas: OpenType/TrueType da Microsoft/Apple, sem ler código FreeType):**
1. Parser SFNT: tabelas `cmap` (char→glyph), `glyf`/`loca` (outlines quadráticos), `head`/`hhea`/`hmtx` (métricas), `kern`/`GPOS` básico (kerning — v2).
2. Rasterizador de outline: scanline com cobertura fracionária (anti-aliasing por área), curvas quadráticas achatadas por subdivisão. **Sem hinting na v1** (o interpretador de bytecode TrueType é o que separa G de GG; em tamanhos de UI ≥12px sob AA, a perda é aceitável — mesmo trade-off do stb_truetype).
3. Implementar `FontEngineInterface`: carregar face, gerar glyphs sob demanda pro atlas do RmlUi, métricas de linha.
4. Pré-requisitos: `SOV-ALLOC` + `SOV-MATH` se a fronteira for Opção A; nenhum se Opção C (usa new/cmath normais).
**Riscos a registrar:** fontes CFF/OTF (outlines cúbicos PostScript) ficam fora da v1 — documentar "TTF only"; cobertura Unicode via `cmap` format 4/12 apenas; teste com a Open Sans já embarcada (NOTICE) + golden opt-in em GPU real.
**Aceite:** suíte verde com `RMLUI_FONT_ENGINE=none` + engine próprio; screenshot diff tolerado (AA diferente do FreeType é esperado — atualizar goldens conscientemente).

### 5.5 AUD-TRANS-3 — GLFW: recomendação de NÃO internalizar (registrar, não executar)

**Recomendação técnica (não decisão):** adiar indefinidamente, possivelmente pra sempre. Razões: (1) o **build embed-only já entrega o produto sem GLFW** — o caminho de menor dependência existe desde a v0.2.1 e é o que o consumidor real usa; (2) reimplementar janela+input+contexto é falar X11 wire *e* Wayland wire *e* EGL/GLX — G de esforço pra substituir a peça com **menor valor de soberania** (GLFW é zlib, minúscula, estável, trocável por SDL); (3) o risco de regressão em matriz de compositors/drivers é o mais alto de todas as peças. Se um dia o `App` standalone precisar de soberania de janela, o degrau honesto é **Wayland-only via wire protocol por socket Unix** (`SOV-SYS2`) + EGL — X11 nunca. Registrar essa recomendação no corpo do `L1-INTERNALIZE` quando o TODO.md for tocado (AW3), pra decisão futura do líder.

### 5.6 AUD-TRANS-4 — RmlUi: o endgame, por subsistema, de fora pra dentro

Não é "uma internalização", é um programa de anos. Sequência que minimiza risco (cada passo mantém a suíte como contrato):
1. **`RenderInterface` de efeitos próprio** — reimplementar clean-room o pipeline de efeitos (glow/blur/mask/gradiente/backdrop) que hoje é o `RenderInterface_GL3` do RmlUi embrulhado (`render_gl3.hpp:2`). É a alma do produto e o maior valor de diferenciação; shaders GLSL próprios, mesma semântica RCSS. T-shirt: G isolado.
2. **Font engine** = AUD-TRANS-2 (já coberto).
3. **Core** (parser RML/RCSS, cascade/specificity, layout block/inline/flex, data-binding, eventos) — GG absoluto; só ativar com Camada 1 madura (API consolidada pós AUD-TEC-6, v1.0+), consumidores múltiplos e a suíte muito maior que os 31 testes de hoje (o contrato comportamental precisa ser denso o bastante pra validar um motor de layout novo).
**Pré-condição transversal:** manter a regra do repo — upstreams em `examples/` (gitignored) só pra estudo/RE conceitual; **reimplementação sem cópia** (base MPL-2.0 não contaminada, ADR-0006/0007).

### 5.7 Ordem de prioridade consolidada (risco×valor) e o que NÃO internalizar

1. **AUD-TRANS-5** (ADR-0009, decisão de fronteira) — bloqueia tudo; custo baixo; fazer primeiro.
2. **AUD-TRANS-1** (gl3w, P) — quick win, prova o processo, remove 1 item do NOTICE.
3. **`SOV-ALLOC`/`SOV-FCONV`** (Camada 0, M cada) — valor próprio independente da transição; podem correr em paralelo pós-`core-v0.1.0`.
4. **AUD-TRANS-2** (FreeType, G) — maior razão valor/esforço entre as grandes (porta de plugin pronta).
5. **AUD-TRANS-4.1** (RenderInterface de efeitos, G) — alto valor de produto; após 2.
6. **AUD-TRANS-4.3** (RmlUi core, GG) — endgame, sem prazo.
- **Não internalizar:** Mesa/libGL/driver/kernel (irredutível — a analogia é a mesma da regra local-first do líder: o driver de GPU é a "inferência" desta stack); **GLFW** (recomendação 5.5); **libstdc++** enquanto a fronteira não for decidida (reimplementar STL é não-objetivo — na Opção A entra só o `SOV-CXXRT` mínimo).

---

## 6. Apêndice — fora de escopo / adiados

1. **`glintfx/build_asan/` e `glintfx/build_embed/` untracked na raiz do glintfx** (git status inicial): builds scratch de sessões anteriores. Não tocados (podem conter estado útil); ver AUD-MEM-7.2 pra política futura.
2. **INBOX itens não-endereçados confirmados ainda válidos** (nenhuma ação além do que já está registrado): 2ª causa UBSan `WidgetScroll` (supressões presentes em `ubsan_suppressions.txt`, reverificação no CI real pendente); jitter ~3px em `overflow-y:auto` (RmlUi upstream); `libxkbcommon-dev` no workflow GitHub; em-dashes em `ui_layer.hpp`.
3. **CI Codeberg "waiting" na tag v0.3.1** (INBOX): não verificado nesta auditoria (exigiria acesso ao painel Codeberg; a API não expõe logs — ver memória de CI). Continua como item de infra na INBOX.
4. **`consumer-example/`**: não auditado em profundidade contra a API v0.4.0 (o capítulo de docs confirmou que existe e é citado no README); se os snippets pinarem tag antiga, a correção do AUD-TEC-8 item 3 deve varrê-lo junto (`grep -rn "GIT_TAG\|v0\.[0-9]" consumer-example/`).
5. **Convergência de contagem de testes**: CLAUDE.md do repo, AGENTS.md, README e CHANGELOG estão TODOS corretos em 31/16 — a única fonte errada era a memória (AUD-MEM-1/2). Registro positivo: a disciplina de higiene de docs versionadas do repo funcionou; a de memória não acompanhou.
6. **Positivos dignos de nota** (pra não auditar só o defeito): encapsulamento da fronteira pública impecável (zero vazamento de tipo de terceiro; pImpl consistente); doc-comments bilíngues completos em 100% da API pública; SPDX em 100% dos arquivos de código das duas camadas; Camada 0 sem nenhum bug funcional de corretude nos pontos clássicos (INT_MIN, saturação, overlap, promoções variádicas); harness próprio da Camada 0 coerente com o Makefile.
