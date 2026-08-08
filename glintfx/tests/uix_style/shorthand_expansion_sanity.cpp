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

#include "uix/style/property_registry.hpp"

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

using glintfx::uix::style::all_shorthands;
using glintfx::uix::style::expand_shorthand;
using glintfx::uix::style::find_property;
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

// ===========================================================================
// EN: `ESC-2` -- +7 shorthands, closing 13 -> 20 (docs/uix-rcss.md section 6.2, `ADR-0022`'s own
//     "corpus is sequencing data, never a boundary" doctrine: none of these 7 are cut/deferred by
//     "low measured use"). Cases 12-19 below are the 7 new shorthands themselves (Box: `border-
//     width`/`inset`/`nav`; FallThrough: `font`/`perspective-origin`/`transform-origin`/
//     `flex-flow`); case 20 is `UIX-RCSS-ERRATA-8`'s own direct pin (the semantic fix
//     `expand_fallthrough` needed so the 4 new FallThrough shorthands, `font` above all, do not
//     require every item to be claimed); case 21 is the 13->20 coverage cross-check; case 22 is
//     the optional `flex: none <extra>` sibling finding.
// PT: `ESC-2` -- +7 shorthands, fechando 13 -> 20 (seção 6.2 do docs/uix-rcss.md, a própria
//     doutrina "corpus é dado de sequenciamento, nunca fronteira" da `ADR-0022`: nenhum destes 7 é
//     cortado/adiado por "uso baixo medido"). Os casos 12-19 abaixo são os 7 shorthands novos em si
//     (Box: `border-width`/`inset`/`nav`; FallThrough: `font`/`perspective-origin`/
//     `transform-origin`/`flex-flow`); o caso 20 é o próprio pino direto da `UIX-RCSS-ERRATA-8` (o
//     conserto semântico que o `expand_fallthrough` precisava pros 4 shorthands FallThrough novos,
//     o `font` acima de todos, não exigirem todo item reivindicado); o caso 21 é o cross-check de
//     cobertura 13->20; o caso 22 é o achado irmão opcional `flex: none <extra>`.
// ===========================================================================

// ---------------------------------------------------------------------------
// EN: Case 12 -- `border-width`'s own Box expansion (`ESC-2`), targeting the 4 `-width` longhands
//     `border`/`border-color` already reach via `RecursiveRepeat`/`Box` -- upstream's own
//     `StyleSheetSpecification.cpp:286`: `RegisterShorthand(ShorthandId::BorderWidth,
//     "border-width", "border-top-width, border-right-width, border-bottom-width,
//     border-left-width", ShorthandType::Box)`. Same `Box` algorithm margin/padding already use,
//     just new target names -- 1-value and 4-value pinned (docs/uix-rcss.md section 6.3's own
//     table already covers 2/3-value, via the shared `expand_box`, not re-pinned per shorthand).
// PT: Caso 12 -- a própria expansão Box de `border-width` (`ESC-2`), mirando os 4 longhands
//     `-width` que `border`/`border-color` já alcançam via `RecursiveRepeat`/`Box` -- o próprio
//     `StyleSheetSpecification.cpp:286` do upstream: `RegisterShorthand(ShorthandId::BorderWidth,
//     "border-width", "border-top-width, border-right-width, border-bottom-width,
//     border-left-width", ShorthandType::Box)`. Mesmo algoritmo `Box` que margin/padding já usam,
//     só nomes-alvo novos -- 1-valor e 4-valor pinados (a própria tabela da seção 6.3 do
//     docs/uix-rcss.md já cobre 2/3-valor, via o `expand_box` compartilhado, não re-pinado
//     por-shorthand).
// ---------------------------------------------------------------------------
void test_border_width_box_expansion() {
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("border-width", "2dp", &out) == ShorthandExpandStatus::Ok,
          "border-width 1-value: Ok");
    check_value(out, "border-top-width", "2dp", "border-width 1-value: top");
    check_value(out, "border-right-width", "2dp", "border-width 1-value: right");
    check_value(out, "border-bottom-width", "2dp", "border-width 1-value: bottom");
    check_value(out, "border-left-width", "2dp", "border-width 1-value: left");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("border-width", "1dp 2dp 3dp 4dp", &out) == ShorthandExpandStatus::Ok,
          "border-width 4-value: Ok");
    check_value(out, "border-top-width", "1dp", "border-width 4-value: top = v0");
    check_value(out, "border-right-width", "2dp", "border-width 4-value: right = v1");
    check_value(out, "border-bottom-width", "3dp", "border-width 4-value: bottom = v2");
    check_value(out, "border-left-width", "4dp", "border-width 4-value: left = v3");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 13 -- `inset`'s own Box expansion (`ESC-2`), targeting `top`/`right`/`bottom`/`left`
