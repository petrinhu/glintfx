#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
#
# EN: RMLX-0/F4 -- RmlUi whitelist gate. Guards the confinement F1-F3 already bought:
#     with `#include <RmlUi/...>` moved into `glintfx/src/rml/` (F1) and `engine.cpp`/
#     `ui_layer.cpp` weaned off RmlUi entirely (F2/F3), nothing stops a NEW file from
#     reintroducing the include tomorrow and silently undoing three fatias of work. This
#     script is that stop.
#
#     Four checks, run in order (a/b/c blocking, d report-only):
#       (a) any `#include` of RmlUi in glintfx/src/, glintfx/include/, glintfx/demos/
#           OUTSIDE glintfx/src/rml/ -> FAIL (file:line).
#       (b) any `#include` of RmlUi in glintfx/tests/ outside the FROZEN whitelist of 4
#           files (domrw_sanity.cpp, focus_sanity.cpp, form_events_sanity.cpp,
#           document_reload_leak.cpp) -> FAIL. Caps the pre-existing test debt: a NEW
#           test that includes RmlUi directly fails the gate.
#       (c) value-type tokens (Rml::String/Vector2/Colourb/Variant/Input/Log) in CODE
#           (comments stripped) outside src/rml/, INCLUDING glintfx/tests/ (RMLX-0
#           2026-08-05 -- see "FILE ENUMERATION FIX" below) -> FAIL (file:line). The
#           FROZEN 4 test files whitelisted by check (b) are exempt here too, by full
#           relative path (see RML_TOKEN_EXCEPTIONS below for why).
#       (d) report-only, ALWAYS printed: count of files with residual opaque `Rml::`
#           usage (forward-declared pointers -- Rml::Context*, Rml::SystemInterface* --
#           i.e. anything NOT already caught by (c)'s value-type list) outside the
#           whitelist. Format: "divida opaca: N arquivos". Printed even when N is 0 --
#           "zero declared" proves someone looked, an absent line does not.
#
#     Deliberately grep-based on `#include` DIRECTIVES for (a)/(b), never on a raw
#     substring of the whole file -- same doctrine as tools/check_encapsulation.sh
#     (GLINTFX_BACKEND_GLFW contains the substring "GLFW" but is not a GLFW header
#     include, so a naive grep must not trip on it; the analogous risk here would be a
#     macro or identifier containing "Rml" that is not an #include).
#
#     FILE ENUMERATION FIX (RMLX-0, 2026-08-05): adversarial review found 4 gaps in
#     WHAT the gate scans (not in the matching logic above), all silent exit-0 holes:
#       1. the TOKEN checks ((c)/(d)) never scanned glintfx/tests/ at all -- a NEW
#          test file with a raw `Rml::String` token, no #include, passed clean. Fixed
#          by adding RML_TESTS_DIR to check (c)'s roots (see decision above); (d) is
#          left scoped to RML_SRC_ROOTS on purpose, its job is opaque debt in the
#          product, not test debt already tracked by (b)/(c).
#       2. in glintfx/tests/, check (b)'s own `find` only matched *.cpp/*.hpp -- a
#          NEW *.h test helper with a live RmlUi #include escaped it.
#       3. `.cc`/`.cxx`/`.hh`/`.hxx` were outside every check's `find` everywhere --
#          convention (the repo uses .cpp/.hpp today), not a gate.
#       4. `find` without `-L` treats a symlink as type l, never type f, so `-type f`
#          silently dropped a symlink pointing at a file with a real violation.
#     Fix: one shared rml_find_files() (below) used by ALL FOUR checks -- `find -L`
#     (follows symlinks) over the SAME 8-extension set (.cpp .hpp .cc .cxx .hh .hxx
#     .h .c). One enumeration rule so a future extension/symlink fix lands once, not
#     four times independently drifting (three of the four already had).
#
#     Comment stripping for (c)/(d): glintfx's public headers carry DOZENS of legitimate
#     `Rml::` mentions in doc-comments explaining what a wrapper encapsulates (measured:
#     55 in ui_layer.hpp alone) -- a raw grep for the token list would false-positive on
#     every one of them. strip_comments() below removes both `//` line comments
#     (including one that starts mid-line, after real code -- a naive `^\s*//` anchor
#     misses exactly that case) and `/* ... */` block comments (state carried across
#     lines via the RML_WL_IN_BLOCK global), with minimal string/char-literal awareness
#     so a stray `//` or `/*` inside a `"..."`/`'...'` does not truncate real code.
#
#     KNOWN, FROZEN, DOCUMENTED EXCEPTION -- the RmlUi GLFW platform-backend adapter
#     pair (glintfx/src/app.cpp, glintfx/src/system_glfw_dedup.hpp): discovered while
#     building this gate (2026-08-04), NOT called out in the F4 brief, whose own quick
#     verification (`grep -rln '^#include <RmlUi' glintfx/src/ | grep -v
#     '^glintfx/src/rml/'`) is angle-bracket-only and does not match the quoted
#     `#include "RmlUi_Platform_GLFW.h"` these two files carry. RMLX-0/F1's own census
#     counted 19 files with a real RmlUi #include pre-F1; F1 moved exactly 17 into
#     src/rml/, leaving these 2 -- App owns the live GLFWwindow* that RmlUi's upstream
#     SystemInterface_GLFW needs at construction time (only available post
#     WindowGlfw::create()), so the platform-backend header has to reach app.cpp
#     directly, and system_glfw_dedup.hpp is App-mode's thin LogMessage-override
#     subclass of that same upstream type (LOGTHR-1, TODO.md) -- hence its literal
#     `Rml::Log::Type`/`Rml::String` signature, mandated by the base class it overrides,
#     not a leak of glintfx's own choosing.
#
#     RMLX-0 (2026-08-05): the exemption for check (a) is scoped to the SINGLE include
#     text `RmlUi_Platform_GLFW.h` (RML_INCLUDE_EXCEPTION_ALLOWED below), not to the
#     whole file -- adversarial review proved the earlier whole-file skip was a real
#     hole: planting an unrelated `#include <RmlUi/Core/ElementDocument.h>` in app.cpp
#     (zero relation to the GLFW bridge) still returned OK/exit 0. check_include_gate()
#     now filters, for files in RML_INCLUDE_EXCEPTIONS, only the matches whose text is
#     the allowed include; any OTHER RmlUi #include in those same two files still
#     fails check (a). check (c)'s RML_TOKEN_EXCEPTIONS is unrelated and stays a
#     whole-file skip (system_glfw_dedup.hpp only, app.cpp has zero non-comment Rml::
#     token use) -- growing either list, or widening RML_INCLUDE_EXCEPTION_ALLOWED, is
#     a decision for the líder, not something this script's author gets to do
#     unilaterally by adding a path to satisfy a red run.
#
# PT: RMLX-0/F4 -- gate de whitelist do RmlUi. Guarda o confinamento que F1-F3 já
#     compraram: com o `#include <RmlUi/...>` movido para `glintfx/src/rml/` (F1) e
#     `engine.cpp`/`ui_layer.cpp` desmamados do RmlUi por completo (F2/F3), nada impede
#     um arquivo NOVO de reintroduzir o include amanhã e desfazer em silêncio três
#     fatias de trabalho. Este script é essa trava.
#
#     Quatro checagens, nesta ordem (a/b/c bloqueantes, d report-only):
#       (a) qualquer `#include` de RmlUi em glintfx/src/, glintfx/include/,
#           glintfx/demos/ FORA de glintfx/src/rml/ -> FAIL (arquivo:linha).
#       (b) qualquer `#include` de RmlUi em glintfx/tests/ fora da whitelist
#           CONGELADA de 4 arquivos (domrw_sanity.cpp, focus_sanity.cpp,
#           form_events_sanity.cpp, document_reload_leak.cpp) -> FAIL. Trava a dívida
#           pré-existente: um teste NOVO que inclua RmlUi direto reprova o gate.
#       (c) tokens de tipo-valor (Rml::String/Vector2/Colourb/Variant/Input/Log) em
#           CÓDIGO (comentário removido) fora de src/rml/, INCLUINDO glintfx/tests/
#           (RMLX-0 2026-08-05 -- ver "CONSERTO DE ENUMERAÇÃO DE ARQUIVO" abaixo) ->
#           FAIL (arquivo:linha). Os 4 arquivos de teste congelados na whitelist do
#           check (b) ficam isentos aqui também, pelo caminho relativo completo (ver
#           RML_TOKEN_EXCEPTIONS abaixo pro motivo).
#       (d) report-only, SEMPRE impresso: contagem de arquivos com uso opaco residual
#           de `Rml::` (ponteiro via fwd-decl -- Rml::Context*, Rml::SystemInterface* --
#           ou seja, tudo que (c) NÃO já pega pela lista de tipo-valor) fora da
#           whitelist. Formato: "divida opaca: N arquivos". Impresso mesmo quando N é 0
#           -- "zero declarado" prova que alguém olhou, linha ausente não prova nada.
#
#     Deliberadamente baseado em grep de DIRETIVAS `#include` para (a)/(b), nunca em
#     substring crua do arquivo inteiro -- mesma doutrina do tools/check_encapsulation.sh
#     (GLINTFX_BACKEND_GLFW contém a substring "GLFW" mas não é include de header GLFW,
#     então um grep ingênuo não pode disparar nela; o risco análogo aqui seria uma macro
#     ou identificador contendo "Rml" que não é um #include).
#
#     CONSERTO DE ENUMERAÇÃO DE ARQUIVO (RMLX-0, 2026-08-05): revisão adversarial achou
#     4 lacunas no que o gate VARRE (não na lógica de casamento acima), todas furos
#     silenciosos de exit-0:
#       1. os checks de TOKEN ((c)/(d)) nunca varriam glintfx/tests/ -- um arquivo de
#          teste NOVO com um token `Rml::String` cru, sem #include, passava limpo.
#          Consertado adicionando RML_TESTS_DIR às raízes do check (c) (ver decisão
#          acima); o (d) fica de propósito escopado a RML_SRC_ROOTS -- seu trabalho é
#          dívida opaca no produto, não dívida de teste já rastreada por (b)/(c).
#       2. em glintfx/tests/, o próprio `find` do check (b) só casava *.cpp/*.hpp --
#          um *.h NOVO de apoio de teste com #include de RmlUi vivo escapava.
#       3. `.cc`/`.cxx`/`.hh`/`.hxx` ficavam fora do `find` de todo check, em todo
#          lugar -- convenção (o repo usa .cpp/.hpp hoje), não gate.
#       4. `find` sem `-L` trata symlink como type l, nunca type f, então `-type f`
#          descartava em silêncio um symlink apontando pra um arquivo com violação
#          real.
#     Conserto: um rml_find_files() compartilhado (abaixo) usado pelos QUATRO checks --
#     `find -L` (segue symlink) sobre o MESMO conjunto de 8 extensões (.cpp .hpp .cc
#     .cxx .hh .hxx .h .c). Uma regra de enumeração só, pra um conserto futuro de
#     extensão/symlink pousar uma vez, não quatro vezes divergindo independentemente
#     (três das quatro já tinham divergido).
#
#     Remoção de comentário para (c)/(d): os headers públicos da glintfx carregam
#     DEZENAS de menções legítimas a `Rml::` em doc-comment explicando o que um wrapper
#     encapsula (medido: 55 só em ui_layer.hpp) -- um grep cru na lista de tokens daria
#     falso positivo em cada uma. O strip_comments() abaixo remove tanto comentário de
#     linha `//` (inclusive um que começa NO MEIO da linha, depois de código de verdade
#     -- uma âncora ingênua `^\s*//` erra exatamente esse caso) quanto comentário de
#     bloco `/* ... */` (estado carregado entre linhas via o global
#     RML_WL_IN_BLOCK), com consciência mínima de literal de string/char pra um `//` ou
#     `/*` perdido dentro de um `"..."`/`'...'` não truncar código de verdade.
#
#     EXCEÇÃO CONHECIDA, CONGELADA E DOCUMENTADA -- o par adaptador de backend GLFW do
#     RmlUi (glintfx/src/app.cpp, glintfx/src/system_glfw_dedup.hpp): descoberta ao
#     construir este gate (2026-08-04), NÃO citada no brief da F4, cuja própria
#     verificação rápida (`grep -rln '^#include <RmlUi' glintfx/src/ | grep -v
#     '^glintfx/src/rml/'`) é só-angle-bracket e não casa o `#include
#     "RmlUi_Platform_GLFW.h"` (aspas) que estes dois arquivos carregam. O próprio censo
#     da RMLX-0/F1 contou 19 arquivos com #include real de RmlUi pré-F1; F1 moveu
#     exatamente 17 para src/rml/, deixando estes 2 -- App é dona do GLFWwindow* vivo
#     que o SystemInterface_GLFW upstream do RmlUi exige na construção (só disponível
#     pós WindowGlfw::create()), então o header do backend de plataforma precisa
#     alcançar app.cpp direto, e system_glfw_dedup.hpp é a subclasse fina de override de
#     LogMessage desse mesmo tipo upstream, modo App (LOGTHR-1, TODO.md) -- daí a
#     assinatura literal `Rml::Log::Type`/`Rml::String`, exigida pela classe-base que
#     ela sobrescreve, não um vazamento de escolha própria da glintfx.
#
#     RMLX-0 (2026-08-05): a isenção do check (a) é escopada ao texto ÚNICO de include
#     `RmlUi_Platform_GLFW.h` (RML_INCLUDE_EXCEPTION_ALLOWED abaixo), não ao arquivo
#     inteiro -- revisão adversarial provou que o pulo do arquivo inteiro era um furo
#     real: plantar um `#include <RmlUi/Core/ElementDocument.h>` não-relacionado em
#     app.cpp (zero relação com a ponte GLFW) ainda devolvia OK/exit 0. O
#     check_include_gate() agora filtra, pros arquivos em RML_INCLUDE_EXCEPTIONS, só os
#     casamentos cujo texto é o include permitido; qualquer OUTRO #include de RmlUi
#     nesses mesmos dois arquivos continua reprovando o check (a). O
#     RML_TOKEN_EXCEPTIONS do check (c) não tem relação e continua sendo pulo de
#     arquivo inteiro (só system_glfw_dedup.hpp, app.cpp tem zero uso de token Rml::
#     fora de comentário) -- aumentar qualquer uma das duas listas, ou alargar
#     RML_INCLUDE_EXCEPTION_ALLOWED, é decisão do líder, não algo que o autor deste
#     script decide sozinho pra deixar um run vermelho passar.
#
# Usage / Uso:
#   tools/check_rml_whitelist.sh              # selftest, then the real check (repo root CWD)
#   tools/check_rml_whitelist.sh --selftest   # selftest only (self-contained, mktemp, no repo needed)
#
# Exit status: 0 = clean; 1 = a blocking violation was found ((a)/(b)/(c), or the
# selftest itself failed to prove both sides); 2 = usage/environment error.
set -euo pipefail

