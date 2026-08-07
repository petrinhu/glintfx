// SPDX-License-Identifier: Apache-2.0
// EN: UIX-PROP-REGISTRY -- functional/coverage test for glintfx::uix::style's property registry
//     (property_registry.hpp). Standalone, no parser, no cascade, no value resolution -- see that
//     header's own header comment for the full scope/boundary. Two jobs, mirroring this task's
//     own Definition of Done: (1) prove the table's own shape (107 entries, alphabetical,
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
//     provar a própria forma da tabela (107 entradas, alfabética, busca-por-nome funciona,
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
// EN: Case 1 -- table shape: exactly 107 entries (docs/uix-rcss.md section 6.1's own declared
//     size, raised from 72 by `ESC-1`'s own +35 rows), sorted ascending byte-wise by name (section
//     6's own required `PROP`-line sort order, `std::string::operator<`, no locale -- reused
//     verbatim, not reinvented, same reasoning lexer.hpp's own header cites for
//     docs/uix-dom.md's identical rule).
// PT: Caso 1 -- forma da tabela: exatamente 107 entradas (o próprio tamanho declarado na seção 6.1
//     do docs/uix-rcss.md, elevado de 72 pelas próprias +35 linhas da `ESC-1`), ordenadas
//     ascendente byte-a-byte por nome (a própria ordem de sort exigida pra linha `PROP` da seção 6,
//     `std::string::operator<`, sem locale -- reusada verbatim, não reinventada, mesmo raciocínio
//     que o próprio cabeçalho do lexer.hpp cita pra regra idêntica do docs/uix-dom.md).
// ---------------------------------------------------------------------------
void test_table_shape_and_sort_order() {
  auto table = all_properties();
  check(table.size() == 107, "table has exactly 107 entries (docs/uix-rcss.md section 6.1)");
  for (std::size_t i = 1; i < table.size(); ++i) {
    check(table[i - 1].name < table[i].name, "table is sorted ascending, byte-wise, by name");
  }
}

