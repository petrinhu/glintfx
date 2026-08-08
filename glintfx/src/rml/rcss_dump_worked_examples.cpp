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

#include <cmath>
#include <cstdio>
#include <limits>
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
// EN: `UIX-QUANTIZE-MAGNITUDE`'s own auditoria-dominó sibling fix -- `polygon()` sides magnitude
//     guard before `std::lround()` (`rcss_dump.cpp`, `polygon_sides` local, "auditoria-dominó
//     sibling" comment).
//
//     🔴 OWN FINDING, MEASURED, not merely a regression test: `decorator: polygon(1e20, red)` is
//     REGRESSION coverage, not DISCRIMINATING coverage, and this paragraph exists so a future
//     reader does not mistake the difference. `sides: 1e20` is finite, so it survives
//     `PropertyParserNumber`'s own parse and reaches this function with `sides`/`fill`/`rotation`
//     all non-null (measured with a temporary debug print, restored byte-identical against the
//     committed blob afterward, `git diff` confirmed clean). With this guard TEMPORARILY disabled
//     (a `sed` toggle on a throwaway copy, never the shared tree -- see this item's own delivery
//     report, `/var/tmp/mut-quantize/relatorio.md`), `std::lround(1e20f)` on THIS platform
//     (x86-64/glibc/clang, `cvttsd2si`-based) measured **exactly `LONG_MIN`
//     (`-9223372036854775808`)** -- not garbage-that-happens-to-be-anything, a DETERMINISTIC
//     hardware sentinel for "out of range" -- which the PRE-EXISTING `sides_int < 3 || > 1024`
//     check one line below ALSO rejects, producing the IDENTICAL observable outcome
//     (`decorator=none`) with or without this guard. **On this specific hardware, no finite
//     `float` can make this test discriminate the new guard from the old range check's own
//     accidental self-healing** (`cvttsd2si`'s own out-of-range/NaN sentinel is always a huge
//     negative number regardless of the SIGN of the overflow, so `-1e20f` measures the same way).
//     This is the "prove the mutation reached the code" lesson turned around: I do NOT get to
//     claim this assertion proves the guard's own branch executed, because measurement proved the
//     opposite -- removing the guard changes NOTHING observable here. The guard's own
//     justification is NOT this test: it is the C++ standard's own classification of an
//     out-of-representable-range `std::lround()` result as unspecified behavior, a classification
//     that does not promise `LONG_MIN` on a different compiler/optimization level/architecture,
//     and is exactly the "do not rely on today's accidental hardware behavior" discipline this
//     whole item exists to enforce elsewhere. This test is kept as a real, valuable regression
//     (proves `polygon(1e20, red)` never crashes and always degrades to `decorator=none`, both
//     WITH and WITHOUT this guard -- a fact worth pinning either way) and the doc comment says so
//     honestly rather than silently inflating it into proof it is not.
// PT: O próprio sibling de auditoria-dominó da `UIX-QUANTIZE-MAGNITUDE` -- a guarda de magnitude
//     do `polygon()` sides antes do `std::lround()` (`rcss_dump.cpp`, `polygon_sides` local,
//     comentário "auditoria-dominó sibling").
//
//     🔴 ACHADO PRÓPRIO, MEDIDO, não meramente um teste de regressão: `decorator: polygon(1e20,
//     red)` é cobertura de REGRESSÃO, não cobertura DISCRIMINANTE, e este parágrafo existe pra um
//     futuro leitor não confundir as duas. `sides: 1e20` é finito, então sobrevive ao parse do
//     `PropertyParserNumber` e chega nesta função com `sides`/`fill`/`rotation` todos não-nulos
//     (medido com um print de debug temporário, restaurado byte-idêntico contra o blob commitado
//     depois, `git diff` confirmou limpo). Com esta guarda TEMPORARIAMENTE desligada (um toggle de
//     `sed` numa cópia descartável, nunca a árvore compartilhada -- ver o próprio relatório de
//     entrega deste item, `/var/tmp/mut-quantize/relatorio.md`), `std::lround(1e20f)` NESTA
//     plataforma (x86-64/glibc/clang, baseado em `cvttsd2si`) mediu **exatamente `LONG_MIN`
//     (`-9223372036854775808`)** -- não lixo-que-calha-de-ser-qualquer-coisa, um sentinel
//     DETERMINÍSTICO de hardware pra "fora de faixa" -- que a checagem PRÉ-EXISTENTE
//     `sides_int < 3 || > 1024` uma linha abaixo TAMBÉM rejeita, produzindo o MESMO resultado
//     observável (`decorator=none`) com ou sem esta guarda. **Nesta plataforma específica, nenhum
//     `float` finito consegue fazer este teste discriminar a guarda nova do próprio
//     auto-conserto acidental da checagem de faixa antiga** (o próprio sentinel de fora-de-faixa/
//     NaN do `cvttsd2si` é sempre um número negativo enorme independente do SINAL do overflow,
//     então `-1e20f` mede igual). Esta é a lição "prove que a mutação chegou no código" ao
//     contrário: eu NÃO tenho como afirmar que esta asserção prova que o ramo da guarda executou,
//     porque a medição provou o oposto -- remover a guarda não muda NADA observável aqui. A
//     própria justificativa da guarda NÃO é este teste: é a própria classificação do padrão C++
//     pra um resultado de `std::lround()` fora da faixa representável como comportamento
//     não-especificado, uma classificação que não promete `LONG_MIN` num compilador/nível de
//     otimização/arquitetura diferente, e é exatamente a disciplina "não confiar no comportamento
//     acidental de hoje do hardware" que este item inteiro existe pra impor em outro lugar. Este
//     teste fica como regressão real e valiosa (prova que `polygon(1e20, red)` nunca crasha e
//     sempre degrada pra `decorator=none`, TANTO com quanto sem esta guarda -- fato que vale a
//     pena pinar de qualquer jeito) e o comentário diz isso com honestidade em vez de inflar em
//     silêncio pra prova que não é.
void test_quantize_magnitude_polygon_sides_sibling(Harness& h) {
  const std::string rml =
      "<rml><head><style>\n"
      "#p { decorator: polygon(1e20, red); }\n"
      "</style></head><body><div id=\"p\"></div></body></rml>";
  Rml::ElementDocument* doc = h.load(rml);
  check(doc != nullptr, "polygon sides sibling: document loaded");
  if (!doc) return;

  const std::string dump = glintfx::rcss_dump_document(doc);
  check_eq(extract_prop(dump, "STATE none\n", "body/0", "decorator"), "none",
           "polygon(1e20, red): no crash, degrades to decorator=none -- REGRESSION coverage only, "
           "NOT discriminating for this guard on this platform (see header comment above: the "
           "pre-existing [3,1024] check self-heals std::lround()'s own measured LONG_MIN fallout "
           "identically with or without this guard)");

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
// EN: `UIX-QUANTIZE-MAGNITUDE` -- mirrors side B's own `test_quantize_magnitude_ceiling()`
//     (`glintfx/tests/uix_style/value_compute_sanity.cpp`) byte-for-byte, same rationale, same
//     literal ceiling value (`140737488355328.0 = 2^47`, deliberately duplicated per this file's
//     own `kQuantizeMagnitudeCeiling` comment -- ADR-0020, the two dumper sides do not share
//     code). Pure function, no document/Harness needed, same footing as `test_15_4_*` above. See
//     that side-B test's own header comment for the full rationale each numbered case below
//     mirrors.
// PT: `UIX-QUANTIZE-MAGNITUDE` -- espelha byte-a-byte o próprio `test_quantize_magnitude_ceiling()`
//     do lado B (`glintfx/tests/uix_style/value_compute_sanity.cpp`), mesmo raciocínio, mesmo
//     valor literal de teto (`140737488355328.0 = 2^47`, deliberadamente duplicado per o próprio
//     comentário do `kQuantizeMagnitudeCeiling` deste arquivo -- ADR-0020, os dois lados do dumper
//     não compartilham código). Função pura, nenhum documento/Harness necessário, o mesmo patamar
//     do `test_15_4_*` acima. Ver o próprio comentário de cabeçalho daquele teste do lado B pro
//     raciocínio completo que cada caso numerado abaixo espelha.
void test_quantize_magnitude_ceiling() {
  const double kMaxD = 140737488355328.0; // 2^47, MUST match kQuantizeMagnitudeCeiling above
  const float kMaxF = static_cast<float>(kMaxD);
  check(static_cast<double>(kMaxF) == kMaxD,
        "kMaxQuantizeMagnitude (2^47) is exactly representable in float32 -- test setup sanity");
  const std::string kSaturatedPos = "140737488355328.0000";
  const std::string kSaturatedNeg = "-140737488355328.0000";

  // (1) far below the ceiling -- ordinary corpus-scale value, untouched.
  check_eq(glintfx::rcss_quantize(999999.0f), "999999.0000", "far below ceiling: untouched");
  check_eq(glintfx::rcss_quantize(-999999.0f), "-999999.0000", "far below ceiling, negative: untouched");

  // (2) exactly at the ceiling -- the LAST unclamped value.
  check_eq(glintfx::rcss_quantize(kMaxF), kSaturatedPos, "exactly at the ceiling: NOT clamped");
  check_eq(glintfx::rcss_quantize(-kMaxF), kSaturatedNeg, "exactly at the negative ceiling: NOT clamped");

  // (3) the very next float32 above/below the ceiling -- the FIRST clamped value.
  const float kJustAbove = std::nextafterf(kMaxF, std::numeric_limits<float>::max());
  const float kJustBeyondNeg = std::nextafterf(-kMaxF, std::numeric_limits<float>::lowest());
  check_eq(glintfx::rcss_quantize(kJustAbove), kSaturatedPos,
           "one float32 step past the ceiling: clamped, saturates to the SAME string as (2)");
  check_eq(glintfx::rcss_quantize(kJustBeyondNeg), kSaturatedNeg,
           "one float32 step past the negative ceiling: clamped, saturates to the SAME string as (2)");

  // (4) the OLD divergent zone -- side A's own long long UB threshold (~9.2234e14) used to be UB
  //     HERE; side B's own unsigned long long UB threshold (~1.8447e15) was not. Both now agree.
  check_eq(glintfx::rcss_quantize(1.0e15f), kSaturatedPos,
           "old divergent zone (9.2234e14 < 1e15 < 1.8447e15): saturated, side A no longer UB here");
  check_eq(glintfx::rcss_quantize(-1.0e15f), kSaturatedNeg, "old divergent zone, negative mirror");
  check_eq(glintfx::rcss_quantize(9.223372e14f), kSaturatedPos,
           "at this file's own OLD long long UB threshold (~LLONG_MAX/10000): saturated, no "
           "longer UB");
  check_eq(glintfx::rcss_quantize(1.844674e15f), kSaturatedPos,
           "at side B's own OLD unsigned long long UB threshold (~ULLONG_MAX/10000): also "
           "saturated");

  // (5) FLT_MAX -- the true float32 ceiling. No crash, no UB, same defined string.
  check_eq(glintfx::rcss_quantize(std::numeric_limits<float>::max()), kSaturatedPos,
           "FLT_MAX: 23 orders of magnitude past the old thresholds, still saturates cleanly");
  check_eq(glintfx::rcss_quantize(std::numeric_limits<float>::lowest()), kSaturatedNeg,
           "-FLT_MAX: negative mirror");
}

// ---------------------------------------------------------------------------
// EN: 🔴 OWN-FINDING FOLLOW-UP, second pass -- the líder REVERSED the premultiplied-as-is
//     decision (rcss_dump.cpp's own header comment, "SPEC-VS-UPSTREAM DIVERGENCE" note, has the
//     full argument: real upstream un-premultiplies at the exact point it serializes these two
//     fields to text, `TypeConverter.cpp:223`/`:256`). This file's own gap-finding still holds
//     unchanged -- every color in every golden this file had, before this test, used
//     `alpha=0xff`, where straight and premultiplied print IDENTICAL bytes
//     (`(x*255)/255 == x` exactly, both directions) -- only the CORRECT expected value for the
//     non-opaque cases flips. The math below is written against the NOW-correct
//     `color_hex_straight_from_premultiplied()` (un-premultiplies, `rcss_dump.cpp`).
//
//     🔴 The printed value is NOT the author's own written value -- measured, not asserted. The
//     stored `ColourbPremultiplied` already lost information at PARSE time (truncating integer
//     division, `Colour.h:76-82`); un-premultiplying (`ToNonPremultiplied()`, also truncating)
//     does not recover it:
//
//     `#22D3EE80` (alpha=0x80=128) is stored premultiplied as `#11697780`
//     (`r'=(0x22*128)/255=17`, `g'=(0xd3*128)/255=105`, `b'=(0xee*128)/255=119`); un-premultiplying
//     THAT gives `r=(0x11*255)/128=(17*255)/128=33=0x21`, `g=(0x69*255)/128=(105*255)/128=209=0xd1`,
//     `b=(0x77*255)/128=(119*255)/128=237=0xed`, `a=128=0x80` -> **`#21d1ed80`**, NOT the author's
//     own `#22d3ee80`. This is the golden that distinguishes "un-premultiply" from "print as
//     stored" -- the thing every PRIOR golden in this file could not do.
//
//     `#C9A24B40` (alpha=0x40=64) is stored as `#32281240`; un-premultiplying gives
//     `r=(0x32*255)/64=(50*255)/64=199=0xc7`, `g=(0x28*255)/64=(40*255)/64=159=0x9f`,
//     `b=(0x12*255)/64=(18*255)/64=71=0x47`, `a=64=0x40` -> **`#c79f4740`**.
//
//     `#22D3EE00`/alpha=0: stored premultiplied as `#00000000` (every channel truncates to
//     `(x*0)/255=0` regardless of the original RGB). Un-premultiplying it: `ToNonPremultiplied()`'s
//     own guard is `alpha > 0 ? (red*255)/alpha : 0` -- for `alpha=0` this returns `0` for every
//     channel too, so the two readings COINCIDE at `#00000000` for this one input. This is the
//     ONE case this test cannot use to tell the two implementations apart by byte output; kept
//     anyway because it is still the strongest CONCEPTUAL anchor (a division-by-zero guard exists
//     and is exercised, proving the real implementation handles the edge case explicitly rather
//     than by accident of the byte math above happening to also land on zero).
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
// PT: 🔴 CONTINUAÇÃO DE ACHADO PRÓPRIO, segunda passada -- o líder REVERTEU a decisão
//     premultiplicado-como-está (o próprio comentário de cabeçalho do rcss_dump.cpp, nota
//     "DIVERGÊNCIA SPEC-VS-UPSTREAM", tem o argumento completo: o próprio upstream real
//     des-premultiplica no exato ponto em que serializa estes dois campos pra texto,
//     `TypeConverter.cpp:223`/`:256`). O achado-de-lacuna deste arquivo continua valendo sem
//     mudança -- toda cor de todo golden que este arquivo tinha, antes deste teste, usava
//     `alpha=0xff`, onde reto e premultiplicado imprimem bytes IDÊNTICOS (`(x*255)/255 == x`
//     exatamente, nos dois sentidos) -- só o valor esperado CORRETO pros casos não-opacos vira.
//     A conta abaixo é escrita contra o `color_hex_straight_from_premultiplied()`
//     (des-premultiplica, `rcss_dump.cpp`) AGORA correto.
//
//     🔴 O valor impresso NÃO é o valor escrito pelo autor -- medido, não afirmado. O
//     `ColourbPremultiplied` guardado já perdeu informação em tempo de PARSE (divisão inteira
//     truncante, `Colour.h:76-82`); des-premultiplicar (`ToNonPremultiplied()`, também truncante)
//     não a recupera:
//
//     `#22D3EE80` (alfa=0x80=128) é guardado premultiplicado como `#11697780`
//     (`r'=(0x22*128)/255=17`, `g'=(0xd3*128)/255=105`, `b'=(0xee*128)/255=119`);
//     des-premultiplicar ISSO dá `r=(0x11*255)/128=(17*255)/128=33=0x21`,
//     `g=(0x69*255)/128=(105*255)/128=209=0xd1`, `b=(0x77*255)/128=(119*255)/128=237=0xed`,
//     `a=128=0x80` -> **`#21d1ed80`**, NÃO o `#22d3ee80` do próprio autor. Este é o golden que
//     distingue "des-premultiplica" de "imprime como guardado" -- a coisa que golden NENHUM
//     anterior deste arquivo conseguia.
//
//     `#C9A24B40` (alfa=0x40=64) é guardado como `#32281240`; des-premultiplicar dá
//     `r=(0x32*255)/64=(50*255)/64=199=0xc7`, `g=(0x28*255)/64=(40*255)/64=159=0x9f`,
//     `b=(0x12*255)/64=(18*255)/64=71=0x47`, `a=64=0x40` -> **`#c79f4740`**.
//
//     `#22D3EE00`/alfa=0: guardado premultiplicado como `#00000000` (todo canal trunca pra
//     `(x*0)/255=0` independente do RGB original). Des-premultiplicando: a própria guarda do
//     `ToNonPremultiplied()` é `alpha > 0 ? (red*255)/alpha : 0` -- pra `alpha=0` isso devolve `0`
//     em todo canal também, então as duas leituras COINCIDEM em `#00000000` pra esta entrada. Este
//     é o ÚNICO caso que este teste não consegue usar pra distinguir as duas implementações só
//     pelo byte de saída; mantido mesmo assim porque continua sendo a âncora CONCEITUAL mais
//     forte (uma guarda de divisão-por-zero existe e é exercitada, provando que a implementação
//     real trata o caso de borda explicitamente, não por acaso da conta de byte acima também
//     cair em zero).
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

  check_eq(extract_prop(dump, "STATE none\n", "body/0", "box-shadow"), "#21d1ed80;0.0000px;0.0000px;0.0000px;0.0000px;false",
           "own-finding box-shadow non-opaque alpha (#22D3EE80 stored #11697780 -> un-premultiplied #21d1ed80)");
  check_eq(extract_prop(dump, "STATE none\n", "body/1", "box-shadow"), "#00000000;0.0000px;0.0000px;0.0000px;0.0000px;false",
           "own-finding box-shadow alpha=0 (does not distinguish the two readings by byte -- see comment above)");
  check_eq(extract_prop(dump, "STATE none\n", "body/2", "decorator"), "linear-gradient(90.0000;#ff0000ff:0.0000%;#c79f4740:100.0000%)",
           "own-finding gradient-stop non-opaque alpha (#C9A24B40 stored #32281240 -> un-premultiplied #c79f4740)");
  check_eq(extract_prop(dump, "STATE none\n", "body/3", "decorator"), "linear-gradient(90.0000;#ff0000ff:0.0000%;#00000000:100.0000%)",
           "own-finding gradient-stop alpha=0 (does not distinguish the two readings by byte -- see comment above)");

  doc->Close();
  h.engine.update();
}

