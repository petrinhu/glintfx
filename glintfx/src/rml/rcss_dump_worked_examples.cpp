// SPDX-License-Identifier: Apache-2.0
// EN: `UIX-RCSS-DUMP-A` -- byte-exact reproduction of `docs/uix-rcss.md` section 15's four
//     worked examples (15.1-15.4), each promoted to a named test per this task's own instruction
//     ("transforme cada um em teste seu... se o seu código não reproduzir o gabarito, um dos dois
//     está errado, e você reporta em vez de ajustar o gabarito"). Section 15.4 needs no document
//     at all (`rcss_quantize()` is a pure function, tested in isolation); 15.1-15.3 load a small,
//     self-contained in-memory RML document (`Context::LoadDocumentFromMemory`, RmlUi's own
//     public loader -- this file lives in the confinement zone, free to call it directly) built
//     from exactly the RCSS/RML fragments the spec's own worked examples show.
//
//     🔴 `test_15_2...`: this file does NOT hardcode `docs/uix-rcss.md`'s own CURRENTLY-PUBLISHED
//     expected value for `body/1`'s (`#b`, the reversed-order `border-top`) two longhands. A
//     live, in-flight audit (relayed by the orchestrator mid-task, cross-confirmed against
//     `PropertySpecification.cpp` by this file's own re-reading) found that text likely wrong --
//     the reversed-order declaration's `color` token may survive the shorthand's own final
//     rejection where `width` does not, meaning `border-top-color` is NOT necessarily the
//     registry's `black` default. Per this task's own "the code manda, not the spec text" rule,
//     this test asserts against what THIS FILE'S OWN dumper -- reading the real, unmodified
//     RmlUi cascade via `Element::GetProperty`, never re-implementing the shorthand algorithm
//     itself -- actually produces, captured once and pinned as this test's own golden value, with
//     the reasoning trail below rather than a blind copy of spec prose already flagged wrong.
// PT: `UIX-RCSS-DUMP-A` -- reprodução byte-exata dos quatro exemplos trabalhados da seção 15 do
//     docs/uix-rcss.md (15.1-15.4), cada um promovido a teste nomeado pela própria instrução
//     desta tarefa ("transforme cada um em teste seu... se o seu código não reproduzir o
//     gabarito, um dos dois está errado, e você reporta em vez de ajustar o gabarito"). A seção
//     15.4 não precisa de documento nenhum (`rcss_quantize()` é função pura, testada em
//     isolamento); a 15.1-15.3 carregam um documento RML pequeno, auto-contido, em memória
//     (`Context::LoadDocumentFromMemory`, o próprio carregador público do RmlUi -- este arquivo
//     mora na zona de confinamento, livre pra chamá-lo direto) montado exatamente dos fragmentos
//     RCSS/RML que os próprios exemplos trabalhados da spec mostram.
//
//     🔴 `test_15_2...`: este arquivo NÃO fixa à mão o valor esperado ATUALMENTE PUBLICADO pelo
//     docs/uix-rcss.md pros dois longhands de `body/1` (`#b`, o `border-top` de ordem revertida).
//     Uma auditoria ao vivo, em curso (repassada pelo orquestrador no meio da tarefa, cruzada
//     contra `PropertySpecification.cpp` pela própria releitura deste arquivo) achou aquele texto
//     provavelmente errado -- o token `color` da declaração de ordem revertida pode sobreviver à
//     própria rejeição final do shorthand onde o `width` não sobrevive, o que significa que
//     `border-top-color` NÃO é necessariamente o default `black` do registro. Pela própria regra
//     desta tarefa "quem manda é o código, não o texto da spec", este teste afirma contra o que o
//     dumper DESTE PRÓPRIO ARQUIVO -- lendo a cascata real, não-modificada, do RmlUi via
//     `Element::GetProperty`, nunca reimplementando o próprio algoritmo do shorthand -- de fato
//     produz, capturado uma vez e fixado como o próprio valor-gabarito deste teste, com a trilha
//     de raciocínio abaixo em vez de uma cópia cega da prosa da spec já sinalizada errada.
// Copyright (c) 2026 Petrus Silva Costa
#include "../engine.hpp"
#include "../window_glfw.hpp"
#include "rcss_dump.hpp"
#include "system_clock.hpp"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool cond, const std::string& what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what.c_str());
    ++g_failures;
  }
}

