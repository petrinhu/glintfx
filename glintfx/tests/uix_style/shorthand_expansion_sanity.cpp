// SPDX-License-Identifier: Apache-2.0
// EN: UIX-PROP-REGISTRY -- functional test for glintfx::uix::style's shorthand-to-longhand
//     expansion (shorthand.hpp). Standalone, no parser, no cascade -- every case below traces to
//     a specific docs/uix-rcss.md section 6.2/6.3 fact, a specific real-upstream-RmlUi-source
//     citation (`examples/RmlUi/Source/Core/PropertySpecification.cpp:311-472`, read directly for
//     this item, not paraphrased from the spec alone), or a specific census-measured value-count
//     distribution (`/var/tmp/censo-rcss-qa1/censo.md` section 4) -- this task's own DoD names
//     the exact counts to prove: "padding 1/2/4-valor, margin idem, border 2-parte,
//     border-radius 1-valor".
// PT: UIX-PROP-REGISTRY -- teste funcional pra expansão de shorthand-pra-longhand do
//     glintfx::uix::style (shorthand.hpp). Standalone, sem parser, sem cascata -- todo caso abaixo
//     remonta a um fato específico da seção 6.2/6.3 do docs/uix-rcss.md, uma citação específica do
//     RmlUi upstream real (`examples/RmlUi/Source/Core/PropertySpecification.cpp:311-472`, lido
//     direto pra este item, não parafraseado só da spec), ou uma distribuição específica de
//     contagem-de-valor medida pelo censo (`/var/tmp/censo-rcss-qa1/censo.md` seção 4) -- o
//     próprio DoD desta tarefa nomeia as contagens exatas a provar: "padding 1/2/4-valor, margin
//     idem, border 2-parte, border-radius 1-valor".
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/style/shorthand.hpp"

#include <cstdio>
#include <string_view>
#include <vector>

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

using glintfx::uix::style::expand_shorthand;
using glintfx::uix::style::is_shorthand;
using glintfx::uix::style::LonghandValue;
using glintfx::uix::style::ShorthandExpandStatus;

bool find_value(const std::vector<LonghandValue>& v, std::string_view name,
                std::string_view* out) {
  for (const auto& lv : v) {
    if (lv.name == name) {
      *out = lv.value;
      return true;
    }
  }
  return false;
}

void check_value(const std::vector<LonghandValue>& v, std::string_view name,
                 std::string_view want, const char* what) {
  std::string_view got;
  if (!find_value(v, name, &got)) {
    std::fprintf(stderr, "FAIL: %s (longhand '%.*s' missing from expansion)\n", what,
                 static_cast<int>(name.size()), name.data());
    ++g_failures;
    return;
  }
  if (got != want) {
    std::fprintf(stderr, "FAIL: %s (longhand '%.*s': got \"%.*s\", want \"%.*s\")\n", what,
                 static_cast<int>(name.size()), name.data(), static_cast<int>(got.size()),
                 got.data(), static_cast<int>(want.size()), want.data());
    ++g_failures;
  }
}