SCRIPT_NAME="$(basename "$0")"

# -----------------------------------------------------------------------------
# EN: Config -- paths are relative to CWD, which must be the repo root (same
#     convention as tools/check_encapsulation.sh).
# PT: Config -- caminhos relativos ao CWD, que precisa ser a raiz do repo (mesma
#     convenção do tools/check_encapsulation.sh).
# -----------------------------------------------------------------------------
RML_SRC_ROOTS=(glintfx/src glintfx/include glintfx/demos)
RML_DIR="glintfx/src/rml"
RML_TESTS_DIR="glintfx/tests"

# EN: check (b) -- frozen whitelist of test FILES allowed to #include RmlUi directly.
#     Matched by full relative path (RML_TESTS_DIR-prefixed, same string `find` yields
#     for $f below) -- NOT by basename. RMLX-0 adversarial review proved the
#     basename-only match was a real gate hole: a NEW file at
#     glintfx/tests/subdir/domrw_sanity.cpp (same basename as the frozen debt entry,
#     but a different, un-audited file) with a live RmlUi #include passed the gate
#     clean. Full-path matching closes that: only the exact frozen path is exempt.
# PT: check (b) -- whitelist congelada de arquivos de teste com permissão de #include
#     direto de RmlUi. Casado pelo caminho relativo COMPLETO (prefixado por
#     RML_TESTS_DIR, a mesma string que o `find` devolve pro $f abaixo) -- NÃO pelo
#     basename. A revisão adversarial da RMLX-0 provou que o casamento só-por-basename
#     era um furo real do gate: um arquivo NOVO em
#     glintfx/tests/subdir/domrw_sanity.cpp (mesmo basename da entrada de dívida
#     congelada, mas um arquivo diferente, não auditado) com um #include de RmlUi vivo
#     passava o gate limpo. Casar pelo caminho completo fecha isso: só o caminho
#     congelado exato fica isento.
RML_TEST_WHITELIST=(
  "glintfx/tests/domrw_sanity.cpp"
  "glintfx/tests/focus_sanity.cpp"
  "glintfx/tests/form_events_sanity.cpp"
  "glintfx/tests/document_reload_leak.cpp"
)

# EN: check (c) in glintfx/tests/ -- a SEPARATE, SMALLER, DOCUMENTED exemption from
#     RML_TEST_WHITELIST above. Discovered live (RMLX-0, 2026-08-05) the moment
#     check (c) was widened to scan glintfx/tests/ at all (closing gap 1 of the F4
#     file-enumeration fix -- see the header comment's "FILE ENUMERATION FIX"):
#     `check_token_gate glintfx/tests` came back red on the UNMODIFIED real tree,
#     not just on the planted fixture. Root cause: these two files test the
#     RMLX-0 confinement boundary ITSELF -- glintfx/tests/input_map_sanity.cpp
#     exercises src/rml/input_map.hpp's Key -> Rml::Input::KeyIdentifier mapping,
#     glintfx/tests/type_bridge_review_sanity.cpp exercises src/rml/type_bridge.hpp's
#     value-type round-trip -- and asserting the conversion is correct means
#     literally writing the target Rml:: type's name in the assertion. Confirmed
#     NEITHER file #includes RmlUi directly (`grep -n '^#include' <file>` on both:
#     only `../src/rml/{input_map,type_bridge}.hpp` and stdlib headers) -- so
#     check (a)/(b), which only look at #include DIRECTIVES, correctly never had
#     reason to flag them; only check (c)'s raw-token scan does, and only once it
#     started looking at tests/ at all. This is why the exemption is a DISTINCT
#     list from RML_TEST_WHITELIST, not a merge into it: RML_TEST_WHITELIST is
#     "old test debt that #includes RmlUi directly" (check (b)'s concern);
#     RML_TEST_TOKEN_EXCEPTIONS is "test that must reference Rml:: value types on
#     purpose to verify a conversion, without ever #including RmlUi" (check (c)'s
#     concern, unrelated cause). Matched by FULL relative path, same doctrine as
#     every other exception list in this file -- growing this list beyond these
#     two, or merging it into RML_TEST_WHITELIST, is a decision for the líder.
# PT: check (c) em glintfx/tests/ -- uma isenção SEPARADA, MENOR e DOCUMENTADA da
#     RML_TEST_WHITELIST acima. Descoberta ao vivo (RMLX-0, 2026-08-05) no momento
#     em que o check (c) foi alargado pra varrer glintfx/tests/ (fechando a lacuna
#     1 do conserto de enumeração de arquivo da F4 -- ver "CONSERTO DE ENUMERAÇÃO
#     DE ARQUIVO" no comentário de cabeçalho): `check_token_gate glintfx/tests`
#     voltou vermelho na árvore real NÃO MODIFICADA, não só na fixture plantada.
#     Causa raiz: estes dois arquivos testam a própria fronteira de confinamento
#     RMLX-0 -- glintfx/tests/input_map_sanity.cpp exercita o mapeamento Key ->
#     Rml::Input::KeyIdentifier de src/rml/input_map.hpp,
#     glintfx/tests/type_bridge_review_sanity.cpp exercita a ida-e-volta de
#     tipo-valor de src/rml/type_bridge.hpp -- e afirmar que a conversão está
#     correta significa escrever literalmente o nome do tipo Rml:: alvo na
#     asserção. Confirmado que NENHUM dos dois dá #include direto de RmlUi
#     (`grep -n '^#include' <arquivo>` nos dois: só
#     `../src/rml/{input_map,type_bridge}.hpp` e headers de stdlib) -- então os
#     checks (a)/(b), que só olham DIRETIVA de #include, corretamente nunca
#     tiveram motivo pra acusá-los; só a varredura de token cru do check (c) tem,
#     e só depois que ele passou a olhar tests/. É por isso que a isenção é uma
#     lista DISTINTA da RML_TEST_WHITELIST, não uma fusão nela: RML_TEST_WHITELIST
#     é "dívida de teste velha que dá #include direto de RmlUi" (preocupação do
#     check (b)); RML_TEST_TOKEN_EXCEPTIONS é "teste que precisa referenciar tipo
#     de valor Rml:: de propósito pra verificar uma conversão, sem nunca dar
#     #include de RmlUi" (preocupação do check (c), causa não-relacionada). Casada
#     pelo caminho relativo COMPLETO, mesma doutrina de toda outra lista de
#     exceção deste arquivo -- aumentar esta lista além destes dois, ou fundi-la
#     na RML_TEST_WHITELIST, é decisão do líder.
RML_TEST_TOKEN_EXCEPTIONS=(
  "glintfx/tests/input_map_sanity.cpp"
  "glintfx/tests/type_bridge_review_sanity.cpp"
)