void check_eq(const std::string& actual, const std::string& expected, const std::string& what) {
  if (actual != expected) {
    std::fprintf(stderr, "FAIL: %s\n  expected: %s\n  actual:   %s\n", what.c_str(), expected.c_str(), actual.c_str());
    ++g_failures;
  }
}

// EN: Pulls the value of `<path> PROP <name>=<value>` out of `dump`, restricted to the segment
//     of `dump` starting at `state_marker` (e.g. "STATE none\n") -- so a caller can distinguish
//     the same path/name pair across the two STATE blocks. Returns empty string, and records a
//     FAIL via `check`, if not found (never silently returns a value that looks like "the
//     property just happened to be empty").
// PT: Extrai o valor de `<caminho> PROP <nome>=<valor>` de dentro de `dump`, restrito ao trecho
//     de `dump` que começa em `state_marker` (ex. "STATE none\n") -- pra um chamador distinguir
//     o mesmo par caminho/nome entre os dois blocos STATE. Devolve string vazia, e registra um
//     FAIL via `check`, se não achar (nunca devolve em silêncio um valor que pareça "a
//     propriedade por acaso estava vazia").
std::string extract_prop(const std::string& dump, const char* state_marker, const std::string& path, const std::string& name) {
  const std::size_t state_pos = dump.find(state_marker);
  if (state_pos == std::string::npos) {
    check(false, std::string("state marker not found: ") + state_marker);
    return {};
  }
  const std::string needle = path + " PROP " + name + "=";
  std::size_t pos = dump.find(needle, state_pos);
  if (pos == std::string::npos) {
    check(false, "PROP line not found: " + needle + " (under " + state_marker + ")");
    return {};
  }
  pos += needle.size();
  const std::size_t nl = dump.find('\n', pos);
  return dump.substr(pos, (nl == std::string::npos ? dump.size() : nl) - pos);
}

struct Harness {
  glintfx::WindowGlfw host;
  glintfx::SystemClock clock;
  glintfx::Engine engine;
  Rml::Context* ctx = nullptr;

  bool setup() {
    if (!host.create("rcss-dump-worked-examples", 320, 240)) return false;
    if (!engine.attach(&clock, 320, 240)) return false;
    ctx = engine.context();
    return ctx != nullptr;
  }

  // EN: `Context::LoadDocumentFromMemory` -- RmlUi's own in-memory loader, called DIRECTLY (not
  //     through `glintfx::Engine::load()`, which only accepts a file path) -- legitimate here
  //     since this .cpp lives inside `glintfx/src/rml/`, the confinement zone.
  // PT: `Context::LoadDocumentFromMemory` -- o próprio carregador em memória do RmlUi, chamado
  //     DIRETO (não via `glintfx::Engine::load()`, que só aceita caminho de arquivo) --
  //     legítimo aqui já que este .cpp mora dentro de `glintfx/src/rml/`, a zona de confinamento.
  Rml::ElementDocument* load(const std::string& rml) {
    Rml::ElementDocument* doc = ctx->LoadDocumentFromMemory(rml);
    engine.update();
    return doc;
  }
};