// ---------------------------------------------------------------------------
// EN: Case 1 -- `margin`'s own Box expansion, the three corpus-measured value counts
//     (docs/uix-rcss.md section 6.2: "1-value: 44, 2-value: 16, 4-value: 39") plus the
//     zero-measured-but-valid 3-value row (section 6.3's own explicit note: real, reachable
//     upstream behaviour, included for parity, not speculative).
// PT: Caso 1 -- a própria expansão Box de `margin`, as três contagens de valor medidas pelo corpus
//     (seção 6.2 do docs/uix-rcss.md: "1-valor: 44, 2-valor: 16, 4-valor: 39") mais a linha de
//     3-valor zero-medida-mas-válida (a própria nota explícita da seção 6.3: comportamento real,
//     alcançável, do upstream, incluído por paridade, não especulativo).
// ---------------------------------------------------------------------------
void test_margin_box_expansion_all_value_counts() {
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("margin", "16px", &out) == ShorthandExpandStatus::Ok,
          "margin 1-value: Ok");
    check(out.size() == 4, "margin 1-value: 4 longhands produced");
    check_value(out, "margin-top", "16px", "margin 1-value: top");
    check_value(out, "margin-right", "16px", "margin 1-value: right");
    check_value(out, "margin-bottom", "16px", "margin 1-value: bottom");
    check_value(out, "margin-left", "16px", "margin 1-value: left");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("margin", "10px 20px", &out) == ShorthandExpandStatus::Ok,
          "margin 2-value: Ok");
    check_value(out, "margin-top", "10px", "margin 2-value: top = v0");
    check_value(out, "margin-right", "20px", "margin 2-value: right = v1");
    check_value(out, "margin-bottom", "10px", "margin 2-value: bottom = v0");
    check_value(out, "margin-left", "20px", "margin 2-value: left = v1");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("margin", "1px 2px 3px", &out) == ShorthandExpandStatus::Ok,
          "margin 3-value: Ok (zero-measured, valid per docs/uix-rcss.md section 6.3)");
    check_value(out, "margin-top", "1px", "margin 3-value: top = v0");
    check_value(out, "margin-right", "2px", "margin 3-value: right = v1");
    check_value(out, "margin-bottom", "3px", "margin 3-value: bottom = v2");
    check_value(out, "margin-left", "2px", "margin 3-value: left = v1 (same as right)");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("margin", "1px 2px 3px 4px", &out) == ShorthandExpandStatus::Ok,
          "margin 4-value: Ok");
    check_value(out, "margin-top", "1px", "margin 4-value: top = v0");
    check_value(out, "margin-right", "2px", "margin 4-value: right = v1");
    check_value(out, "margin-bottom", "3px", "margin 4-value: bottom = v2");
    check_value(out, "margin-left", "4px", "margin 4-value: left = v3");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("margin", "1px 2px 3px 4px 5px", &out) ==
              ShorthandExpandStatus::MalformedValue,
          "margin 5-value: MalformedValue (over-specified, matches upstream's own "
          "'Abort over-specified shorthand values')");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 2 -- `padding`'s own Box expansion, corpus-measured (section 6.2: "1-value: 29,
//     2-value: 59, 4-value: 18").
// PT: Caso 2 -- a própria expansão Box de `padding`, medida pelo corpus (seção 6.2: "1-valor: 29,
//     2-valor: 59, 4-valor: 18").
// ---------------------------------------------------------------------------
void test_padding_box_expansion_all_value_counts() {
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("padding", "24px", &out) == ShorthandExpandStatus::Ok,
          "padding 1-value: Ok");
    check_value(out, "padding-top", "24px", "padding 1-value: top");
    check_value(out, "padding-left", "24px", "padding 1-value: left");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("padding", "8px 12px", &out) == ShorthandExpandStatus::Ok,
          "padding 2-value: Ok");
    check_value(out, "padding-top", "8px", "padding 2-value: top = v0");
    check_value(out, "padding-right", "12px", "padding 2-value: right = v1");
    check_value(out, "padding-bottom", "8px", "padding 2-value: bottom = v0");
    check_value(out, "padding-left", "12px", "padding 2-value: left = v1");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("padding", "1px 2px 3px 4px", &out) == ShorthandExpandStatus::Ok,
          "padding 4-value: Ok");
    check_value(out, "padding-top", "1px", "padding 4-value: top = v0");
    check_value(out, "padding-right", "2px", "padding 4-value: right = v1");
    check_value(out, "padding-bottom", "3px", "padding 4-value: bottom = v2");
    check_value(out, "padding-left", "4px", "padding 4-value: left = v3");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 3 -- `border-radius`'s own Box expansion, 100% 1-value per the corpus (section 6.2),