// ---------------------------------------------------------------------------
// EN: Case 2 -- lookup: a handful of representative entries (spot-checked against
//     docs/uix-rcss.md section 6.1's own literal cells), plus the THREE ⚠️-flagged surprising
//     `inherited: true` entries the spec itself warns "nobody fixes them later" (`focus`,
//     `opacity`, and `ESC-1`'s own `text-decoration`), plus `ESC-1`'s own seven representative new
//     rows (one per shape this slice introduced: sole-Keyword at the very top of the sort order,
//     `(Keyword,Color)`, `(Keyword,Number)`, `(Keyword,String)`, plain `Length` with a unitless
//     initial, and `Composite` with an empty initial), plus a name genuinely outside the table
//     (not just unmeasured -- a real CSS property the pinned RmlUi build itself does not register,
//     proving this registry is RmlUi-parity, not CSS-the-whole-language) returns `nullptr`
//     (fail-high contract, header "Fail-high policy").
// PT: Caso 2 -- busca: um punhado de entradas representativas (conferidas à mão contra as
//     próprias células literais da seção 6.1 do docs/uix-rcss.md), mais as TRÊS entradas
//     `inherited: true` surpreendentes marcadas com ⚠️ que a própria spec avisa "ninguém conserta
//     depois" (`focus`, `opacity`, e o próprio `text-decoration` da `ESC-1`), mais as próprias sete
//     linhas novas representativas da `ESC-1` (uma por forma que esta fatia introduziu:
//     Keyword-único no topo da própria ordem de sort, `(Keyword,Color)`, `(Keyword,Number)`,
//     `(Keyword,String)`, `Length` puro com inicial sem unidade, e `Composite` com inicial vazio),
//     mais um nome genuinamente fora da tabela (não só não-medido -- uma propriedade CSS real que o
//     próprio build fixado do RmlUi não registra, provando que este registro é paridade-RmlUi, não
//     CSS-a-linguagem-inteira) retornando `nullptr` (contrato fail-high, "Política fail-high" no
//     cabeçalho).
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

  // EN: `ESC-1`'s own THIRD surprising ⚠️ entry (see this function's own header) -- real CSS
  //     declares `text-decoration` `Inherited: no`; RmlUi registers `inherited: true`, confirmed
  //     directly at `StyleSheetSpecification.cpp:362`, not guessed.
  // PT: a própria TERCEIRA entrada ⚠️ surpreendente da `ESC-1` (ver o próprio cabeçalho desta
  //     função) -- o CSS real declara `text-decoration` `Inherited: no`; o RmlUi registra
  //     `inherited: true`, confirmado direto em `StyleSheetSpecification.cpp:362`, não chutado.
  const PropertyInfo* text_decoration = find_property("text-decoration");
  check(text_decoration != nullptr, "lookup: 'text-decoration' found");
  if (text_decoration != nullptr) {
    check(text_decoration->inherited == true,
          "lookup: 'text-decoration' IS inherited -- the THIRD surprising ⚠️ entry (ESC-1), "
          "diverging from real CSS's own 'Inherited: no', confirmed at the RmlUi call site");
    check(text_decoration->domain == ValueDomain::Keyword,
          "lookup: 'text-decoration' domain is Keyword (no alternate domain)");
    check(text_decoration->initial_value == "none",
          "lookup: 'text-decoration' initial value is 'none'");
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

  // EN: `ESC-1` spot-checks -- one representative row per NEW shape this slice introduced (see
  //     this function's own header for which shape each one stands in for), verified against
  //     `StyleSheetSpecification.cpp:248-436`'s own call sites, not guessed.
  // PT: spot-checks da `ESC-1` -- uma linha representativa por forma NOVA que esta fatia
  //     introduziu (ver o próprio cabeçalho desta função pra qual forma cada uma representa),
  //     verificadas contra os próprios call sites de `StyleSheetSpecification.cpp:248-436`, não
  //     chutadas.
  const PropertyInfo* caret_color = find_property("caret-color");
  check(caret_color != nullptr, "lookup: 'caret-color' found (ESC-1)");
  if (caret_color != nullptr) {
    check(caret_color->inherited == true,
          "lookup: 'caret-color' IS inherited -- agrees with real CSS's own inherited "
          "'caret-color', NOT a divergence (no ⚠️ for this one, unlike text-decoration above)");
    check(caret_color->has_alternate_domain,
          "lookup: 'caret-color' has an alternate domain ('keyword(auto) or color')");
    check(caret_color->domain == ValueDomain::Keyword,
          "lookup: 'caret-color' primary domain is Keyword");
    check(caret_color->alternate_domain == ValueDomain::Color,
          "lookup: 'caret-color' alternate domain is Color");
    check(caret_color->initial_value == "auto", "lookup: 'caret-color' initial value is 'auto'");
  }

  const PropertyInfo* font_weight = find_property("font-weight");
  check(font_weight != nullptr, "lookup: 'font-weight' found (ESC-1)");
  if (font_weight != nullptr) {
    check(font_weight->inherited == true, "lookup: 'font-weight' is inherited");
    check(font_weight->has_alternate_domain,
          "lookup: 'font-weight' has an alternate domain ('keyword(normal,bold) or number')");
    check(font_weight->domain == ValueDomain::Keyword,
          "lookup: 'font-weight' primary domain is Keyword");
    check(font_weight->alternate_domain == ValueDomain::Number,
          "lookup: 'font-weight' alternate domain is Number (the normal=400/bold=700 mapping is "
          "parser-side, owner ESC-16 -- this registry only stores the domain TAG)");
    check(font_weight->initial_value == "normal",
          "lookup: 'font-weight' initial value is 'normal'");
  }

  const PropertyInfo* nav_up = find_property("nav-up");
  check(nav_up != nullptr, "lookup: 'nav-up' found (ESC-1)");
  if (nav_up != nullptr) {
    check(nav_up->inherited == false, "lookup: 'nav-up' is not inherited");
    check(nav_up->has_alternate_domain,
          "lookup: 'nav-up' has an alternate domain ('keyword(...) or string')");
    check(nav_up->domain == ValueDomain::Keyword, "lookup: 'nav-up' primary domain is Keyword");
    check(nav_up->alternate_domain == ValueDomain::String,
          "lookup: 'nav-up' alternate domain is String");
    check(nav_up->initial_value == "none", "lookup: 'nav-up' initial value is 'none'");
  }

  const PropertyInfo* rmlui_direction = find_property("-rmlui-direction");
  check(rmlui_direction != nullptr, "lookup: '-rmlui-direction' found (ESC-1)");
  if (rmlui_direction != nullptr) {
    check(rmlui_direction->inherited == true, "lookup: '-rmlui-direction' is inherited");
    check(rmlui_direction->domain == ValueDomain::Keyword,
          "lookup: '-rmlui-direction' domain is Keyword (no alternate domain)");
    check(rmlui_direction->initial_value == "auto",
          "lookup: '-rmlui-direction' initial value is 'auto'");
  }
  // EN: `-rmlui-direction` sorts byte-wise BEFORE `-rmlui-language` (`'d' < 'l'`), and both sort
  //     before every plain-letter name (`'-'` is `0x2D`, less than `'a'` `0x61`) -- so
  //     `-rmlui-direction` is index 0 of `all_properties()`, the very top of the sort order this
  //     table's own contract requires (section 6's own byte-wise rule, Case 1 above).
  // PT: `-rmlui-direction` ordena byte-a-byte ANTES de `-rmlui-language` (`'d' < 'l'`), e as duas
  //     ordenam antes de todo nome de letra pura (`'-'` é `0x2D`, menor que `'a'` `0x61`) -- então
  //     `-rmlui-direction` é o índice 0 do `all_properties()`, o próprio topo da ordem de sort que
  //     o próprio contrato desta tabela exige (a própria regra byte-wise da seção 6, Caso 1 acima).
  check(!all_properties().empty() && all_properties()[0].name == "-rmlui-direction",
        "lookup: '-rmlui-direction' is the very first entry in sort order (top of the table)");

  const PropertyInfo* scrollbar_margin = find_property("scrollbar-margin");
  check(scrollbar_margin != nullptr, "lookup: 'scrollbar-margin' found (ESC-1)");
  if (scrollbar_margin != nullptr) {
    check(scrollbar_margin->inherited == false, "lookup: 'scrollbar-margin' is not inherited");
    check(scrollbar_margin->domain == ValueDomain::Length,
          "lookup: 'scrollbar-margin' domain is Length (no alternate domain)");
    check(scrollbar_margin->initial_value == "0",
          "lookup: 'scrollbar-margin' initial value is the literal '0', UNITLESS -- transcribed "
          "verbatim from StyleSheetSpecification.cpp:384, never widened to '0px'");
  }

  const PropertyInfo* font_effect = find_property("font-effect");
  check(font_effect != nullptr, "lookup: 'font-effect' found (ESC-1)");
  if (font_effect != nullptr) {
    check(font_effect->inherited == true, "lookup: 'font-effect' is inherited");
    check(font_effect->domain == ValueDomain::Composite,
          "lookup: 'font-effect' domain is Composite (no alternate domain)");
    check(font_effect->initial_value.empty(),
          "lookup: 'font-effect' initial value is empty ('*(empty)*')");
  }

  // EN: `z-index` is `ESC-1`'s own INVERSION of what this same lookup used to prove: before this
  //     slice it was the fail-high exemplar (genuinely absent); now it is a positive hit like
  //     every other entry above. `grid-template-rows` takes over as the fail-high exemplar below --
  //     a REAL CSS property this table does NOT register, proving fail-high still works and that
  //     this registry's own closed set is RmlUi-parity, not "every CSS property that exists".
  // PT: `z-index` é a própria INVERSÃO da `ESC-1` do que esta mesma busca costumava provar: antes
  //     desta fatia era o exemplar fail-high (genuinamente ausente); agora é um acerto positivo
  //     como toda outra entrada acima. `grid-template-rows` assume como o exemplar fail-high
  //     abaixo -- uma propriedade CSS REAL que esta tabela NÃO registra, provando que o fail-high
  //     ainda funciona e que o próprio conjunto fechado deste registro é paridade-RmlUi, não "toda
  //     propriedade CSS que existe".
  const PropertyInfo* z_index = find_property("z-index");
  check(z_index != nullptr, "lookup: 'z-index' found (ESC-1 -- was the fail-high exemplar before)");
  if (z_index != nullptr) {
    check(z_index->inherited == false, "lookup: 'z-index' is not inherited");
    check(z_index->has_alternate_domain,
          "lookup: 'z-index' has an alternate domain ('keyword(auto) or number')");
    check(z_index->domain == ValueDomain::Keyword, "lookup: 'z-index' primary domain is Keyword");
    check(z_index->alternate_domain == ValueDomain::Number,
          "lookup: 'z-index' alternate domain is Number");
    check(z_index->initial_value == "auto", "lookup: 'z-index' initial value is 'auto'");
  }

  check(find_property("grid-template-rows") == nullptr,
        "lookup: 'grid-template-rows' is NOT in the table -- a real CSS property the pinned "
        "RmlUi build itself does not register (not merely zero-measured), fail-high nullptr, not "
        "a crash, not a guess");
  check(find_property("") == nullptr, "lookup: empty name returns nullptr, not UB");
}