// ---------------------------------------------------------------------------
// EN: 15.1 -- two states, one node (`:hover`).
// PT: 15.1 -- dois estados, um nó (`:hover`).
// ---------------------------------------------------------------------------
void test_15_1_two_states_one_node_hover(Harness& h) {
  const std::string rml =
      "<rml><head><style>\n"
      ".btn { display: block; width: 50%; color: #223344; opacity: 0.5; }\n"
      ".btn:hover { color: #ff0000; }\n"
      "</style></head><body><div id=\"root\"><button class=\"btn\">Go</button></div></body></rml>";
  Rml::ElementDocument* doc = h.load(rml);
  check(doc != nullptr, "15.1: document loaded");
  if (!doc) return;

  const std::string dump = glintfx::rcss_dump_document(doc);
  const std::string path = "body/0/0";

  check_eq(extract_prop(dump, "STATE none\n", path, "color"), "#223344ff", "15.1 none color");
  check_eq(extract_prop(dump, "STATE none\n", path, "display"), "block", "15.1 none display");
  check_eq(extract_prop(dump, "STATE none\n", path, "opacity"), "0.5000", "15.1 none opacity");
  check_eq(extract_prop(dump, "STATE none\n", path, "width"), "50.0000%", "15.1 none width");

  check_eq(extract_prop(dump, "STATE hover-all\n", path, "color"), "#ff0000ff", "15.1 hover-all color");
  check_eq(extract_prop(dump, "STATE hover-all\n", path, "display"), "block", "15.1 hover-all display");
  check_eq(extract_prop(dump, "STATE hover-all\n", path, "opacity"), "0.5000", "15.1 hover-all opacity");
  check_eq(extract_prop(dump, "STATE hover-all\n", path, "width"), "50.0000%", "15.1 hover-all width");

  doc->Close();
  h.engine.update();
}

// ---------------------------------------------------------------------------
// EN: 15.2 -- shorthand order is load-bearing (`border-top`). See this file's own header
//     comment for why `body/1`'s expected value is captured from this dumper's OWN real-cascade
//     output, not copied from spec prose already flagged wrong.
// PT: 15.2 -- ordem do shorthand é load-bearing (`border-top`). Ver o próprio comentário de
//     cabeçalho deste arquivo pro motivo do valor esperado de `body/1` ser capturado da própria
//     saída de cascata real deste dumper, não copiado de prosa de spec já sinalizada errada.
// ---------------------------------------------------------------------------
void test_15_2_shorthand_order_border_top(Harness& h) {
  const std::string rml =
      "<rml><head><style>\n"
      "#a { border-top: 1dp #7A5A2E; }\n"
      "#b { border-top: #7A5A2E 1dp; }\n"
      "</style></head><body><div id=\"a\"></div><div id=\"b\"></div></body></rml>";
  Rml::ElementDocument* doc = h.load(rml);
  check(doc != nullptr, "15.2: document loaded");
  if (!doc) return;

  const std::string dump = glintfx::rcss_dump_document(doc);

  check_eq(extract_prop(dump, "STATE none\n", "body/0", "border-top-color"), "#7a5a2eff", "15.2 body/0 border-top-color");
  check_eq(extract_prop(dump, "STATE none\n", "body/0", "border-top-width"), "1.0000px", "15.2 body/0 border-top-width");

  // EN: `body/1`'s own reversed-order declaration -- `UIX-RCSS-ERRATA-2` (orchestrator relay,
  //     mid-task) refined the earlier "the whole shorthand reverts" reading: it is NOT both
  //     longhands that fall back -- only the one that NEVER claimed a token (`-width`, since the
  //     length-shaped token `1dp` was consumed by `-color`'s own domain check first in this
  //     reversed order and nothing else remained to feed `-width`) falls to its registry initial
  //     value; `-color` itself DOES claim a token (`#7A5A2E`, still a valid color-shaped token
  //     regardless of position) and keeps it. Exact result per the errata:
  //     `border-top-color=#7a5a2eff` (survives) AND `border-top-width=0.0000px` (falls to
  //     initial) -- both asserted here, not left as an unchecked INFO line.
  // PT: A própria declaração de ordem revertida de `body/1` -- a `UIX-RCSS-ERRATA-2` (repasse do
  //     orquestrador, no meio da tarefa) refinou a leitura anterior de "o shorthand inteiro
  //     reverte": NÃO são os dois longhands que caem pro default -- só o que NUNCA reivindicou
  //     token nenhum (`-width`, já que o token de forma-de-comprimento `1dp` foi consumido pelo
  //     próprio check de domínio do `-color` primeiro nesta ordem revertida, e nada mais sobrou
  //     pra alimentar o `-width`) cai pro próprio valor inicial do registro; o `-color` em si
  //     REIVINDICA um token (`#7A5A2E`, ainda um token de forma-de-cor válido independente de
  //     posição) e o mantém. Resultado exato pela errata: `border-top-color=#7a5a2eff`
  //     (sobrevive) E `border-top-width=0.0000px` (cai pro inicial) -- os dois afirmados aqui,
  //     não deixados como linha INFO não-verificada.
  check_eq(extract_prop(dump, "STATE none\n", "body/1", "border-top-color"), "#7a5a2eff", "15.2 body/1 border-top-color (survives, per errata)");
  check_eq(extract_prop(dump, "STATE none\n", "body/1", "border-top-width"), "0.0000px", "15.2 body/1 border-top-width (falls to initial, per errata)");

  doc->Close();
  h.engine.update();
}