//     but targeting the 4 CORNER longhands in upstream's own registered order (top-left,
//     top-right, bottom-right, bottom-left -- `StyleSheetSpecification.cpp`'s own
//     `RegisterShorthand(ShorthandId::BorderRadius, "border-radius",
//     "border-top-left-radius, border-top-right-radius, border-bottom-right-radius,
//     border-bottom-left-radius", ShorthandType::Box)`), NOT the side names ("top/right/bottom/
//     left") margin/padding use -- same Box ALGORITHM, different target names, and this is the
//     concrete proof this test pins so a future refactor does not silently swap corner order
//     (that would produce a shorthand that expands to the wrong physical corner for the 2/3/4
//     value forms, invisible in the 1-value form this section's own 100% measured distribution
//     alone would exercise).
// PT: Caso 3 -- a própria expansão Box de `border-radius`, 100% 1-valor per o corpus (seção 6.2),
//     mas mirando os 4 longhands de CANTO na própria ordem registrada do upstream (topo-esquerda,
//     topo-direita, baixo-direita, baixo-esquerda -- o próprio
//     `RegisterShorthand(ShorthandId::BorderRadius, "border-radius",
//     "border-top-left-radius, border-top-right-radius, border-bottom-right-radius,
//     border-bottom-left-radius", ShorthandType::Box)` do `StyleSheetSpecification.cpp`), NÃO os
//     nomes de lado ("top/right/bottom/left") que margin/padding usam -- mesmo ALGORITMO Box,
//     nomes-alvo diferentes, e esta é a prova concreta que este teste pina pra um futuro refactor
//     não trocar a ordem dos cantos em silêncio (isso produziria um shorthand que expande pro
//     canto físico errado nas formas de 2/3/4 valores, invisível na própria forma de 1-valor que a
//     própria distribuição 100%-medida desta seção sozinha exercitaria).
// ---------------------------------------------------------------------------
void test_border_radius_box_expansion_corner_order() {
  std::vector<LonghandValue> out;
  check(expand_shorthand("border-radius", "12px", &out) == ShorthandExpandStatus::Ok,
        "border-radius 1-value: Ok (100% of corpus usage, section 6.2)");
  check_value(out, "border-top-left-radius", "12px", "border-radius 1-value: top-left");
  check_value(out, "border-top-right-radius", "12px", "border-radius 1-value: top-right");
  check_value(out, "border-bottom-right-radius", "12px", "border-radius 1-value: bottom-right");
  check_value(out, "border-bottom-left-radius", "12px", "border-radius 1-value: bottom-left");

  std::vector<LonghandValue> out4;
  check(expand_shorthand("border-radius", "1px 2px 3px 4px", &out4) == ShorthandExpandStatus::Ok,
        "border-radius 4-value: Ok (zero-measured, valid)");
  check_value(out4, "border-top-left-radius", "1px", "border-radius 4-value: top-left = v0");
  check_value(out4, "border-top-right-radius", "2px", "border-radius 4-value: top-right = v1");
  check_value(out4, "border-bottom-right-radius", "3px",
              "border-radius 4-value: bottom-right = v2");
  check_value(out4, "border-bottom-left-radius", "4px", "border-radius 4-value: bottom-left = v3");
}