//     THEMSELVES as the longhand names (not `inset-top` etc -- upstream's own
//     `StyleSheetSpecification.cpp:313`: `RegisterShorthand(ShorthandId::Inset, "inset", "top,
//     right, bottom, left", ShorthandType::Box)`; those 4 names are already registered longhands,
//     property_registry.cpp's own `two("top", ...)`/`two("right", ...)`/`two("bottom", ...)`/
//     `two("left", ...)` rows, docs/uix-rcss.md section 6.1).
// PT: Caso 13 -- a própria expansão Box de `inset` (`ESC-2`), mirando `top`/`right`/`bottom`/
//     `left` ELES MESMOS como os nomes de longhand (não `inset-top` etc -- o próprio
//     `StyleSheetSpecification.cpp:313` do upstream: `RegisterShorthand(ShorthandId::Inset,
//     "inset", "top, right, bottom, left", ShorthandType::Box)`; esses 4 nomes já são longhands
//     registrados, as próprias linhas `two("top", ...)`/`two("right", ...)`/`two("bottom", ...)`/
//     `two("left", ...)` do property_registry.cpp, seção 6.1 do docs/uix-rcss.md).
// ---------------------------------------------------------------------------
void test_inset_box_expansion() {
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("inset", "4dp", &out) == ShorthandExpandStatus::Ok,
          "inset 1-value: Ok");
    check_value(out, "top", "4dp", "inset 1-value: top");
    check_value(out, "right", "4dp", "inset 1-value: right");
    check_value(out, "bottom", "4dp", "inset 1-value: bottom");
    check_value(out, "left", "4dp", "inset 1-value: left");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("inset", "1dp 2dp 3dp 4dp", &out) == ShorthandExpandStatus::Ok,
          "inset 4-value: Ok");
    check_value(out, "top", "1dp", "inset 4-value: top = v0");
    check_value(out, "right", "2dp", "inset 4-value: right = v1");
    check_value(out, "bottom", "3dp", "inset 4-value: bottom = v2");
    check_value(out, "left", "4dp", "inset 4-value: left = v3");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 14 -- `nav`'s own Box expansion (`ESC-2`), SIDE order up/right/down/left -- upstream's
//     own `StyleSheetSpecification.cpp:382`: `RegisterShorthand(ShorthandId::Nav, "nav", "nav-up,
//     nav-right, nav-down, nav-left", ShorthandType::Box)`. Same `Box` algorithm, but "up" stands
//     in for "top" and "down" for "bottom" -- pinned by 4-value (each position gets a DISTINCT
//     value) so a future refactor cannot silently swap `nav-down`/`nav-left` order, the same
//     "order is load-bearing" proof `test_border_radius_box_expansion_corner_order` (case 3 above)
//     already gives for `border-radius`'s own diverging corner order.
// PT: Caso 14 -- a própria expansão Box de `nav` (`ESC-2`), ordem de LADO up/right/down/left -- o
//     próprio `StyleSheetSpecification.cpp:382` do upstream: `RegisterShorthand(ShorthandId::Nav,
//     "nav", "nav-up, nav-right, nav-down, nav-left", ShorthandType::Box)`. Mesmo algoritmo `Box`,
//     mas "up" no lugar de "top" e "down" no lugar de "bottom" -- pinado por 4-valor (cada posição
//     recebe um valor DISTINTO) pra um futuro refactor não trocar a ordem `nav-down`/`nav-left` em
//     silêncio, a mesma prova "ordem é load-bearing" que o próprio
//     `test_border_radius_box_expansion_corner_order` (caso 3 acima) já dá pra própria ordem de
//     canto divergente de `border-radius`.
// ---------------------------------------------------------------------------
void test_nav_box_expansion_up_right_down_left_order() {
  std::vector<LonghandValue> out;
  check(expand_shorthand("nav", "a b c d", &out) == ShorthandExpandStatus::Ok, "nav 4-value: Ok");
  check_value(out, "nav-up", "a", "nav 4-value: up = v0");
  check_value(out, "nav-right", "b", "nav 4-value: right = v1");
  check_value(out, "nav-down", "c", "nav 4-value: down = v2");
  check_value(out, "nav-left", "d", "nav 4-value: left = v3");

  std::vector<LonghandValue> out1;
  check(expand_shorthand("nav", "auto", &out1) == ShorthandExpandStatus::Ok, "nav 1-value: Ok");
  check_value(out1, "nav-up", "auto", "nav 1-value: up");
  check_value(out1, "nav-right", "auto", "nav 1-value: right");
  check_value(out1, "nav-down", "auto", "nav 1-value: down");
  check_value(out1, "nav-left", "auto", "nav 1-value: left");
}