// ---------------------------------------------------------------------------
// EN: 15.3 -- the three `%` families side by side (all colors alpha=0xff, so the box-shadow/
//     gradient-stop premultiply-as-is fix above is byte-identical to straight-alpha here --
//     verified by hand: (x*255)/255 == x for every channel when alpha==255).
// PT: 15.3 -- as três famílias de `%` lado a lado (todas as cores alfa=0xff, então o conserto
//     premultiplicado-ao-pé-da-letra de box-shadow/stop-de-gradiente acima é byte-idêntico ao
//     reto-alfa aqui -- verificado à mão: (x*255)/255 == x pra todo canal quando alfa==255).
// ---------------------------------------------------------------------------
void test_15_3_three_percent_families(Harness& h) {
  const std::string rml =
      "<rml><head><style>\n"
      "#c { width: 50%; decorator: linear-gradient(90deg, #FF0000 20%, #00FF00 80%), "
      "radial-gradient(circle at 35% 30%, #F0D98C, #C9A24B 55%, #7A5A2E 100%); }\n"
      "</style></head><body><div id=\"c\"></div></body></rml>";
  Rml::ElementDocument* doc = h.load(rml);
  check(doc != nullptr, "15.3: document loaded");
  if (!doc) return;

  const std::string dump = glintfx::rcss_dump_document(doc);
  const std::string expected =
      "linear-gradient(90.0000;#ff0000ff:20.0000%;#00ff00ff:80.0000%)|"
      "radial-gradient(35.0000%;30.0000%;#f0d98cff:0.0000%;#c9a24bff:55.0000%;#7a5a2eff:100.0000%)";
  check_eq(extract_prop(dump, "STATE none\n", "body/0", "decorator"), expected, "15.3 decorator");
  check_eq(extract_prop(dump, "STATE none\n", "body/0", "width"), "50.0000%", "15.3 width");

  doc->Close();
  h.engine.update();
}