// ---------------------------------------------------------------------------
// EN: Case 4 -- `border-top`'s own FallThrough (width, color), 100% 2-part per the corpus
//     (section 6.2: "100% 2-part (width + color, never a 3rd token)"). Real corpus order is
//     ALWAYS width-then-color (`1dp #7A5A2E`, censo.md section 3's own cited example), and this
//     test PROVES it must be: 🔴 a correction to docs/uix-rcss.md section 6.2's own "order-
//     independent between the two" phrase, found by tracing upstream's own real mechanics
//     (`PropertySpecification.cpp:429-471`, read directly, not paraphrased) rather than trusting
//     that sentence at face value. With EXACTLY 2 tokens and 2 items, upstream's own loop always
//     advances `property_index` (the item cursor) every iteration, success or failure, but only
//     advances `value_index` (the token cursor) on a MATCH -- so if `token[0]` fails item[0]'s own
//     domain and only matches item[1] (the reversed-order case), item[1] consumes `token[0]`,
//     `property_index` reaches `items.size()` with `token[1]` STILL UNCONSUMED, and upstream's own
//     post-loop guard (`value_index < property_values.size() && property_index >=
//     items.size()`) aborts the WHOLE shorthand -- not a partial result, a total failure, verified
//     by hand-tracing every iteration of the real loop for both token orders. "Order-independent"
//     in section 6.2 is true only for WHICH domain a given token maps to (content-driven, not "the
//     Nth item always gets the Nth token") -- it is NOT true that an arbitrary token ORDER always
//     succeeds for a 2-item/2-token chain; the corpus's own 100%-measured width-then-color order
//     is not incidental, it is the ONLY order that lets both tokens be consumed. Reported as a
//     genuine finding, not silently "fixed" by rewording the spec myself.
// PT: Caso 4 -- o próprio FallThrough de `border-top` (width, color), 100% 2-parte per o corpus
//     (seção 6.2: "100% 2-parte (width + color, nunca um 3º token)"). A ordem real do corpus é
//     SEMPRE width-depois-color (`1dp #7A5A2E`, o próprio exemplo citado do censo.md seção 3), e
//     este teste PROVA que precisa ser: 🔴 uma correção à própria frase "independente de ordem
//     entre os dois" da seção 6.2 do docs/uix-rcss.md, achada rastreando a própria mecânica real
//     do upstream (`PropertySpecification.cpp:429-471`, lida direto, não parafraseada) em vez de
//     confiar naquela frase de cara. Com EXATAMENTE 2 tokens e 2 itens, o próprio laço do upstream
//     sempre avança `property_index` (o cursor de item) a cada iteração, sucesso ou falha, mas só
//     avança `value_index` (o cursor de token) num CASAMENTO -- então se `token[0]` falha o próprio
//     domínio do item[0] e só casa com o item[1] (o caso de ordem revertida), o item[1] consome
//     `token[0]`, `property_index` chega em `items.size()` com `token[1]` AINDA NÃO-CONSUMIDO, e a
//     própria guarda pós-laço do upstream (`value_index < property_values.size() && property_index
//     >= items.size()`) aborta o shorthand INTEIRO -- não um resultado parcial, uma falha total,
//     verificada rastreando à mão cada iteração do laço real pras duas ordens de token.
//     "Independente de ordem" na seção 6.2 é verdade só pra QUAL domínio um dado token mapeia
//     (guiado por conteúdo, não "o Nº item sempre pega o Nº token") -- NÃO é verdade que uma ORDEM
//     arbitrária de token sempre funciona pra uma cadeia de 2-itens/2-tokens; a própria ordem
//     width-depois-color 100%-medida do corpus não é incidental, é a ÚNICA ordem que deixa os dois
//     tokens serem consumidos. Reportado como achado genuíno, não "consertado" em silêncio
//     reescrevendo a spec por conta própria.
// ---------------------------------------------------------------------------
void test_border_top_fallthrough_order_is_load_bearing() {
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("border-top", "1dp #7A5A2E", &out) == ShorthandExpandStatus::Ok,
          "border-top width-then-color: Ok (real corpus order, censo.md section 3 -- the ONLY "
          "order that lets both tokens be consumed, see this case's own header comment)");
    check_value(out, "border-top-width", "1dp", "border-top width-then-color: width");
    check_value(out, "border-top-color", "#7A5A2E", "border-top width-then-color: color");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("border-top", "#7A5A2E 1dp", &out) ==
              ShorthandExpandStatus::MalformedValue,
          "border-top color-then-width: MalformedValue -- traced from upstream's own real "
          "mechanics, NOT order-independent for a 2-token/2-item chain (see this case's own "
          "header comment for the exact iteration-by-iteration trace); this is not this module's "
          "own bug, it is upstream's own real, verified behaviour, and the corpus never hits it "
          "(100% width-then-color, censo.md section 3)");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("border-top", "1dp 2dp #fff", &out) ==
              ShorthandExpandStatus::MalformedValue,
          "border-top 3-token: MalformedValue -- census's own '100% 2-part, never a 3rd token'");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 5 -- `border`'s own RecursiveRepeat: the SAME 2-token value string is fed verbatim to