// ---------------------------------------------------------------------------
// EN: Case 3 -- `ESC-1` RECLASSIFICATION (function name kept unchanged on purpose: this exact
//     identifier is cited verbatim as pinned evidence by docs/uix-rcss.md section 6.1's own
//     `max-height`/`max-width` paragraph -- renaming it would break that citation for zero
//     behavioural gain). `max-height`/`max-width` are STILL registry entries with ZERO corpus
//     justification anywhere (not measured directly, and not reachable through any measured
//     shorthand's own expansion -- verified by exhaustive `comm` against the real corpus and the
//     real glintfx tree, see this item's own delivery notes for the full accounting) -- that fact
//     does not change. What changes is the FRAMING: before `ESC-1`, this was "the one registry
//     entry with zero corpus justification", a standalone anomaly; after `ESC-1`, `ESC-1`'s own 35
//     new rows are ALL zero-corpus BY DESIGN (`docs/adr/0022-paridade-total-com-o-motor-substituido.md`,
//     `docs/rmlx-subset.md` §7's "if the engine being replaced accepts it, ours accepts it"), so
//     `max-height`/`max-width` are no longer the one exception -- they are the PRECEDENT the
//     general rule generalizes from. This is NOT a bug in this test or this table -- docs/uix-rcss.md
//     section 6.1's own table lists them, and "the spec is the contract" (this task's own brief) --
//     it is a DECLARED, PROVEN gap this test still pins so it cannot silently grow (a zero-corpus
//     entry OUTSIDE both the census AND `ESC-1`'s own parity-authorized 35 would be new, unreported
//     drift, not this same known precedent).
// PT: Caso 3 -- RECLASSIFICAÇÃO da `ESC-1` (nome da função mantido de propósito: este identificador
//     exato é citado verbatim como evidência fixada pelo próprio parágrafo `max-height`/`max-width`
//     da seção 6.1 do docs/uix-rcss.md -- renomear quebraria aquela citação por zero ganho
//     comportamental). `max-height`/`max-width` AINDA são entradas de registro com ZERO
//     justificativa de corpus em lugar nenhum (não medidas diretamente, e não alcançáveis por
//     nenhuma expansão de shorthand medido -- verificado por `comm` exaustivo contra o corpus real
//     e a própria árvore da glintfx, ver as próprias notas de entrega deste item pra contagem
//     completa) -- esse fato não muda. O que muda é o ENQUADRAMENTO: antes da `ESC-1`, isto era "a
//     única entrada de registro com zero justificativa de corpus", uma anomalia isolada; depois da
//     `ESC-1`, as próprias 35 linhas novas dela são TODAS zero-corpus POR DESENHO
//     (`docs/adr/0022-paridade-total-com-o-motor-substituido.md`, §7 do `docs/rmlx-subset.md`, "se
//     o motor que está sendo substituído aceita, o nosso aceita"), então `max-height`/`max-width`
//     deixam de ser a única exceção -- viram o PRECEDENTE de que a regra geral generaliza. Isto NÃO
//     é um bug deste teste nem desta tabela -- a própria tabela da seção 6.1 do docs/uix-rcss.md as
//     lista, e "a spec é o contrato" (o próprio briefing desta tarefa) -- é um vão DECLARADO,
//     PROVADO que este teste ainda pina pra ele não crescer em silêncio (uma entrada zero-corpus
//     FORA tanto do censo QUANTO das próprias 35 autorizadas-por-paridade da `ESC-1` seria drift
//     novo, não-reportado, não este mesmo precedente já conhecido).
// ---------------------------------------------------------------------------
void test_max_height_max_width_are_the_one_known_unexplained_gap() {
  check(find_property("max-height") != nullptr,
        "max-height IS a registry entry (docs/uix-rcss.md section 6.1), despite zero corpus "
        "justification -- pinned, not silently dropped; now ESC-1's own precedent, not a standalone "
        "anomaly");
  check(find_property("max-width") != nullptr,
        "max-width IS a registry entry, same declared, unexplained-by-corpus gap as max-height -- "
        "same reclassification");
}

