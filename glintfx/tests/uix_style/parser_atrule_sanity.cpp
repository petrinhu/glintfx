// SPDX-License-Identifier: Apache-2.0
// EN: UIX-SHEET-PARSER -- declaration expansion (longhand pass-through, shorthand expansion,
//     unknown-property drop) + at-rule structuring (`@font-face` raw capture, `@keyframes`
//     structural records) + the three recovery scopes this task's own brief names as an acceptance
//     criterion ("uma regra malformada não pode derrubar a folha"), each proven by its OWN case,
//     never conflated -- see parser.hpp's own header comment, "Recovery is an acceptance
//     criterion", for exactly which scope each case below belongs to. Zero RmlUi/GLFW/GL.
// PT: UIX-SHEET-PARSER -- expansão de declaração (passagem direta de longhand, expansão de
//     shorthand, descarte de propriedade desconhecida) + estruturação de at-rule (captura crua de
//     `@font-face`, registros estruturais de `@keyframes`) + os três escopos de recuperação que o
//     próprio briefing desta tarefa nomeia como critério de aceite ("uma regra malformada não pode
//     derrubar a folha"), cada um provado pelo PRÓPRIO caso, nunca confundidos -- ver o próprio
//     comentário de cabeçalho do parser.hpp, "Recuperação é critério de aceite", pra exatamente a
//     qual escopo cada caso abaixo pertence. Zero RmlUi/GLFW/GL.
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/style/parser.hpp"

#include <cstdio>
#include <string>

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

using glintfx::uix::style::FontFaceRule;
using glintfx::uix::style::KeyframesRule;
using glintfx::uix::style::parse_inline_style;
using glintfx::uix::style::parse_stylesheet;
using glintfx::uix::style::PropertyDeclaration;
using glintfx::uix::style::Rule;

bool has_declaration(const std::vector<PropertyDeclaration>& decls, std::string_view name,
                     std::string_view value) {
  for (const auto& d : decls) {
    if (d.name == name && d.value == value) {
      return true;
    }
  }
  return false;
}