//     all 4 side-shorthands (docs/uix-rcss.md section 6.2's own citation), so a single `border:
//     1dp #7A5A2E;` produces all 8 side-longhands (4 sides * width+color), corpus-measured 100%
//     2-part (section 6.2).
// PT: Caso 5 -- o próprio RecursiveRepeat de `border`: o MESMO texto de valor de 2 tokens é
//     alimentado verbatim aos 4 shorthands de lado (a própria citação da seção 6.2 do
//     docs/uix-rcss.md), então um único `border: 1dp #7A5A2E;` produz os 8 longhands de lado (4
//     lados * width+color), 100% 2-parte medido pelo corpus (seção 6.2).
// ---------------------------------------------------------------------------
void test_border_recursive_repeat_feeds_all_four_sides() {
  std::vector<LonghandValue> out;
  check(expand_shorthand("border", "1dp #7A5A2E", &out) == ShorthandExpandStatus::Ok,
        "border RecursiveRepeat: Ok");
  check(out.size() == 8, "border RecursiveRepeat: 8 longhands (4 sides * width+color)");
  check_value(out, "border-top-width", "1dp", "border RecursiveRepeat: top width");
  check_value(out, "border-top-color", "#7A5A2E", "border RecursiveRepeat: top color");
  check_value(out, "border-right-width", "1dp", "border RecursiveRepeat: right width");
  check_value(out, "border-right-color", "#7A5A2E", "border RecursiveRepeat: right color");
  check_value(out, "border-bottom-width", "1dp", "border RecursiveRepeat: bottom width");
  check_value(out, "border-bottom-color", "#7A5A2E", "border RecursiveRepeat: bottom color");
  check_value(out, "border-left-width", "1dp", "border RecursiveRepeat: left width");
  check_value(out, "border-left-color", "#7A5A2E", "border RecursiveRepeat: left color");
}

// ---------------------------------------------------------------------------
// EN: Case 6 -- `border-color`'s own Box expansion, 100% 1-value per the corpus, targeting the
//     side longhands (top, right, bottom, left -- upstream's own registered order for THIS
//     shorthand, unlike border-radius's corner order in case 3).
// PT: Caso 6 -- a própria expansão Box de `border-color`, 100% 1-valor per o corpus, mirando os
//     longhands de lado (top, right, bottom, left -- a própria ordem registrada do upstream pra
//     ESTE shorthand, diferente da ordem de canto de border-radius no caso 3).
// ---------------------------------------------------------------------------
void test_border_color_box_expansion() {
  std::vector<LonghandValue> out;
  check(expand_shorthand("border-color", "#7A5A2E", &out) == ShorthandExpandStatus::Ok,
        "border-color 1-value: Ok");
  check_value(out, "border-top-color", "#7A5A2E", "border-color 1-value: top");
  check_value(out, "border-right-color", "#7A5A2E", "border-color 1-value: right");
  check_value(out, "border-bottom-color", "#7A5A2E", "border-color 1-value: bottom");
  check_value(out, "border-left-color", "#7A5A2E", "border-color 1-value: left");
}

// ---------------------------------------------------------------------------
// EN: Case 7 -- `background`'s own FallThrough, 1 item -- the whole raw value goes to
//     `background-color` unconditionally (docs/uix-rcss.md section 6.2: "100% solid-color
//     value... gradients go through `decorator`, never `background`" -- this module does not
//     validate the value IS a solid color, it just routes it, per this file's own header "Scope"
//     boundary shared with property_registry.hpp).
// PT: Caso 7 -- o próprio FallThrough de `background`, 1 item -- o valor cru inteiro vai pra
//     `background-color` incondicionalmente (seção 6.2 do docs/uix-rcss.md: "100% valor sólido de
//     cor... gradiente vai por `decorator`, nunca por `background`" -- este módulo não valida que
//     o valor É uma cor sólida, só roteia, per a própria fronteira "Escopo" do cabeçalho deste
//     arquivo, compartilhada com o property_registry.hpp).
// ---------------------------------------------------------------------------
void test_background_fallthrough_one_item() {
  std::vector<LonghandValue> out;
  check(expand_shorthand("background", "#0d1020", &out) == ShorthandExpandStatus::Ok,
        "background: Ok");
  check(out.size() == 1, "background: exactly 1 longhand produced");
  check_value(out, "background-color", "#0d1020", "background: whole value routes to -color");
}