// ---------------------------------------------------------------------------
// EN: `UIX-RCSS-CONFORMIDADE` -- section 9.1's own worked example, LITERAL, byte-exact, reproduced
//     for the first time in THIS file (the two-layer `box-shadow` declaration the document itself
//     quotes: `box-shadow: #22D3EE 0dp 0dp 0dp 1dp inset, #22D3EE26 0dp 0dp 16dp 0dp;`). Everything
//     this file had before this test used DIFFERENT literals
//     (`test_own_finding_premultiplied_alpha_has_teeth` above uses `#22D3EE80`/`#C9A24B40`/
//     `#22D3EE00`) -- close, but never the document's own §9.1 literal itself, meaning the exact
//     text a second implementer would copy-paste out of `docs/uix-rcss.md` had never been run
//     through THIS side's real dumper until now. Side B (`glintfx/tests/uix_style/
//     value_compute_sanity.cpp`, `test_worked_example_9_1_box_shadow`) already had this exact
//     literal; this test closes the asymmetry (`UIX-RCSS-DUMP-A-DESPREMULT`'s own residue note,
//     `TODO.md`). Expected value taken VERBATIM from §9.1's own worked-example block, not derived
//     by reading `rcss_dump.cpp` (this task's own restriction: the expected answer comes from the
//     document, never from the implementation under test).
// PT: `UIX-RCSS-CONFORMIDADE` -- o próprio exemplo trabalhado da seção 9.1, LITERAL, byte-exato,
//     reproduzido pela primeira vez NESTE arquivo (a declaração `box-shadow` de duas camadas que o
//     próprio documento cita: `box-shadow: #22D3EE 0dp 0dp 0dp 1dp inset, #22D3EE26 0dp 0dp 16dp
//     0dp;`). Tudo que este arquivo tinha antes deste teste usava literais DIFERENTES
//     (`test_own_finding_premultiplied_alpha_has_teeth` acima usa `#22D3EE80`/`#C9A24B40`/
//     `#22D3EE00`) -- perto, mas nunca o próprio literal da §9.1, o que significa que o texto exato
//     que um segundo implementador copiaria e colaria de `docs/uix-rcss.md` nunca tinha passado
//     pelo dumper real DESTE lado até agora. O lado B (`glintfx/tests/uix_style/
//     value_compute_sanity.cpp`, `test_worked_example_9_1_box_shadow`) já tinha esse literal exato;
//     este teste fecha a assimetria (nota de resíduo da `UIX-RCSS-DUMP-A-DESPREMULT`, `TODO.md`).
//     Valor esperado tirado VERBATIM do próprio bloco de exemplo trabalhado da §9.1, não derivado
//     lendo o `rcss_dump.cpp` (a própria restrição desta tarefa: a resposta esperada vem do
//     documento, nunca da implementação sob teste).
void test_9_1_box_shadow_literal_worked_example(Harness& h) {
  const std::string rml =
      "<rml><head><style>\n"
      "#k { box-shadow: #22D3EE 0dp 0dp 0dp 1dp inset, #22D3EE26 0dp 0dp 16dp 0dp; }\n"
      "</style></head><body><div id=\"k\"></div></body></rml>";
  Rml::ElementDocument* doc = h.load(rml);
  check(doc != nullptr, "9.1 literal: document loaded");
  if (!doc) return;

  const std::string dump = glintfx::rcss_dump_document(doc);
  check_eq(extract_prop(dump, "STATE none\n", "body/0", "box-shadow"),
           "#22d3eeff;0.0000px;0.0000px;0.0000px;1.0000px;true|"
           "#21d0ea26;0.0000px;0.0000px;16.0000px;0.0000px;false",
           "9.1 literal box-shadow= line, byte-exact against docs/uix-rcss.md's own §9.1 worked "
           "example text");

  doc->Close();
  h.engine.update();
}