// ---------------------------------------------------------------------------
// EN: `ESC-1` NEW CASE -- enumerate the closed 35-name space itself, don't just search inside it
//     (this repo's own "enumere o espaço pequeno, não busque dentro dele" house rule, cited by
//     dumper.cpp's own header for a structurally identical reason). `table.size() == 107` (Case 1)
//     plus the sort-order assertion plus the handful of Case 2 spot-checks do NOT, on their own,
//     prove these 107 rows are these EXACT 107 names -- they would pass identically if `ESC-1` had
//     accidentally added 35 DIFFERENT rows totalling 107, sorted, with the 7 spot-checked names
//     unchanged. This case closes that hole directly: every one of the 35 names this task's own
//     plan enumerated (independently re-derived against
//     `glintfx/build/_deps/rmlui-src/Source/Core/StyleSheetSpecification.cpp:248-436`, the pinned
//     build, not the `examples/RmlUi` study clone -- see this item's own delivery notes) is present,
//     and the count of that hardcoded list is self-checked at exactly 35 so a future edit to this
//     very array cannot silently drift from the number this task's own contract fixes.
// PT: NOVO CASO da `ESC-1` -- enumera o próprio espaço fechado de 35 nomes, não só busca dentro
//     dele (a própria regra da casa deste repo "enumere o espaço pequeno, não busque dentro dele",
//     citada pelo próprio cabeçalho do dumper.cpp por um motivo estruturalmente idêntico).
//     `table.size() == 107` (Caso 1) mais a asserção de ordem de sort mais o punhado de
//     spot-checks do Caso 2 NÃO provam, sozinhos, que estas 107 linhas são estes 107 nomes EXATOS
//     -- passariam idênticos se a `ESC-1` tivesse acidentalmente somado 35 linhas DIFERENTES
//     totalizando 107, ordenadas, com os 7 nomes spot-checados inalterados. Este caso fecha esse
//     buraco direto: cada um dos 35 nomes que o próprio plano desta tarefa enumerou (re-derivados
//     de forma independente contra
//     `glintfx/build/_deps/rmlui-src/Source/Core/StyleSheetSpecification.cpp:248-436`, o build
//     fixado, não o clone de estudo `examples/RmlUi` -- ver as próprias notas de entrega deste
//     item) está presente, e a contagem desta lista hardcoded se autochecka em exatamente 35 pra
//     uma futura edição deste mesmo array não conseguir divergir em silêncio do número que o
//     próprio contrato desta tarefa fixa.
// ---------------------------------------------------------------------------
void test_esc1_thirty_five_new_properties_are_exactly_these_names() {
  static constexpr std::string_view kEsc1NewNames[] = {
      "-rmlui-direction",
      "-rmlui-language",
      "align-content",
      "align-self",
      "caret-color",
      "clear",
      "clip",
      "drag",
      "fill-image",
      "flex-direction",
      "flex-wrap",
      "float",
      "font-effect",
      "font-kerning",
      "font-style",
      "font-weight",
      "image-color",
      "nav-down",
      "nav-left",
      "nav-right",
      "nav-up",
      "overscroll-behavior",
      "perspective",
      "perspective-origin-x",
      "perspective-origin-y",
      "pointer-events",
      "scrollbar-margin",
      "text-decoration",
      "transform-origin-x",
      "transform-origin-y",
      "transform-origin-z",
      "transition",
      "visibility",
      "word-break",
      "z-index",
  };
  static constexpr std::size_t kEsc1NewCount = sizeof(kEsc1NewNames) / sizeof(kEsc1NewNames[0]);
  check(kEsc1NewCount == 35, "this test's own hardcoded ESC-1 new-name list has 35 names (self-check)");

  int found = 0;
  std::vector<std::string_view> missing;
  for (std::string_view name : kEsc1NewNames) {
    if (find_property(name) != nullptr) {
      ++found;
    } else {
      missing.push_back(name);
    }
  }

  std::fprintf(stdout, "SCOPE: %zu ESC-1 new names enumerated, %d found in the registry, %zu missing\n",
               kEsc1NewCount, found, missing.size());
  for (std::string_view name : missing) {
    std::fprintf(stderr, "  missing: %.*s\n", static_cast<int>(name.size()), name.data());
  }

  check(missing.empty(), "every one of ESC-1's own 35 new names resolves via find_property()");
  check(found == 35, "all 35 ESC-1 new names found (enumeration, not a directed search)");
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
  test_esc1_thirty_five_new_properties_are_exactly_these_names();
  test_every_measured_property_is_covered();

  if (g_failures > 0) {
    std::fprintf(stderr, "property_registry_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("property_registry_sanity: PASS");
  return 0;
}
