// SPDX-License-Identifier: Apache-2.0
// EN: UIX-RCSS-LEXER -- functional/token-shape unit test for glintfx::uix::style::Lexer.
//     Standalone, no parser, no selector matching, no cascade, no property registry -- see
//     glintfx/src/uix/style/lexer.hpp's own header comment for the full scope/boundary this
//     module holds itself to. Mirrors glintfx/tests/uix/lexer_tokens_sanity.cpp's own shape (this
//     module's DOM sibling): every case below traces back to a specific
//     `/var/tmp/censo-rcss-qa1/censo.md` fact, a specific real-upstream-RmlUi-source citation
//     (`examples/RmlUi/Source/Core/StyleSheetParser.cpp`), or a specific header-comment decision
//     cited inline -- this is deliberately not "make up plausible CSS edge cases", it is "prove
//     the tokenizer handles what the real corpus needs and nothing this arc has not decided yet".
// PT: UIX-RCSS-LEXER -- teste unit de forma-de-token/funcional pro glintfx::uix::style::Lexer.
//     Standalone, sem parser, sem casamento de seletor, sem cascata, sem registro de propriedade
//     -- ver o próprio comentário de cabeçalho do glintfx/src/uix/style/lexer.hpp pro
//     escopo/fronteira completos a que este módulo se prende. Espelha a própria forma do
//     glintfx/tests/uix/lexer_tokens_sanity.cpp (o irmão DOM deste módulo): todo caso abaixo
//     remonta a um fato específico do `/var/tmp/censo-rcss-qa1/censo.md`, uma citação específica
//     do RmlUi upstream real (`examples/RmlUi/Source/Core/StyleSheetParser.cpp`), ou uma decisão
//     específica de comentário de cabeçalho citada inline -- deliberadamente isto não é "inventar
//     casos de borda CSS plausíveis", é "provar que o tokenizador trata o que o corpus real
//     precisa e nada que este arco ainda não decidiu".
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/style/lexer.hpp"

#include <cstdio>
#include <string>
#include <string_view>