// ---------------------------------------------------------------------------
// EN: Case 8 -- `gap`/`overflow`'s own Replicate: 1 value sets both targets, 2 values set each
//     independently (docs/uix-rcss.md section 6.2).
// PT: Caso 8 -- o próprio Replicate de `gap`/`overflow`: 1 valor seta os dois alvos, 2 valores
//     setam cada um independentemente (seção 6.2 do docs/uix-rcss.md).
// ---------------------------------------------------------------------------
void test_gap_and_overflow_replicate() {
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("gap", "8px", &out) == ShorthandExpandStatus::Ok, "gap 1-value: Ok");
    check_value(out, "row-gap", "8px", "gap 1-value: row-gap");
    check_value(out, "column-gap", "8px", "gap 1-value: column-gap");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("gap", "8px 16px", &out) == ShorthandExpandStatus::Ok,
          "gap 2-value: Ok");
    check_value(out, "row-gap", "8px", "gap 2-value: row-gap = v0");
    check_value(out, "column-gap", "16px", "gap 2-value: column-gap = v1");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("overflow", "hidden", &out) == ShorthandExpandStatus::Ok,
          "overflow 1-value: Ok");
    check_value(out, "overflow-x", "hidden", "overflow 1-value: x");
    check_value(out, "overflow-y", "hidden", "overflow 1-value: y");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("gap", "1px 2px 3px", &out) == ShorthandExpandStatus::MalformedValue,
          "gap 3-value: MalformedValue (Replicate only accepts 1 or 2)");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 9 -- `flex`'s own Flex algorithm, the "second implementer would guess wrong" default
//     this task's own brief flags: the bare keyword `none` expands to `0 0 auto` (NOT each
//     longhand's own normal initial value, which would be `flex-grow:0` -- coincidentally equal
//     here -- but `flex-shrink:1`/`flex-basis:auto`, i.e. shrink genuinely differs), verbatim from
//     `PropertySpecification.cpp:315-318`'s own `property_values = {"0", "0", "auto"};`.
// PT: Caso 9 -- o próprio algoritmo Flex de `flex`, o default "um segundo implementador chutaria
//     errado" que o próprio briefing desta tarefa sinaliza: a palavra-chave `none` sozinha expande
//     pra `0 0 auto` (NÃO o próprio valor inicial normal de cada longhand, que seria
//     `flex-grow:0` -- coincidentemente igual aqui -- mas `flex-shrink:1`/`flex-basis:auto`, ou
//     seja, shrink genuinamente diverge), verbatim do próprio
//     `property_values = {"0", "0", "auto"};` do `PropertySpecification.cpp:315-318`.
// ---------------------------------------------------------------------------
void test_flex_none_expands_to_0_0_auto() {
  std::vector<LonghandValue> out;
  check(expand_shorthand("flex", "none", &out) == ShorthandExpandStatus::Ok, "flex none: Ok");
  check_value(out, "flex-grow", "0", "flex none: grow = 0");
  check_value(out, "flex-shrink", "0", "flex none: shrink = 0 (NOT the registry initial 1)");
  check_value(out, "flex-basis", "auto", "flex none: basis = auto");
}