// ---------------------------------------------------------------------------
// EN: Case 15 -- `font`'s own FallThrough expansion (`ESC-2`), 4 items (style, weight, size,
//     family) -- upstream's own `StyleSheetSpecification.cpp:359`: `RegisterShorthand(
//     ShorthandId::Font, "font", "font-style, font-weight, font-size, font-family",
//     ShorthandType::FallThrough)`. Every sub-case below is a distinct routing trace against the
//     real generic loop (`PropertySpecification.cpp:433-471`, cited in shorthand.cpp's own header,
//     "The two routing classifiers"), not a guess -- `font`'s own grammar makes SUB-specification
//     (omitting style and/or weight) the NORMAL authoring shape, not an edge case, which is why
//     this is also the case that exercises `UIX-RCSS-ERRATA-8`'s own fix
//     (`expand_fallthrough`'s never-claimed-item-without-default now OMITS, does not
//     `MalformedValue`) the hardest.
// PT: Caso 15 -- a própria expansão FallThrough de `font` (`ESC-2`), 4 itens (style, weight, size,
//     family) -- o próprio `StyleSheetSpecification.cpp:359` do upstream: `RegisterShorthand(
//     ShorthandId::Font, "font", "font-style, font-weight, font-size, font-family",
//     ShorthandType::FallThrough)`. Todo subcaso abaixo é um rastro de roteamento distinto contra o
//     próprio laço genérico real (`PropertySpecification.cpp:433-471`, citado no próprio cabeçalho
//     do shorthand.cpp, "Os dois classificadores de roteamento"), não um chute -- a própria
//     gramática do `font` faz da sub-especificação (omitir style e/ou weight) a forma NORMAL de
//     autoria, não um caso de borda, que é por isso que este também é o caso que mais exercita o
//     próprio conserto da `UIX-RCSS-ERRATA-8` (o item nunca-reivindicado-sem-default do
//     `expand_fallthrough` agora OMITE, não dá `MalformedValue`).
// ---------------------------------------------------------------------------
void test_font_fallthrough_unquoted_forms() {
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("font", "italic bold 16px X", &out) == ShorthandExpandStatus::Ok,
          "font 4-token, all claimed: Ok");
    check_value(out, "font-style", "italic", "font 4-token: style");
    check_value(out, "font-weight", "bold", "font 4-token: weight");
    check_value(out, "font-size", "16px", "font 4-token: size");
    check_value(out, "font-family", "X", "font 4-token: family");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("font", "bold 16px X", &out) == ShorthandExpandStatus::Ok,
          "font 3-token, style omitted: Ok ('bold' fails the style classifier, claimed by weight)");
    check(out.size() == 3, "font 3-token: exactly 3 longhands (style OMITTED, not defaulted)");
    check_value(out, "font-weight", "bold", "font 3-token: weight");
    check_value(out, "font-size", "16px", "font 3-token: size");
    check_value(out, "font-family", "X", "font 3-token: family");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("font", "16px X", &out) == ShorthandExpandStatus::Ok,
          "font 2-token, style+weight omitted: Ok");
    check(out.size() == 2, "font 2-token: exactly 2 longhands");
    check_value(out, "font-size", "16px", "font 2-token: size");
    check_value(out, "font-family", "X", "font 2-token: family");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("font", "X", &out) == ShorthandExpandStatus::Ok,
          "font 1-token, family alone: Ok (fails style/weight/size, claimed by family's own "
          "catch-all)");
    check(out.size() == 1, "font 1-token: exactly 1 longhand");
    check_value(out, "font-family", "X", "font 1-token: family");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("font", "700 16px X", &out) == ShorthandExpandStatus::Ok,
          "font 3-token, numeric weight: Ok ('700' fails style, claimed by weight's own "
          "looks_like_number_token half, not the normal/bold keyword half)");
    check(out.size() == 3, "font numeric weight: exactly 3 longhands (style omitted)");
    check_value(out, "font-weight", "700", "font numeric weight: weight = the bare number");
    check_value(out, "font-size", "16px", "font numeric weight: size");
    check_value(out, "font-family", "X", "font numeric weight: family");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("font", "normal 16px X", &out) == ShorthandExpandStatus::Ok,
          "font 'normal' ambiguous token: Ok, routes to the FIRST item that accepts it");
    check(out.size() == 3, "font 'normal': exactly 3 longhands (weight omitted, not style)");
    check_value(out, "font-style", "normal",
                "font 'normal': claimed by style (item 0 tried first), never reaches weight even "
                "though 'normal' is ALSO in weight's own keyword set");
    check_value(out, "font-size", "16px", "font 'normal': size");
    check_value(out, "font-family", "X", "font 'normal': family");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("font", "16px Times New Roman", &out) ==
              ShorthandExpandStatus::MalformedValue,
          "font 4-token unquoted multi-word family: MalformedValue -- 'Times' claims family (the "
          "chain's own catch-all), leaving 'New'/'Roman' unclaimed with no item left "
          "(over-specified guard, unrelated to the quote-tokenizer)");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 16 -- `font`'s own quoted `font-family` forms (`ESC-2`), the tokenizer trait this