namespace {

int g_failures = 0;

// EN: CI-ARCH-WERROR-STB domino sweep (2026-08-06): unlike the DOM sibling
//     (tests/uix/lexer_tokens_sanity.cpp), every assertion in THIS file goes through check_eq()
//     or expect_kind() -- no case here needed a raw boolean check(). The unused function was
//     copied-as-boilerplate from that sibling and never called; clang 22.1.8 + -Werror (the
//     arch-container CI leg) flags it as -Wunused-function. Removed rather than kept dead --
//     Arch was the only leg strict enough to catch this, and it was masked until now by the
//     unrelated stb_image_write.h -Werror build break this same task fixed (see CMakeLists.txt).
// PT: Varredura dominó do CI-ARCH-WERROR-STB (2026-08-06): diferente do irmão DOM
//     (tests/uix/lexer_tokens_sanity.cpp), toda asserção NESTE arquivo passa por check_eq() ou
//     expect_kind() -- nenhum caso aqui precisou de um check() booleano cru. A função não-usada
//     foi copiada como boilerplate daquele irmão e nunca chamada; o clang 22.1.8 + -Werror (a
//     perna arch-container do CI) acusa como -Wunused-function. Removida em vez de mantida morta
//     -- o Arch foi a única perna estrita o bastante pra pegar isto, e ficou mascarada até agora
//     pela quebra de build não-relacionada do stb_image_write.h que esta mesma tarefa consertou
//     (ver CMakeLists.txt).
void check_eq(std::string_view got, std::string_view want, const char* what) {
  if (got != want) {
    std::fprintf(stderr, "FAIL: %s (got \"%.*s\", want \"%.*s\")\n", what,
                 static_cast<int>(got.size()), got.data(), static_cast<int>(want.size()),
                 want.data());
    ++g_failures;
  }
}

using glintfx::uix::style::Lexer;
using glintfx::uix::style::Token;
using glintfx::uix::style::TokenKind;

const char* kind_name(TokenKind k) {
  switch (k) {
    case TokenKind::At:
      return "At";
    case TokenKind::Prelude:
      return "Prelude";
    case TokenKind::BraceOpen:
      return "BraceOpen";
    case TokenKind::BraceClose:
      return "BraceClose";
    case TokenKind::Declaration:
      return "Declaration";
    case TokenKind::Comment:
      return "Comment";
    case TokenKind::EndOfFile:
      return "EndOfFile";
    case TokenKind::Error:
      return "Error";
  }
  return "?";
}

// EN: Same "read as a flat ordered list" ergonomics as the DOM sibling's own expect_kind -- see
//     that file's own comment for the reasoning, not re-derived here.
// PT: Mesma ergonomia "lê como lista plana ordenada" do próprio expect_kind do irmão DOM -- ver o
//     próprio comentário daquele arquivo pro raciocínio, não re-derivado aqui.
Token expect_kind(Lexer& lex, TokenKind want, const char* what) {
  Token tok = lex.next();
  if (tok.kind != want) {
    std::fprintf(stderr, "FAIL: %s (got kind %s, want %s)\n", what, kind_name(tok.kind),
                 kind_name(want));
    ++g_failures;
  }
  return tok;
}

// ---------------------------------------------------------------------------
// EN: Case 1 -- one selector, one declaration: the absolute minimum shape. `Prelude` carries the
//     RAW, byte-verbatim selector text (unparsed -- "sem casamento de seletor" is this arc's own
//     boundary); `Declaration` bundles name+value into ONE token, mirroring the DOM sibling's own
//     `Attr` bundling reasoning (lexer.hpp header, "Token model").
// PT: Caso 1 -- um seletor, uma declaração: a forma mínima absoluta. `Prelude` carrega o texto CRU,
//     byte-verbatim do seletor (não-parseado -- "sem casamento de seletor" é a própria fronteira
//     deste arco); `Declaration` empacota nome+valor num ÚNICO token, espelhando o próprio
//     raciocínio de empacotamento do `Attr` do irmão DOM (comentário de cabeçalho do lexer.hpp,
//     "Modelo de token").
// ---------------------------------------------------------------------------
void test_one_selector_one_declaration() {
  Lexer lex(".foo { color: red; }");

  Token prelude = expect_kind(lex, TokenKind::Prelude, "1decl: Prelude");
  check_eq(prelude.text, ".foo ", "1decl: prelude text is raw, unstripped, unparsed");

  expect_kind(lex, TokenKind::BraceOpen, "1decl: BraceOpen");

  Token decl = expect_kind(lex, TokenKind::Declaration, "1decl: Declaration");
  check_eq(decl.text, "color", "1decl: declaration name is whitespace-stripped");
  check_eq(decl.value, "red", "1decl: declaration value is whitespace-stripped");

  expect_kind(lex, TokenKind::BraceClose, "1decl: BraceClose");
  expect_kind(lex, TokenKind::EndOfFile, "1decl: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 2 -- multiple declarations, no trailing `;` after the last one (matches upstream's own
//     ReadProperties permissiveness for the final declaration -- `StyleSheetParser.cpp`'s own
//     post-loop `if (state == VALUE && !name.empty() && !value.empty())` handling; see this file's
//     `test_last_declaration_without_trailing_semicolon` below for the header-comment-cited case).
// PT: Caso 2 -- múltiplas declarações, sem `;` final depois da última (casa com a própria
//     permissividade do ReadProperties do upstream pra declaração final -- o próprio tratamento
//     pós-laço `if (state == VALUE && !name.empty() && !value.empty())` do
//     `StyleSheetParser.cpp`; ver `test_last_declaration_without_trailing_semicolon` abaixo neste
//     arquivo pro caso citado no comentário de cabeçalho).
// ---------------------------------------------------------------------------
void test_multiple_declarations() {
  Lexer lex("a{width:1px;height:2px;}");

  Token prelude = expect_kind(lex, TokenKind::Prelude, "multi: Prelude");
  check_eq(prelude.text, "a", "multi: prelude is bare tag name");
  expect_kind(lex, TokenKind::BraceOpen, "multi: BraceOpen");

  Token d1 = expect_kind(lex, TokenKind::Declaration, "multi: Declaration 1");
  check_eq(d1.text, "width", "multi: decl1 name");
  check_eq(d1.value, "1px", "multi: decl1 value");

  Token d2 = expect_kind(lex, TokenKind::Declaration, "multi: Declaration 2");
  check_eq(d2.text, "height", "multi: decl2 name");
  check_eq(d2.value, "2px", "multi: decl2 value");

  expect_kind(lex, TokenKind::BraceClose, "multi: BraceClose");
  expect_kind(lex, TokenKind::EndOfFile, "multi: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 3 -- comma-separated selector list stays ONE raw `Prelude` token, comma untouched --
//     censo.md section 1.2 measures 15 real instances (`.a, .b, .c { }` shape), the líder
//     authorized this form into the RMLX-2 subset (TODO.md `RMLX-2` decision 1), but selector
//     LIST-SPLITTING (deciding ".a" and ".b" are two independent selectors) is parser territory,
//     not this lexer's -- exactly the same "sem casamento de seletor" boundary as case 1's own
//     comment states. This is the "não pode engasgar" proof for that specific census finding.
// PT: Caso 3 -- lista de seletores separada por vírgula fica UM token `Prelude` cru só, vírgula
//     intocada -- o censo.md seção 1.2 mede 15 instâncias reais (forma `.a, .b, .c { }`), o líder
//     autorizou esta forma no subconjunto da RMLX-2 (decisão 1 da entrada `RMLX-2` do TODO.md), mas
//     DIVIDIR a lista de seletores (decidir que ".a" e ".b" são dois seletores independentes) é
//     território do parser, não deste lexer -- exatamente a mesma fronteira "sem casamento de
//     seletor" que o próprio comentário do caso 1 declara. Esta é a prova de "não engasgar" pra
//     esse achado específico do censo.
// ---------------------------------------------------------------------------
void test_comma_selector_list_stays_one_prelude() {
  Lexer lex("#hpbox.wide, #namebox.wide, #flagwide.wide { display: block; }");

  Token prelude = expect_kind(lex, TokenKind::Prelude, "comma-list: Prelude");
  check_eq(prelude.text, "#hpbox.wide, #namebox.wide, #flagwide.wide ",
           "comma-list: comma is ordinary prelude text, not a delimiter at this layer");
  expect_kind(lex, TokenKind::BraceOpen, "comma-list: BraceOpen");
  expect_kind(lex, TokenKind::Declaration, "comma-list: Declaration");
  expect_kind(lex, TokenKind::BraceClose, "comma-list: BraceClose");
  expect_kind(lex, TokenKind::EndOfFile, "comma-list: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 4 -- `box-shadow`'s own multi-layer, comma-separated value form (censo.md section 4.1:
//     "cor PRIMEIRO", 7 real 2-layer declarations) must survive as ONE `Declaration` value, raw,
//     comma and all -- this lexer does not know `box-shadow` is a property with special
//     comma-list-of-layers semantics (that is a property-registry/parser concern, RMLX-2's, not
//     this tokenizer's), it just has to not choke on the bytes.
// PT: Caso 4 -- a forma própria de `box-shadow`, multi-camada separada por vírgula (censo.md seção
//     4.1: "cor PRIMEIRO", 7 declarações reais de 2 camadas) precisa sobreviver como UM valor de
//     `Declaration` só, cru, vírgula e tudo -- este lexer não sabe que `box-shadow` é uma
//     propriedade com semântica especial de lista-de-camadas-por-vírgula (isso é preocupação de
//     registro-de-propriedade/parser, da RMLX-2, não deste tokenizador), ele só precisa não
//     engasgar nos bytes.
// ---------------------------------------------------------------------------
void test_box_shadow_multilayer_value_survives_whole() {
  Lexer lex(".x { box-shadow: #22D3EE 0dp 0dp 0dp 1dp inset, #22D3EE26 0dp 0dp 16dp 0dp; }");

  expect_kind(lex, TokenKind::Prelude, "box-shadow: Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "box-shadow: BraceOpen");
  Token decl = expect_kind(lex, TokenKind::Declaration, "box-shadow: Declaration");
  check_eq(decl.text, "box-shadow", "box-shadow: name");
  check_eq(decl.value, "#22D3EE 0dp 0dp 0dp 1dp inset, #22D3EE26 0dp 0dp 16dp 0dp",
           "box-shadow: whole comma-separated multi-layer value, verbatim");
  expect_kind(lex, TokenKind::BraceClose, "box-shadow: BraceClose");
}

// ---------------------------------------------------------------------------
// EN: Case 5 -- `decorator`'s nested-parens, internal-commas sub-language (censo.md section 9:
//     "a propriedade mais rica do corpus", 203 instances) must ALSO survive as one raw value --
//     same "not this lexer's grammar to parse" reasoning as case 4, extended to parentheses.
// PT: Caso 5 -- a sub-linguagem de parênteses aninhados e vírgulas internas do `decorator`
//     (censo.md seção 9: "a propriedade mais rica do corpus", 203 instâncias) TAMBÉM precisa
//     sobreviver como um valor cru só -- mesmo raciocínio "não é gramática deste lexer parsear" do
//     caso 4, estendido a parênteses.
// ---------------------------------------------------------------------------
void test_decorator_nested_parens_and_commas_survive_whole() {
  Lexer lex(".x { decorator: linear-gradient(135deg, #6a0ddc, #e02828, #0f8a4a); }");

  expect_kind(lex, TokenKind::Prelude, "decorator: Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "decorator: BraceOpen");
  Token decl = expect_kind(lex, TokenKind::Declaration, "decorator: Declaration");
  check_eq(decl.text, "decorator", "decorator: name");
  check_eq(decl.value, "linear-gradient(135deg, #6a0ddc, #e02828, #0f8a4a)",
           "decorator: whole nested-paren value, verbatim, commas untouched");
  expect_kind(lex, TokenKind::BraceClose, "decorator: BraceClose");
}

// ---------------------------------------------------------------------------
// EN: Case 6 -- a quoted value: `;`/`}` INSIDE the quotes must not terminate the value early.
//     Mirrors `StyleSheetParser.cpp`'s own `QUOTE` state (ReadProperties, `case QUOTE`) -- only a
//     `"` starts/ends it (single quotes are NOT special here, unlike RML's attribute-value quoting
//     -- a real, documented divergence between the two grammars, see this file's own header
//     comment for the citation).
// PT: Caso 6 -- um valor entre aspas: `;`/`}` DENTRO das aspas não pode terminar o valor cedo.
//     Espelha o próprio estado `QUOTE` do `StyleSheetParser.cpp` (ReadProperties, `case QUOTE`) --
//     só `"` abre/fecha (aspas simples NÃO são especiais aqui, diferente do quoting de valor de
//     atributo do RML -- uma divergência real, documentada, entre as duas gramáticas, ver o
//     próprio comentário de cabeçalho deste arquivo pra citação).
// ---------------------------------------------------------------------------
void test_quoted_value_semicolon_and_brace_inside_quotes_do_not_terminate() {
  Lexer lex(R"(.x { font-family: "Sans; Serif} Weird"; })");

  expect_kind(lex, TokenKind::Prelude, "quoted: Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "quoted: BraceOpen");
  Token decl = expect_kind(lex, TokenKind::Declaration, "quoted: Declaration");
  check_eq(decl.text, "font-family", "quoted: name");
  check_eq(decl.value, R"("Sans; Serif} Weird")",
           "quoted: value keeps its quotes, and the ';'/'}' inside them did not terminate it");
  expect_kind(lex, TokenKind::BraceClose, "quoted: BraceClose");
}

// ---------------------------------------------------------------------------
// EN: Case 7 -- `@font-face` is a flat declarations block, EXACTLY the same shape as an ordinary
//     rule -- no special nesting (upstream: `StyleSheetParser.cpp`'s own `at_rule_identifier ==
//     "font-face"` branch calls `ReadProperties` directly, same call as the ordinary-rule branch).
//     censo.md section 7: 20 real instances, always `font-family` + `src`.
// PT: Caso 7 -- `@font-face` é um bloco de declarações plano, EXATAMENTE a mesma forma de uma regra
//     comum -- sem aninhamento especial (upstream: o próprio ramo `at_rule_identifier ==
//     "font-face"` do `StyleSheetParser.cpp` chama `ReadProperties` direto, a mesma chamada do ramo
//     de regra comum). censo.md seção 7: 20 instâncias reais, sempre `font-family` + `src`.
// ---------------------------------------------------------------------------
void test_font_face_is_flat_declarations() {
  Lexer lex(R"(@font-face { font-family: "Sans"; src: "sans.ttf"; })");

  expect_kind(lex, TokenKind::At, "font-face: At");
  Token prelude = expect_kind(lex, TokenKind::Prelude, "font-face: Prelude");
  check_eq(prelude.text, "font-face ", "font-face: at-rule name+params, raw");

  expect_kind(lex, TokenKind::BraceOpen, "font-face: BraceOpen");
  Token d1 = expect_kind(lex, TokenKind::Declaration, "font-face: Declaration 1");
  check_eq(d1.text, "font-family", "font-face: decl1 name");
  check_eq(d1.value, R"("Sans")", "font-face: decl1 value");
  Token d2 = expect_kind(lex, TokenKind::Declaration, "font-face: Declaration 2");
  check_eq(d2.text, "src", "font-face: decl2 name");
  check_eq(d2.value, R"("sans.ttf")", "font-face: decl2 value");
  expect_kind(lex, TokenKind::BraceClose, "font-face: BraceClose");
  expect_kind(lex, TokenKind::EndOfFile, "font-face: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 8 -- `@keyframes`' own TWO-LEVEL nesting: the outer `{` opens a SECOND structural
//     (prelude-scanning) region, not a declarations block -- each inner `{` (one per `from`/`to`/
//     `N%` sub-block) is what actually opens flat declarations. This is the one place this lexer
//     special-cases an at-rule NAME (case-sensitive "keyframes", matching upstream's own
//     `at_rule_identifier == "keyframes"` check, `StyleSheetParser.cpp`, no `ToLower` call unlike
//     RML's tag-name folding) -- see lexer.hpp header, "keyframes nesting", for the full argument
//     of why this is a lexer-level fact, not parser guesswork: WITHOUT it, this exact real corpus
//     line (`gusworld_battle_cockpit.rml:214`, `@keyframes spin { from {...} to {...} }`, verified
//     by direct `sed`/`grep` read of the fixture) cannot be tokenized without desyncing brace
//     balance -- proven inline in lexer_hardening_sanity.cpp's own
//     `test_keyframes_special_case_is_load_bearing`.
// PT: Caso 8 -- o aninhamento de DOIS NÍVEIS do próprio `@keyframes`: o `{` externo abre uma
//     SEGUNDA região estrutural (scanning de prelúdio), não um bloco de declarações -- cada `{`
//     interno (um por sub-bloco `from`/`to`/`N%`) é quem de fato abre declarações planas. Este é o
//     único lugar em que este lexer trata especialmente um NOME de at-rule (case-sensitive
//     "keyframes", casando com o próprio check `at_rule_identifier == "keyframes"` do upstream,
//     `StyleSheetParser.cpp`, sem chamada `ToLower`, diferente da dobra de nome-de-tag do RML) --
//     ver o cabeçalho do lexer.hpp, "aninhamento de keyframes", pro argumento completo de por que
//     isto é um fato de nível-lexer, não chute de parser: SEM isto, esta linha exata de corpus real
//     (`gusworld_battle_cockpit.rml:214`, `@keyframes spin { from {...} to {...} }`, verificada por
//     leitura direta `sed`/`grep` da fixture) não consegue ser tokenizada sem desincronizar o
//     balanço de chaves -- provado inline no `test_keyframes_special_case_is_load_bearing` do
//     lexer_hardening_sanity.cpp.
// ---------------------------------------------------------------------------
void test_keyframes_two_level_nesting() {
  Lexer lex("@keyframes spin { from { transform: rotate(0deg); } to { transform: rotate(360deg); } }");

  expect_kind(lex, TokenKind::At, "keyframes: At");
  Token outer_prelude = expect_kind(lex, TokenKind::Prelude, "keyframes: outer Prelude");
  check_eq(outer_prelude.text, "keyframes spin ", "keyframes: at-rule name+params");

  expect_kind(lex, TokenKind::BraceOpen, "keyframes: outer BraceOpen (nested-structural mode)");

  Token from_prelude = expect_kind(lex, TokenKind::Prelude, "keyframes: 'from' Prelude");
  check_eq(from_prelude.text, " from ",
           "keyframes: 'from' is a plain prelude, not a Declaration -- leading space kept "
           "verbatim, same unstripped-accumulation rule as every other Prelude token");
  expect_kind(lex, TokenKind::BraceOpen, "keyframes: 'from' BraceOpen (declaration mode)");
  Token from_decl = expect_kind(lex, TokenKind::Declaration, "keyframes: 'from' Declaration");
  check_eq(from_decl.text, "transform", "keyframes: 'from' decl name");
  check_eq(from_decl.value, "rotate(0deg)", "keyframes: 'from' decl value");
  expect_kind(lex, TokenKind::BraceClose, "keyframes: 'from' BraceClose");

  Token to_prelude = expect_kind(lex, TokenKind::Prelude, "keyframes: 'to' Prelude");
  check_eq(to_prelude.text, " to ", "keyframes: 'to' prelude, raw, leading space included");
  expect_kind(lex, TokenKind::BraceOpen, "keyframes: 'to' BraceOpen (declaration mode)");
  Token to_decl = expect_kind(lex, TokenKind::Declaration, "keyframes: 'to' Declaration");
  check_eq(to_decl.value, "rotate(360deg)", "keyframes: 'to' decl value");
  expect_kind(lex, TokenKind::BraceClose, "keyframes: 'to' BraceClose");

  // EN: The single space between "}" (closing 'to') and the outer "}" is, at this point, back in
  //     Structural (nested) mode -- unlike Declaration mode's own internal "silently discard
  //     trailing whitespace before '}'" loop (scan_declaration()'s own NAME-run-hits-'}' path),
  //     Structural mode has no such absorption: ANY non-delimiter byte, including a lone space,
  //     starts its own Prelude run. This is the concrete, corpus-irrelevant-but-real consequence
  //     of "Prelude is byte-verbatim, unstripped" (lexer.hpp header, "Token model").
  // PT: O único espaço entre o "}" (fechando 'to') e o "}" externo está, neste ponto, de volta em
  //     modo Estrutural (aninhado) -- diferente do próprio laço interno "descarta whitespace final
  //     antes de '}' em silêncio" do modo Declaration (o próprio caminho
  //     NAME-run-bate-em-'}' do scan_declaration()), o modo Estrutural não tem essa absorção:
  //     QUALQUER byte não-delimitador, inclusive um espaço solto, começa o PRÓPRIO trecho de
  //     Prelude. Esta é a consequência concreta, irrelevante-pro-corpus-mas-real, de "Prelude é
  //     byte-verbatim, não-stripado" (comentário de cabeçalho do lexer.hpp, "Modelo de token").
  Token trailing_space = expect_kind(lex, TokenKind::Prelude, "keyframes: trailing-space Prelude");
  check_eq(trailing_space.text, " ", "keyframes: the lone space before the outer '}'");

  expect_kind(lex, TokenKind::BraceClose, "keyframes: outer BraceClose");
  expect_kind(lex, TokenKind::EndOfFile, "keyframes: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 9 -- a percentage keyframe selector (`50%`) is just ordinary Prelude text -- this
//     lexer does not know `%` is special inside `@keyframes` (that is RMLX-8's job, per
//     docs/uix-rcss.md section 10, "a fourth, distinct meaning of `%`... this dump does not
//     attempt to resolve"). censo.md section 7: `50%` is the ONE non-from/to selector measured.
// PT: Caso 9 -- um seletor de keyframe percentual (`50%`) é só texto de Prelude comum -- este lexer
//     não sabe que `%` é especial dentro de `@keyframes` (isso é trabalho da RMLX-8, per
//     docs/uix-rcss.md seção 10, "um quarto significado distinto de `%`... este dump não tenta
//     resolver"). censo.md seção 7: `50%` é o ÚNICO seletor não-from/to medido.
// ---------------------------------------------------------------------------
void test_keyframes_percent_selector_is_plain_prelude() {
  Lexer lex("@keyframes blink { from { opacity: 1; } 50% { opacity: 0.25; } to { opacity: 1; } }");

  expect_kind(lex, TokenKind::At, "percent: At");
  expect_kind(lex, TokenKind::Prelude, "percent: outer Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "percent: outer BraceOpen");

  expect_kind(lex, TokenKind::Prelude, "percent: 'from' Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "percent: 'from' BraceOpen");
  expect_kind(lex, TokenKind::Declaration, "percent: 'from' Declaration");
  expect_kind(lex, TokenKind::BraceClose, "percent: 'from' BraceClose");

  Token pct_prelude = expect_kind(lex, TokenKind::Prelude, "percent: '50%' Prelude");
  check_eq(pct_prelude.text, " 50% ", "percent: raw '50%' text, no numeric interpretation");
  expect_kind(lex, TokenKind::BraceOpen, "percent: '50%' BraceOpen");
  expect_kind(lex, TokenKind::Declaration, "percent: '50%' Declaration");
  expect_kind(lex, TokenKind::BraceClose, "percent: '50%' BraceClose");

  expect_kind(lex, TokenKind::Prelude, "percent: 'to' Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "percent: 'to' BraceOpen");
  expect_kind(lex, TokenKind::Declaration, "percent: 'to' Declaration");
  expect_kind(lex, TokenKind::BraceClose, "percent: 'to' BraceClose");

  // EN: Same trailing-lone-space-becomes-its-own-Prelude fact as
  //     test_keyframes_two_level_nesting's own closing comment explains.
  // PT: Mesmo fato de espaço-final-solto-vira-o-próprio-Prelude que o próprio comentário de
  //     fechamento do test_keyframes_two_level_nesting explica.
  Token trailing_space = expect_kind(lex, TokenKind::Prelude, "percent: trailing-space Prelude");
  check_eq(trailing_space.text, " ", "percent: the lone space before the outer '}'");

  expect_kind(lex, TokenKind::BraceClose, "percent: outer BraceClose");
}

// ---------------------------------------------------------------------------
// EN: Case 10 -- `/* ... */` comments, both before a selector and between declarations, are their
//     own `Comment` token (a DELIBERATE divergence from upstream's own character-level, zero-width
//     comment ELISION -- see lexer.hpp header, "Comment handling", for the full argument and why
//     this is reported as an open question rather than silently decided).
// PT: Caso 10 -- comentários `/* ... */`, tanto antes de um seletor quanto entre declarações, são
//     seu PRÓPRIO token `Comment` (uma divergência DELIBERADA da própria ELISÃO de comentário de
//     nível-caractere, largura-zero, do upstream -- ver o cabeçalho do lexer.hpp, "Trato de
//     comentário", pro argumento completo e por que isto é reportado como pergunta em aberto em vez
//     de decidido em silêncio).
// ---------------------------------------------------------------------------
void test_comments_before_selector_and_between_declarations() {
  Lexer lex("/* header */.foo{/* c */color:red;}");

  Token c1 = expect_kind(lex, TokenKind::Comment, "comments: leading Comment");
  check_eq(c1.text, " header ", "comments: leading comment body");

  Token prelude = expect_kind(lex, TokenKind::Prelude, "comments: Prelude");
  check_eq(prelude.text, ".foo", "comments: prelude after the leading comment");

  expect_kind(lex, TokenKind::BraceOpen, "comments: BraceOpen");

  Token c2 = expect_kind(lex, TokenKind::Comment, "comments: mid-block Comment");
  check_eq(c2.text, " c ", "comments: mid-block comment body");

  Token decl = expect_kind(lex, TokenKind::Declaration, "comments: Declaration after comment");
  check_eq(decl.text, "color", "comments: decl name after comment");
  check_eq(decl.value, "red", "comments: decl value after comment");

  expect_kind(lex, TokenKind::BraceClose, "comments: BraceClose");
}

// ---------------------------------------------------------------------------
// EN: Case 11 -- THE census's own surprise #2 (censo.md section 8): `//` is NOT recognized RCSS
//     comment syntax (verified directly against `examples/RmlUi/Source/Core/StyleSheetParser.cpp`
//     -- `ReadCharacter` only special-cases a `/` followed by `*`; a `/` followed by anything else,
//     including a second `/`, is returned as an ORDINARY character). This is real, measured,
//     PRODUCTION markup (3 of the 16 `save_load_menu__*.rml` fixtures ship this verbatim). This
//     lexer must not choke on it -- the two `/` bytes and the prose after them simply become part
//     of the ordinary Prelude run, exactly like upstream's own (mis)behaviour.
// PT: Caso 11 -- a própria surpresa nº2 do censo (censo.md seção 8): `//` NÃO é sintaxe de
//     comentário RCSS reconhecida (verificado direto contra o
//     `examples/RmlUi/Source/Core/StyleSheetParser.cpp` -- o `ReadCharacter` só trata
//     especialmente um `/` seguido de `*`; um `/` seguido de qualquer outra coisa, inclusive um
//     segundo `/`, é devolvido como caractere COMUM). Isto é markup real, medido, de PRODUÇÃO (3
//     das 16 fixtures `save_load_menu__*.rml` rodam isto verbatim). Este lexer não pode engasgar
//     nisso -- os dois bytes `/` e a prosa depois deles simplesmente viram parte do trecho de
//     Prelude comum, exatamente como o (mau) comportamento do próprio upstream.
// ---------------------------------------------------------------------------
void test_double_slash_is_not_a_comment_becomes_prelude_prose() {
  Lexer lex("// nao e comentario RCSS de verdade\n.btn-back:hover{color:red;}");

  Token prelude = expect_kind(lex, TokenKind::Prelude, "double-slash: Prelude");
  check_eq(prelude.text, "// nao e comentario RCSS de verdade\n.btn-back:hover",
           "double-slash: the '//' prose becomes literal prelude text, glued to the real selector "
           "-- matches real, measured upstream (mis)behaviour, see censo.md section 8");
  expect_kind(lex, TokenKind::BraceOpen, "double-slash: BraceOpen");
  expect_kind(lex, TokenKind::Declaration, "double-slash: Declaration");
  expect_kind(lex, TokenKind::BraceClose, "double-slash: BraceClose");
  expect_kind(lex, TokenKind::EndOfFile, "double-slash: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 12 -- an empty declaration (stray `;`, or `;;`) is silently skipped, never emitted as a
//     `Declaration` and never an `Error` -- matches upstream's own `NAME` state handling of `;`
//     with an empty name (`StyleSheetParser.cpp`'s `ReadProperties`, `case ';': ... if
//     (!name.empty()) { log a warning; name.clear(); }` -- the warning is upstream's, this module
//     has no logger, "zero logging" per this repo's own dom_tree.cpp precedent already cited in
//     lexer.hpp).
// PT: Caso 12 -- uma declaração vazia (`;` solto, ou `;;`) é silenciosamente pulada, nunca emitida
//     como `Declaration` e nunca um `Error` -- casa com o próprio tratamento do estado `NAME` do
//     upstream pra `;` com nome vazio (`ReadProperties` do `StyleSheetParser.cpp`, `case ';': ...
//     if (!name.empty()) { loga aviso; name.clear(); }` -- o aviso é do upstream, este módulo não
//     tem logger, "zero logging" pelo próprio precedente do dom_tree.cpp deste repo já citado no
//     lexer.hpp).
// ---------------------------------------------------------------------------
void test_empty_declarations_are_silently_skipped() {
  Lexer lex("a{;;color:red;;}");

  expect_kind(lex, TokenKind::Prelude, "empty-decl: Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "empty-decl: BraceOpen");
  Token decl = expect_kind(lex, TokenKind::Declaration, "empty-decl: the one real Declaration");
  check_eq(decl.text, "color", "empty-decl: name");
  check_eq(decl.value, "red", "empty-decl: value");
  expect_kind(lex, TokenKind::BraceClose, "empty-decl: BraceClose (trailing ';;' also skipped)");
  expect_kind(lex, TokenKind::EndOfFile, "empty-decl: EndOfFile");
}

// ---------------------------------------------------------------------------
// EN: Case 13 -- an UNKNOWN/unmeasured at-rule name (anything other than the case-sensitive
//     literal "keyframes") defaults to flat Declaration-mode children, the SAME default an
//     ordinary rule gets -- a DECLARED, deliberate limitation (not a silent guess): this is
//     correct for `@font-face`/`@decorator`/`@spritesheet` (all genuinely flat upstream), and
//     UNVERIFIED for a hypothetical `@media` (0 measured instances, census section 0 and
//     docs/rmlx-subset.md section 13 both name it a real-zero exclusion) -- see lexer.hpp header,
//     "At-rule mode table", for the full argument and the explicit teto.
// PT: Caso 13 -- um nome de at-rule DESCONHECIDO/não-medido (qualquer coisa além do literal
//     case-sensitive "keyframes") cai no default de filhos em modo Declaration plano, o MESMO
//     default que uma regra comum recebe -- uma limitação DECLARADA, deliberada (não um chute em
//     silêncio): isto é correto pra `@font-face`/`@decorator`/`@spritesheet` (todos genuinamente
//     planos no upstream), e NÃO-VERIFICADO pra um `@media` hipotético (0 instâncias medidas, censo
//     seção 0 e docs/rmlx-subset.md seção 13 nomeiam os dois como exclusão real-zero) -- ver o
//     cabeçalho do lexer.hpp, "Tabela de modo de at-rule", pro argumento completo e o teto
//     explícito.
// ---------------------------------------------------------------------------
void test_unknown_at_rule_defaults_to_flat_declarations() {
  Lexer lex("@media (min-width: 10px) { color: red; }");

  expect_kind(lex, TokenKind::At, "unknown-at-rule: At");
  expect_kind(lex, TokenKind::Prelude, "unknown-at-rule: Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "unknown-at-rule: BraceOpen (defaults to Declaration mode)");
  Token decl = expect_kind(lex, TokenKind::Declaration,
                           "unknown-at-rule: 'color: red' tokenizes as an ordinary Declaration "
                           "(documented limitation: a real @media body with NESTED rules would "
                           "NOT tokenize correctly here -- see lexer.hpp header)");
  check_eq(decl.text, "color", "unknown-at-rule: decl name");
  expect_kind(lex, TokenKind::BraceClose, "unknown-at-rule: BraceClose");
}

// ---------------------------------------------------------------------------
// EN: Case 14 -- keyframes case-sensitivity: matches upstream's own EXACT-match `at_rule_identifier
//     == "keyframes"` check (no `ToLower`, `StyleSheetParser.cpp`) -- `@KEYFRAMES`/`@Keyframes`
//     does NOT trigger nested-structural mode, falls to the same default as case 13.
// PT: Caso 14 -- case-sensitivity de keyframes: casa com o próprio check de casamento EXATO
//     `at_rule_identifier == "keyframes"` do upstream (sem `ToLower`, `StyleSheetParser.cpp`) --
//     `@KEYFRAMES`/`@Keyframes` NÃO dispara o modo estrutural aninhado, cai no mesmo default do
//     caso 13.
// ---------------------------------------------------------------------------
void test_keyframes_match_is_case_sensitive() {
  Lexer lex("@KEYFRAMES spin { from { opacity: 1; } }");

  expect_kind(lex, TokenKind::At, "case-sensitive: At");
  expect_kind(lex, TokenKind::Prelude, "case-sensitive: Prelude");
  expect_kind(lex, TokenKind::BraceOpen, "case-sensitive: BraceOpen (NOT nested -- wrong case)");
  // EN: If this had wrongly entered nested-structural mode, the next token would be another
  //     Prelude ("from ") followed by a BraceOpen, not a Declaration -- this assertion is the
  //     concrete proof of which branch was taken.
  // PT: Se isto tivesse entrado errado em modo estrutural aninhado, o próximo token seria outro
  //     Prelude ("from ") seguido de um BraceOpen, não uma Declaration -- esta asserção é a prova
  //     concreta de qual ramo foi tomado.
  Token decl = expect_kind(lex, TokenKind::Declaration,
                           "case-sensitive: 'from { opacity' etc. tokenizes as flat declarations, "
                           "proving @KEYFRAMES did not match the lowercase-only special case");
  check_eq(decl.text, "from { opacity", "case-sensitive: raw name run, '{' is ordinary here");
  check_eq(decl.value, "1", "case-sensitive: value up to the first ';'");
}

} // namespace

int main() {
  test_one_selector_one_declaration();
  test_multiple_declarations();
  test_comma_selector_list_stays_one_prelude();
  test_box_shadow_multilayer_value_survives_whole();
  test_decorator_nested_parens_and_commas_survive_whole();
  test_quoted_value_semicolon_and_brace_inside_quotes_do_not_terminate();
  test_font_face_is_flat_declarations();
  test_keyframes_two_level_nesting();
  test_keyframes_percent_selector_is_plain_prelude();
  test_comments_before_selector_and_between_declarations();
  test_double_slash_is_not_a_comment_becomes_prelude_prose();
  test_empty_declarations_are_silently_skipped();
  test_unknown_at_rule_defaults_to_flat_declarations();
  test_keyframes_match_is_case_sensitive();

  if (g_failures > 0) {
    std::fprintf(stderr, "lexer_tokens_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("lexer_tokens_sanity: PASS");
  return 0;
}