bool has_declaration_name(const std::vector<PropertyDeclaration>& decls, std::string_view name) {
  for (const auto& d : decls) {
    if (d.name == name) {
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------------------------------
// EN: Longhand declaration -- registered directly in property_registry.hpp, kept as-is.
// PT: Declaração longhand -- registrada direto no property_registry.hpp, mantida como está.
// ---------------------------------------------------------------------------
void test_longhand_passthrough() {
  auto result = parse_stylesheet(".x { color: white; opacity: 0.5; }");
  check(result.sheet && result.sheet->rules.size() == 1, "longhand_passthrough: one rule");
  if (!result.sheet || result.sheet->rules.empty()) return;
  const auto& decls = result.sheet->rules[0].declarations;
  check(decls.size() == 2, "longhand_passthrough: exactly 2 declarations (no shorthand involved)");
  check(has_declaration(decls, "color", "white"), "longhand_passthrough: color == white");
  check(has_declaration(decls, "opacity", "0.5"), "longhand_passthrough: opacity == 0.5");
}

// ---------------------------------------------------------------------------
// EN: Shorthand declaration -- `margin: 1dp 2dp;` (Box algorithm, 2-value CSS box-model expansion)
//     expands to its 4 longhand constituents, NEVER its own `"margin"` entry.
// PT: Declaração shorthand -- `margin: 1dp 2dp;` (algoritmo Box, expansão CSS de box-model
//     2-valores) expande pros 4 constituintes longhand dela, NUNCA a própria entrada `"margin"`.
// ---------------------------------------------------------------------------
void test_shorthand_expansion() {
  auto result = parse_stylesheet(".x { margin: 1dp 2dp; }");
  check(result.sheet && result.sheet->rules.size() == 1, "shorthand_expansion: one rule");
  if (!result.sheet || result.sheet->rules.empty()) return;
  const auto& decls = result.sheet->rules[0].declarations;
  check(decls.size() == 4, "shorthand_expansion: 2-value margin expands to 4 longhands");
  check(has_declaration(decls, "margin-top", "1dp"), "shorthand_expansion: margin-top == 1dp");
  check(has_declaration(decls, "margin-right", "2dp"), "shorthand_expansion: margin-right == 2dp");
  check(has_declaration(decls, "margin-bottom", "1dp"), "shorthand_expansion: margin-bottom == 1dp");
  check(has_declaration(decls, "margin-left", "2dp"), "shorthand_expansion: margin-left == 2dp");
  check(!has_declaration_name(decls, "margin"),
        "shorthand_expansion: no literal 'margin' entry survives");
}

// ---------------------------------------------------------------------------
// EN: RECOVERY, scope (a) DECLARATION -- an unknown property name is dropped, every OTHER
//     declaration in the SAME rule block still applies (docs/uix-rcss.md section 11).
// PT: RECUPERAÇÃO, escopo (a) DECLARAÇÃO -- um nome de propriedade desconhecido é descartado, toda
//     OUTRA declaração no MESMO bloco de regra ainda se aplica (seção 11 do docs/uix-rcss.md).
// ---------------------------------------------------------------------------
void test_recovery_unknown_property_dropped() {
  auto result = parse_stylesheet(".x { colour-typo: red; color: white; }");
  check(!result.error.has_value(), "recovery_unknown_property: no fatal error");
  check(result.sheet && result.sheet->rules.size() == 1, "recovery_unknown_property: rule still registers");
  if (!result.sheet || result.sheet->rules.empty()) return;
  const auto& decls = result.sheet->rules[0].declarations;
  check(decls.size() == 1, "recovery_unknown_property: only the known declaration survives");
  check(has_declaration(decls, "color", "white"), "recovery_unknown_property: 'color' survives");
  check(!has_declaration_name(decls, "colour-typo"), "recovery_unknown_property: typo dropped");
  check(!result.diagnostics.empty(), "recovery_unknown_property: a ParseDiagnostic was recorded");
}

// ---------------------------------------------------------------------------
// EN: RECOVERY, scope (b) RULE -- a rule whose ONLY selector (not a comma-list -- see
//     UIX-RCSS-ERRATA-2's own per-entry recovery for that case, `parser_selector_sanity.cpp`'s own
//     `test_comma_list_invalid_entry_drops_only_itself`) is unsupported/malformed fails the WHOLE
//     rule (zero surviving entries, nothing left to register), but the SHEET keeps parsing: a
//     well-formed rule right after it still registers.
// PT: RECUPERAÇÃO, escopo (b) REGRA -- uma regra cujo ÚNICO seletor (não uma lista-vírgula -- ver a
//     própria recuperação por-entrada da UIX-RCSS-ERRATA-2 pra aquele caso, o próprio
//     `test_comma_list_invalid_entry_drops_only_itself` do `parser_selector_sanity.cpp`) é
//     não-suportado/malformado reprova a regra INTEIRA (zero entradas sobreviventes, nada sobra pra
//     registrar), mas a FOLHA continua parseando: uma regra bem-formada logo depois ainda registra.
// ---------------------------------------------------------------------------
void test_recovery_malformed_rule_does_not_kill_sheet() {
  auto result = parse_stylesheet("* { color: red; } .b { color: blue; } .c { color: green; }");
  check(!result.error.has_value(), "recovery_malformed_rule: no fatal error");
  check(result.sheet != nullptr, "recovery_malformed_rule: sheet is non-null");
  if (!result.sheet) return;
  check(result.sheet->rules.size() == 2,
        "recovery_malformed_rule: the two well-formed rules after the malformed one both registered");
  check(!result.diagnostics.empty(), "recovery_malformed_rule: a ParseDiagnostic was recorded");
}

// ---------------------------------------------------------------------------
// EN: RECOVERY, scope (c) AT-RULE -- an unrecognised at-rule name (`@media`, zero-measured,
//     docs/rmlx-subset.md section 13) has its whole `{...}` body discarded, logged, and the sheet
//     keeps parsing right after it.
// PT: RECUPERAÇÃO, escopo (c) AT-RULE -- um nome de at-rule não-reconhecido (`@media`,
//     zero-medido, seção 13 do docs/rmlx-subset.md) tem o corpo `{...}` inteiro descartado, logado,
//     e a folha continua parseando logo depois.
// ---------------------------------------------------------------------------
void test_recovery_unknown_at_rule_skipped() {
  auto result =
      parse_stylesheet("@media screen { .inner { color: red; } } .ok { color: blue; }");
  check(!result.error.has_value(), "recovery_unknown_at_rule: no fatal error");
  check(result.sheet != nullptr, "recovery_unknown_at_rule: sheet is non-null");
  if (!result.sheet) return;
  check(result.sheet->rules.size() == 1,
        "recovery_unknown_at_rule: only '.ok' registered, '@media' body fully skipped");
  check(result.sheet->font_faces.empty() && result.sheet->keyframes.empty(),
        "recovery_unknown_at_rule: '@media' produced no font-face/keyframes record");
  check(!result.diagnostics.empty(), "recovery_unknown_at_rule: a ParseDiagnostic was recorded");
}

// ---------------------------------------------------------------------------
// EN: `@font-face` -- a structural record, RAW attributes, NOT run through property_registry.hpp
//     (see parser.hpp header's own 🔴 exception paragraph): `src` and `-rmlui-fallback-face`, not
//     in that table at all, must survive verbatim rather than being dropped as "unknown property".
// PT: `@font-face` -- um registro estrutural, atributos CRUS, NÃO rodados pelo property_registry.hpp
//     (ver o próprio parágrafo de exceção 🔴 do cabeçalho do parser.hpp): `src` e
//     `-rmlui-fallback-face`, fora daquela tabela de jeito nenhum, precisam sobreviver verbatim em
//     vez de descartados como "propriedade desconhecida".
// ---------------------------------------------------------------------------
void test_font_face_structural_record() {
  auto result = parse_stylesheet(
      "@font-face { font-family: \"Inter\"; src: url(inter.ttf); -rmlui-fallback-face: 1; }");
  check(!result.error.has_value(), "font_face: no fatal error");
  check(result.sheet && result.sheet->font_faces.size() == 1, "font_face: one FontFaceRule");
  check(result.sheet && result.sheet->rules.empty(), "font_face: zero ordinary Rules");
  if (!result.sheet || result.sheet->font_faces.empty()) return;
  const FontFaceRule& ff = result.sheet->font_faces[0];
  check(has_declaration(ff.attributes, "font-family", "\"Inter\""),
        "font_face: font-family survives raw, quotes intact");
  check(has_declaration_name(ff.attributes, "src"), "font_face: 'src' survives (not in the §6.1 table)");
  check(has_declaration_name(ff.attributes, "-rmlui-fallback-face"),
        "font_face: '-rmlui-fallback-face' survives");
}

// ---------------------------------------------------------------------------
// EN: `@font-face` written with NO literal space (0x20) between the at-rule word and its own '{'
//     (a newline instead, `"@font-face\n{"`) -- a real corpus shape, found via this item's own
//     corpus test (`glintfx/demos/showcase/showcase.rcss:10`). This parser's own at-rule-name
//     classification must NOT require the same literal-space-only split the lexer uses to decide
//     `@keyframes`'s own MODE (that split point is load-bearing for keeping this parser's
//     traversal synced to the token STREAM SHAPE the lexer already committed to, per parser.hpp
//     header's own "At-rule structuring" paragraph) -- but `@font-face` has no such mode-dependency
//     (every non-"keyframes" at-rule gets the SAME flat Declaration-mode body regardless of its own
//     word), so this parser is free to (and must) tolerate trailing whitespace OTHER than a literal
//     space in the extracted word.
// PT: `@font-face` escrito SEM espaço literal (0x20) nenhum entre a palavra do at-rule e o próprio
//     '{' dele (uma quebra de linha no lugar, `"@font-face\n{"`) -- uma forma real de corpus,
//     achada pelo próprio teste de corpus deste item (`glintfx/demos/showcase/showcase.rcss:10`).
//     A própria classificação de nome-de-at-rule deste parser NÃO pode exigir a mesma divisão
//     só-por-espaço-literal que o lexer usa pra decidir o próprio MODO de `@keyframes` (aquele
//     ponto de divisão é portante pra manter a própria travessia deste parser sincronizada com a
//     FORMA DO FLUXO DE TOKEN que o lexer já se comprometeu, per o próprio parágrafo "Estruturação
//     de at-rule" do cabeçalho do parser.hpp) -- mas `@font-face` não tem essa dependência de modo
//     nenhuma (todo at-rule que não "keyframes" recebe o MESMO corpo em modo Declaration plano
//     independente da própria palavra dele), então este parser é livre pra (e precisa) tolerar
//     whitespace final QUE NÃO espaço literal na palavra extraída.
// ---------------------------------------------------------------------------
void test_font_face_newline_before_brace() {
  auto result = parse_stylesheet("@font-face\n{ font-family: \"Inter\"; }");
  check(!result.error.has_value(), "font_face_newline: no fatal error");
  check(result.sheet && result.sheet->font_faces.size() == 1,
        "font_face_newline: one FontFaceRule (not misclassified as 'unknown at-rule')");
  check(result.diagnostics.empty(),
        "font_face_newline: zero diagnostics -- a real @font-face must not be dropped");
}

// ---------------------------------------------------------------------------
// EN: A whitespace-only `Prelude` token BETWEEN two structural constructs (here: the blank line(s)
//     between a closed rule's own '}' and a following comment, a real corpus shape --
//     `glintfx/tests/app_process_event_scene.rcss:27-29`) must NOT be treated as an attempted,
//     malformed rule -- it carries no author content at all, it is pure inter-token whitespace the
//     lexer's own byte-verbatim `Prelude` grammar (lexer.hpp header, "Token model") happens to
//     surface as its own token. This parser must recognise "stripped-empty Prelude" as a NO-OP
//     (silently skip, zero diagnostic), the same way this repo's own DOM sibling filters
//     whitespace-only `Text` as a TREE INVARIANT rather than surfacing it as content.
// PT: Um token `Prelude` só-whitespace ENTRE duas construções estruturais (aqui: a(s) linha(s) em
//     branco entre o próprio '}' de uma regra fechada e um comentário seguinte, uma forma real de
//     corpus -- `glintfx/tests/app_process_event_scene.rcss:27-29`) NÃO pode ser tratado como uma
//     regra malformada, tentada -- não carrega conteúdo de autor nenhum, é whitespace puro
//     entre-token que a própria gramática byte-verbatim de `Prelude` do lexer (cabeçalho do
//     lexer.hpp, "Modelo de token") por acaso expõe como o próprio token. Este parser precisa
//     reconhecer "Prelude vazio-após-strip" como um NO-OP (pula em silêncio, zero diagnóstico), do
//     mesmo jeito que o próprio irmão DOM deste repo filtra `Text` só-whitespace como INVARIANTE DE
//     ÁRVORE em vez de expor como conteúdo.
// ---------------------------------------------------------------------------
void test_whitespace_only_prelude_between_rules_is_not_diagnosed() {
  auto result = parse_stylesheet(".a { color: red; }\n\n/* a comment */\n\n.b { color: blue; }");
  check(!result.error.has_value(), "whitespace_prelude: no fatal error");
  check(result.sheet && result.sheet->rules.size() == 2,
        "whitespace_prelude: both real rules registered");
  check(result.diagnostics.empty(),
        "whitespace_prelude: zero diagnostics -- inter-token whitespace is not a malformed rule");
}

// ---------------------------------------------------------------------------
// EN: `@keyframes` -- a structural record: the animation name, and an ordered list of
//     `KeyframeBlock`s, each with its own UNRESOLVED selector_text ("0%"/"50%"/"100%") and its own
//     declarations run through the SAME registry/shorthand expansion an ordinary rule's would (a
//     shorthand inside a keyframe body still expands). Mirrors the real corpus line
//     `gusworld_battle_cockpit.rml:214`, `@keyframes spin { from {...} to {...} }`
//     (docs/rmlx-subset.md's own load-bearing lexer citation).
// PT: `@keyframes` -- um registro estrutural: o nome da animação, e uma lista ordenada de
//     `KeyframeBlock`s, cada um com o PRÓPRIO selector_text NÃO-RESOLVIDO ("0%"/"50%"/"100%") e as
//     PRÓPRIAS declarações rodadas pela MESMA expansão de registro/shorthand que as de uma regra
//     comum teriam (um shorthand dentro de um corpo de keyframe ainda expande). Espelha a linha
//     real de corpus `gusworld_battle_cockpit.rml:214`,
//     `@keyframes spin { from {...} to {...} }` (a própria citação de lexer portante do
//     docs/rmlx-subset.md).
// ---------------------------------------------------------------------------
void test_keyframes_structural_record() {
  auto result = parse_stylesheet(
      "@keyframes spin { from { transform: rotate(0deg); } to { transform: rotate(360deg); } }");
  check(!result.error.has_value(), "keyframes: no fatal error");
  check(result.sheet && result.sheet->keyframes.size() == 1, "keyframes: one KeyframesRule");
  if (!result.sheet || result.sheet->keyframes.empty()) return;
  const KeyframesRule& kr = result.sheet->keyframes[0];
  check(kr.name == "spin", "keyframes: name == 'spin'");
  check(kr.blocks.size() == 2, "keyframes: two blocks (from/to)");
  if (kr.blocks.size() == 2) {
    check(kr.blocks[0].selector_text == "from", "keyframes: block 0 selector_text == 'from'");
    check(has_declaration(kr.blocks[0].declarations, "transform", "rotate(0deg)"),
          "keyframes: block 0 declaration transform == rotate(0deg)");
    check(kr.blocks[1].selector_text == "to", "keyframes: block 1 selector_text == 'to'");
    check(has_declaration(kr.blocks[1].declarations, "transform", "rotate(360deg)"),
          "keyframes: block 1 declaration transform == rotate(360deg)");
  }
  check(result.sheet->rules.empty(), "keyframes: zero ordinary Rules produced");
}

// ---------------------------------------------------------------------------
// EN: `@keyframes` with `0%`/`50%`/`100%` percentage selectors (not just from/to) -- the same
//     structural handling, no percentage math attempted.
// PT: `@keyframes` com seletores de porcentagem `0%`/`50%`/`100%` (não só from/to) -- o mesmo
//     tratamento estrutural, nenhuma matemática de porcentagem tentada.
// ---------------------------------------------------------------------------
void test_keyframes_percentage_selectors() {
  auto result = parse_stylesheet(
      "@keyframes pulse { 0% { opacity: 0; } 50% { opacity: 1; } "
      "100% { opacity: 0; } }");
  check(result.sheet && result.sheet->keyframes.size() == 1, "keyframes_pct: one KeyframesRule");
  if (!result.sheet || result.sheet->keyframes.empty()) return;
  const KeyframesRule& kr = result.sheet->keyframes[0];
  check(kr.name == "pulse", "keyframes_pct: name == 'pulse'");
  check(kr.blocks.size() == 3, "keyframes_pct: three blocks");
  if (kr.blocks.size() == 3) {
    check(kr.blocks[0].selector_text == "0%", "keyframes_pct: block 0 == '0%'");
    check(kr.blocks[1].selector_text == "50%", "keyframes_pct: block 1 == '50%'");
    check(kr.blocks[2].selector_text == "100%", "keyframes_pct: block 2 == '100%'");
  }
}

// ---------------------------------------------------------------------------
// EN: FATAL error -- a genuine lexer-level sticky Error (an unterminated comment here) propagates
//     as `ParseError`, `sheet` is null. Distinct from every recovery case above -- see parser.hpp
//     header's own "Recovery is an acceptance criterion" closing paragraph.
// PT: Erro FATAL -- um `Error` pegajoso genuíno de nível-lexer (um comentário não-terminado aqui)
//     se propaga como `ParseError`, `sheet` é nulo. Distinto de todo caso de recuperação acima --
//     ver o próprio parágrafo de fechamento "Recuperação é critério de aceite" do cabeçalho do
//     parser.hpp.
// ---------------------------------------------------------------------------
void test_fatal_lexer_error_propagates() {
  auto result = parse_stylesheet(".x { color: red; } /* unterminated");
  check(result.sheet == nullptr, "fatal_lexer_error: sheet is null");
  check(result.error.has_value(), "fatal_lexer_error: error is set");
  if (result.error) {
    check(!result.error->message.empty(), "fatal_lexer_error: message is non-empty");
  }
}

// ---------------------------------------------------------------------------
// EN: `UIX-INLINE-STYLE` -- `parse_inline_style` is the `style="..."` attribute's OWN entry point,
//     never a synthetic-selector wrap of `parse_stylesheet` -- it starts the `Lexer` directly in
//     Declaration mode (`docs/uix-rcss.md` `UIX-RCSS-ERRATA-6`, mirroring real upstream's own
//     `StyleSheetParser::ParseProperties`, a SEPARATE entry point from `Parse()`). Basic form: the
//     `npc_dialogue__no_com_3_escolhas.rml` fixture's own real inline attribute
//     (`style="decorator: image( retrato.png cover );"`), the exact one measured by this item's own
//     QA pass to reach the registry through unchanged (longhand pass-through, same
//     `apply_declaration` this file's own `test_longhand_passthrough` above already exercises for
//     rule bodies).
// PT: `UIX-INLINE-STYLE` -- o `parse_inline_style` é o PRÓPRIO ponto de entrada do atributo
//     `style="..."`, nunca um envelope de seletor sintético do `parse_stylesheet` -- começa o
//     `Lexer` direto em modo Declaration (`docs/uix-rcss.md` `UIX-RCSS-ERRATA-6`, espelhando o
//     próprio `StyleSheetParser::ParseProperties` do upstream real, um ponto de entrada SEPARADO do
//     `Parse()`). Forma básica: o próprio atributo inline real da fixture
//     `npc_dialogue__no_com_3_escolhas.rml` (`style="decorator: image( retrato.png cover );"`), o
//     exato medido pela própria passada de QA deste item pra chegar no registro sem mudança
//     (passagem direta de longhand, o MESMO `apply_declaration` que o próprio
//     `test_longhand_passthrough` acima deste arquivo já exercita pra corpo de regra).
// ---------------------------------------------------------------------------
void test_inline_style_basic_declarations() {
  auto result = parse_inline_style("decorator: image( retrato.png cover ); color: white;");
  check(!result.error.has_value(), "inline_style_basic: no fatal error");
  check(result.diagnostics.empty(), "inline_style_basic: no diagnostics for well-formed input");
  check(result.declarations.size() == 2, "inline_style_basic: exactly 2 declarations");
  check(has_declaration_name(result.declarations, "decorator"),
        "inline_style_basic: 'decorator' reached the registry");
  check(has_declaration(result.declarations, "color", "white"),
        "inline_style_basic: 'color' == 'white', longhand pass-through unchanged");
}

// ---------------------------------------------------------------------------
// EN: BOUNDARY -- `style=""` (attribute present, value empty). Legal, a no-op: zero declarations,
//     zero diagnostics, no fatal error -- the exact same "nothing to parse" outcome an empty rule
//     body (`.x {}`) already has via `parse_stylesheet`, never a malformed-input diagnostic.
// PT: FRONTEIRA -- `style=""` (atributo presente, valor vazio). Legal, um no-op: zero declarações,
//     zero diagnósticos, nenhum erro fatal -- o EXATO MESMO resultado "nada pra parsear" que um
//     corpo de regra vazio (`.x {}`) já tem via `parse_stylesheet`, nunca um diagnóstico de input
//     malformado.
// ---------------------------------------------------------------------------
void test_inline_style_empty_attribute_is_a_clean_noop() {
  auto result = parse_inline_style("");
  check(!result.error.has_value(), "inline_style_empty: no fatal error");
  check(result.diagnostics.empty(), "inline_style_empty: no diagnostics");
  check(result.declarations.empty(), "inline_style_empty: zero declarations");
}

// ---------------------------------------------------------------------------
// EN: BOUNDARY -- an attribute made ENTIRELY of whitespace (`style="   \t\n "`). Same clean no-op
//     as the empty-string case above -- `Lexer::scan_declaration`'s own EOF-permissiveness already
//     treats a whitespace-only dangling NAME run as "nothing attempted", never a partial
//     identifier, so this needs no special case in `parse_inline_style` itself to stay clean.
// PT: FRONTEIRA -- um atributo feito INTEIRAMENTE de whitespace (`style="   \t\n "`). O MESMO no-op
//     limpo do caso string-vazia acima -- o próprio `Lexer::scan_declaration`'s EOF-permissiveness
//     já trata um trecho de NOME pendurado só-whitespace como "nada tentado", nunca um
//     identificador parcial, então isto não precisa de caso especial nenhum no próprio
//     `parse_inline_style` pra ficar limpo.
// ---------------------------------------------------------------------------
void test_inline_style_whitespace_only_attribute_is_a_clean_noop() {
  auto result = parse_inline_style("   \t\n ");
  check(!result.error.has_value(), "inline_style_whitespace_only: no fatal error");
  check(result.diagnostics.empty(), "inline_style_whitespace_only: no diagnostics");
  check(result.declarations.empty(), "inline_style_whitespace_only: zero declarations");
}

// ---------------------------------------------------------------------------
// EN: BOUNDARY -- a NAME with no ':' ever found before a ';' (`style="garbage;"`). Silently
//     dropped, zero diagnostics -- this is NOT a new rule `parse_inline_style` invents, it is the
//     PRE-EXISTING `scan_declaration()` NAME-state `;` handling (lexer.cpp, "Found name with no
//     value" upstream-equivalent silent discard) this function inherits unchanged by reusing the
//     SAME lexer in Declaration mode -- proven identical to the rule-body case via the SAME input
//     wrapped in a `{}` body through `parse_stylesheet`, both producing zero surviving
//     declarations and zero diagnostics, so a `style="..."` attribute never gets a STRICTER (or
//     LOOSER) reading of this one malformed shape than an ordinary rule body already has.
// PT: FRONTEIRA -- um NOME sem ':' nunca achado antes de um ';' (`style="garbage;"`). Descartado em
//     silêncio, zero diagnósticos -- isto NÃO é uma regra nova que o `parse_inline_style` inventa, é
//     o próprio tratamento PRÉ-EXISTENTE de ';' do estado NAME do `scan_declaration()`
//     (lexer.cpp, descarte silencioso equivalente ao "Found name with no value" do upstream) que
//     esta função herda sem mudança por reusar o MESMO lexer em modo Declaration -- provado
//     idêntico ao caso de corpo-de-regra via o MESMO input envelopado num corpo `{}` pelo
//     `parse_stylesheet`, os dois produzindo zero declarações sobreviventes e zero diagnósticos,
//     então um atributo `style="..."` nunca recebe uma leitura MAIS ESTRITA (nem MAIS FROUXA) desta
//     uma forma malformada do que um corpo de regra comum já tem.
// ---------------------------------------------------------------------------
void test_inline_style_name_with_no_colon_is_silently_dropped_same_as_rule_body() {
  auto inline_result = parse_inline_style("garbage;");
  check(!inline_result.error.has_value(), "inline_style_no_colon: no fatal error");
  check(inline_result.diagnostics.empty(),
        "inline_style_no_colon: no diagnostics -- pre-existing lexer-level silent discard");
  check(inline_result.declarations.empty(), "inline_style_no_colon: zero declarations");

  auto rule_result = parse_stylesheet(".x { garbage; }");
  check(rule_result.sheet && rule_result.sheet->rules.size() == 1,
        "inline_style_no_colon: rule-body parity setup -- the wrapping rule still registers");
  if (rule_result.sheet && !rule_result.sheet->rules.empty()) {
    check(rule_result.sheet->rules[0].declarations.empty(),
          "inline_style_no_colon: rule-body twin ALSO drops it silently -- same lexer, same "
          "outcome, proving parity rather than a new inline-only rule");
  }
  check(rule_result.diagnostics.empty(),
        "inline_style_no_colon: rule-body twin ALSO produces zero diagnostics");
}

// ---------------------------------------------------------------------------
// EN: BOUNDARY -- an extra trailing `;` after a well-formed declaration (`style="color: red;;"`).
//     The empty declaration between the two `;` is silently skipped -- one declaration survives,
//     zero diagnostics, same pre-existing `scan_declaration()` behaviour a stray `;;` already has
//     inside an ordinary rule body.
// PT: FRONTEIRA -- um ';' extra sobrando depois de uma declaração bem-formada
//     (`style="color: red;;"`). A declaração vazia entre os dois ';' é pulada em silêncio -- uma
//     declaração sobrevive, zero diagnósticos, mesmo comportamento pré-existente do
//     `scan_declaration()` que um `;;` solto já tem dentro de um corpo de regra comum.
// ---------------------------------------------------------------------------
void test_inline_style_extra_trailing_semicolon_is_silently_dropped() {
  auto result = parse_inline_style("color: red;;");
  check(!result.error.has_value(), "inline_style_extra_semicolon: no fatal error");
  check(result.diagnostics.empty(), "inline_style_extra_semicolon: no diagnostics");
  check(result.declarations.size() == 1,
        "inline_style_extra_semicolon: exactly one declaration survives");
  check(has_declaration(result.declarations, "color", "red"),
        "inline_style_extra_semicolon: 'color' == 'red' unaffected by the stray trailing ';'");
}

// ---------------------------------------------------------------------------
// EN: BOUNDARY + THE SILENCE-DIES CASE -- a valid declaration followed by garbage (an unknown
//     property name): `style="color: red; not-a-real-property: 5;"`. `color` survives; the
//     unknown-property declaration is dropped AND emits a `ParseDiagnostic` naming the raw
//     offending text, the SAME `apply_declaration` path (and the SAME message shape) an unknown
//     property inside an ordinary `{}` rule body already produces
//     (`test_recovery_unknown_property_dropped` above, same file) -- proving the silence this
//     item's own brief names ("o silêncio morre") is closed: a malformed inline declaration is no
//     longer invisible, it is diagnosed exactly like its `<style>` sibling.
// PT: FRONTEIRA + O CASO QUE MATA O SILÊNCIO -- uma declaração válida seguida de lixo (um nome de
//     propriedade desconhecido): `style="color: red; not-a-real-property: 5;"`. `color` sobrevive;
//     a declaração de propriedade desconhecida é descartada E emite um `ParseDiagnostic` nomeando
//     o texto cru ofensor, o MESMO caminho `apply_declaration` (e a MESMA forma de mensagem) que uma
//     propriedade desconhecida dentro de um corpo de regra `{}` comum já produz
//     (`test_recovery_unknown_property_dropped` acima, mesmo arquivo) -- provando que o silêncio
//     que o próprio briefing deste item nomeia ("o silêncio morre") está fechado: uma declaração
//     inline malformada não é mais invisível, é diagnosticada exatamente como a própria irmã dela
//     de `<style>`.
// ---------------------------------------------------------------------------
void test_inline_style_valid_followed_by_garbage_diagnoses() {
  auto result = parse_inline_style("color: red; not-a-real-property: 5;");
  check(!result.error.has_value(), "inline_style_valid_then_garbage: no fatal error");
  check(result.declarations.size() == 1,
        "inline_style_valid_then_garbage: only the known declaration survives");
  check(has_declaration(result.declarations, "color", "red"),
        "inline_style_valid_then_garbage: 'color' == 'red' survives");
  check(!has_declaration_name(result.declarations, "not-a-real-property"),
        "inline_style_valid_then_garbage: the unknown property never reaches declarations");
  check(!result.diagnostics.empty(),
        "inline_style_valid_then_garbage: THE SILENCE DIES -- a ParseDiagnostic was recorded");
  if (!result.diagnostics.empty()) {
    check(result.diagnostics[0].message.find("not-a-real-property") != std::string::npos,
          "inline_style_valid_then_garbage: the diagnostic names the raw offending text, same "
          "minimum-logging-content discipline as docs/uix-rcss.md section 11");
  }
}

} // namespace

int main() {
  test_longhand_passthrough();
  test_shorthand_expansion();
  test_recovery_unknown_property_dropped();
  test_recovery_malformed_rule_does_not_kill_sheet();
  test_recovery_unknown_at_rule_skipped();
  test_font_face_structural_record();
  test_font_face_newline_before_brace();
  test_whitespace_only_prelude_between_rules_is_not_diagnosed();
  test_keyframes_structural_record();
  test_keyframes_percentage_selectors();
  test_fatal_lexer_error_propagates();
  test_inline_style_basic_declarations();
  test_inline_style_empty_attribute_is_a_clean_noop();
  test_inline_style_whitespace_only_attribute_is_a_clean_noop();
  test_inline_style_name_with_no_colon_is_silently_dropped_same_as_rule_body();
  test_inline_style_extra_trailing_semicolon_is_silently_dropped();
  test_inline_style_valid_followed_by_garbage_diagnoses();

  if (g_failures > 0) {
    std::fprintf(stderr, "parser_atrule_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("parser_atrule_sanity: PASS");
  return 0;
}