// ---------------------------------------------------------------------------
// EN: `UIX-RCSS-CONFORMIDADE` -- section 9.2.1's own auto-spacing algorithm has FOUR numbered
//     rules. The one worked example the document gives (reused verbatim by `test_15_3` above)
//     only reaches rule 1 (explicit position kept, `55%`/`100%`) and rule 2 (first stop, no
//     explicit position, assigned `0%`) -- it never has a stop that is BOTH unpositioned AND last
//     (rule 3), nor a run of two-or-more consecutive unpositioned stops between two positioned
//     neighbors (rule 4, the only rule with a real formula, and per this task's own briefing "the
//     place an off-by-one is most likely"). Side B (`value_compute_sanity.cpp`,
//     `test_gradient_stop_auto_spacing_general_run`) already enumerates rule 4 directly against the
//     pure `resolve_gradient_stop_positions()` function; this side has no equivalent low-level
//     function (Side A's dumper walks the real RmlUi decorator instance, never a standalone
//     stop-spacing routine), so both rules are pinned here through the real pipeline instead, two
//     small `linear-gradient(...)` fixtures built BY HAND from the algorithm's own four numbered
//     steps (§9.2.1), not by reading `rcss_dump.cpp`:
//       rule 3: `linear-gradient(90deg, #FF0000 10%, #00FF00)` -- 2 stops, first explicit `10%`,
//       LAST has no explicit position -> rule 3 assigns it `100%`.
//       rule 4: `linear-gradient(90deg, #FF0000 0%, #0000FF, #00FF00, #FFFF00 100%)` -- 4 stops,
//       first/last explicit (`0%`/`100%`), a K=2 run of unpositioned stops in between -> rule 4's
//       own formula, `P_before + i*(P_after-P_before)/(K+1)` for i=1..K, gives
//       `0 + 1*(100-0)/3 = 33.3333%` and `0 + 2*(100-0)/3 = 66.6667%`. All four colors carry an
//       implicit `alpha=ff`, so the premultiply/un-premultiply round trip (§7.1/`ERRATA-4`) is a
//       no-op here (`channel*255/255=channel` exactly) -- this test is about stop POSITIONS, not
//       about re-testing color lossiness already covered above.
// PT: `UIX-RCSS-CONFORMIDADE` -- o próprio algoritmo de auto-espaçamento da seção 9.2.1 tem QUATRO
//     regras numeradas. O único exemplo trabalhado que o documento dá (reusado verbatim pelo
//     `test_15_3` acima) só alcança a regra 1 (posição explícita mantida, `55%`/`100%`) e a regra 2
//     (primeiro stop, sem posição explícita, recebe `0%`) -- nunca tem um stop que seja AO MESMO
//     TEMPO sem posição E último (regra 3), nem um trecho de dois-ou-mais stops sem posição
//     consecutivos entre dois vizinhos posicionados (regra 4, a única regra com fórmula de fato, e
//     per o próprio briefing desta tarefa "o lugar mais provável de um off-by-one"). O lado B
//     (`value_compute_sanity.cpp`, `test_gradient_stop_auto_spacing_general_run`) já enumera a
//     regra 4 direto contra a própria função pura `resolve_gradient_stop_positions()`; este lado
//     não tem função de baixo nível equivalente (o dumper deste lado percorre a instância real de
//     decorator do RmlUi, nunca uma rotina standalone de espaçamento de stop), então as duas regras
//     são fixadas aqui pelo pipeline real, em vez disso, com duas fixtures pequenas de
//     `linear-gradient(...)` montadas À MÃO a partir dos próprios quatro passos numerados do
//     algoritmo (§9.2.1), não lendo o `rcss_dump.cpp`.
void test_9_2_1_auto_spacing_rules_3_and_4(Harness& h) {
  const std::string rml =
      "<rml><head><style>\n"
      "#m { decorator: linear-gradient(90deg, #FF0000 10%, #00FF00); }\n"
      "#n { decorator: linear-gradient(90deg, #FF0000 0%, #0000FF, #00FF00, #FFFF00 100%); }\n"
      "</style></head><body><div id=\"m\"></div><div id=\"n\"></div></body></rml>";
  Rml::ElementDocument* doc = h.load(rml);
  check(doc != nullptr, "9.2.1 rules 3/4: document loaded");
  if (!doc) return;

  const std::string dump = glintfx::rcss_dump_document(doc);

  check_eq(extract_prop(dump, "STATE none\n", "body/0", "decorator"),
           "linear-gradient(90.0000;#ff0000ff:10.0000%;#00ff00ff:100.0000%)",
           "9.2.1 rule 3: last stop, no explicit position, assigned 100.0000%");
  check_eq(extract_prop(dump, "STATE none\n", "body/1", "decorator"),
           "linear-gradient(90.0000;#ff0000ff:0.0000%;#0000ffff:33.3333%;#00ff00ff:66.6667%;"
           "#ffff00ff:100.0000%)",
           "9.2.1 rule 4: K=2 unpositioned run interpolated evenly between 0% and 100% neighbors "
           "(0+1*100/3=33.3333%, 0+2*100/3=66.6667%)");

  doc->Close();
  h.engine.update();
}

