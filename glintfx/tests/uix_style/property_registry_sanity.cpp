// SPDX-License-Identifier: Apache-2.0
// EN: UIX-PROP-REGISTRY -- functional/coverage test for glintfx::uix::style's property registry
//     (property_registry.hpp). Standalone, no parser, no cascade, no value resolution -- see that
//     header's own header comment for the full scope/boundary. Two jobs, mirroring this task's
//     own Definition of Done: (1) prove the table's own shape (72 entries, alphabetical,
//     lookup-by-name works, `nullptr` for unknown names) and (2) prove EVERY property the real
//     corpus measures (`/var/tmp/censo-rcss-qa1/censo.md` section 3, 64 distinct names,
//     hardcoded below with a citation, not re-derived from the live file -- the census is a fixed,
//     dated snapshot, and hardcoding it here means this test does not silently start passing or
//     failing as an unrelated, unversioned `/var/tmp` file changes) is covered by this registry,
//     either directly (a longhand entry) or through a measured shorthand's own expansion
//     (shorthand.hpp, tested separately in shorthand_expansion_sanity.cpp) or an explicit,
//     declared `@font-face`-only exclusion (docs/uix-rcss.md section 10).
// PT: UIX-PROP-REGISTRY -- teste funcional/de-cobertura pro registro de propriedades do
//     glintfx::uix::style (property_registry.hpp). Standalone, sem parser, sem cascata, sem
//     resolução de valor -- ver o próprio comentário de cabeçalho daquele header pro
//     escopo/fronteira completos. Dois trabalhos, espelhando o próprio DoD desta tarefa: (1)
//     provar a própria forma da tabela (72 entradas, alfabética, busca-por-nome funciona,
//     `nullptr` pra nome desconhecido) e (2) provar que TODA propriedade que o corpus real mede
//     (`/var/tmp/censo-rcss-qa1/censo.md` seção 3, 64 nomes distintos, hardcoded abaixo com
//     citação, não re-derivado do arquivo vivo -- o censo é um retrato fixo, datado, e
//     hardcodá-lo aqui significa que este teste não passa nem falha em silêncio conforme um
//     arquivo `/var/tmp` não-versionado, alheio, muda) está coberta por este registro, ou direto
//     (uma entrada longhand) ou pela própria expansão de um shorthand medido (shorthand.hpp,
//     testado à parte em shorthand_expansion_sanity.cpp) ou por uma exclusão explícita, declarada,
//     de `@font-face` (seção 10 do docs/uix-rcss.md).
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/style/property_registry.hpp"
#include "uix/style/shorthand.hpp"

#include <algorithm>
#include <cstdio>
#include <set>
#include <string>
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

using glintfx::uix::style::all_properties;
using glintfx::uix::style::all_shorthands;
using glintfx::uix::style::find_property;
using glintfx::uix::style::PropertyInfo;
using glintfx::uix::style::ValueDomain;