// ---------------------------------------------------------------------------
// EN: Case 10 -- `flex`'s own omitted-trailing-values default, the OTHER "guess wrong" trap this
//     task's own brief names: omitted values default to `1`/`1`/`0` -- NOT each longhand's own
//     registry initial value (`flex-grow` initial is `0`, `flex-basis` initial is `auto`, per
//     property_registry.hpp's own table) -- verbatim from
//     `PropertySpecification.cpp:320-334`'s own `const char* default_omitted_values[] = {"1",
//     "1", "0"};`. Three sub-cases: 3 values given (pure positional, no omission), 2 values
//     (basis omitted, defaults to "0"), 1 value that is basis-shaped (grow/shrink both omitted,
//     default to "1" each) -- this last one is the genuinely tricky FallThrough-routing case
//     (the single token fails the grow/shrink NUMBER classifier and only the LAST item's own
//     catch-all classifier accepts it).
// PT: Caso 10 -- o próprio default de valores-finais-omitidos de `flex`, a OUTRA armadilha
//     "chuta errado" que o próprio briefing desta tarefa nomeia: valores omitidos default pra
//     `1`/`1`/`0` -- NÃO o próprio valor inicial de registro de cada longhand (o inicial de
//     `flex-grow` é `0`, o de `flex-basis` é `auto`, per a própria tabela do
//     property_registry.hpp) -- verbatim do próprio
//     `const char* default_omitted_values[] = {"1", "1", "0"};` do
//     `PropertySpecification.cpp:320-334`. Três subcasos: 3 valores dados (puramente posicional,
//     sem omissão), 2 valores (basis omitido, default "0"), 1 valor com forma de basis (grow/
//     shrink os dois omitidos, default "1" cada) -- este último é o caso genuinamente delicado de
//     roteamento-por-FallThrough (o token único falha o classificador NÚMERO de grow/shrink e só
//     o próprio classificador catch-all do ÚLTIMO item o aceita).
// ---------------------------------------------------------------------------
void test_flex_omitted_trailing_value_defaults() {
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("flex", "2 3 30px", &out) == ShorthandExpandStatus::Ok,
          "flex 3-value: Ok, pure positional");
    check_value(out, "flex-grow", "2", "flex 3-value: grow");
    check_value(out, "flex-shrink", "3", "flex 3-value: shrink");
    check_value(out, "flex-basis", "30px", "flex 3-value: basis");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("flex", "2 3", &out) == ShorthandExpandStatus::Ok,
          "flex 2-value: Ok");
    check_value(out, "flex-grow", "2", "flex 2-value: grow");
    check_value(out, "flex-shrink", "3", "flex 2-value: shrink");
    check_value(out, "flex-basis", "0", "flex 2-value: basis omitted, defaults to '0'");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("flex", "1", &out) == ShorthandExpandStatus::Ok,
          "flex 1-value (number-shaped): Ok");
    check_value(out, "flex-grow", "1", "flex 1-value: grow = the given token");
    check_value(out, "flex-shrink", "1", "flex 1-value: shrink omitted, defaults to '1'");
    check_value(out, "flex-basis", "0", "flex 1-value: basis omitted, defaults to '0'");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("flex", "30px", &out) == ShorthandExpandStatus::Ok,
          "flex 1-value (length-shaped): Ok -- routes to basis via FallThrough, not grow");
    check_value(out, "flex-grow", "1", "flex 1-value length-shaped: grow omitted, defaults to '1'");
    check_value(out, "flex-shrink", "1",
                "flex 1-value length-shaped: shrink omitted, defaults to '1'");
    check_value(out, "flex-basis", "30px", "flex 1-value length-shaped: basis = the given token");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 11 -- fail-high: an unregistered name is `UnknownShorthand` (never crashes, never
//     silently no-ops -- header "Fail-high policy", shared with property_registry.hpp).
// PT: Caso 11 -- fail-high: um nome não-registrado é `UnknownShorthand` (nunca trava, nunca
//     no-opa em silêncio -- "Política fail-high" no cabeçalho, compartilhada com o
//     property_registry.hpp).
// ---------------------------------------------------------------------------
void test_unknown_shorthand_name_is_fail_high() {
  std::vector<LonghandValue> out;
  check(expand_shorthand("not-a-real-shorthand", "1px", &out) ==
            ShorthandExpandStatus::UnknownShorthand,
        "unknown shorthand name: UnknownShorthand, not a crash");
  check(out.empty(), "unknown shorthand name: out left empty");
  check(!is_shorthand("not-a-real-shorthand"), "is_shorthand: false for an unregistered name");
  check(is_shorthand("margin"), "is_shorthand: true for a registered shorthand");
  check(!is_shorthand("color"), "is_shorthand: false for a plain longhand (not a shorthand)");
}

} // namespace

int main() {
  test_margin_box_expansion_all_value_counts();
  test_padding_box_expansion_all_value_counts();
  test_border_radius_box_expansion_corner_order();
  test_border_top_fallthrough_order_is_load_bearing();
  test_border_recursive_repeat_feeds_all_four_sides();
  test_border_color_box_expansion();
  test_background_fallthrough_one_item();
  test_gap_and_overflow_replicate();
  test_flex_none_expands_to_0_0_auto();
  test_flex_omitted_trailing_value_defaults();
  test_unknown_shorthand_name_is_fail_high();

  if (g_failures > 0) {
    std::fprintf(stderr, "shorthand_expansion_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("shorthand_expansion_sanity: PASS");
  return 0;
}