// ---------------------------------------------------------------------------
// EN: `ESC-6` -- the 8 functional colour notations (`rgb()`/`rgba()`/`hsl()`/`hsla()`/`lab()`/
//     `lch()`/`oklab()`/`oklch()`), side A only (`glintfx/src/rml/rcss_dump.cpp`'s own independent
//     `parse_color_token` mini-parser, the ONLY place this side parses a color from raw text --
//     used exclusively by `polygon()`'s own `<fill>` sub-property, `decorator_polygon.cpp:786`'s
//     own passthrough "string" parser deferring real color parsing to
//     `polygon_fill_value()`/`parse_color_token()`). Every case below routes through
//     `decorator: polygon(<sides>, <fill>, <rotation>)` (rotation ALWAYS given explicitly, `0deg`
//     -- this test is not about the sub-property's own optional-with-default behaviour, unrelated
//     to `ESC-6`'s own scope, so it is pinned rather than left to a default this test does not
//     otherwise exercise).
//
//     GOLDEN PROVENANCE -- per `ADR-0020`'s own independence discipline, every expected value below
//     was computed by a THIRD, independent transcription of the pin
//     (`examples/RmlUi/Source/Core/PropertyParserColour.cpp`, 548 lines, read in full for this
//     task): a standalone Python/`numpy.float32` calculator
//     (`/var/tmp/builds/.../scratchpad/esc6_golden_calc.py`, not committed), forcing `float32`
//     rounding at every intermediate step to mirror C++ `float` precision, NEVER reading this
//     file's own `rcss_dump.cpp` implementation nor side B's
//     (`glintfx/src/uix/style/value_compute.cpp`). Two disjoint trust tiers, both documented
//     per-case below:
//       (1) MATHEMATICALLY EXACT anchors -- every intermediate value is provably `0.0f`/`1.0f`/a
//           small exact integer by IEEE754 zero-propagation (adding/subtracting/multiplying by an
//           EXACT `0.0f` or `1.0f` never rounds), so these do not depend on trusting the
//           calculator's `pow`/`cos`/`sin`/`fmod` to match glibc's `powf`/`cosf`/`sinf`/`fmodf`
//           bit-for-bit -- only on IEEE754's OWN mandated exactness for `+`,`-`,`*`,`/`, which every
//           conforming implementation shares.
//       (2) NON-TRIVIAL anchors (real matrix multiplies, real `pow`/`cos`/`sin`) -- chosen with a
//           wide truncation-boundary margin (every computed channel comfortably inside its
//           `[k/255, (k+1)/255)` bucket, verified by inspecting the calculator's own raw `float32`
//           output before truncation, not merely trusted) EXCEPT where called out below as
//           deliberately testing the truncation boundary itself (`#j`, mirrors this file's own
//           `test_15_4`/`test_own_finding_*` precedent of pinning an exact, verified boundary
//           rather than a comfortable one).
// PT: `ESC-6` -- as 8 notações funcionais de cor (`rgb()`/`rgba()`/`hsl()`/`hsla()`/`lab()`/
//     `lch()`/`oklab()`/`oklch()`), só lado A (o próprio mini-parser independente
//     `parse_color_token` do `glintfx/src/rml/rcss_dump.cpp`, o ÚNICO lugar em que este lado
//     parseia cor de texto cru -- usado exclusivamente pela própria sub-propriedade `<fill>` do
//     `polygon()`, o próprio parser passthrough "string" do `decorator_polygon.cpp:786` adiando o
//     parse de cor de fato pro `polygon_fill_value()`/`parse_color_token()`). Todo caso abaixo passa
//     por `decorator: polygon(<sides>, <fill>, <rotation>)` (rotation SEMPRE dado explícito, `0deg`
//     -- este teste não é sobre o próprio comportamento opcional-com-default da sub-propriedade,
//     fora do próprio escopo da `ESC-6`, então é fixado em vez de deixado pro default que este teste
//     não exercita de outra forma).
//
//     PROVENIÊNCIA DO GABARITO -- pela própria disciplina de independência do `ADR-0020`, todo
//     valor esperado abaixo foi computado por uma TERCEIRA transcrição, independente, do pin
//     (`examples/RmlUi/Source/Core/PropertyParserColour.cpp`, 548 linhas, lidas por inteiro pra esta
//     tarefa): uma calculadora standalone Python/`numpy.float32`
//     (`/var/tmp/builds/.../scratchpad/esc6_golden_calc.py`, não commitada), forçando arredondamento
//     `float32` a cada passo intermediário pra espelhar a precisão `float` do C++, nunca lendo o
//     próprio `rcss_dump.cpp` deste arquivo nem o do lado B
//     (`glintfx/src/uix/style/value_compute.cpp`). Duas camadas de confiança disjuntas, cada uma
//     documentada por-caso abaixo:
//       (1) âncoras MATEMATICAMENTE EXATAS -- todo valor intermediário é provadamente `0.0f`/`1.0f`/
//           um inteiro pequeno exato por propagação-de-zero do IEEE754 (somar/subtrair/multiplicar
//           por um `0.0f` ou `1.0f` EXATO nunca arredonda), então não dependem de confiar que o
//           `pow`/`cos`/`sin`/`fmod` da calculadora bate byte a byte com `powf`/`cosf`/`sinf`/`fmodf`
//           da glibc -- só na própria exatidão mandatada do IEEE754 pra `+`,`-`,`*`,`/`, que toda
//           implementação conformante compartilha.
//       (2) âncoras NÃO-TRIVIAIS (multiplicação de matriz de fato, `pow`/`cos`/`sin` de fato) --
//           escolhidas com margem larga da fronteira de truncamento (todo canal computado
//           confortavelmente dentro do próprio balde `[k/255, (k+1)/255)`, verificado inspecionando
//           a própria saída `float32` crua da calculadora antes de truncar, não meramente confiado)
//           EXCETO onde nomeado abaixo como deliberadamente testando a própria fronteira de
//           truncamento (`#j`, espelha o próprio precedente `test_15_4`/`test_own_finding_*` deste
//           arquivo de fixar uma fronteira exata, verificada, em vez de uma confortável).
// ---------------------------------------------------------------------------
void test_esc6_functional_colors(Harness& h) {
  const std::string rml =
      "<rml><head><style>\n"
      "#a { decorator: polygon(6, rgb(255, 0, 0), 0deg); }\n"
      "#b { decorator: polygon(6, rgba(10, 20, 30, 128), 0deg); }\n"
      "#c { decorator: polygon(6, rgb(50%, 100%, 0%), 0deg); }\n"
      "#d { decorator: polygon(6, hsl(0, 100%, 50%), 0deg); }\n"
      "#e { decorator: polygon(6, hsla(240, 100%, 50%, 0.5), 0deg); }\n"
      "#f { decorator: polygon(6, lab(0 0 0), 0deg); }\n"
      "#g { decorator: polygon(6, lch(0 0 0), 0deg); }\n"
      "#h { decorator: polygon(6, oklab(0 0 0), 0deg); }\n"
      "#i { decorator: polygon(6, oklch(0 0 0), 0deg); }\n"
      "#j { decorator: polygon(6, lab(53.24 80.09 67.20 / 0.5), 0deg); }\n"
      "#k { decorator: polygon(6, lch(60 50 30), 0deg); }\n"
      "#l { decorator: polygon(6, oklch(0.7 0.15 145), 0deg); }\n"
      "#m { decorator: polygon(6, lab(50 none none), 0deg); }\n"
      "</style></head><body>"
      "<div id=\"a\"></div><div id=\"b\"></div><div id=\"c\"></div><div id=\"d\"></div>"
      "<div id=\"e\"></div><div id=\"f\"></div><div id=\"g\"></div><div id=\"h\"></div>"
      "<div id=\"i\"></div><div id=\"j\"></div><div id=\"k\"></div><div id=\"l\"></div>"
      "<div id=\"m\"></div>"
      "</body></rml>";
  Rml::ElementDocument* doc = h.load(rml);
  check(doc != nullptr, "ESC-6: document loaded");
  if (!doc) return;

  const std::string dump = glintfx::rcss_dump_document(doc);
  auto polygon_at = [&](int idx) { return extract_prop(dump, "STATE none\n", "body/" + std::to_string(idx), "decorator"); };

  // (1) EXACT: rgb()/rgba() integer form -- atoi only, zero float ops, zero rounding ambiguity.
  check_eq(polygon_at(0), "polygon(6.0000;#ff0000ff;0.0000)", "ESC-6 rgb(255,0,0): pure int, exact");
  check_eq(polygon_at(1), "polygon(6.0000;#0a141e80;0.0000)",
           "ESC-6 rgba(10,20,30,128): plain-int alpha (NOT percent, NOT bare-atof-as-fraction -- "
           "rgba's alpha shares the SAME int-domain parse as R/G/B, PropertyParserColour.cpp:275-286)");

  // (2) EXACT-MARGIN: rgb() percent form -- one float multiply/truncate per channel
  //     (`(float)atof(strip%) * (255.0f/100.0f)`, `:281`), margin-verified: 50%->127.5 truncates to
  //     127 regardless of which side of .5 float noise lands on; 100%->255.0 verified by the
  //     calculator's own raw float32 output to land AT (not just below) 255.0, so the truncation is
  //     not a coincidence of Python-vs-C++ float noise.
  check_eq(polygon_at(2), "polygon(6.0000;#7fff00ff;0.0000)", "ESC-6 rgb(50%,100%,0%): percent form");

  // (3) EXACT: hsl(0,100%,50%) is pure red -- H=0,S=1,L=0.5 makes every intermediate value exactly
  //     0.0f/0.5f/1.0f (HSL_f's own `k`/`a`/`min`/`max` terms all land on EXACT float32 values for
  //     these inputs, hand-traced against PropertyParserColour.cpp:11-36).
  check_eq(polygon_at(3), "polygon(6.0000;#ff0000ff;0.0000)", "ESC-6 hsl(0,100%,50%): exact pure red");

  // (4) EXACT-MARGIN: hsla(240,100%,50%,0.5) is pure blue, alpha=0.5*255=127.5->127. H=240 puts the
  //     HSL_f `k` terms within ~1e-6 of the exact 0/4/8/12 lattice (float32 noise from `h*(1/30)`
  //     not being exactly 8.0) -- margin-traced by hand (this file's own comment) to be >>1e-6 away
  //     from any branch-selection boundary in HSL_f's own nested min/max, so the noise washes out
  //     to the SAME R=0,G=0,B=1.0 exact result either way.
  check_eq(polygon_at(4), "polygon(6.0000;#0000ff7f;0.0000)", "ESC-6 hsla(240,100%,50%,0.5): pure blue, alpha 127");

  // (5)-(8) MATHEMATICALLY EXACT: all-zero lab()/lch()/oklab()/oklch() collapse to black by
  //     IEEE754 zero-propagation alone (every `+0`/`-0`/`*0` is exact; CIELAB's own linear
  //     f-inverse branch subtracts the SAME `16.0f/116.0f` literal it just added, giving an EXACT
  //     `0.0f`, not merely a small number -- hand-verified in this task's own delivery report).
  //     alpha default "1.0" parses to exactly 1.0f (a trivially-exact float32 literal).
  check_eq(polygon_at(5), "polygon(6.0000;#000000ff;0.0000)", "ESC-6 lab(0 0 0): exact black (IEEE754 zero-propagation)");
  check_eq(polygon_at(6), "polygon(6.0000;#000000ff;0.0000)", "ESC-6 lch(0 0 0): exact black (chroma=0 -> a=b=0, same as lab)");
  check_eq(polygon_at(7), "polygon(6.0000;#000000ff;0.0000)", "ESC-6 oklab(0 0 0): exact black (IEEE754 zero-propagation)");
  check_eq(polygon_at(8), "polygon(6.0000;#000000ff;0.0000)", "ESC-6 oklch(0 0 0): exact black (chroma=0 -> a=b=0, same as oklab)");

  // (9) NON-TRIVIAL, DELIBERATE TRUNCATION BOUNDARY: lab(53.24 80.09 67.20 / 0.5) -- real CIELAB
  //     matrix + `pow()`-based sRGB transfer, slash-alpha syntax. The calculator's own raw (pre-
  //     truncation) red channel measured 0.99997705f -- comfortably `< 1.0f` (by ~230x a single
  //     float32 ULP at this magnitude) but close enough to 1.0 that `r*255=254.994` TRUNCATES to
  //     254 (`0xfe`), not 255 -- a real, deliberate boundary case (mirrors `test_15_4`'s own
  //     "truncates toward zero, does not round" discipline), not a margin oversight. Alpha
  //     0.5*255=127.5 truncates to 127 (`0x7f`) with a comfortable margin.
  check_eq(polygon_at(9), "polygon(6.0000;#fe00007f;0.0000)",
           "ESC-6 lab(53.24 80.09 67.20 / 0.5): near-red, DELIBERATE truncation boundary (raw "
           "r=0.99997705f truncates to 254, not 255 -- proves (int) truncates toward zero rather "
           "than rounds)");

  // (10)-(11) NON-TRIVIAL, comfortable margins: lch() polar->Cartesian->matrix, oklch() likewise --
  //     every computed channel measured well inside its truncation bucket by the calculator's own
  //     raw float32 output (not merely assumed).
  check_eq(polygon_at(10), "polygon(6.0000;#df6e67ff;0.0000)", "ESC-6 lch(60 50 30): polar->Cartesian, real matrix, comfortable margins");
  check_eq(polygon_at(11), "polygon(6.0000;#5ab660ff;0.0000)", "ESC-6 oklch(0.7 0.15 145): Oklab polar form, comfortable margins");

  // (12) 'none' keyword: lab(50 none none) == lab(50 0 0) -- both axes read as exactly 0.0f
  //     (PropertyParserColour.cpp:362-363/383-384/402-403/417-418's own uniform "'none' -> 0.0f"
  //     rule, present in all four families).
  check_eq(polygon_at(12), "polygon(6.0000;#767676ff;0.0000)", "ESC-6 lab(50 none none): 'none' reads as 0.0f, same as lab(50 0 0)");

  doc->Close();
  h.engine.update();
}

} // namespace

int main() {
  test_15_4_quantize_boundary();
  test_quantize_magnitude_ceiling();

  Harness h;
  if (!h.setup()) {
    std::puts("FAIL: harness setup (window/engine attach)");
    return 2;
  }

  test_15_1_two_states_one_node_hover(h);
  test_15_2_shorthand_order_border_top(h);
  test_15_3_three_percent_families(h);
  test_quantize_magnitude_polygon_sides_sibling(h);
  test_own_finding_premultiplied_alpha_has_teeth(h);
  test_9_1_box_shadow_literal_worked_example(h);
  test_9_2_1_auto_spacing_rules_3_and_4(h);
  test_esc6_functional_colors(h);

  if (g_failures == 0) {
    std::puts(
        "rcss_dump_worked_examples OK (4 worked examples: 15.1, 15.2, 15.3, 15.4; plus own-finding "
        "premultiplied-alpha-has-teeth; plus UIX-RCSS-CONFORMIDADE additions: 9.1 literal, 9.2.1 "
        "rules 3+4; plus UIX-QUANTIZE-MAGNITUDE ceiling coverage; plus ESC-6 functional colors)");
    return 0;
  }
  std::printf("rcss_dump_worked_examples: %d failure(s)\n", g_failures);
  return 10;
}