//     shorthand is the ONLY one of the 20 to need (`split_whitespace`'s own doc-comment,
//     shorthand.cpp). Every sub-case traces a specific upstream `ParsePropertyValues` state
//     transition (`PropertySpecification.cpp:513-682`), cited inline.
// PT: Caso 16 -- as próprias formas de `font-family` entre aspas do `font` (`ESC-2`), o traço de
//     tokenizador que este shorthand é o ÚNICO dos 20 a precisar (o próprio comentário de doc do
//     `split_whitespace`, shorthand.cpp). Todo subcaso rastreia uma transição de estado específica
//     do próprio `ParsePropertyValues` do upstream (`PropertySpecification.cpp:513-682`), citada
//     inline.
// ---------------------------------------------------------------------------
void test_font_fallthrough_quoted_forms() {
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("font", "16px \"Times New Roman\"", &out) ==
              ShorthandExpandStatus::Ok,
          "font double-quoted multi-word family: Ok (the quote makes the internal spaces part of "
          "ONE token, PropertySpecification.cpp:627-653's own VALUE_QUOTE accumulation)");
    check(out.size() == 2, "font double-quoted family: exactly 2 longhands");
    check_value(out, "font-size", "16px", "font double-quoted family: size");
    check_value(out, "font-family", "Times New Roman",
                "font double-quoted family: the quote marks themselves are excluded from the "
                "token, per :637's own SubmitExactValue call");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("font", "16px 'Lato Sans'", &out) == ShorthandExpandStatus::Ok,
          "font single-quoted multi-word family: Ok (upstream's own open_quote_character is "
          "whichever of '\"'/'\\'' opened the run, :553,580)");
    check_value(out, "font-family", "Lato Sans", "font single-quoted family: internal space kept");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("font", "16px \"Lato", &out) == ShorthandExpandStatus::Ok,
          "font unterminated quote: Ok, but the dangling quote produces NO token for it (upstream "
          "silently discards the tail, :672-679's own post-loop 'if (state == VALUE)' never fires "
          "for a dangling VALUE_QUOTE)");
    check(out.size() == 1,
          "font unterminated quote: only 1 longhand (family never claimed, no token for it)");
    check_value(out, "font-size", "16px", "font unterminated quote: size is the only survivor");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("font", "16px \"\"", &out) == ShorthandExpandStatus::Ok,
          "font empty-string family: Ok (an empty quoted run IS its own token, upstream's own "
          "SubmitExactValue at :637 submits unconditionally, unlike the plain-run "
          "empty-after-strip filter)");
    check(out.size() == 2, "font empty-string family: 2 longhands (family IS claimed, empty)");
    check_value(out, "font-family", "",
                "font empty-string family: the empty string itself is the claimed value, "
                "kCatchAll accepts it same as any other token");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("font", "16px \"Lato\\\"Sans\"", &out) == ShorthandExpandStatus::Ok,
          "font escaped-quote family (\"escape\" trait, section 2 of the CTO plan): Ok, the "
          "escaped quote does not end the string early (boundary parity with upstream) -- but see "
          "this module's own NAMED divergence, split_whitespace's own doc-comment: the backslash "
          "byte itself is kept LITERAL, not interpreted the way upstream's own real "
          "ParsePropertyValues would (which would produce the family value Lato\"Sans, backslash "
          "removed)");
    check(out.size() == 2, "font escaped-quote family: 2 longhands");
    check_value(out, "font-family", "Lato\\\"Sans",
                "font escaped-quote family: this module's own asserted (not upstream's byte-exact) "
                "behaviour -- backslash kept literal, quote boundary still correctly recognised as "
                "internal, not a terminator");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 17 -- `perspective-origin`'s own FallThrough expansion (`ESC-2`), 2 items (`-x`, `-y`)