# EN: check (a) -- see the header comment above ("KNOWN, FROZEN, DOCUMENTED
#     EXCEPTION") for why these two, and only these two, are here. NOTE (RMLX-0,
#     2026-08-05): being on this list no longer exempts the whole file -- see
#     RML_INCLUDE_EXCEPTION_ALLOWED right below.
# PT: check (a) -- ver o comentário de cabeçalho acima ("EXCEÇÃO CONHECIDA,
#     CONGELADA E DOCUMENTADA") pro motivo de estes dois, e só estes dois, estarem
#     aqui. NOTA (RMLX-0, 2026-08-05): estar nesta lista não isenta mais o arquivo
#     inteiro -- ver RML_INCLUDE_EXCEPTION_ALLOWED logo abaixo.
RML_INCLUDE_EXCEPTIONS=(
  "glintfx/src/app.cpp"
  "glintfx/src/system_glfw_dedup.hpp"
)

# EN: check (a) -- the ONLY include text that RML_INCLUDE_EXCEPTIONS files are
#     allowed to carry. Any OTHER RmlUi #include in those same files still fails
#     check (a) -- this is what closes the "isenta o arquivo inteiro" hole an
#     adversarial reviewer proved (planting <RmlUi/Core/ElementDocument.h> in
#     app.cpp used to pass clean). Matched as a plain grep -F substring against the
#     already-matched #include line, not re-parsed -- the line already passed
#     RML_INCLUDE_PATTERN, so this only needs to tell "the GLFW bridge quoted
#     include" apart from any other Rml include text.
# PT: check (a) -- o ÚNICO texto de include que os arquivos de
#     RML_INCLUDE_EXCEPTIONS têm permissão de carregar. Qualquer OUTRO #include de
#     RmlUi nesses mesmos arquivos continua reprovando o check (a) -- é isso que
#     fecha o furo de "isenta o arquivo inteiro" que uma revisão adversarial provou
#     (plantar <RmlUi/Core/ElementDocument.h> em app.cpp passava limpo). Casado como
#     substring simples (grep -F) contra a linha de #include já casada, sem
#     re-parsear -- a linha já passou por RML_INCLUDE_PATTERN, então só precisa
#     distinguir "o include entre aspas da ponte GLFW" de qualquer outro texto de
#     include Rml.
RML_INCLUDE_EXCEPTION_ALLOWED='RmlUi_Platform_GLFW.h'

# EN: check (c) -- same rationale, narrower: only system_glfw_dedup.hpp has real
#     (non-comment) Rml::Log/Rml::String token use; app.cpp's Rml:: mentions are 100%
#     comment.
# PT: check (c) -- mesma racional, mais estreita: só system_glfw_dedup.hpp tem uso
#     real (fora de comentário) de token Rml::Log/Rml::String; as menções a Rml:: em
#     app.cpp são 100% comentário.
RML_TOKEN_EXCEPTIONS=(
  "glintfx/src/system_glfw_dedup.hpp"
)

# EN: Matches an #include directive whose argument contains "Rml" -- catches both
#     angle-bracket Core headers (<RmlUi/Core/Types.h>) and the quoted GLFW
#     platform-backend header ("RmlUi_Platform_GLFW.h"). Anchored to the directive
#     itself (line-start, only whitespace before `#`), not free text in the file --
#     see check_encapsulation.sh's own header for why that anchoring matters.
# PT: Casa uma diretiva #include cujo argumento contém "Rml" -- pega tanto headers Core
#     de angle-bracket (<RmlUi/Core/Types.h>) quanto o header de backend de plataforma
#     GLFW entre aspas ("RmlUi_Platform_GLFW.h"). Ancorado na própria diretiva (início
#     de linha, só espaço antes do `#`), não em texto livre do arquivo -- ver o
#     cabeçalho do próprio check_encapsulation.sh pro motivo de a ancoragem importar.
RML_INCLUDE_PATTERN='^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"][^>"]*Rml[^>"]*[>"]'

# EN: check (c)'s blocking value-type token list.
# PT: lista bloqueante de tokens de tipo-valor do check (c).
RML_TOKEN_PATTERN='Rml::(String|Vector2|Colourb|Variant|Input|Log)'

# EN: check (d)'s generic opaque-usage pattern (any Rml::Identifier).
# PT: padrão genérico de uso opaco do check (d) (qualquer Rml::Identificador).
RML_ANY_TOKEN_PATTERN='Rml::[A-Za-z_][A-Za-z0-9_]*'