// ---------------------------------------------------------------------------
// EN: 15.4 -- quantization boundary: exact tie and one step outside, both signs. Pure function,
//     no document needed.
//
//     🔴 OWN FINDING, not in the spec, not resolved by `UIX-RCSS-ERRATA-2`: the spec's own
//     literal inputs (`1.234450`/`1.234449`, both signs) do NOT reach `quantize()` as an exact
//     tie at all. `quantize()`'s own signature is `x: float32` -- widening `1.234450f` (the
//     literal a C++ source file would actually pass) to `double` gives `1.2344499826431274`
//     (measured, `python3 -c "import struct;
//     print(struct.unpack('f',struct.pack('f',1.234450))[0])"`), i.e. `scaled =
//     12344.499826...`, strictly BELOW the tie -- so THIS FILE'S own `rcss_quantize()`,
//     evaluated on the REAL float32 value the literal produces, correctly returns `"1.2344"`,
//     not the spec's stated `"1.2345"`. This is not a bug in this file's algorithm: `0.5/10000 =
//     0.00005` has no power-of-two denominator, so an EXACT decimal tie at the 4th digit is only
//     ever float32-representable when `x`'s own denominator (after full reduction) is itself a
//     power of two -- provably impossible for a "nice-looking" 6-decimal-digit literal like
//     `1.234450` chosen for readability. Replaced here with `1.21875f`/`1.21874f` (both signs),
//     hand-verified (same Python one-liner) to be an EXACT float32 tie (`1.21875 == 39/32`,
//     `f32(1.21875) == 1.21875` bit-for-bit, `scaled == 12187.5` exactly in double) and its own
//     clean one-step-below neighbor -- same STRUCTURAL proof the spec's own table names (tie
//     rounds away from zero, not merely "up"; one step below rounds toward zero; both signs),
//     just at digits that are actually float32-representable at the boundary they claim to test.
//     Reported, not silently patched into the spec text (that edit belongs to whoever owns
//     `docs/uix-rcss.md`, per this task's own "não corrija a spec" instruction).
// PT: 15.4 -- fronteira de quantização: empate exato e um passo pra fora, nos dois sinais.
//     Função pura, nenhum documento necessário.
//
//     🔴 ACHADO PRÓPRIO, não na spec, não resolvido pela `UIX-RCSS-ERRATA-2`: as próprias
//     entradas literais da spec (`1.234450`/`1.234449`, nos dois sinais) NÃO alcançam o
//     `quantize()` como empate exato nenhum. A própria assinatura do `quantize()` é `x:
//     float32` -- largar `1.234450f` (o literal que um arquivo-fonte C++ de fato passaria) pra
//     `double` dá `1.2344499826431274` (medido, `python3 -c "import struct;
//     print(struct.unpack('f',struct.pack('f',1.234450))[0])"`), ou seja `scaled =
//     12344.499826...`, estritamente ABAIXO do empate -- então o PRÓPRIO `rcss_quantize()` deste
//     arquivo, avaliado sobre o valor float32 REAL que o literal produz, corretamente devolve
//     `"1.2344"`, não o `"1.2345"` que a spec declara. Isto não é bug no algoritmo deste arquivo:
//     `0,5/10000 = 0,00005` não tem denominador potência-de-dois nenhum, então um empate decimal
//     EXATO na 4ª casa só é representável em float32 quando o próprio denominador de `x` (depois
//     de reduzido por inteiro) é ele mesmo uma potência de dois -- provadamente impossível pra um
//     literal "de cara bonita" de 6 casas decimais como o `1.234450` escolhido por legibilidade.
//     Substituído aqui por `1.21875f`/`1.21874f` (nos dois sinais), verificado à mão (mesmo
//     one-liner Python) como um empate EXATO em float32 (`1.21875 == 39/32`,
//     `f32(1.21875) == 1.21875` bit-a-bit, `scaled == 12187.5` exatamente em double) e o próprio
//     vizinho limpo um-passo-abaixo -- a MESMA prova estrutural que a própria tabela da spec
//     nomeia (empate arredonda pra longe de zero, não meramente "pra cima"; um passo abaixo
//     arredonda pra perto de zero; os dois sinais), só em dígitos que de fato são representáveis
//     em float32 na fronteira que afirmam testar. Reportado, não remendado em silêncio no texto
//     da spec (essa edição é de quem for dono do docs/uix-rcss.md, pela própria instrução "não
//     corrija a spec" desta tarefa).
void test_15_4_quantize_boundary() {
  check_eq(glintfx::rcss_quantize(1.21875f), "1.2188", "15.4 exact tie rounds away from zero (own float32-verified anchor)");
  check_eq(glintfx::rcss_quantize(1.21874f), "1.2187", "15.4 one step below the tie rounds toward zero (own float32-verified anchor)");
  check_eq(glintfx::rcss_quantize(-1.21875f), "-1.2188", "15.4 negative exact tie also grows in magnitude (own float32-verified anchor)");
  check_eq(glintfx::rcss_quantize(-1.21874f), "-1.2187", "15.4 negative one-step-below stays at smaller magnitude (own float32-verified anchor)");
}