//     -- upstream's own `StyleSheetSpecification.cpp:392`: `RegisterShorthand(
//     ShorthandId::PerspectiveOrigin, "perspective-origin", "perspective-origin-x,
//     perspective-origin-y", ShorthandType::FallThrough)`. Neither item is `kCatchAll` (unlike
//     `font-family`/`border-*-color`) -- both are a real 2-way domain, `keyword(...)` OR
//     `length_percent` (`:390-391`), so an out-of-domain token (`banana`) genuinely fails BOTH and
//     the shorthand correctly aborts.
// PT: Caso 17 -- a própria expansão FallThrough de `perspective-origin` (`ESC-2`), 2 itens (`-x`,
//     `-y`) -- o próprio `StyleSheetSpecification.cpp:392` do upstream: `RegisterShorthand(
//     ShorthandId::PerspectiveOrigin, "perspective-origin", "perspective-origin-x,
//     perspective-origin-y", ShorthandType::FallThrough)`. Nenhum item é `kCatchAll` (diferente de
//     `font-family`/`border-*-color`) -- os dois são um domínio real de 2 vias, `keyword(...)` OU
//     `length_percent` (`:390-391`), então um token fora-do-domínio (`banana`) genuinamente falha
//     OS DOIS e o shorthand aborta corretamente.
// ---------------------------------------------------------------------------
void test_perspective_origin_fallthrough() {
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("perspective-origin", "center", &out) == ShorthandExpandStatus::Ok,
          "perspective-origin 'center': Ok, ambiguous token claimed by -x (tried first)");
    check(out.size() == 1, "perspective-origin 'center': only -x claimed, -y omitted");
    check_value(out, "perspective-origin-x", "center", "perspective-origin 'center': -x");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("perspective-origin", "top", &out) == ShorthandExpandStatus::Ok,
          "perspective-origin 'top': Ok, 'top' fails -x's own domain, claimed by -y");
    check(out.size() == 1, "perspective-origin 'top': only -y claimed, -x omitted");
    check_value(out, "perspective-origin-y", "top", "perspective-origin 'top': -y");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("perspective-origin", "left 25%", &out) == ShorthandExpandStatus::Ok,
          "perspective-origin 2-value: Ok");
    check_value(out, "perspective-origin-x", "left", "perspective-origin 2-value: -x");
    check_value(out, "perspective-origin-y", "25%",
                "perspective-origin 2-value: -y (%, a length-shaped token per "
                "looks_like_length_token)");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("perspective-origin", "banana", &out) ==
              ShorthandExpandStatus::MalformedValue,
          "perspective-origin 'banana': MalformedValue, fails BOTH -x's and -y's own domain "
          "(neither is a catch-all)");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 18 -- `transform-origin`'s own FallThrough expansion (`ESC-2`), 3 items (`-x`, `-y`,