# -----------------------------------------------------------------------------
# EN: strip_comments FILE -- prints FILE with `//` line comments and `/* ... */` block
#     comments removed, one output line per input line (so line numbers used by
#     downstream grep -n stay correct). Minimal string/char-literal awareness: a `/`
#     inside `"..."`/`'...'` does not start a comment. Not a full C++ tokenizer, but it
#     DOES understand C++ raw-string literals (`R"delim(...)delim"`, RMLX-0
#     2026-08-05, see below) since real files this gate scans use them
#     (glintfx/src/draw2d.cpp's GLSL shader sources).
#
#     RMLX-0 (2026-08-05) -- raw-string furo and fix: `in_block` is GLOBAL, carried
#     across lines (same as always), but the ORIGINAL awk had zero awareness of
#     `R"delim(...)delim"`. A raw string's body is opaque -- none of `//`, `/*`, `"`,
#     `'` inside it mean anything -- but the old state machine parsed it as plain code,
#     so an unescaped `/*` inside a multi-line raw string (with no `*/` on the same
#     line) opened a REAL `in_block=1` that survived past the raw string's own closing
#     `)delim"` and silently swallowed every line of real code after it, including a
#     value-type `Rml::` token check (c) exists to catch. Proved live: planting
#     `Rml::String leaked_token_should_be_caught` right after such a raw string made
#     the gate return OK/exit 0. Fix: a second GLOBAL, cross-line state pair
#     (`raw_active`, `raw_delim`) mirrors `in_block`'s persistence. On a `R"` not
#     already inside a string/char/comment, the awk scans forward on the SAME line
#     (a raw string's delimiter cannot itself contain a newline -- C++ grammar) for a
#     d-char-sequence of up to 16 chars (matching the standard's own cap) followed by
#     `(`; space/tab/`(`/`)`/backslash/`"` are excluded from the delimiter charset
#     (`"` is technically unspecified by the grammar, but excluding it removes an
#     ambiguity this simple parser has no need to court). On success, everything from
#     `R"` through the matched `(` is copied to `out` verbatim and `raw_active` turns
#     on; every subsequent character (this line and, via the global, however many
#     lines follow) is copied through UNINTERPRETED until the exact closing
#     `)delim"` is found, which turns `raw_active` back off. FAIL-CLOSED by
#     construction on both sides of the risk: (1) if the `R"...(` scan does not find a
#     valid delimiter+`(` on the same line, `raw_active` is never entered and the `R`
#     is emitted as a plain character -- the following `"` then opens ordinary
#     `in_str` handling, which is the strictly safer of the two readings whenever this
#     parser is unsure it is looking at a real raw string; (2) once inside
#     `raw_active`, nothing short of the EXACT closing delimiter turns it back off --
#     an unterminated raw string swallows the rest of the file rather than guess a
#     close, which is the same "erred toward keeping too much, never toward dropping
#     code silently" bias the whole function already had for `in_str`/`in_chr`.
# PT: strip_comments ARQUIVO -- imprime ARQUIVO com comentário de linha `//` e de
#     bloco `/* ... */` removidos, uma linha de saída por linha de entrada (pra os
#     números de linha usados pelo grep -n rio abaixo continuarem corretos). Consciência
#     mínima de literal string/char: um `/` dentro de `"..."`/`'...'` não abre
#     comentário. Não é um tokenizer C++ completo, mas ENTENDE raw-string literal de
#     C++ (`R"delim(...)delim"`, RMLX-0 2026-08-05, ver abaixo) -- arquivo real que
#     este gate varre usa isso (as fontes de shader GLSL do
#     glintfx/src/draw2d.cpp).
#
#     RMLX-0 (2026-08-05) -- o furo do raw string e o conserto: `in_block` é GLOBAL,
#     carregado entre linhas (como sempre foi), mas o awk ORIGINAL tinha zero
#     conhecimento de `R"delim(...)delim"`. O corpo de um raw string é opaco -- nada
#     de `//`, `/*`, `"`, `'` dentro dele significa qualquer coisa -- mas a máquina de
#     estados velha interpretava tudo como código comum, então um `/*` sem escape
#     dentro de um raw string multi-linha (sem `*/` na mesma linha) abria um
#     `in_block=1` REAL que sobrevivia além do próprio fechamento `)delim"` do raw
#     string e engolia em silêncio toda linha de código de verdade depois dele,
#     inclusive um token `Rml::` de tipo-valor que o check (c) existe pra pegar.
#     Provado ao vivo: plantar `Rml::String leaked_token_should_be_caught` logo depois
#     de um raw string desses fazia o gate devolver OK/exit 0. Conserto: um segundo par
#     de estado GLOBAL, persistente entre linhas (`raw_active`, `raw_delim`) espelha a
#     persistência de `in_block`. Ao ver `R"` fora de string/char/comentário, o awk
#     varre pra frente NA MESMA LINHA (o delimitador de um raw string não pode conter
#     newline -- gramática de C++) por uma d-char-sequence de até 16 caracteres
#     (mesmo teto da norma) seguida de `(`; espaço/tab/`(`/`)`/backslash/`"` ficam de
#     fora do conjunto de caracteres do delimitador (`"` tecnicamente não é vedado pela
#     gramática, mas excluir remove uma ambiguidade que este parser simples não precisa
#     correr). Em caso de sucesso, tudo de `R"` até o `(` casado é copiado pro `out` ao
#     pé da letra e `raw_active` liga; todo caractere seguinte (desta linha e, via o
#     global, de quantas linhas vierem depois) é copiado SEM interpretação até achar o
#     `)delim"` de fechamento exato, que desliga `raw_active` de novo. FALHA FECHADA por
#     construção nos dois lados do risco: (1) se a varredura `R"...(` não acha
#     delimitador+`(` válido na mesma linha, `raw_active` nunca entra e o `R` é
#     emitido como caractere normal -- a `"` seguinte então abre o `in_str` comum, que
#     é a leitura estritamente mais segura sempre que este parser não tem certeza de
#     estar olhando pra um raw string de verdade; (2) uma vez dentro de `raw_active`,
#     nada além do delimitador de fechamento EXATO desliga de novo -- um raw string
#     não-terminado engole o resto do arquivo em vez de arriscar um fechamento por
#     achismo, o mesmo viés de "errar pro lado de guardar demais, nunca de derrubar
#     código em silêncio" que a função inteira já tinha pra `in_str`/`in_chr`.
# -----------------------------------------------------------------------------
strip_comments() {
  awk '
    BEGIN { in_block = 0; raw_active = 0; raw_delim = "" }
    {
      line = $0
      n = length(line)
      out = ""
      in_str = 0
      in_chr = 0
      i = 1
      while (i <= n) {
        c = substr(line, i, 1)
        if (in_block) {
          if (c == "*" && substr(line, i + 1, 1) == "/") {
            in_block = 0
            i += 2
            continue
          }
          i += 1
          continue
        }
        # RMLX-0 (2026-08-05): raw-string body -- opaque, copied through
        # uncondicionalmente ate o fechamento EXATO ")delim\"" (raw_delim e o
        # delimitador capturado na abertura, ver abaixo). Precisa vir ANTES de
        # in_str/in_chr porque um raw string pode conter aspas/apostrofos
        # desemparelhados no proprio corpo (razao de existir da sintaxe).
        if (raw_active) {
          dlen = length(raw_delim)
          if (c == ")" && substr(line, i + 1, dlen) == raw_delim && substr(line, i + 1 + dlen, 1) == "\"") {
            out = out c raw_delim "\""
            raw_active = 0
            raw_delim = ""
            i += 2 + dlen
            continue
          }
          out = out c
          i += 1
          continue
        }
        if (in_str) {
          out = out c
          if (c == "\\") { out = out substr(line, i + 1, 1); i += 2; continue }
          if (c == "\"") { in_str = 0 }
          i += 1
          continue
        }
        if (in_chr) {
          out = out c
          if (c == "\\") { out = out substr(line, i + 1, 1); i += 2; continue }
          if (c == "'"'"'") { in_chr = 0 }
          i += 1
          continue
        }
        if (c == "\"") { in_str = 1; out = out c; i += 1; continue }
        if (c == "'"'"'") { in_chr = 1; out = out c; i += 1; continue }
        # RMLX-0 (2026-08-05): tentativa de abertura de raw string -- R" seguido de
        # d-char-sequence (<=16 chars, sem espaco/tab/parenteses/backslash/aspas) e
        # "(". Se nao casar, FALHA FECHADA: nao consome nada, o "R" vira caractere
        # comum e a "\"" seguinte abre in_str normal na proxima iteracao -- a leitura
        # mais segura quando este parser simples nao tem certeza.
        if (c == "R" && substr(line, i + 1, 1) == "\"") {
          j = i + 2
          delim = ""
          delim_len = 0
          raw_open_ok = 0
          while (j <= n) {
            dc = substr(line, j, 1)
            if (dc == "(") { raw_open_ok = 1; break }
            if (delim_len >= 16 || dc == " " || dc == "\t" || dc == ")" || dc == "\\" || dc == "\"") { break }
            delim = delim dc
            delim_len += 1
            j += 1
          }
          if (raw_open_ok) {
            out = out substr(line, i, j - i + 1)
            raw_active = 1
            raw_delim = delim
            i = j + 1
            continue
          }
        }
        if (c == "/" && substr(line, i + 1, 1) == "/") { break }
        if (c == "/" && substr(line, i + 1, 1) == "*") { in_block = 1; i += 2; continue }
        out = out c
        i += 1
      }
      print out
    }
  ' "$1"
}

# EN: True if NEEDLE equals one of the remaining args.
# PT: Verdadeiro se NEEDLE é igual a um dos argumentos restantes.
rml_array_contains() {
  local needle="$1"
  shift
  local x
  for x in "$@"; do
    [[ "$x" == "$needle" ]] && return 0
  done
  return 1
}

# -----------------------------------------------------------------------------
# EN: rml_find_files ROOT_1 [ROOT_2 ...] -- single, shared file enumeration for ALL
#     FOUR checks (a)/(b)/(c)/(d): NUL-separated, sorted, following symlinks
#     (`find -L`, so a symlink to a regular file with a leaking include/token is not
#     invisible to the gate -- `-type f` without `-L` treats the symlink itself as
#     type l, never f, and drops it silently) over the SAME 8-extension set
#     (.cpp .hpp .cc .cxx .hh .hxx .h .c -- RMLX-0 2026-08-05 widened this from the
#     narrower, and in one case DIFFERENT, subset the four call sites used to
#     hardcode independently: check (b)'s own `find` only matched *.cpp/*.hpp,
#     silently letting a NEW *.h test helper carry a live RmlUi #include past the
#     gate). One function, one enumeration rule, so a future extension or symlink
#     fix only has to land in ONE place.
# PT: rml_find_files RAIZ_1 [RAIZ_2 ...] -- enumeração de arquivo única e
#     compartilhada pelos QUATRO checks (a)/(b)/(c)/(d): NUL-separado, ordenado,
#     seguindo symlink (`find -L`, pra um symlink apontando pra um arquivo comum
#     com include/token vazando não ficar invisível ao gate -- `-type f` sem `-L`
#     trata o próprio symlink como type l, nunca f, e descarta em silêncio) sobre o
#     MESMO conjunto de 8 extensões (.cpp .hpp .cc .cxx .hh .hxx .h .c -- RMLX-0
#     2026-08-05 alargou isso do subconjunto mais estreito, e num caso DIFERENTE,
#     que os quatro pontos de chamada hardcodeavam de forma independente: o próprio
#     `find` do check (b) só casava *.cpp/*.hpp, deixando em silêncio um *.h NOVO de
#     apoio de teste carregar um #include de RmlUi vivo pelo gate). Uma função, uma
#     regra de enumeração, pra um conserto futuro de extensão ou symlink só
#     precisar pousar em UM lugar.
# -----------------------------------------------------------------------------
rml_find_files() {
  find -L "$@" -type f \( \
    -name '*.cpp' -o -name '*.hpp' -o -name '*.cc' -o -name '*.cxx' \
    -o -name '*.hh' -o -name '*.hxx' -o -name '*.h' -o -name '*.c' \
  \) -print0 2>/dev/null | sort -z
}

# -----------------------------------------------------------------------------
# EN: check_include_gate SRC_ROOT_1 [SRC_ROOT_2 ...] -- shared engine for (a): finds
#     every file matching rml_find_files()'s extension set under the given roots,
#     following symlinks, skips anything under
#     EXCL_DIR (global, set by caller) and anything in EXCL_FILES (global array),
#     and reports arquivo:linha for any #include matching RML_INCLUDE_PATTERN.
#     EXCL_DIR/EXCL_FILES are read as globals (not params) to keep call sites in
#     check (a)/selftest terse -- same "unqualified globals read by convention"
#     pattern tools/preci.sh documents for its own classify_touched_files().
# PT: check_include_gate RAIZ_1 [RAIZ_2 ...] -- motor compartilhado do (a): acha todo
#     arquivo do conjunto de extensão do rml_find_files() sob as raízes dadas,
#     seguindo symlink, pula tudo sob EXCL_DIR (global, setado
#     pelo chamador) e tudo em EXCL_FILES (array global), e reporta arquivo:linha pra
#     qualquer #include que case RML_INCLUDE_PATTERN. EXCL_DIR/EXCL_FILES são lidos
#     como globais (não parâmetros) pra manter os pontos de chamada de (a)/selftest
#     enxutos -- mesmo padrão de "globais sem qualificador lidos por convenção" que o
#     tools/preci.sh documenta pra sua própria classify_touched_files().
# -----------------------------------------------------------------------------
check_include_gate() {
  local violations=0
  local f matches
  while IFS= read -r -d '' f; do
    case "$f" in
      "${EXCL_DIR}"/*) continue ;;
    esac
    matches="$(grep -nE "$RML_INCLUDE_PATTERN" "$f" || true)"
    # EN: RMLX-0 (2026-08-05) -- an EXCL_FILES entry no longer skips the whole file:
    #     only lines whose include text is the single allowed one
    #     (RML_INCLUDE_EXCEPTION_ALLOWED) are dropped from `matches`. Any OTHER RmlUi
    #     #include the file carries survives the filter and still fails below.
    # PT: RMLX-0 (2026-08-05) -- uma entrada de EXCL_FILES não pula mais o arquivo
    #     inteiro: só as linhas cujo texto de include é o único permitido
    #     (RML_INCLUDE_EXCEPTION_ALLOWED) são retiradas de `matches`. Qualquer OUTRO
    #     #include de RmlUi que o arquivo carregue sobrevive ao filtro e continua
    #     reprovando abaixo.
    if [[ -n "$matches" ]] && rml_array_contains "$f" "${EXCL_FILES[@]}"; then
      matches="$(printf '%s\n' "$matches" | grep -vF "$RML_INCLUDE_EXCEPTION_ALLOWED" || true)"
    fi
    if [[ -n "$matches" ]]; then
      echo "RmlUi #include fora da whitelist em $f:" >&2
      echo "$matches" >&2
      violations=1
    fi
  done < <(rml_find_files "$@")
  return "$violations"
}

# -----------------------------------------------------------------------------
# EN: check_test_whitelist_gate TESTS_DIR -- (b): every RmlUi #include in TESTS_DIR
#     must be in a file whose FULL RELATIVE PATH (as `find` yields it -- i.e.
#     TESTS_DIR-prefixed, not basename) is in EXCL_FILES (global, caller-set to the
#     frozen 4). Basename-only matching used to let a NEW file at
#     TESTS_DIR/subdir/<frozen-basename> ride through on the debt entry's name alone;
#     matching the full path closes that hole. Scans rml_find_files()'s full
#     8-extension set (RMLX-0 2026-08-05 -- this used to hardcode its OWN narrower
#     `find -name '*.cpp' -o -name '*.hpp'` here, independently of every other
#     check's extension list, and had already drifted: a NEW *.h test helper
#     carrying a live RmlUi #include escaped it silently).
# PT: check_test_whitelist_gate DIR_TESTES -- (b): todo #include de RmlUi em
#     DIR_TESTES precisa estar num arquivo cujo CAMINHO RELATIVO COMPLETO (como o
#     `find` devolve -- prefixado por DIR_TESTES, não basename) está em EXCL_FILES
#     (global, setado pelo chamador pros 4 congelados). Casar só pelo basename deixava
#     um arquivo NOVO em DIR_TESTES/subdir/<basename-congelado> passar de carona no
#     nome da entrada de dívida; casar pelo caminho completo fecha esse furo. Varre o
#     conjunto completo de 8 extensões do rml_find_files() (RMLX-0 2026-08-05 -- isto
#     hardcodeava um `find -name '*.cpp' -o -name '*.hpp'` PRÓPRIO aqui, mais estreito
#     e independente da lista de extensão de todo outro check, e já tinha divergido:
#     um *.h NOVO de apoio de teste carregando um #include de RmlUi vivo escapava em
#     silêncio).
# -----------------------------------------------------------------------------
check_test_whitelist_gate() {
  local tests_dir="$1"
  local violations=0
  local f matches
  while IFS= read -r -d '' f; do
    if rml_array_contains "$f" "${EXCL_FILES[@]}"; then
      continue
    fi
    matches="$(grep -nE "$RML_INCLUDE_PATTERN" "$f" || true)"
    if [[ -n "$matches" ]]; then
      echo "RmlUi #include em teste NOVO fora da whitelist congelada de 4, em $f:" >&2
      echo "$matches" >&2
      violations=1
    fi
  done < <(rml_find_files "$tests_dir")
  return "$violations"
}

# -----------------------------------------------------------------------------
# EN: check_token_gate SRC_ROOT_1 [...] -- (c): comment-stripped scan for
#     RML_TOKEN_PATTERN outside EXCL_DIR, skipping EXCL_FILES (global, caller-set to
#     the frozen exception -- RMLX-0 2026-08-05: the run_real_check() call site now
#     passes RML_TESTS_DIR as one of the roots too, see the comment there for why,
#     with EXCL_FILES widened to also cover the check (b) frozen 4).
# PT: check_token_gate RAIZ_1 [...] -- (c): varredura com comentário removido por
#     RML_TOKEN_PATTERN fora de EXCL_DIR, pulando EXCL_FILES (global, setado pelo
#     chamador pra exceção congelada -- RMLX-0 2026-08-05: o ponto de chamada em
#     run_real_check() agora passa RML_TESTS_DIR como uma das raízes também, ver o
#     comentário lá pro motivo, com EXCL_FILES alargado pra cobrir também os 4
#     congelados do check (b)).
# -----------------------------------------------------------------------------
check_token_gate() {
  local violations=0
  local f matches
  while IFS= read -r -d '' f; do
    case "$f" in
      "${EXCL_DIR}"/*) continue ;;
    esac
    if rml_array_contains "$f" "${EXCL_FILES[@]}"; then
      continue
    fi
    matches="$(strip_comments "$f" | grep -nE "$RML_TOKEN_PATTERN" || true)"
    if [[ -n "$matches" ]]; then
      echo "token de tipo-valor RmlUi fora de $RML_DIR em $f:" >&2
      echo "$matches" >&2
      violations=1
    fi
  done < <(rml_find_files "$@")
  return "$violations"
}

# -----------------------------------------------------------------------------
# EN: opaque_debt_count SRC_ROOT_1 [...] -- (d): counts files (not occurrences) with
#     comment-stripped `Rml::Identifier` usage outside EXCL_DIR whose matching lines
#     are NOT already fully accounted for by RML_TOKEN_PATTERN. Report-only, no
#     concept of EXCL_FILES here on purpose -- (d) is meant to surface ALL residual
#     opaque debt, including inside the (a)/(c) frozen exceptions, not hide it.
#     Deliberately still scoped to RML_SRC_ROOTS only (unlike (c) as of RMLX-0
#     2026-08-05, which now also scans RML_TESTS_DIR) -- (d)'s job is opaque debt in
#     the PRODUCT, not test debt (b)/(c) already track by name.
# PT: opaque_debt_count RAIZ_1 [...] -- (d): conta arquivos (não ocorrências) com uso
#     `Rml::Identificador` (comentário removido) fora de EXCL_DIR cujas linhas casadas
#     NÃO já são totalmente cobertas por RML_TOKEN_PATTERN. Só relatório, sem conceito
#     de EXCL_FILES aqui de propósito -- (d) existe pra expor TODA a dívida opaca
#     residual, inclusive dentro das exceções congeladas de (a)/(c), não escondê-la.
#     De propósito continua escopado só a RML_SRC_ROOTS (diferente do (c) desde a
#     RMLX-0 2026-08-05, que agora varre RML_TESTS_DIR também) -- o trabalho do (d) é
#     dívida opaca do PRODUTO, não dívida de teste que (b)/(c) já rastreiam por nome.
# -----------------------------------------------------------------------------
opaque_debt_count() {
  local count=0
  local f stripped residual
  while IFS= read -r -d '' f; do
    case "$f" in
      "${EXCL_DIR}"/*) continue ;;
    esac
    stripped="$(strip_comments "$f")"
    residual="$(printf '%s\n' "$stripped" | grep -E "$RML_ANY_TOKEN_PATTERN" | grep -vE "$RML_TOKEN_PATTERN" || true)"
    if [[ -n "$residual" ]]; then
      count=$((count + 1))
    fi
  done < <(rml_find_files "$@")
  echo "$count"
}

# -----------------------------------------------------------------------------
# EN: run_real_check -- the 4 checks against the actual repo tree (CWD = repo root).
# PT: run_real_check -- as 4 checagens contra a árvore real do repo (CWD = raiz do repo).
# -----------------------------------------------------------------------------
run_real_check() {
  for d in "${RML_SRC_ROOTS[@]}" "$RML_DIR" "$RML_TESTS_DIR"; do
    if [[ ! -d "$d" ]]; then
      echo "error: '$d' not found (run from repo root)" >&2
      return 2
    fi
  done

  local overall=0

  EXCL_DIR="$RML_DIR"
  EXCL_FILES=("${RML_INCLUDE_EXCEPTIONS[@]}")
  if ! check_include_gate "${RML_SRC_ROOTS[@]}"; then
    overall=1
  fi

  EXCL_FILES=("${RML_TEST_WHITELIST[@]}")
  if ! check_test_whitelist_gate "$RML_TESTS_DIR"; then
    overall=1
  fi

  # EN: RMLX-0 (2026-08-05) -- check (c) now scans RML_TESTS_DIR too, closing gap 1
  #     from the F4 fix ("os checks de TOKEN nunca varrem glintfx/tests/"): a NEW test
  #     file with a raw value-type Rml:: token, no #include at all, used to pass the
  #     gate silently (check (a)/(b) only look at #include DIRECTIVES, never at the
  #     token itself). EXCL_FILES is deliberately widened to ALSO cover
  #     RML_TEST_WHITELIST, not just RML_TOKEN_EXCEPTIONS -- DECISION (líder to
  #     confirm): the check (b) frozen 4 (domrw_sanity.cpp, focus_sanity.cpp,
  #     form_events_sanity.cpp, document_reload_leak.cpp) already #include RmlUi
  #     directly ON PURPOSE (that is what check (b) whitelists), so their value-type
  #     Rml:: token use is expected debt, not a new leak -- exempting them from check
  #     (c) too avoids re-flagging debt already accounted for by (b) under a
  #     different check. This is an EXPLICIT choice, not the accidental blanket-skip
  #     gap (b) itself just closed for basename collisions: matched by FULL relative
  #     path (RML_TEST_WHITELIST already stores full paths), never basename. ALSO
  #     RML_TEST_TOKEN_EXCEPTIONS -- FOUND LIVE (líder to confirm): the moment check
  #     (c) started scanning tests/, the UNMODIFIED real tree came back red (not just
  #     the planted fixture) because of 2 pre-existing files that test the src/rml/
  #     boundary itself (input_map_sanity.cpp, type_bridge_review_sanity.cpp) -- see
  #     the comment at RML_TEST_TOKEN_EXCEPTIONS's declaration above for the full
  #     rationale and the proof that neither #includes RmlUi directly. Any OTHER
  #     test file -- new or old, not in any of the THREE frozen lists -- still fails
  #     on a raw token.
  # PT: RMLX-0 (2026-08-05) -- o check (c) agora varre RML_TESTS_DIR também, fechando
  #     a lacuna 1 do conserto F4 ("os checks de TOKEN nunca varrem glintfx/tests/"):
  #     um arquivo de teste NOVO com um token Rml:: de tipo-valor cru, sem #include
  #     nenhum, passava o gate em silêncio (os checks (a)/(b) só olham DIRETIVA de
  #     #include, nunca o token em si). EXCL_FILES é alargado de propósito pra cobrir
  #     TAMBÉM RML_TEST_WHITELIST, não só RML_TOKEN_EXCEPTIONS -- DECISÃO (líder
  #     confirma): os 4 congelados do check (b) (domrw_sanity.cpp, focus_sanity.cpp,
  #     form_events_sanity.cpp, document_reload_leak.cpp) já dão #include direto de
  #     RmlUi DE PROPÓSITO (é isso que o check (b) permite), então o uso deles de
  #     token Rml:: de tipo-valor é dívida esperada, não um vazamento novo -- isentá-
  #     los do check (c) também evita reacusar dívida que o (b) já contabiliza sob
  #     outro check. Isto é uma escolha EXPLÍCITA, não a isenção-de-arquivo-inteiro
  #     acidental que o próprio (b) acabou de fechar pra colisão de basename: casado
  #     pelo caminho relativo COMPLETO (RML_TEST_WHITELIST já guarda caminho
  #     completo), nunca basename. TAMBÉM RML_TEST_TOKEN_EXCEPTIONS -- ACHADO AO VIVO
  #     (líder confirma): assim que o check (c) passou a varrer tests/, a árvore real
  #     NÃO MODIFICADA voltou vermelha (não só a fixture plantada) por causa de 2
  #     arquivos pré-existentes que testam a fronteira src/rml/ em si
  #     (input_map_sanity.cpp, type_bridge_review_sanity.cpp) -- ver o comentário na
  #     declaração de RML_TEST_TOKEN_EXCEPTIONS acima pro motivo completo e a prova
  #     de que nenhum dos dois dá #include direto de RmlUi. Qualquer OUTRO arquivo de
  #     teste -- novo ou velho, fora das TRÊS listas congeladas -- continua
  #     reprovando com token cru.
  EXCL_DIR="$RML_DIR"
  EXCL_FILES=("${RML_TOKEN_EXCEPTIONS[@]}" "${RML_TEST_WHITELIST[@]}" "${RML_TEST_TOKEN_EXCEPTIONS[@]}")
  if ! check_token_gate "${RML_SRC_ROOTS[@]}" "$RML_TESTS_DIR"; then
    overall=1
  fi

  EXCL_DIR="$RML_DIR"
  local debt
  debt="$(opaque_debt_count "${RML_SRC_ROOTS[@]}")"
  echo "divida opaca: ${debt} arquivos"

  if [[ "$overall" -ne 0 ]]; then
    echo "FAIL: RmlUi vazou da whitelist -- ver violações acima" >&2
    echo "FAIL: RmlUi leaked out of the whitelist -- see violations above" >&2
    return 1
  fi

  echo "OK: nenhum #include/token de RmlUi fora da whitelist (glintfx/src/rml/ + exceções congeladas)"
  echo "OK: no RmlUi #include/token outside the whitelist (glintfx/src/rml/ + frozen exceptions)"
  return 0
}

# -----------------------------------------------------------------------------
# EN: run_selftest -- SEED-GATE-NAO-GUARDA-SI-MESMO discipline: builds two throwaway
#     fixture trees under mktemp (never touches the real repo), and proves the gate
#     has teeth on BOTH sides -- a clean tree passes, a tree with a planted violation
#     in each of (a)/(b)/(c) fails. If any of the 4 assertions below is wrong, this
#     function itself fails loudly, and the caller (main, below) aborts BEFORE running
#     the real check -- a toothless gate must not silently report "OK" on the real
#     tree.
# PT: run_selftest -- disciplina SEED-GATE-NAO-GUARDA-SI-MESMO: monta duas árvores de
#     fixture descartáveis sob mktemp (nunca toca o repo real), e prova que o gate tem
#     dente dos DOIS lados -- árvore limpa passa, árvore com violação plantada em cada
#     um de (a)/(b)/(c) falha. Se qualquer uma das 4 asserções abaixo estiver errada,
#     esta função falha alto, e quem chama (main, abaixo) aborta ANTES de rodar o check
#     real -- um gate sem dente não pode reportar "OK" em silêncio na árvore real.
# -----------------------------------------------------------------------------
run_selftest() {
  local tmp
  tmp="$(mktemp -d "${TMPDIR:-/tmp}/rml-whitelist-selftest.XXXXXX")"
  # EN: No early `return` between here and the cleanup `rm -rf "$tmp"` below by
  #     construction, so a plain call-at-the-end is enough -- deliberately NOT
  #     `trap ... RETURN`: that trap fires again when the OUTER caller (main) returns
  #     too, by which point `tmp` is out of scope and `set -u` turns the stale
  #     `rm -rf "$tmp"` into an "unbound variable" abort (measured live while writing
  #     this script). This is throwaway scratch, not repo state, so the plain `rm`
  #     here is fine (it is entirely inside our own mktemp dir, never the shared
  #     working tree the CLAUDE.md "never rm scratch" rule guards).
  # PT: Nenhum `return` antecipado entre aqui e o `rm -rf "$tmp"` de limpeza abaixo,
  #     por construção, então uma chamada só no fim basta -- deliberadamente NÃO
  #     `trap ... RETURN`: essa trap dispara de novo quando o chamador EXTERNO (main)
  #     retorna também, e nesse ponto `tmp` já saiu de escopo e o `set -u` transforma o
  #     `rm -rf "$tmp"` obsoleto num abort de "variável não associada" (medido ao vivo
  #     escrevendo este script). É scratch descartável, não estado do repo, então o
  #     `rm` cru aqui é seguro (está inteiramente dentro do nosso próprio dir de
  #     mktemp, nunca a working tree compartilhada que a regra "nunca rm scratch" do
  #     CLAUDE.md protege).

  local ok=1

  # --- fixture 1: clean tree -- MUST pass all 3 blocking checks. ---
  local clean="$tmp/clean"
  mkdir -p "$clean/src/rml" "$clean/include/glintfx" "$clean/demos" "$clean/tests"
  cat > "$clean/src/rml/bootstrap.hpp" <<'EOF'
#pragma once
#include <RmlUi/Core/Context.h>
namespace demo { inline Rml::Context* ctx() { return nullptr; } }
EOF
  cat > "$clean/src/widget.cpp" <<'EOF'
// EN: no RmlUi contact here. Mentions "RmlUi" only in a comment, never in an #include.
#include <string>
namespace demo { std::string name() { return "widget"; } }
EOF
  cat > "$clean/include/glintfx/widget.hpp" <<'EOF'
#pragma once
// EN: dozens-of-comment-mentions stress case: "//" trailing after real code below,
// which strip_comments() must still strip.
int widget_id(); // wraps Rml::Element::GetId() upstream (comment-only, not code)
EOF
  cat > "$clean/tests/widget_sanity.cpp" <<'EOF'
#include "widget.hpp"
int main() { return widget_id() == 0 ? 0 : 1; }
EOF

  EXCL_DIR="$clean/src/rml"
  EXCL_FILES=()
  if ! check_include_gate "$clean/src" "$clean/include" "$clean/demos"; then
    echo "selftest FAIL: check (a) reprovou a fixture LIMPA (falso positivo)" >&2
    ok=0
  fi
  EXCL_FILES=()
  if ! check_test_whitelist_gate "$clean/tests"; then
    echo "selftest FAIL: check (b) reprovou a fixture LIMPA (falso positivo)" >&2
    ok=0
  fi
  EXCL_DIR="$clean/src/rml"
  EXCL_FILES=()
  if ! check_token_gate "$clean/src" "$clean/include" "$clean/demos"; then
    echo "selftest FAIL: check (c) reprovou a fixture LIMPA (falso positivo)" >&2
    ok=0
  fi

  # --- fixture 2: dirty tree -- MUST fail (a), (b) and (c) independently. ---
  local dirty="$tmp/dirty"
  mkdir -p "$dirty/src/rml" "$dirty/include/glintfx" "$dirty/demos" "$dirty/tests"
  cp "$clean/src/rml/bootstrap.hpp" "$dirty/src/rml/bootstrap.hpp"

  # (a): RmlUi include planted OUTSIDE src/rml/.
  cat > "$dirty/src/leaky.cpp" <<'EOF'
#include <RmlUi/Core/Element.h>
void f() {}
EOF

  # (b): RmlUi include in a test NOT in the frozen whitelist.
  cat > "$dirty/tests/new_widget_sanity.cpp" <<'EOF'
#include <RmlUi/Core/Context.h>
int main() { return 0; }
EOF

  # (c): value-type token in real code outside src/rml/ (no #include needed to trip
  #      this check on its own -- proves (c) is independent of (a)).
  cat > "$dirty/src/typeleak.cpp" <<'EOF'
// EN: no #include here on purpose -- (c) must catch the token even without (a) firing.
void g(const Rml::String& s) { (void)s; }
EOF

  EXCL_DIR="$dirty/src/rml"
  EXCL_FILES=()
  if check_include_gate "$dirty/src" "$dirty/include" "$dirty/demos"; then
    echo "selftest FAIL: check (a) NAO reprovou a fixture SUJA (falso negativo -- gate sem dente)" >&2
    ok=0
  fi
  EXCL_FILES=()
  if check_test_whitelist_gate "$dirty/tests"; then
    echo "selftest FAIL: check (b) NAO reprovou a fixture SUJA (falso negativo -- gate sem dente)" >&2
    ok=0
  fi
  EXCL_DIR="$dirty/src/rml"
  EXCL_FILES=()
  if check_token_gate "$dirty/src" "$dirty/include" "$dirty/demos"; then
    echo "selftest FAIL: check (c) NAO reprovou a fixture SUJA (falso negativo -- gate sem dente)" >&2
    ok=0
  fi

  # --- fixture 3: RMLX-0 basename-collision exploit -- proves check (b) now matches
  #     by FULL PATH, not basename. Adversarial review of the pre-fix gate proved a
  #     real hole: a NEW file whose basename collided with a frozen whitelist entry
  #     (but lived at a DIFFERENT path, e.g. a subdir) rode the gate clean because
  #     only the basename was compared. Two independent trees, each isolated from any
  #     other violation, so a failure here can only mean the collision itself: (3a)
  #     the legitimately whitelisted file at its exact frozen path must still pass;
  #     (3b) a same-basename file at a DIFFERENT, un-whitelisted path must still fail.
  # PT: fixture 3 -- exploit de colisão de basename da RMLX-0 -- prova que o check (b)
  #     agora casa por CAMINHO COMPLETO, não por basename. A revisão adversarial do
  #     gate pré-conserto provou um furo real: um arquivo NOVO cujo basename colidia
  #     com uma entrada congelada da whitelist (mas vivia em um caminho DIFERENTE, ex.
  #     um subdir) passava o gate limpo porque só o basename era comparado. Duas
  #     árvores independentes, cada uma isolada de qualquer outra violação, pra uma
  #     falha aqui só poder significar a própria colisão: (3a) o arquivo
  #     legitimamente whitelisted no seu caminho congelado exato ainda tem de passar;
  #     (3b) um arquivo de mesmo basename num caminho DIFERENTE, não-whitelisted,
  #     ainda tem de falhar.
  local collision_ok="$tmp/collision_ok"
  mkdir -p "$collision_ok/tests"
  cat > "$collision_ok/tests/domrw_sanity.cpp" <<'EOF'
#include <RmlUi/Core/ElementDocument.h>
int main() { return 0; }
EOF
  EXCL_FILES=("$collision_ok/tests/domrw_sanity.cpp")
  if ! check_test_whitelist_gate "$collision_ok/tests"; then
    echo "selftest FAIL: check (b) reprovou o arquivo whitelisted legitimo no caminho completo exato (falso positivo)" >&2
    ok=0
  fi

  local collision_bad="$tmp/collision_bad"
  mkdir -p "$collision_bad/tests/subdir"
  cat > "$collision_bad/tests/subdir/domrw_sanity.cpp" <<'EOF'
#include <RmlUi/Core/ElementDocument.h>
int main() { return 0; }
EOF
  EXCL_FILES=("$collision_bad/tests/domrw_sanity.cpp")
  if check_test_whitelist_gate "$collision_bad/tests"; then
    echo "selftest FAIL: check (b) NAO reprovou colisao de basename em subdir nao-whitelisted (falso negativo -- o furo RMLX-0 que a revisao adversarial provou)" >&2
    ok=0
  fi

  # --- fixture 4/5: RMLX-0 whole-file-exemption exploit -- proves check (a) exempts
  #     only the ALLOWED include text (RmlUi_Platform_GLFW.h) in an EXCL_FILES entry,
  #     not the whole file. Adversarial review of the pre-fix gate proved a real
  #     hole: planting ANY RmlUi #include (e.g. <RmlUi/Core/ElementDocument.h>,
  #     wholly unrelated to the GLFW bridge) in glintfx/src/app.cpp still passed the
  #     gate clean, because the whole file was skipped once its path matched
  #     EXCL_FILES. Two independent trees: (4) the single legitimately-exempt
  #     include, alone in an excepted file, must still pass; (5) the SAME excepted
  #     file with an unrelated RmlUi #include ALSO present must still fail.
  # PT: fixture 4/5 -- exploit de isenção do arquivo inteiro da RMLX-0 -- prova que o
  #     check (a) isenta só o texto de include PERMITIDO (RmlUi_Platform_GLFW.h) num
  #     arquivo de EXCL_FILES, não o arquivo inteiro. A revisão adversarial do gate
  #     pré-conserto provou um furo real: plantar QUALQUER #include de RmlUi (ex.
  #     <RmlUi/Core/ElementDocument.h>, sem relação nenhuma com a ponte GLFW) em
  #     glintfx/src/app.cpp ainda passava o gate limpo, porque o arquivo inteiro era
  #     pulado assim que o caminho casava EXCL_FILES. Duas árvores independentes: (4)
  #     o único include legitimamente isento, sozinho num arquivo isento, ainda tem
  #     de passar; (5) o MESMO arquivo isento com um #include de RmlUi não
  #     relacionado TAMBÉM presente ainda tem de falhar.
  local except_ok="$tmp/except_ok"
  mkdir -p "$except_ok/src"
  cat > "$except_ok/src/app.cpp" <<'EOF'
#include "RmlUi_Platform_GLFW.h"
void f() {}
EOF
  EXCL_DIR="$except_ok/src/rml"
  EXCL_FILES=("$except_ok/src/app.cpp")
  if ! check_include_gate "$except_ok/src"; then
    echo "selftest FAIL: check (a) reprovou o unico include permitido no arquivo isento (falso positivo)" >&2
    ok=0
  fi

  local except_bad="$tmp/except_bad"
  mkdir -p "$except_bad/src"
  cat > "$except_bad/src/app.cpp" <<'EOF'
#include "RmlUi_Platform_GLFW.h"
#include <RmlUi/Core/ElementDocument.h>
void f() {}
EOF
  EXCL_DIR="$except_bad/src/rml"
  EXCL_FILES=("$except_bad/src/app.cpp")
  if check_include_gate "$except_bad/src"; then
    echo "selftest FAIL: check (a) NAO reprovou include RmlUi nao-relacionado plantado no arquivo isento (falso negativo -- o furo RMLX-0 que a revisao adversarial provou)" >&2
    ok=0
  fi

  # --- fixture 6/7: RMLX-0 raw-string comment-swallow exploit -- proves
  #     strip_comments() understands C++ raw-string literals
  #     (R"delim(...)delim") and does not let an unterminated `/*` INSIDE a raw
  #     string's body open a real block-comment state that survives past the raw
  #     string's own close and swallows real code after it. Adversarial review of
  #     the pre-fix gate proved a real hole: planting
  #     `Rml::String leaked_token_should_be_caught` right after a multi-line raw
  #     string whose body had an unescaped `/*` (no matching `*/` on that same
  #     line) still returned OK/exit 0, because `in_block` -- global, carried
  #     across lines -- opened on that stray `/*` and never closed, silently
  #     deleting every line after it, including the real token. Two independent
  #     trees, non-empty delimiter ("GLSL", not the trivial empty-delimiter case)
  #     so the delimiter-matching path itself is exercised, not just the fast
  #     empty-delim shortcut: (6) a real `Rml::String` token AFTER such a raw
  #     string must still be caught (proves the fix); (7) the SAME
  #     unterminated-looking `/*` inside a raw string, but with ZERO Rml:: contact
  #     anywhere in the file, must stay a clean pass (proves the fix does not
  #     overcorrect into a false positive on legitimate shader-source raw
  #     strings -- the exact genre of text glintfx/src/draw2d.cpp carries for
  #     real).
  # PT: fixture 6/7 -- exploit de engolir-comentario via raw string da RMLX-0 --
  #     prova que o strip_comments() entende raw-string literal de C++
  #     (R"delim(...)delim") e nao deixa um `/*` sem fechar DENTRO do corpo de um
  #     raw string abrir um estado de comentario-de-bloco real que sobrevive alem
  #     do proprio fechamento do raw string e engole codigo de verdade depois
  #     dele. A revisao adversarial do gate pre-conserto provou um furo real:
  #     plantar `Rml::String leaked_token_should_be_caught` logo apos um raw
  #     string multi-linha cujo corpo tinha um `/*` sem escape (sem `*/` casado
  #     na mesma linha) ainda devolvia OK/exit 0, porque `in_block` -- global,
  #     carregado entre linhas -- abria naquele `/*` perdido e nunca fechava,
  #     apagando em silencio toda linha depois dele, inclusive o token real. Duas
  #     arvores independentes, com delimitador NAO-vazio ("GLSL", nao o caso
  #     trivial de delimitador vazio) pra exercitar o proprio caminho de
  #     casamento de delimitador, nao so o atalho rapido de delim vazio: (6) um
  #     token `Rml::String` real DEPOIS de um raw string desses ainda tem de ser
  #     pego (prova o conserto); (7) o MESMO `/*` com jeito de nao-fechado dentro
  #     de um raw string, mas com ZERO contato Rml:: no arquivo inteiro, tem de
  #     continuar passando limpo (prova que o conserto nao super-corrige pra
  #     falso positivo em raw string de shader legitimo -- o mesmo genero de
  #     texto que glintfx/src/draw2d.cpp carrega de verdade).
  local rawleak="$tmp/rawleak"
  mkdir -p "$rawleak/src"
  cat > "$rawleak/src/shader_leak.cpp" <<'EOF'
// EN: raw string body below carries an unterminated `/*` marker -- the awk state
// machine must NOT treat this as opening a real block comment that survives past
// the raw string's own close.
const char* kShader = R"GLSL(
/* this looks like it opens a block comment but never closes inside the raw string
void main() {}
)GLSL";
void leak(const Rml::String& s) { (void)s; }
EOF
  EXCL_DIR="$rawleak/src/rml"
  EXCL_FILES=()
  if check_token_gate "$rawleak/src"; then
    echo "selftest FAIL: check (c) NAO reprovou o token Rml::String apos um raw string com /* nao fechado (falso negativo -- o furo de raw string que a revisao adversarial provou)" >&2
    ok=0
  fi

  local rawclean="$tmp/rawclean"
  mkdir -p "$rawclean/src"
  cat > "$rawclean/src/shader_clean.cpp" <<'EOF'
// EN: same unterminated-looking `/*` inside a raw string body, but with ZERO Rml::
// contact anywhere in the file -- must stay a clean pass end-to-end (proves the fix
// does not overcorrect into a false positive on legitimate shader-source raw strings).
const char* kShader = R"GLSL(
/* also looks like an open block comment, still just shader text */
void main() {}
)GLSL";
EOF
  EXCL_DIR="$rawclean/src/rml"
  EXCL_FILES=()
  if ! check_token_gate "$rawclean/src"; then
    echo "selftest FAIL: check (c) reprovou raw string legitima (com /* no corpo) sem nenhum token Rml:: (falso positivo)" >&2
    ok=0
  fi

  # --- fixture 8: RMLX-0 gap 1 -- token checks never scanned glintfx/tests/. Three
  #     independent sub-cases: (8a) a NEW test file with a raw Rml:: value-type
  #     token and NO #include at all must now fail check (c); (8b) the check (b)
  #     frozen whitelist, EXEMPTED from check (c) too by the explicit RMLX-0
  #     decision (documented at the run_real_check() call site), must still pass
  #     when it legitimately uses RmlUi types, matched by FULL path; (8c) a
  #     DIFFERENT file that merely shares a frozen entry's basename must NOT ride
  #     the exemption -- same basename-collision doctrine fixture 3 already proved
  #     for check (b), now proved for check (c)'s own EXCL_FILES match too.
  # PT: fixture 8 -- lacuna 1 da RMLX-0 -- os checks de token nunca varriam
  #     glintfx/tests/. Tres sub-casos independentes: (8a) um arquivo de teste NOVO
  #     com um token Rml:: de tipo-valor cru e SEM #include nenhum agora tem de
  #     reprovar o check (c); (8b) a whitelist congelada do check (b), ISENTA do
  #     check (c) tambem pela decisao explicita da RMLX-0 (documentada no ponto de
  #     chamada de run_real_check()), ainda tem de passar quando usa tipo RmlUi de
  #     verdade, casada pelo caminho COMPLETO; (8c) um arquivo DIFERENTE que so
  #     compartilha o basename de uma entrada congelada NAO pode andar de carona na
  #     isencao -- mesma doutrina de colisao de basename que a fixture 3 ja provou
  #     pro check (b), agora provada pro proprio casamento de EXCL_FILES do check
  #     (c).
  local tests_token_leak="$tmp/tests_token_leak"
  mkdir -p "$tests_token_leak/tests"
  cat > "$tests_token_leak/tests/token_sem_include.cpp" <<'EOF'
#include <string>
void f(const Rml::String& s) { (void)s; }
EOF
  EXCL_DIR="$tests_token_leak/nonexistent_rml"
  EXCL_FILES=()
  if check_token_gate "$tests_token_leak/tests"; then
    echo "selftest FAIL: check (c) NAO reprovou token Rml:: cru em glintfx/tests/, sem #include algum (falso negativo -- lacuna 1 da RMLX-0)" >&2
    ok=0
  fi

  local tests_token_exempt="$tmp/tests_token_exempt"
  mkdir -p "$tests_token_exempt/tests"
  cat > "$tests_token_exempt/tests/domrw_sanity.cpp" <<'EOF'
#include <RmlUi/Core/ElementDocument.h>
void f(const Rml::String& s) { (void)s; }
EOF
  EXCL_DIR="$tests_token_exempt/nonexistent_rml"
  EXCL_FILES=("$tests_token_exempt/tests/domrw_sanity.cpp")
  if ! check_token_gate "$tests_token_exempt/tests"; then
    echo "selftest FAIL: check (c) reprovou Rml::String legitimo no arquivo de teste congelado isento (falso positivo -- a isencao explicita da RMLX-0 nao esta funcionando)" >&2
    ok=0
  fi

  local tests_token_collision="$tmp/tests_token_collision"
  mkdir -p "$tests_token_collision/tests/subdir"
  cat > "$tests_token_collision/tests/subdir/domrw_sanity.cpp" <<'EOF'
#include <RmlUi/Core/ElementDocument.h>
void f(const Rml::String& s) { (void)s; }
EOF
  EXCL_DIR="$tests_token_collision/nonexistent_rml"
  EXCL_FILES=("$tests_token_collision/tests/domrw_sanity.cpp")
  if check_token_gate "$tests_token_collision/tests"; then
    echo "selftest FAIL: check (c) NAO reprovou colisao de basename em subdir nao-isento (falso negativo -- a isencao vazou por basename, nao por caminho completo)" >&2
    ok=0
  fi

  # --- fixture 8d/8e: RML_TEST_TOKEN_EXCEPTIONS -- the SEPARATE exemption found
  #     live when the real tree (not just a synthetic fixture) came back red the
  #     moment check (c) started scanning tests/ (input_map_sanity.cpp,
  #     type_bridge_review_sanity.cpp -- see the exception's own declaration
  #     comment for the full story). Mirrors 8b/8c's shape but against the
  #     SEPARATE list: (8d) a file at the exact frozen path, with no RmlUi
  #     #include at all (proving this exemption is independent of check (b)'s),
  #     must still pass check (c); (8e) a same-basename file at a DIFFERENT path
  #     must NOT ride the exemption.
  # PT: fixture 8d/8e -- RML_TEST_TOKEN_EXCEPTIONS -- a isencao SEPARADA achada ao
  #     vivo quando a arvore real (nao so uma fixture sintetica) voltou vermelha
  #     no momento em que o check (c) passou a varrer tests/
  #     (input_map_sanity.cpp, type_bridge_review_sanity.cpp -- ver o comentario da
  #     propria declaracao da excecao pra historia completa). Espelha o formato de
  #     8b/8c mas contra a lista SEPARADA: (8d) um arquivo no caminho congelado
  #     exato, sem #include de RmlUi nenhum (provando que esta isencao e
  #     independente da do check (b)), ainda tem de passar o check (c); (8e) um
  #     arquivo de mesmo basename num caminho DIFERENTE nao pode andar de carona
  #     na isencao.
  local tests_bridge_exempt="$tmp/tests_bridge_exempt"
  mkdir -p "$tests_bridge_exempt/tests"
  cat > "$tests_bridge_exempt/tests/input_map_sanity.cpp" <<'EOF'
#include <string>
void f() { int x = static_cast<int>(Rml::Input::KI_UP); (void)x; }
EOF
  EXCL_DIR="$tests_bridge_exempt/nonexistent_rml"
  EXCL_FILES=("$tests_bridge_exempt/tests/input_map_sanity.cpp")
  if ! check_token_gate "$tests_bridge_exempt/tests"; then
    echo "selftest FAIL: check (c) reprovou token Rml:: legitimo no arquivo de teste-fronteira isento (falso positivo -- RML_TEST_TOKEN_EXCEPTIONS nao esta funcionando)" >&2
    ok=0
  fi

  local tests_bridge_collision="$tmp/tests_bridge_collision"
  mkdir -p "$tests_bridge_collision/tests/subdir"
  cat > "$tests_bridge_collision/tests/subdir/input_map_sanity.cpp" <<'EOF'
#include <string>
void f() { int x = static_cast<int>(Rml::Input::KI_UP); (void)x; }
EOF
  EXCL_DIR="$tests_bridge_collision/nonexistent_rml"
  EXCL_FILES=("$tests_bridge_collision/tests/input_map_sanity.cpp")
  if check_token_gate "$tests_bridge_collision/tests"; then
    echo "selftest FAIL: check (c) NAO reprovou colisao de basename em subdir nao-isento na RML_TEST_TOKEN_EXCEPTIONS (falso negativo -- a isencao vazou por basename, nao por caminho completo)" >&2
    ok=0
  fi

  # --- fixture 9: RMLX-0 gap 2 -- in glintfx/tests/, check (b)'s own `find` only
  #     matched *.cpp/*.hpp. A NEW *.h test helper with a live RmlUi #include must
  #     now fail check (b).
  # PT: fixture 9 -- lacuna 2 da RMLX-0 -- em glintfx/tests/, o proprio `find` do
  #     check (b) so casava *.cpp/*.hpp. Um *.h NOVO de apoio de teste com
  #     #include de RmlUi vivo agora tem de reprovar o check (b).
  local tests_header_leak="$tmp/tests_header_leak"
  mkdir -p "$tests_header_leak/tests"
  cat > "$tests_header_leak/tests/novo_widget.h" <<'EOF'
#include <RmlUi/Core/Element.h>
EOF
  EXCL_FILES=()
  if check_test_whitelist_gate "$tests_header_leak/tests"; then
    echo "selftest FAIL: check (b) NAO reprovou #include de RmlUi num .h NOVO de teste (falso negativo -- lacuna 2 da RMLX-0)" >&2
    ok=0
  fi

  # --- fixture 10: RMLX-0 gap 3 -- .cc/.cxx/.hh/.hxx were outside every check's
  #     `find` everywhere. A live RmlUi #include in a .cc file (check a) and a raw
  #     value-type token in a .hh file (check c) must both now be caught.
  # PT: fixture 10 -- lacuna 3 da RMLX-0 -- .cc/.cxx/.hh/.hxx ficavam fora do
  #     `find` de todo check, em todo lugar. Um #include de RmlUi vivo num arquivo
  #     .cc (check a) e um token de tipo-valor cru num arquivo .hh (check c) agora
  #     tem os dois de ser pegos.
  local ext_leak="$tmp/ext_leak"
  mkdir -p "$ext_leak/src/rml"
  cat > "$ext_leak/src/leaky.cc" <<'EOF'
#include <RmlUi/Core/Element.h>
void f() {}
EOF
  cat > "$ext_leak/src/typeleak.hh" <<'EOF'
void g(const Rml::String& s) { (void)s; }
EOF
  EXCL_DIR="$ext_leak/src/rml"
  EXCL_FILES=()
  if check_include_gate "$ext_leak/src"; then
    echo "selftest FAIL: check (a) NAO reprovou #include de RmlUi em arquivo .cc (falso negativo -- lacuna 3 da RMLX-0, extensao fora do find)" >&2
    ok=0
  fi
  EXCL_DIR="$ext_leak/src/rml"
  EXCL_FILES=()
  if check_token_gate "$ext_leak/src"; then
    echo "selftest FAIL: check (c) NAO reprovou token Rml:: em arquivo .hh (falso negativo -- lacuna 3 da RMLX-0, extensao fora do find)" >&2
    ok=0
  fi

  # --- fixture 11: RMLX-0 gap 4 -- `find` without `-L` treats a symlink as type l,
  #     never type f, so a symlink pointing at a file with a real violation was
  #     silently dropped. Real file lives OUTSIDE the scanned tree; only a symlink
  #     inside it points at the violation -- a pass here can only mean `-L` is
  #     working, not that the real file was scanned directly.
  # PT: fixture 11 -- lacuna 4 da RMLX-0 -- `find` sem `-L` trata symlink como type
  #     l, nunca type f, entao um symlink apontando pra um arquivo com violacao
  #     real era descartado em silencio. O arquivo real vive FORA da arvore
  #     varrida; so um symlink dentro dela aponta pra violacao -- uma reprovacao
  #     aqui so pode significar que o `-L` esta funcionando, nao que o arquivo real
  #     foi varrido direto.
  local symlink_leak="$tmp/symlink_leak"
  mkdir -p "$symlink_leak/real" "$symlink_leak/src/rml"
  cat > "$symlink_leak/real/hidden_leaky.cpp" <<'EOF'
#include <RmlUi/Core/Element.h>
void f() {}
EOF
  ln -s "$symlink_leak/real/hidden_leaky.cpp" "$symlink_leak/src/via_symlink.cpp"
  EXCL_DIR="$symlink_leak/src/rml"
  EXCL_FILES=()
  if check_include_gate "$symlink_leak/src"; then
    echo "selftest FAIL: check (a) NAO reprovou #include de RmlUi atras de um symlink (falso negativo -- lacuna 4 da RMLX-0, find sem -L)" >&2
    ok=0
  fi

  rm -rf "$tmp"

  if [[ "$ok" -ne 1 ]]; then
    return 1
  fi
  echo "selftest OK: fixture limpa passa nos 3 checks bloqueantes, fixture suja falha nos 3 (independentemente), a colisao de basename em subdir da RMLX-0 continua bloqueada, a isencao do check (a) agora vale so o include permitido -- nao o arquivo inteiro --, o strip_comments() entende raw string de C++ (pega o token apos /* nao fechado dentro do raw string, e nao falso-positiva em raw string legitima), e as 4 lacunas de enumeracao de arquivo da RMLX-0 (token em tests/, header .h em tests/, extensoes .cc/.cxx/.hh/.hxx, symlink) estao fechadas sem reabrir falso positivo na isencao explicita dos 4 congelados"
  return 0
}

# -----------------------------------------------------------------------------
# EN: main.
# PT: main.
# -----------------------------------------------------------------------------
main() {
  case "${1:-}" in
    --selftest)
      run_selftest
      return $?
      ;;
    -h|--help)
      cat <<EOF
Usage: ${SCRIPT_NAME} [--selftest]
  (no args)    selftest, then the real check against the repo tree in CWD.
  --selftest   selftest only, self-contained (mktemp), no repo needed.
EOF
      return 0
      ;;
    "")
      ;;
    *)
      echo "error: unknown argument '$1' (see --help)" >&2
      return 2
      ;;
  esac

  echo "-- RMLX-0/F4 selftest (SEED-GATE-NAO-GUARDA-SI-MESMO) --"
  if ! run_selftest; then
    echo "FAIL: o proprio gate esta sem dente -- abortando ANTES do check real (nao confiar num gate que nao passou no proprio selftest)" >&2
    return 1
  fi

  echo "-- RMLX-0/F4 check real --"
  run_real_check
}

main "$@"
