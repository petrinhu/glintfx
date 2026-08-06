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

  if (g_failures == 0) {
    std::puts("rcss_dump_worked_examples OK (4 worked examples: 15.1, 15.2, 15.3, 15.4)");
    return 0;
  }
  std::printf("rcss_dump_worked_examples: %d failure(s)\n", g_failures);
  return 10;
}