//     `-z`) -- upstream's own `StyleSheetSpecification.cpp:397`: `RegisterShorthand(
//     ShorthandId::TransformOrigin, "transform-origin", "transform-origin-x, transform-origin-y,
//     transform-origin-z", ShorthandType::FallThrough)`. `-z`'s own domain is plain `length`
//     (`:396`, no keyword half) -- the one item among these 20 shorthands' own FallThrough chains
//     that is neither `kCatchAll` nor a 2-way keyword-or-length domain.
// PT: Caso 18 -- a própria expansão FallThrough de `transform-origin` (`ESC-2`), 3 itens (`-x`,
//     `-y`, `-z`) -- o próprio `StyleSheetSpecification.cpp:397` do upstream: `RegisterShorthand(
//     ShorthandId::TransformOrigin, "transform-origin", "transform-origin-x, transform-origin-y,
//     transform-origin-z", ShorthandType::FallThrough)`. O próprio domínio de `-z` é `length` puro
//     (`:396`, sem a metade palavra-chave) -- o único item entre as cadeias FallThrough destes 20
//     shorthands que não é nem `kCatchAll` nem um domínio de 2 vias palavra-chave-ou-comprimento.
// ---------------------------------------------------------------------------
void test_transform_origin_fallthrough() {
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("transform-origin", "top 10px", &out) == ShorthandExpandStatus::Ok,
          "transform-origin 'top 10px': Ok");
    check(out.size() == 2, "transform-origin 'top 10px': -x omitted (2 longhands only)");
    check_value(out, "transform-origin-y", "top",
                "transform-origin 'top 10px': 'top' fails -x's own domain, claimed by -y");
    check_value(out, "transform-origin-z", "10px",
                "transform-origin 'top 10px': '10px' is then tried against -z (item_index moves "
                "past the already-claimed -y), accepted by -z's own length-only domain");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("transform-origin", "left banana", &out) ==
              ShorthandExpandStatus::MalformedValue,
          "transform-origin 'left banana': MalformedValue -- 'left' claims -x, 'banana' fails -y "
          "AND -z (neither's own domain matches), no item left to claim it");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("transform-origin", "left center 5px", &out) ==
              ShorthandExpandStatus::Ok,
          "transform-origin 3-value: Ok, pure positional");
    check_value(out, "transform-origin-x", "left", "transform-origin 3-value: -x");
    check_value(out, "transform-origin-y", "center", "transform-origin 3-value: -y");
    check_value(out, "transform-origin-z", "5px", "transform-origin 3-value: -z");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 19 -- `flex-flow`'s own FallThrough expansion (`ESC-2`), 2 items (`flex-direction`,
//     `flex-wrap`) -- upstream's own `StyleSheetSpecification.cpp:429`: `RegisterShorthand(
//     ShorthandId::FlexFlow, "flex-flow", "flex-direction, flex-wrap",
//     ShorthandType::FallThrough)`. Both items are CLOSED keyword sets, neither `kCatchAll` --
//     `flex-flow: banana` must fail entirely (upstream's own real `ParseValue` for a keyword-only
//     property type rejects any spelling outside its own enumerated set; a `kCatchAll` last item,
//     the pattern `font-family`/`border-*-color` correctly use, would WRONGLY accept `banana`
//     here, since keyword domains -- unlike `string`/`color` -- are not open).
// PT: Caso 19 -- a própria expansão FallThrough de `flex-flow` (`ESC-2`), 2 itens
//     (`flex-direction`, `flex-wrap`) -- o próprio `StyleSheetSpecification.cpp:429` do upstream:
//     `RegisterShorthand(ShorthandId::FlexFlow, "flex-flow", "flex-direction, flex-wrap",
//     ShorthandType::FallThrough)`. Os dois itens são conjuntos de palavra-chave FECHADOS, nenhum
//     `kCatchAll` -- `flex-flow: banana` precisa falhar por completo (o próprio `ParseValue` real
//     do upstream pra um tipo de propriedade só-palavra-chave rejeita qualquer grafia fora do
//     próprio conjunto enumerado; um último item `kCatchAll`, o padrão que `font-family`/
//     `border-*-color` corretamente usam, aceitaria `banana` ERRADO aqui, já que domínios
//     palavra-chave -- diferente de `string`/`color` -- não são abertos).
// ---------------------------------------------------------------------------
void test_flex_flow_fallthrough() {
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("flex-flow", "row wrap", &out) == ShorthandExpandStatus::Ok,
          "flex-flow 2-value: Ok");
    check_value(out, "flex-direction", "row", "flex-flow 2-value: direction");
    check_value(out, "flex-wrap", "wrap", "flex-flow 2-value: wrap");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("flex-flow", "column-reverse", &out) == ShorthandExpandStatus::Ok,
          "flex-flow 1-value, direction only: Ok");
    check(out.size() == 1, "flex-flow direction-only: wrap omitted");
    check_value(out, "flex-direction", "column-reverse", "flex-flow direction-only: direction");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("flex-flow", "wrap", &out) == ShorthandExpandStatus::Ok,
          "flex-flow 1-value, wrap only: Ok ('wrap' fails direction's own 4-keyword set, claimed "
          "by flex-wrap)");
    check(out.size() == 1, "flex-flow wrap-only: direction omitted");
    check_value(out, "flex-wrap", "wrap", "flex-flow wrap-only: wrap");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("flex-flow", "wrap column", &out) ==
              ShorthandExpandStatus::MalformedValue,
          "flex-flow 'wrap column': MalformedValue -- 'wrap' claims flex-wrap (the chain's own "
          "last item), leaving 'column' unclaimed (over-specified guard)");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("flex-flow", "banana", &out) == ShorthandExpandStatus::MalformedValue,
          "flex-flow 'banana': MalformedValue -- fails BOTH closed keyword sets, no kCatchAll to "
          "rescue it (the trait this case's own header comment names)");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 20 -- `UIX-RCSS-ERRATA-8` (`ESC-2`): sub-specified `border-top`/`border-left` are