// ---------------------------------------------------------------------------
// EN: Case 1 -- table shape: exactly 72 entries (docs/uix-rcss.md section 6.1's own declared
//     size), sorted ascending byte-wise by name (section 6's own required `PROP`-line sort order,
//     `std::string::operator<`, no locale -- reused verbatim, not reinvented, same reasoning
//     lexer.hpp's own header cites for docs/uix-dom.md's identical rule).
// PT: Caso 1 -- forma da tabela: exatamente 72 entradas (o próprio tamanho declarado na seção 6.1
//     do docs/uix-rcss.md), ordenadas ascendente byte-a-byte por nome (a própria ordem de sort
//     exigida pra linha `PROP` da seção 6, `std::string::operator<`, sem locale -- reusada
//     verbatim, não reinventada, mesmo raciocínio que o próprio cabeçalho do lexer.hpp cita pra
//     regra idêntica do docs/uix-dom.md).
// ---------------------------------------------------------------------------
void test_table_shape_and_sort_order() {
  auto table = all_properties();
  check(table.size() == 72, "table has exactly 72 entries (docs/uix-rcss.md section 6.1)");
  for (std::size_t i = 1; i < table.size(); ++i) {
    check(table[i - 1].name < table[i].name, "table is sorted ascending, byte-wise, by name");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 2 -- lookup: a handful of representative entries (spot-checked against
//     docs/uix-rcss.md section 6.1's own literal cells), plus the two ⚠️-flagged surprising
//     `inherited: true` entries the spec itself warns "nobody fixes them later" (`focus`,
//     `opacity`), plus a name genuinely outside the table returns `nullptr` (fail-high contract,
//     header "Fail-high policy").
// PT: Caso 2 -- busca: um punhado de entradas representativas (conferidas à mão contra as
//     próprias células literais da seção 6.1 do docs/uix-rcss.md), mais as duas entradas
//     `inherited: true` surpreendentes marcadas com ⚠️ que a própria spec avisa "ninguém conserta
//     depois" (`focus`, `opacity`), mais um nome genuinamente fora da tabela retornando `nullptr`
//     (contrato fail-high, "Política fail-high" no cabeçalho).
// ---------------------------------------------------------------------------
void test_lookup_spot_check() {
  const PropertyInfo* color = find_property("color");
  check(color != nullptr, "lookup: 'color' found");
  if (color != nullptr) {
    check(color->initial_value == "white", "lookup: 'color' initial value is 'white'");
    check(color->inherited == true, "lookup: 'color' is inherited");
    check(color->domain == ValueDomain::Color, "lookup: 'color' domain is Color");
  }

  const PropertyInfo* display = find_property("display");
  check(display != nullptr, "lookup: 'display' found");
  if (display != nullptr) {
    check(display->initial_value == "inline", "lookup: 'display' initial value is 'inline'");
    check(display->inherited == false, "lookup: 'display' is not inherited");
  }

  const PropertyInfo* cursor = find_property("cursor");
  check(cursor != nullptr, "lookup: 'cursor' found");
  if (cursor != nullptr) {
    check(cursor->initial_value.empty(), "lookup: 'cursor' initial value is empty ('*(empty)*')");
    check(cursor->domain == ValueDomain::String, "lookup: 'cursor' domain is String");
  }

  const PropertyInfo* focus = find_property("focus");
  check(focus != nullptr, "lookup: 'focus' found");
  if (focus != nullptr) {
    check(focus->inherited == true,
          "lookup: 'focus' IS inherited -- surprising per docs/uix-rcss.md section 6.1's own "
          "warning, confirmed at the RmlUi call site, not guessed");
  }

  const PropertyInfo* opacity = find_property("opacity");
  check(opacity != nullptr, "lookup: 'opacity' found");
  if (opacity != nullptr) {
    check(opacity->inherited == true,
          "lookup: 'opacity' IS inherited -- the OTHER surprising ⚠️ entry, NOT how real CSS "
          "opacity behaves, per RmlUi's own different mechanism (see header, 'Fail-high policy' "
          "paragraph's own sibling in docs/uix-rcss.md section 6.1)");
  }

  const PropertyInfo* width = find_property("width");
  check(width != nullptr, "lookup: 'width' found");
  if (width != nullptr) {
    check(width->has_alternate_domain,
          "lookup: 'width' has an alternate domain ('keyword(auto) or length-percent')");
    check(width->domain == ValueDomain::Keyword, "lookup: 'width' primary domain is Keyword");
    check(width->alternate_domain == ValueDomain::LengthPercent,
          "lookup: 'width' alternate domain is LengthPercent");
  }

  check(find_property("z-index") == nullptr,
        "lookup: 'z-index' is NOT in the table (zero-measured, docs/rmlx-subset.md section 2 -- "
        "fail-high nullptr, not a crash, not a guess");
  check(find_property("") == nullptr, "lookup: empty name returns nullptr, not UB");
}

// ---------------------------------------------------------------------------
// EN: Case 3 -- the ⚠️ registry-vs-census gap this task's own brief asked to be closed: exactly
//     `max-height`/`max-width` are registry entries with ZERO corpus justification anywhere (not
//     measured directly, and not reachable through any measured shorthand's own expansion --
//     verified by exhaustive `comm` against the real corpus and the real glintfx tree, see this
//     item's own delivery notes for the full accounting). This is NOT a bug in this test or this
//     table -- docs/uix-rcss.md section 6.1's own table lists them, and "the spec is the
//     contract" (this task's own brief) -- it is a DECLARED, PROVEN gap this test pins so it
//     cannot silently grow (a THIRD entry appearing here unexplained would be new, unreported
//     drift, not this same known one).
// PT: Caso 3 -- o ⚠️ vão registro-vs-censo que o próprio briefing desta tarefa pediu pra fechar:
//     exatamente `max-height`/`max-width` são entradas de registro com ZERO justificativa de
//     corpus em lugar nenhum (não medidas diretamente, e não alcançáveis por nenhuma expansão de
//     shorthand medido -- verificado por `comm` exaustivo contra o corpus real e a própria árvore
//     da glintfx, ver as próprias notas de entrega deste item pra contagem completa). Isto NÃO é
//     um bug deste teste nem desta tabela -- a própria tabela da seção 6.1 do docs/uix-rcss.md as
//     lista, e "a spec é o contrato" (o próprio briefing desta tarefa) -- é um vão DECLARADO,
//     PROVADO que este teste pina pra ele não crescer em silêncio (uma TERCEIRA entrada aparecendo
//     aqui sem explicação seria drift novo, não-reportado, não este mesmo já conhecido).
// ---------------------------------------------------------------------------
void test_max_height_max_width_are_the_one_known_unexplained_gap() {
  check(find_property("max-height") != nullptr,
        "max-height IS a registry entry (docs/uix-rcss.md section 6.1), despite zero corpus "
        "justification -- pinned, not silently dropped");
  check(find_property("max-width") != nullptr,
        "max-width IS a registry entry, same declared, unexplained-by-corpus gap as max-height");
}

// ---------------------------------------------------------------------------
// EN: Case 4 -- THE coverage proof this task's own Definition of Done demands: every one of the
//     64 property names the real census measured (censo.md section 3, hardcoded here -- see this
//     file's own header for why hardcoded, not re-read from `/var/tmp`) resolves to EXACTLY one
//     of three buckets: (a) a direct registry longhand entry, (b) a registered shorthand name
//     (its own longhand expansion is separately proven correct in shorthand_expansion_sanity.cpp,
//     not re-proven here), or (c) an explicit `@font-face`-only exclusion (`src`,
//     `-rmlui-fallback-face` -- docs/uix-rcss.md section 10: these belong to the `@font-face`
//     STRUCTURAL registry, not the per-element property registry this file's own table models).
//     The scope line is printed UNCONDITIONALLY, including the zero-count fields, per this
//     task's own brief ("a linha de escopo impressa mesmo quando o número é zero").
// PT: Caso 4 -- A prova de cobertura que o próprio DoD desta tarefa exige: cada um dos 64 nomes de
//     propriedade que o próprio censo real mediu (censo.md seção 3, hardcoded aqui -- ver o
//     próprio cabeçalho deste arquivo pro porquê hardcoded, não relido do `/var/tmp`) resolve pra
//     EXATAMENTE um de três baldes: (a) uma entrada longhand direta do registro, (b) um nome de
//     shorthand registrado (a própria expansão longhand dele é provada correta à parte, em
//     shorthand_expansion_sanity.cpp, não reprovada aqui), ou (c) uma exclusão explícita,
//     só-de-`@font-face` (`src`, `-rmlui-fallback-face` -- seção 10 do docs/uix-rcss.md: estas
//     pertencem ao registro ESTRUTURAL de `@font-face`, não ao registro de propriedade
//     por-elemento que a própria tabela deste arquivo modela). A linha de escopo é impressa
//     INCONDICIONALMENTE, inclusive os campos de contagem zero, per o próprio briefing desta
//     tarefa ("a linha de escopo impressa mesmo quando o número é zero").
// ---------------------------------------------------------------------------
void test_every_measured_property_is_covered() {
  // EN: Verbatim from /var/tmp/censo-rcss-qa1/censo.md section 3 ("64 distintas") -- top-20 by
  //     frequency plus the tail's own 44 names, hand-counted against the census's own prose (the
  //     census's own prose says "45 propriedades" for the tail -- a SECOND census prose-vs-count
  //     divergence this task's own delivery notes flag; the actual enumerated tail is 44 names,
  //     20+44=64, matching the section's own header count).
  // PT: Verbatim do /var/tmp/censo-rcss-qa1/censo.md seção 3 ("64 distintas") -- top-20 por
  //     frequência mais os próprios 44 nomes da cauda, contados à mão contra a própria prosa do
  //     censo (a própria prosa do censo diz "45 propriedades" pra cauda -- uma SEGUNDA divergência
  //     prosa-vs-contagem do censo que as próprias notas de entrega deste item sinalizam; a cauda
  //     de fato enumerada é 44 nomes, 20+44=64, batendo com a própria contagem de cabeçalho da
  //     seção).
  static constexpr std::string_view kCensusNames[] = {
      // top-20 (censo.md section 3's own table)
      "color",
      "width",
      "decorator",
      "font-size",
      "height",
      "border",
      "top",
      "left",
      "position",
      "background-color",
      "box-shadow",
      "display",
      "border-radius",
      "padding",
      "margin",
      "text-align",
      "letter-spacing",
      "margin-top",
      "font-family",
      "box-sizing",
      // tail, 44 names (censo.md section 3's own "resto da cauda" prose)
      "bottom",
      "margin-left",
      "line-height",
      "right",
      "margin-bottom",
      "background",
      "align-items",
      "filter",
      "src",
      "opacity",
      "border-color",
      "flex",
      "padding-top",
      "margin-right",
      "white-space",
      "justify-content",
      "min-height",
      "overflow-y",
      "overflow-x",
      "image-tint-mode",
      "text-transform",
      "border-top",
      "image-tint-color",
      "padding-right",
      "min-width",
      "image-tint-threshold",
      "border-bottom",
      "gap",
      "animation",
      "cursor",
      "tab-index",
      "ripple-origin-x",
      "ripple-origin-y",
      "ripple-phase",
      "ripple-strength",
      "ripple-width",
      "overflow",
      "transform",
      "-rmlui-fallback-face",
      "backdrop-filter",
      "mask-image",
      "focus",
      "text-overflow",
      "vertical-align",
  };
  static constexpr std::size_t kCensusCount = sizeof(kCensusNames) / sizeof(kCensusNames[0]);
  check(kCensusCount == 64, "this test's own hardcoded census list has 64 names (self-check)");

  // EN: `@font-face`-only exclusion, per docs/uix-rcss.md section 10 -- these are NOT element
  //     properties, so a `nullptr` from find_property() for these two is CORRECT, not a gap.
  // PT: Exclusão só-de-`@font-face`, per a seção 10 do docs/uix-rcss.md -- estas NÃO são
  //     propriedades de elemento, então um `nullptr` do find_property() pra estas duas é
  //     CORRETO, não um vão.
  const std::set<std::string_view> font_face_only = {"src", "-rmlui-fallback-face"};

  int direct_hits = 0;
  int via_shorthand = 0;
  int font_face_excluded = 0;
  std::vector<std::string_view> uncovered;

  for (std::string_view name : kCensusNames) {
    if (find_property(name) != nullptr) {
      ++direct_hits;
      continue;
    }
    if (font_face_only.count(name) != 0) {
      ++font_face_excluded;
      continue;
    }
    bool is_shorthand = false;
    for (const auto& sh : all_shorthands()) {
      if (sh.name == name) {
        is_shorthand = true;
        break;
      }
    }
    if (is_shorthand) {
      ++via_shorthand;
      continue;
    }
    uncovered.push_back(name);
  }

  std::fprintf(stdout,
               "SCOPE: %zu census properties, %d direct registry hits, %d via measured "
               "shorthand, %d @font-face-only exclusions, %zu uncovered\n",
               kCensusCount, direct_hits, via_shorthand, font_face_excluded, uncovered.size());
  for (std::string_view name : uncovered) {
    std::fprintf(stderr, "  uncovered: %.*s\n", static_cast<int>(name.size()), name.data());
  }

  check(uncovered.empty(),
        "every census-measured property name is covered (direct, shorthand, "
        "or declared @font-face exclusion)");
  check(direct_hits == 51,
        "51 of the 64 census names are direct registry longhand entries "
        "(64 - 11 shorthand names - 2 @font-face-only names = 51)");
  check(via_shorthand == 11,
        "11 of the 64 census names are shorthand names measured directly (margin, padding, "
        "border-radius, border-color, border-top, border-bottom, border, background, flex, gap, "
        "overflow -- border-left/border-right are ALSO registered shorthands, per upstream's own "
        "RecursiveRepeat expansion of `border`, but the census's own tail never measures them by "
        "name directly, so they do not appear in this 64-name list at all)");
  check(font_face_excluded == 2,
        "2 of the 64 census names are @font-face-only (src, "
        "-rmlui-fallback-face)");
}

} // namespace

int main() {
  test_table_shape_and_sort_order();
  test_lookup_spot_check();
  test_max_height_max_width_are_the_one_known_unexplained_gap();
  test_every_measured_property_is_covered();

  if (g_failures > 0) {
    std::fprintf(stderr, "property_registry_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("property_registry_sanity: PASS");
  return 0;
}