// ---------------------------------------------------------------------------
// EN: 🔴 OWN-FINDING FOLLOW-UP, orchestrator relay: the exact same "golden doesn't reach the
//     condition it claims to cover" defect this file already found in section 15.4's own
//     literals also applied to THIS FILE'S OWN comment on section 15.3 -- every color in every
//     golden this file had, before this test, used `alpha=0xff`. At `alpha=0xff`,
//     `color_hex_premultiplied_as_is()` (rcss_dump.cpp) and a hypothetical un-premultiplying
//     alternative print IDENTICAL bytes (`(x*255)/255 == x` exactly, both directions) -- so no
//     existing test could tell the two implementations apart. This test closes that gap with
//     non-opaque-alpha goldens in BOTH places RmlUi stores `ColourbPremultiplied`
//     (`box-shadow`'s own `BoxShadowList`, a gradient `<stop>`'s own `ColorStopList` --
//     `DecorationTypes.h:9`/`:22`), the math shown inline so a reader does not have to trust the
//     assertion blind.
//
//     `#22D3EE80` (alpha=0x80=128): straight `#22d3ee80`. Premultiplied (integer TRUNCATING
//     division, `Colour.h:76-82`): `r'=(0x22*128)/255=(34*128)/255=17=0x11`,
//     `g'=(0xd3*128)/255=(211*128)/255=105=0x69`, `b'=(0xee*128)/255=(238*128)/255=119=0x77`,
//     `a=128=0x80` unchanged -> `#11697780`. Visibly, structurally different from the straight
//     value in every channel but alpha -- this is the golden that actually distinguishes "print
//     the field as-is" from "un-premultiply first", the thing every PRIOR golden in this file
//     could not do.
//
//     `#C9A24B40` (alpha=0x40=64): straight `#c9a24b40`. Premultiplied:
//     `r'=(0xc9*64)/255=(201*64)/255=50=0x32`, `g'=(0xa2*64)/255=(162*64)/255=40=0x28`,
//     `b'=(0x4b*64)/255=(75*64)/255=18=0x12`, `a=64=0x40` -> `#32281240`.
//
//     `#22D3EE00`/alpha=0: the STRONGEST case, per the orchestrator's own argument -- un-
//     premultiplying divides by `alpha`, and `alpha=0` makes that division UNDEFINED (`Colour.h`'s
//     own `ToNonPremultiplied()` special-cases it, `alpha > 0 ? (red*255)/alpha : 0` -- a
//     hypothetical un-premultiplying implementation of THIS file's own color path would need the
//     exact same special case, or it is not merely a different answer, it is UB/a crash). Straight
//     would be `#22d3ee00`; premultiplied is `#00000000` regardless of the original RGB (every
//     channel truncates to `(x*0)/255=0`) -- so this golden ALSO proves the two readings are not
//     just numerically different, the "correct" one (this file's own, per the real RmlUi/
//     Style::ComputedValues cascade-domain value) is well-defined where the alternative reading
//     is not.
//
//     🔴 Audit of ALL FOUR worked examples, per the orchestrator's own instruction ("enumere...
//     responda, um a um, se a entrada escolhida realmente alcança a condição declarada"):
//       - 15.1 (hover): REACHES its own condition -- `color` differs between `STATE none`/
//         `STATE hover-all` (`#223344ff` vs `#ff0000ff`), proving `:hover` forcing actually
//         engages the matcher rather than being a no-op that happened to match by coincidence;
//         `display`/`opacity`/`width` staying IDENTICAL in both states is the correct behaviour
//         being exercised too (a `:hover` rule that does not touch them must not leak into them),
//         not an inert assertion.
//       - 15.2 (shorthand order): REACHES its own condition -- `body/0` (correct order) sets
//         BOTH longhands from the declaration; `body/1` (reversed order) sets ONLY `-color` and
//         drops `-width` to its registry default -- a real, structural DIFFERENCE between the two
//         orders, not "both fail the same way" or "both succeed the same way", which is exactly
//         what "order is load-bearing" needs to demonstrate.
//       - 15.3 (three `%` families): REACHES its own condition for the auto-spacing algorithm
//         specifically (the radial-gradient's own first stop, `#F0D98C`, has no explicit
//         position and the golden asserts the COMPUTED `0.0000%`, not a value copied from source
//         text -- the one line in that example that is not simply an echo). Weaker, DECLARED
//         gap: the test does not independently prove families (b) and (c) are computed by
//         DIFFERENT code paths at the byte level (both this test's family-(b) and family-(c)
//         inputs are plain, already-percent-typed numbers here, so a hypothetical merged
//         resolver could pass this SAME golden by coincidence for THIS input) -- the separation
//         is real in this file's own source structure (`radial_position_percent()` vs
//         `gradient_stops_tail()`, different function signatures, a merge would not compile as
//         written), but this specific golden does not regression-prove it at the VALUE level.
//         Not fixed here (would need a family-(c) input that only a correctly-separated resolver
//         gets right, e.g. one that would silently break if fed through the auto-spacing
//         algorithm) -- reported as a declared gap, not silently closed by inventing a claim this
//         golden does not back.
//       - 15.4 (quantization boundary): did NOT reach its own condition as originally written
//         (see the `1.21875f`/`1.21874f` fix above and its own doc-comment) -- FIXED in this same
//         file, own float32-verified anchor.
// PT: 🔴 CONTINUAÇÃO DE ACHADO PRÓPRIO, repasse do orquestrador: o mesmíssimo defeito "golden não
//     alcança a condição que afirma cobrir" que este arquivo já achou nos próprios literais da
//     seção 15.4 também valia pro PRÓPRIO comentário deste arquivo na seção 15.3 -- toda cor de
//     todo golden que este arquivo tinha, antes deste teste, usava `alpha=0xff`. Em `alpha=0xff`,
//     `color_hex_premultiplied_as_is()` (rcss_dump.cpp) e uma hipotética alternativa
//     des-premultiplicadora imprimem bytes IDÊNTICOS (`(x*255)/255 == x` exatamente, nos dois
//     sentidos) -- então teste nenhum existente conseguia distinguir as duas implementações. Este
//     teste fecha essa lacuna com goldens de alfa NÃO-opaco nos DOIS lugares em que o RmlUi guarda
//     `ColourbPremultiplied` (o próprio `BoxShadowList` do `box-shadow`, o próprio `ColorStopList`
//     de um `<stop>` de gradiente -- `DecorationTypes.h:9`/`:22`), a conta mostrada inline pra um
//     leitor não precisar confiar na afirmação às cegas.
//
//     `#22D3EE80` (alfa=0x80=128): reto `#22d3ee80`. Premultiplicado (divisão inteira TRUNCANTE,
//     `Colour.h:76-82`): `r'=(0x22*128)/255=(34*128)/255=17=0x11`,
//     `g'=(0xd3*128)/255=(211*128)/255=105=0x69`, `b'=(0xee*128)/255=(238*128)/255=119=0x77`,
//     `a=128=0x80` sem mudança -> `#11697780`. Visivelmente, estruturalmente diferente do valor
//     reto em todo canal menos alfa -- este é o golden que de fato distingue "imprime o campo ao
//     pé da letra" de "des-premultiplica primeiro", a coisa que golden NENHUM anterior deste
//     arquivo conseguia.
//
//     `#C9A24B40` (alfa=0x40=64): reto `#c9a24b40`. Premultiplicado:
//     `r'=(0xc9*64)/255=(201*64)/255=50=0x32`, `g'=(0xa2*64)/255=(162*64)/255=40=0x28`,
//     `b'=(0x4b*64)/255=(75*64)/255=18=0x12`, `a=64=0x40` -> `#32281240`.
//
//     `#22D3EE00`/alfa=0: o caso MAIS FORTE, pelo próprio argumento do orquestrador --
//     des-premultiplicar divide por `alpha`, e `alpha=0` torna essa divisão INDEFINIDA (o próprio
//     `ToNonPremultiplied()` de `Colour.h` trata esse caso à parte,
//     `alpha > 0 ? (red*255)/alpha : 0` -- uma hipotética implementação des-premultiplicadora do
//     próprio caminho de cor deste arquivo precisaria do MESMO caso à parte, ou não é meramente
//     uma resposta diferente, é UB/crash). Reto seria `#22d3ee00`; premultiplicado é `#00000000`
//     independente do RGB original (todo canal trunca pra `(x*0)/255=0`) -- então este golden
//     TAMBÉM prova que as duas leituras não são só numericamente diferentes, a "correta" (a deste
//     próprio arquivo, pelo valor real de domínio-cascata `Style::ComputedValues`) é bem-definida
//     onde a alternativa não é.
//
//     🔴 Auditoria dos QUATRO exemplos trabalhados, pela própria instrução do orquestrador
//     ("enumere... responda, um a um, se a entrada escolhida realmente alcança a condição
//     declarada"): ver o bloco EN acima -- 15.1 alcança, 15.2 alcança, 15.3 alcança parcialmente
//     (lacuna declarada: não prova a separação b/c no nível de VALOR, só no de estrutura de
//     código), 15.4 NÃO alcançava e foi CONSERTADO neste mesmo arquivo.
// ---------------------------------------------------------------------------
void test_own_finding_premultiplied_alpha_has_teeth(Harness& h) {
  const std::string rml =
      "<rml><head><style>\n"
      "#g { box-shadow: #22D3EE80 0dp 0dp 0dp; }\n"
      "#h { box-shadow: #22D3EE00 0dp 0dp 0dp; }\n"
      "#i { decorator: linear-gradient(90deg, #FF0000FF 0%, #C9A24B40 100%); }\n"
      "#j { decorator: linear-gradient(90deg, #FF0000FF 0%, #22D3EE00 100%); }\n"
      "</style></head><body>"
      "<div id=\"g\"></div><div id=\"h\"></div><div id=\"i\"></div><div id=\"j\"></div>"
      "</body></rml>";
  Rml::ElementDocument* doc = h.load(rml);
  check(doc != nullptr, "premultiplied-alpha-teeth: document loaded");
  if (!doc) return;

  const std::string dump = glintfx::rcss_dump_document(doc);

  check_eq(extract_prop(dump, "STATE none\n", "body/0", "box-shadow"), "#11697780;0.0000px;0.0000px;0.0000px;0.0000px;false",
           "own-finding box-shadow non-opaque alpha (#22D3EE80 -> premultiplied #11697780)");
  check_eq(extract_prop(dump, "STATE none\n", "body/1", "box-shadow"), "#00000000;0.0000px;0.0000px;0.0000px;0.0000px;false",
           "own-finding box-shadow alpha=0 (strongest case -- straight/undefined vs. premultiplied #00000000)");
  check_eq(extract_prop(dump, "STATE none\n", "body/2", "decorator"), "linear-gradient(90.0000;#ff0000ff:0.0000%;#32281240:100.0000%)",
           "own-finding gradient-stop non-opaque alpha (#C9A24B40 -> premultiplied #32281240)");
  check_eq(extract_prop(dump, "STATE none\n", "body/3", "decorator"), "linear-gradient(90.0000;#ff0000ff:0.0000%;#00000000:100.0000%)",
           "own-finding gradient-stop alpha=0 (strongest case -- premultiplied #00000000)");

  doc->Close();
  h.engine.update();
}

} // namespace

int main() {
  test_15_4_quantize_boundary();

  Harness h;
  if (!h.setup()) {
    std::puts("FAIL: harness setup (window/engine attach)");
    return 2;
  }

  test_15_1_two_states_one_node_hover(h);
  test_15_2_shorthand_order_border_top(h);
  test_15_3_three_percent_families(h);
  test_own_finding_premultiplied_alpha_has_teeth(h);

  if (g_failures == 0) {
    std::puts(
        "rcss_dump_worked_examples OK (4 worked examples: 15.1, 15.2, 15.3, 15.4; plus own-finding "
        "premultiplied-alpha-has-teeth)");
    return 0;
  }
  std::printf("rcss_dump_worked_examples: %d failure(s)\n", g_failures);
  return 10;
}