//     SUCCESS, not `MalformedValue` -- the doctrine `expand_fallthrough` used to enforce (every
//     item MUST be claimed or the whole shorthand fails) does not match upstream's own real loop
//     (`PropertySpecification.cpp:433-471`): a never-visited item simply never gets its own
//     `dictionary.SetProperty()` call, it is not an error condition. Directly exercises the fixed
//     branch (`expand_fallthrough`'s own final loop, the `else` that used to `return false`) --
//     reverting that fix to `return false` MUST turn both sub-cases here red (mutation test,
//     `ESC-2`'s own risk (a)).
// PT: Caso 20 -- `UIX-RCSS-ERRATA-8` (`ESC-2`): `border-top`/`border-left` sub-especificados SÃO
//     sucesso, não `MalformedValue` -- a doutrina que o `expand_fallthrough` costumava impor (todo
//     item TEM de ser reivindicado ou o shorthand inteiro falha) não bate com o próprio laço real
//     do upstream (`PropertySpecification.cpp:433-471`): um item nunca-visitado simplesmente nunca
//     recebe a própria chamada `dictionary.SetProperty()` dele, não é condição de erro. Exercita
//     direto o ramo consertado (o próprio laço final do `expand_fallthrough`, o `else` que
//     costumava dar `return false`) -- reverter este conserto pra `return false` PRECISA deixar os
//     dois subcasos aqui vermelhos (teste de mutação, próprio risco (a) da `ESC-2`).
// ---------------------------------------------------------------------------
void test_border_top_sub_specified_values_are_ok_not_malformed() {
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("border-top", "2px", &out) == ShorthandExpandStatus::Ok,
          "border-top width-only: Ok (not MalformedValue -- ERRATA-8)");
    check(out.size() == 1, "border-top width-only: exactly 1 longhand (color OMITTED)");
    check_value(out, "border-top-width", "2px", "border-top width-only: width");
  }
  {
    std::vector<LonghandValue> out;
    check(expand_shorthand("border-top", "red", &out) == ShorthandExpandStatus::Ok,
          "border-top color-only: Ok (not MalformedValue -- ERRATA-8; 'red' fails width's own "
          "looks_like_length_token, claimed by color's own kCatchAll)");
    check(out.size() == 1, "border-top color-only: exactly 1 longhand (width OMITTED)");
    check_value(out, "border-top-color", "red", "border-top color-only: color");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 21 -- coverage: `all_shorthands()` has exactly 20 entries (13 pre-`ESC-2` + 7 new),
//     every descriptor's own `name` is confirmed by `is_shorthand()` too (pins the two lists,
//     `kShorthands[]` and `all_shorthands()`'s own `kDescriptors[]`, against drifting apart), and
//     -- per this task's own DoD -- every one of the 7 NEW shorthand names is confirmed to NOT be
//     a registered longhand (`find_property(name) == nullptr`), the same invariant
//     property_registry_sanity.cpp's own cross-check already proves for the target NAMES a
//     shorthand expands INTO, here proven for the shorthand's own NAME itself.
// PT: Caso 21 -- cobertura: `all_shorthands()` tem exatamente 20 entradas (13 pré-`ESC-2` + 7
//     novas), o próprio `name` de cada descriptor é confirmado pelo `is_shorthand()` também (pina
//     as duas listas, `kShorthands[]` e o próprio `kDescriptors[]` de `all_shorthands()`, contra
//     desalinhar), e -- per o próprio DoD desta tarefa -- cada um dos 7 nomes de shorthand NOVOS é
//     confirmado como NÃO sendo um longhand registrado (`find_property(name) == nullptr`), o mesmo
//     invariante que o próprio cross-check do property_registry_sanity.cpp já prova pros
//     nomes-alvo que um shorthand expande PRA DENTRO, aqui provado pro próprio NOME do shorthand.
// ---------------------------------------------------------------------------
void test_all_shorthands_count_is_20_with_cross_checks() {
  const auto all = all_shorthands();
  check(all.size() == 20, "all_shorthands: exactly 20 entries (13 + ESC-2's own 7)");
  for (const auto& d : all) {
    check(is_shorthand(d.name), "all_shorthands: every descriptor's own name is_shorthand() too");
  }

  static constexpr std::string_view kNewNames[] = {
      "border-width",
      "inset",
      "nav",
      "font",
      "perspective-origin",
      "transform-origin",
      "flex-flow",
  };
  for (std::string_view name : kNewNames) {
    check(is_shorthand(name), "ESC-2 new shorthand: is_shorthand() true");
    check(find_property(name) == nullptr,
          "ESC-2 new shorthand: find_property() nullptr (a shorthand name is never itself a "
          "registered longhand)");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 22 -- `flex: none <extra tokens>` (sibling finding, `ESC-2`, opted IN): upstream's own
//     `property_values[0] == "none"` special case (`PropertySpecification.cpp:314-317`) checks
//     ONLY the first token, discarding any trailing ones -- `flex: none 2` still expands to the
//     literal `{"0","0","auto"}`, the extra `2` silently dropped, matching upstream exactly. The
//     pre-`ESC-2` code required `tokens.size() == 1`, rejecting this real upstream-accepted form
//     as `MalformedValue`.
// PT: Caso 22 -- `flex: none <tokens extra>` (achado irmão, `ESC-2`, optado por INCLUIR): o
//     próprio caso especial `property_values[0] == "none"` do upstream
//     (`PropertySpecification.cpp:314-317`) checa SÓ o primeiro token, descartando quaisquer
//     seguintes -- `flex: none 2` ainda expande pro `{"0","0","auto"}` literal, o `2` extra
//     descartado em silêncio, casando com o upstream exatamente. O código pré-`ESC-2` exigia
//     `tokens.size() == 1`, rejeitando esta forma real aceita pelo upstream como `MalformedValue`.
// ---------------------------------------------------------------------------
void test_flex_none_ignores_extra_tokens() {
  std::vector<LonghandValue> out;
  check(expand_shorthand("flex", "none 2", &out) == ShorthandExpandStatus::Ok,
        "flex 'none 2': Ok, matches upstream's own values[0]==\"none\" special case");
  check_value(out, "flex-grow", "0", "flex 'none 2': grow = 0");
  check_value(out, "flex-shrink", "0", "flex 'none 2': shrink = 0");
  check_value(out, "flex-basis", "auto", "flex 'none 2': basis = auto");
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

  // ESC-2 -- +7 shorthands, closing 13 -> 20.
  test_border_width_box_expansion();
  test_inset_box_expansion();
  test_nav_box_expansion_up_right_down_left_order();
  test_font_fallthrough_unquoted_forms();
  test_font_fallthrough_quoted_forms();
  test_perspective_origin_fallthrough();
  test_transform_origin_fallthrough();
  test_flex_flow_fallthrough();
  test_border_top_sub_specified_values_are_ok_not_malformed();
  test_all_shorthands_count_is_20_with_cross_checks();
  test_flex_none_ignores_extra_tokens();

  if (g_failures > 0) {
    std::fprintf(stderr, "shorthand_expansion_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("shorthand_expansion_sanity: PASS");
  return 0;
}
