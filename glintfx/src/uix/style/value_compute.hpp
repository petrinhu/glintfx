// SPDX-License-Identifier: Apache-2.0
// EN: UIX-VALUE-COMPUTE -- pure value-computation functions for glintfx's own style engine: given
//     a raw declaration value (already isolated by a lexer/cascade slice this item does NOT
//     implement -- no DOM, no cascade, no selector matching, see "Scope" below), produce the exact
//     canonical byte string `docs/uix-rcss.md` sections 7-9 define for the dump. Sibling of
//     property_registry.hpp (which only TAGS a property's domain, `ValueDomain`) and shorthand.hpp
//     (which only ROUTES a shorthand's raw tokens to longhand names) -- this file is the third leg
//     that actually PRINTS a value once its domain and raw text are known. Every rule below traces
//     to a specific docs/uix-rcss.md section, cited per function; the four byte-exact worked
//     examples in that document's own section 15.2-15.4 are this item's own test suite's anchor
//     (`tests/uix_style/value_compute_sanity.cpp`), reproduced verbatim, not paraphrased.
//
//     SCOPE, THIS ITEM'S OWN DECLARED TETO:
//       - IS: `quantize()` (section 8's own explicit round-half-away-from-zero algorithm, not
//         `%.4f`), the per-domain print forms of section 7 (keyword is the caller's job -- it is
//         already the exact registered string, nothing to compute -- number/length/angle/percent/
//         color/string), and the composite grammars of section 9 this item's own brief names:
//         `box-shadow` (9.1), the `decorator`/`mask-image`/`filter`/`backdrop-filter` function
//         list (9.2, including the gradient-stop auto-spacing algorithm of 9.2.1), and `transform`
//         (9.4, the 2D subset the spec itself declares "thinnest, intentionally").
//       - IS NOT a stylesheet parser: every function here takes an ALREADY-ISOLATED single
//         property's raw value text (the shape a lexer's `Declaration.value`, or one of
//         `shorthand::expand_shorthand`'s own `LonghandValue::value` outputs, already has) -- it
//         never sees a whole rule block, a selector, or more than one declaration's value at a
//         time. Splitting a COMPOSITE property's OWN value into its function-call/argument/stop
//         sub-structure (e.g. `box-shadow`'s own comma-separated layers, `decorator`'s own
//         comma/paren-separated function list) is this item's job -- that is section 9's own
//         grammar for ONE property's value, not stylesheet-level parsing.
//       - IS NOT `%` resolution (docs/uix-rcss.md section 5): every symbolic percentage this file
//         prints (`print_percent`, family (a)/(b)/(c) alike) stays a bare `<number>%` -- this
//         module never resolves against a containing block, a gradient axis, or a gradient's own
//         2D space. The ONE percent-shaped exception, per section 5's own text, is gradient-stop
//         AUTO-SPACING (section 9.2.1) -- a pure function of stop INDEX and COUNT, zero box
//         geometry, explicitly authorized to resolve despite the rest of this module staying
//         symbolic; `resolve_gradient_stop_positions()` below is that one function.
//       - IS NOT unit-complete for LENGTH resolution (section 8.1): only `px` (identity) and `dp`
//         (`* dp_ratio`) are resolved -- the two units that cover 2571 of the corpus's ~2742
//         length-ish measured instances (`dp` 2237 + `px` 334, per `RMLX-2`'s own census cited in
//         `TODO.md`). `em`/`rem`/`vw`/`vh`/`PPI_UNIT` need a font-size or viewport context this
//         pure-function layer has no access to (cascade/layout territory, `RMLX-3`'s own boundary,
//         section 8.1's unit family table) -- `parse_length()` returns `ValueComputeStatus::Invalid`
//         for any of them, never a silent wrong number. Unitless `0` is accepted (CSS's own
//         zero-length convention, needed by `box-shadow`'s real corpus offsets, e.g. worked example
//         9.1's own `0dp` tokens) but no OTHER unitless number is (fail-high, not a guess).
//       - IS NOT `animation` (section 9.3), and DELIBERATELY SO -- see this file's own .cpp header
//         for the divergence this item found reading `PropertyParserAnimation.cpp` directly: the
//         spec's own grammar `animation(<keyframes-name>;<duration>;<timing-keyword>;<iterations>;
//         <alternate>;<paused>)` never names a `delay` field, but upstream's own `Animation` struct
//         (`examples/RmlUi/Include/RmlUi/Core/Animation.h:10-18`) has one, populated by the SAME
//         parser this item's own brief cited (`PropertyParserAnimation.cpp:118-204`, the second
//         bare `%fs` token after `duration_found`). Implementing a composite grammar the spec's own
//         contract does not name a field for would be guessing at what byte-exact form a future
//         reader needs, exactly the failure mode this document's own header warns against --
//         reported here and in this item's own delivery notes, not silently invented.
//       - Color parsing honors docs/uix-rcss.md section 13's own authorized set ONLY: the 4 hex
//         forms (`#rgb`/`#rgba`/`#rrggbb`/`#rrggbbaa`) plus exactly the 3 named colors this
//         registry's own initial values and the census's own measured declarations need
//         (`white`, `black`, `transparent`) -- every other RmlUi named color and every functional
//         color form (`rgb()`, `hsl()`, ...) is `ValueComputeStatus::Invalid`, per section 13's own
//         "requires the líder's sign-off" clause, not silently accepted because the parser
//         happened to be easy to extend that far.
//
//     FAIL-HIGH POLICY (docs/uix-rcss.md section 11, `UIX-RCSS-ERRATA-2`'s own correction to
//     `Finding C`/`Finding I` applied -- restated for this item's own shape): `Invalid` from ANY
//     function in this file, including the composite-LIST ones, means "the caller's own WHOLE
//     property declaration is malformed" -- there is no per-entry partial survival anywhere in
//     this module. `compute_decorator_list()`/`compute_transform_list()` are NOT an exception (an
//     earlier pre-errata draft of this file made them one; corrected): a single unrecognised
//     function name or malformed argument shape anywhere inside a `decorator`/`mask-image`/
//     `filter`/`backdrop-filter`/`transform` value aborts the ENTIRE list, matching
//     `PropertyParserDecorator::ParseValue`/`PropertyParserFilter::ParseValue`'s own real
//     `return false` on the FIRST bad entry (section 11's own `Finding C` citation,
//     `PropertyParserDecorator.cpp:63-131`/`PropertyParserFilter.cpp:29-90`) -- upstream never
//     assigns `property.value` until every entry in the list has parsed successfully, so there is
//     no partial list to preserve. The caller decides what `Invalid` means for its own context (a
//     future cascade slice drops the whole declaration and the property falls back to its section
//     6.1 registry initial value / inherited value), same "this module's own job stops at
//     classifying the input" discipline `property_registry.hpp`/`shorthand.hpp` already establish.
//     An input that is genuinely empty or the literal `none` prints the literal `none` and returns
//     `Ok` (section 9's own empty-list rule) -- that is the ONLY way any function in this file
//     prints `none`; a malformed non-empty input is `Invalid`, never silently coerced to `none`.
//     Nothing in this file logs (same zero-logging-inside-a-standalone-module precedent
//     `property_registry.hpp`/`shorthand.hpp` already establish) and nothing in this file throws.
//
//     TETO (hard ceilings, all fail-high, never silently truncate): `kMaxRawValueBytes` (8 KiB --
//     the largest measured RCSS declaration value in this repo's own corpus is under 200 bytes,
//     `/var/tmp/censo-rcss-qa1/censo.md`; ~40x margin) bounds every function below that takes a
//     whole raw value, and `kMaxNestDepth` (8) bounds `polygon()`'s own recursive `fill` argument
//     (a nested `linear-gradient(...)`/`radial-gradient(...)`, docs/uix-rcss.md section 9.2's own
//     table) -- the corpus never nests more than 1 level deep, this ceiling exists only to keep a
//     hostile/adversarial-review-generated input from recursing unboundedly, never to serve a real
//     use case beyond what the corpus already shows.
//
//     NAMESPACE: `glintfx::uix::style`, same module as lexer.hpp/property_registry.hpp/
//     shorthand.hpp.
// PT: UIX-VALUE-COMPUTE -- funções puras de computação de valor pro próprio motor de estilo da
//     glintfx: dado um valor de declaração cru (já isolado por uma fatia de lexer/cascata que este
//     item NÃO implementa -- sem DOM, sem cascata, sem casamento de seletor, ver "Escopo" abaixo),
//     produz a string canônica byte-exata que as seções 7-9 do `docs/uix-rcss.md` definem pro
//     dump. Irmão do property_registry.hpp (que só MARCA o domínio de uma propriedade,
//     `ValueDomain`) e do shorthand.hpp (que só ROTEIA os tokens crus de um shorthand pros nomes
//     longhand) -- este arquivo é a terceira perna que de fato IMPRIME um valor uma vez que o
//     domínio dele e o texto cru são conhecidos. Toda regra abaixo remonta a uma seção específica
//     do docs/uix-rcss.md, citada por função; os quatro exemplos trabalhados byte-exatos das
//     próprias seções 15.2-15.4 daquele documento são a âncora da própria suíte de teste deste
//     item (`tests/uix_style/value_compute_sanity.cpp`), reproduzidos verbatim, não parafraseados.
//
//     ESCOPO, O PRÓPRIO TETO DECLARADO DESTE ITEM:
//       - É: o `quantize()` (o próprio algoritmo explícito de arredondar-meio-pra-longe-de-zero da
//         seção 8, não `%.4f`), as formas de impressão por-domínio da seção 7 (keyword é trabalho
//         do chamador -- já é a própria string registrada exata, nada a computar --
//         número/comprimento/ângulo/porcentagem/cor/string), e as gramáticas compostas da seção 9
//         que o próprio briefing deste item nomeia: `box-shadow` (9.1), a lista de função de
//         `decorator`/`mask-image`/`filter`/`backdrop-filter` (9.2, incluindo o próprio algoritmo
//         de auto-espaçamento de stop de gradiente da 9.2.1), e `transform` (9.4, o subconjunto 2D
//         que a própria spec declara "o mais fino de propósito").
//       - NÃO É um parser de folha de estilo: toda função aqui recebe o texto de valor cru JÁ
//         ISOLADO de uma única propriedade (a forma que o `Declaration.value` de um lexer, ou uma
//         das próprias saídas `LonghandValue::value` do `shorthand::expand_shorthand`, já tem) --
//         nunca vê um bloco de regra inteiro, um seletor, ou mais de um valor de declaração por
//         vez. Decompor o PRÓPRIO valor de uma propriedade COMPOSTA na própria sub-estrutura de
//         chamada-de-função/argumento/stop (ex. as próprias camadas separadas-por-vírgula do
//         `box-shadow`, a própria lista de função separada-por-vírgula/parêntese do `decorator`) é
//         trabalho deste item -- isso é a própria gramática da seção 9 pro valor de UMA
//         propriedade, não parsing em nível de folha de estilo.
//       - NÃO É resolução de `%` (seção 5 do docs/uix-rcss.md): toda porcentagem simbólica que
//         este arquivo imprime (`print_percent`, famílias (a)/(b)/(c) igual) fica um
//         `<número>%` cru -- este módulo nunca resolve contra um containing block, um eixo de
//         gradiente, ou o próprio espaço 2D de um gradiente. A ÚNICA exceção com forma de
//         porcentagem, per o próprio texto da seção 5, é o AUTO-ESPAÇAMENTO de stop de gradiente
//         (seção 9.2.1) -- uma função pura de ÍNDICE e CONTAGEM de stop, zero geometria de caixa,
//         explicitamente autorizada a resolver apesar do resto deste módulo ficar simbólico;
//         `resolve_gradient_stop_positions()` abaixo é essa única função.
//       - NÃO É completo em unidade pra resolução de COMPRIMENTO (seção 8.1): só `px` (identidade)
//         e `dp` (`* dp_ratio`) são resolvidos -- as duas unidades que cobrem 2571 das ~2742
//         instâncias medidas tipo-comprimento do corpus (`dp` 2237 + `px` 334, per o próprio censo
//         da `RMLX-2` citado no `TODO.md`). `em`/`rem`/`vw`/`vh`/`PPI_UNIT` precisam de um contexto
//         de tamanho-de-fonte ou viewport que esta camada de função pura não tem acesso (território
//         de cascata/layout, a própria fronteira da `RMLX-3`, tabela de família de unidade da seção
//         8.1) -- `parse_length()` retorna `ValueComputeStatus::Invalid` pra qualquer um deles,
//         nunca um número silenciosamente errado. `0` sem unidade é aceito (a própria convenção de
//         comprimento-zero do CSS, necessária pros próprios offsets reais do `box-shadow` do
//         corpus, ex. os próprios tokens `0dp` do exemplo trabalhado 9.1) mas NENHUM outro número
//         sem unidade é (fail-high, não um chute).
//       - NÃO É `animation` (seção 9.3), e DELIBERADAMENTE ASSIM -- ver o próprio cabeçalho do
//         .cpp deste arquivo pra divergência que este item achou lendo direto o
//         `PropertyParserAnimation.cpp`: a própria gramática da spec
//         `animation(<keyframes-name>;<duration>;<timing-keyword>;<iterations>;<alternate>;
//         <paused>)` nunca nomeia um campo `delay`, mas o próprio struct `Animation` do upstream
//         (`examples/RmlUi/Include/RmlUi/Core/Animation.h:10-18`) tem um, populado pelo MESMO
//         parser que o próprio briefing deste item citou
//         (`PropertyParserAnimation.cpp:118-204`, o segundo token `%fs` cru depois de
//         `duration_found`). Implementar uma gramática composta pra qual o próprio contrato da
//         spec não nomeia um campo seria chutar qual forma byte-exata um futuro leitor precisa,
//         exatamente o modo de falha que o próprio cabeçalho daquele documento avisa contra --
//         reportado aqui e nas próprias notas de entrega deste item, não inventado em silêncio.
//       - Parsing de cor honra SÓ o próprio conjunto autorizado da seção 13 do docs/uix-rcss.md: as
//         4 formas hex (`#rgb`/`#rgba`/`#rrggbb`/`#rrggbbaa`) mais exatamente as 3 cores nomeadas
//         que os próprios valores iniciais deste registro e as próprias declarações medidas do
//         censo precisam (`white`, `black`, `transparent`) -- toda OUTRA cor nomeada do RmlUi e
//         toda forma de cor funcional (`rgb()`, `hsl()`, ...) é `ValueComputeStatus::Invalid`, per
//         a própria cláusula "exige aval do líder" da seção 13, não aceita em silêncio só porque o
//         parser calhou de ser fácil de estender até ali.
//
//     POLÍTICA FAIL-HIGH (seção 11 do docs/uix-rcss.md, restatada pra própria forma deste item):
//     `ValueComputeStatus::Invalid` de qualquer função de valor único (`parse_color`,
//     `parse_length`, `parse_percent`, `parse_angle`, `compute_box_shadow`,
//     `compute_linear_gradient_args`, `compute_radial_gradient_args`) significa "a própria
//     declaração do chamador está malformada" -- o chamador decide o que isso significa pro
//     próprio contexto dele (uma futura fatia de cascata derruba a declaração inteira, per a
//     própria regra uniforme da seção 11), mesma disciplina "o próprio trabalho deste módulo para
//     em classificar o input" que property_registry.hpp/shorthand.hpp já estabelecem.
//     `compute_decorator_list()`/`compute_transform_list()` são a ÚNICA exceção a retornar um
//     status: per o próprio bullet "esta entrada de decorator é derrubada da própria lista da
//     propriedade dela" da seção 11, um nome de função não-reconhecido ou uma forma de argumento
//     malformada dentro de uma LISTA composta derruba só AQUELA entrada, nunca a lista inteira --
//     então estas duas funções não conseguem falhar como um todo; um input que produz zero
//     entradas válidas imprime o literal `none` (a própria regra de lista-vazia da seção 9), nunca
//     uma string vazia, nunca uma pane. Nada neste arquivo loga (mesmo precedente de
//     zero-logging-dentro-de-um-módulo-standalone que property_registry.hpp/shorthand.hpp já
//     estabelecem) e nada neste arquivo lança exceção.
//
//     TETO (tetos rígidos, todos fail-high, nunca truncam em silêncio): `kMaxRawValueBytes` (8 KiB
//     -- o maior valor de declaração RCSS medido no próprio corpus deste repo tem menos de 200
//     bytes, `/var/tmp/censo-rcss-qa1/censo.md`; margem ~40x) delimita toda função abaixo que
//     recebe um valor cru inteiro, e `kMaxNestDepth` (8) delimita o próprio argumento `fill`
//     recursivo do `polygon()` (um `linear-gradient(...)`/`radial-gradient(...)` aninhado, a
//     própria tabela da seção 9.2 do docs/uix-rcss.md) -- o corpus nunca aninha mais de 1 nível de
//     profundidade, este teto existe só pra um input hostil/gerado-por-revisão-adversarial não
//     recursar sem limite, nunca pra servir um caso de uso real além do que o corpus já mostra.
//
//     NAMESPACE: `glintfx::uix::style`, o mesmo módulo do lexer.hpp/property_registry.hpp/
//     shorthand.hpp.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace glintfx::uix::style {

// EN: See this file's header, "Fail-high policy".
// PT: Ver o "Política fail-high" do cabeçalho deste arquivo.
enum class ValueComputeStatus {
  Ok,
  Invalid,
};

// EN: docs/uix-rcss.md section 8's own ceiling for every whole-raw-value-taking function below --
//     see header "Teto".
// PT: O próprio teto da seção 8 do docs/uix-rcss.md pra toda função que recebe um valor cru
//     inteiro abaixo -- ver "Teto" no cabeçalho.
inline constexpr std::size_t kMaxRawValueBytes = 8192;
inline constexpr int kMaxNestDepth = 8;

// EN: docs/uix-rcss.md section 8's own explicit algorithm (widen to double, scale by 10000, round
//     half-away-from-zero via trunc(scaled + copysign(0.5, scaled)), format as fixed 4-decimal-
//     digit ASCII, '-' prefix iff negative, -0.0 canonicalized to 0.0) -- implemented WITHOUT any
//     libc float-formatting function (no `%f`/`std::to_chars` for the fractional digits): the
//     rounding decision happens once, in `double`, per the algorithm's own text; converting the
//     resulting EXACT integer (`std::trunc`'s own result) to decimal digits is then pure integer
//     arithmetic, never a second rounding decision a libc could make differently. Non-finite input
//     (`NaN`/`Inf`) is not addressed by the spec's own algorithm text -- this implementation
//     defensively canonicalizes it to `"0.0000"` (never a literal `"nan"`/`"inf"` string reaching a
//     dump line) rather than propagating undefined text, a decision this file's own delivery notes
//     name explicitly rather than leaving silent.
// PT: O próprio algoritmo explícito da seção 8 do docs/uix-rcss.md (amplia pra double, escala por
//     10000, arredonda meio-pra-longe-de-zero via trunc(scaled + copysign(0.5, scaled)), formata
//     como ASCII ponto-fixo de 4 casas decimais, prefixo '-' sse negativo, -0.0 canonicalizado pra
//     0.0) -- implementado SEM nenhuma função de formatação de float da libc (nenhum `%f`/
//     `std::to_chars` pros dígitos fracionários): a decisão de arredondamento acontece uma vez, em
//     `double`, per o próprio texto do algoritmo; converter o inteiro EXATO resultante (o próprio
//     resultado do `std::trunc`) pra dígitos decimais é então aritmética inteira pura, nunca uma
//     segunda decisão de arredondamento que uma libc pudesse tomar diferente. Input não-finito
//     (`NaN`/`Inf`) não é endereçado pelo próprio texto do algoritmo da spec -- esta implementação
//     canonicaliza defensivamente pra `"0.0000"` (nunca uma string literal `"nan"`/`"inf"`
//     chegando numa linha de dump) em vez de propagar texto indefinido, uma decisão que as próprias
//     notas de entrega deste item nomeiam explicitamente em vez de deixar em silêncio.
std::string quantize(float x);

// EN: docs/uix-rcss.md section 7's own `number` row -- `quantize(x)`, no suffix, no sign for
//     positive, verbatim.
// PT: A própria linha `number` da seção 7 do docs/uix-rcss.md -- `quantize(x)`, sem sufixo, sem
//     sinal pro positivo, verbatim.
std::string print_number(float x);

// EN: docs/uix-rcss.md section 8.2's own newly-closed print-form rule -- `quantize(x)`'s own
//     unmodified output, NO `deg` suffix (unlike `length`, which section 8.1 explicitly overrides
//     with an added `px` suffix; angle has no such override, so it falls through to `quantize()`
//     bare). `degrees` is already resolved to degrees by the caller (`degrees_from_radians()`
//     below, if the source used `rad`) -- this function does not itself know or care which source
//     unit produced its argument.
// PT: A própria regra de forma-de-impressão recém-fechada da seção 8.2 do docs/uix-rcss.md -- a
//     própria saída não-modificada do `quantize(x)`, SEM sufixo `deg` (diferente de `length`, que
//     a seção 8.1 sobrescreve explicitamente com um sufixo `px` somado; ângulo não tem essa
//     sobrescrita, então cai pro `quantize()` cru). `degrees` já vem resolvido pra graus pelo
//     chamador (`degrees_from_radians()` abaixo, se a fonte usou `rad`) -- esta função não sabe
//     nem se importa sozinha qual unidade de fonte produziu o próprio argumento dela.
std::string print_angle_deg(float degrees);

// EN: docs/uix-rcss.md section 5's own symbolic print form -- `quantize(x)` + literal `%` suffix.
//     Used identically for ALL three `%` families (a/b/c, section 5's own table) AND for gradient-
//     stop positions after auto-spacing (section 9.2.1) -- the families differ in what base a
//     FUTURE resolver divides by, never in how THIS function prints the bare number, per section
//     5's own "one symbolic print form" title.
// PT: A própria forma de impressão simbólica da seção 5 do docs/uix-rcss.md -- `quantize(x)` +
//     sufixo `%` literal. Usada identicamente pras TRÊS famílias de `%` (a/b/c, a própria tabela da
//     seção 5) E pras posições de stop de gradiente pós-auto-espaçamento (seção 9.2.1) -- as
//     famílias diferem em qual base um FUTURO resolvedor divide, nunca em como ESTA função imprime
//     o número cru, per o próprio título "uma forma de impressão simbólica" da seção 5.
std::string print_percent(float pct);

// EN: docs/uix-rcss.md section 8.1's own override -- `quantize(x)` + literal `px` suffix, ALWAYS
//     `px` regardless of the source unit (`dp` and `px` both resolve to this same printed unit).
//     `resolved_px` is already resolved (via `resolve_length_px()` below) -- this function only
//     prints.
// PT: A própria sobrescrita da seção 8.1 do docs/uix-rcss.md -- `quantize(x)` + sufixo `px`
//     literal, SEMPRE `px` independente da unidade de fonte (`dp` e `px` os dois resolvem pro mesmo
//     unidade impressa). `resolved_px` já vem resolvido (via `resolve_length_px()` abaixo) -- esta
//     função só imprime.
std::string print_length_px(float resolved_px);

// EN: docs/uix-rcss.md section 8.2's own conversion, `degrees = radians * (180 / pi)`, verbatim.
// PT: A própria conversão da seção 8.2 do docs/uix-rcss.md, `degrees = radians * (180 / pi)`,
//     verbatim.
float degrees_from_radians(float radians);

// EN: docs/uix-rcss.md section 7's own `string` row -- strips ONE layer of surrounding matching
//     quote characters (`"..."` or `'...'`) if present (the value was authored quoted, per that
//     row's own "no surrounding quotes even if the RCSS source quoted it" clause -- quoting is
//     source syntax, not part of the computed string), then escapes per `docs/uix-dom.md` section
//     2's own 4-rule table (`\`->`\\`, `\n`->`\n` literal-backslash-n, `\r`->`\r`,
//     `\t`->`\t`), applied in that exact order so the escape marker itself is never ambiguous, same
//     rule this document's own section 3 cites by reference rather than reinventing.
// PT: A própria linha `string` da seção 7 do docs/uix-rcss.md -- tira UMA camada de aspas
//     casadas ao redor (`"..."` ou `'...'`) se presente (o valor foi autorado entre aspas, per a
//     própria cláusula "sem aspas ao redor mesmo que a fonte RCSS tenha citado" daquela linha --
//     aspas são sintaxe de fonte, não parte da string computada), depois escapa per a própria
//     tabela de 4 regras da seção 2 do `docs/uix-dom.md` (`\`->`\\`, `\n`->backslash-n literal,
//     `\r`->backslash-r, `\t`->backslash-t), aplicadas nesta ordem exata pro próprio marcador de
//     escape nunca ficar ambíguo, mesma regra que a própria seção 3 deste documento cita por
//     referência em vez de reinventar.
std::string print_string(std::string_view raw);

// EN: docs/uix-rcss.md section 7.1's own canonical color -- see header "Scope" for the exact
//     authorized set (4 hex forms, 3 named colors). `raw` must already be whitespace-trimmed by
//     the caller (same "this module does not re-derive what a lexer already stripped" convention
//     property_registry.hpp/shorthand.hpp hold themselves to).
// PT: A própria cor canônica da seção 7.1 do docs/uix-rcss.md -- ver "Escopo" no cabeçalho pro
//     próprio conjunto autorizado exato (4 formas hex, 3 cores nomeadas). `raw` já precisa vir
//     whitespace-trimado pelo chamador (mesma convenção "este módulo não re-deriva o que um lexer
//     já tirou" que property_registry.hpp/shorthand.hpp se prendem).
struct Rgba8 {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 0xff;
};
ValueComputeStatus parse_color(std::string_view raw, Rgba8* out);

// EN: 8-digit lowercase `#rrggbbaa`, straight (non-premultiplied) alpha -- docs/uix-rcss.md
//     section 7.1's own exact form.
// PT: `#rrggbbaa` de 8 dígitos minúsculos, alpha reto (não-premultiplicado) -- a própria forma
//     exata da seção 7.1 do docs/uix-rcss.md.
std::string print_color(const Rgba8& c);

// EN: docs/uix-rcss.md section 8.1's own two resolved units -- see header "Scope" for why only
//     these two (`em`/`rem`/`vw`/`vh` are `ValueComputeStatus::Invalid`, not silently guessed).
//     Unitless `0` parses as `{0, Px}` (CSS's own zero-length convention).
// PT: As próprias duas unidades resolvidas da seção 8.1 do docs/uix-rcss.md -- ver "Escopo" no
//     cabeçalho pro porquê só destas duas (`em`/`rem`/`vw`/`vh` são `ValueComputeStatus::Invalid`,
//     não chutadas em silêncio). `0` sem unidade parseia como `{0, Px}` (a própria convenção de
//     comprimento-zero do CSS).
enum class LengthUnit {
  Px,
  Dp,
};
ValueComputeStatus parse_length(std::string_view raw, float* out_value, LengthUnit* out_unit);

// EN: `px` is identity, `dp` multiplies by `dp_ratio` -- docs/uix-rcss.md section 8.1's own two
//     resolved members of the `LENGTH` unit family this module supports.
// PT: `px` é identidade, `dp` multiplica por `dp_ratio` -- os próprios dois membros resolvidos da
//     família de unidade `LENGTH` da seção 8.1 do docs/uix-rcss.md que este módulo suporta.
float resolve_length_px(float value, LengthUnit unit, float dp_ratio);

// EN: Parses a bare `<number>%` token (e.g. `"50%"`) into its numeric part -- does NOT resolve
//     against any base (section 5's own "stays symbolic" rule); the caller decides which of the
//     three families (section 5's own table) this percent belongs to.
// PT: Parseia um token `<número>%` cru (ex. `"50%"`) pra própria parte numérica -- NÃO resolve
//     contra base nenhuma (a própria regra "fica simbólico" da seção 5); o chamador decide a qual
//     das três famílias (a própria tabela da seção 5) esta porcentagem pertence.
ValueComputeStatus parse_percent(std::string_view raw, float* out_percent);

// EN: Parses `<number>deg` or `<number>rad` (section 8.2's own accepted input units, "full unit
//     parity" per the líder's own decision) into DEGREES already (rad->deg conversion applied
//     here, per `degrees_from_radians()` above) -- the result is ready for `print_angle_deg()`
//     directly.
// PT: Parseia `<número>deg` ou `<número>rad` (as próprias unidades de entrada aceitas da seção 8.2,
//     "paridade completa de unidade" per a própria decisão do líder) já pra GRAUS (conversão
//     rad->deg aplicada aqui, per `degrees_from_radians()` acima) -- o resultado já está pronto pro
//     `print_angle_deg()` direto.
ValueComputeStatus parse_angle(std::string_view raw, float* out_deg);

// EN: docs/uix-rcss.md section 9.2.1's own auto-spacing algorithm, verbatim, as a PURE function of
//     stop count and which stops carry an explicit position -- zero box geometry, zero parsing,
//     exactly the "resolved despite everything else staying symbolic" exception section 5 names.
//     `explicit_positions_percent[i]` is `std::nullopt` iff stop `i` had no explicit `<position%>`
//     in the source; the result is always `explicit_positions_percent.size()` entries, every one
//     assigned (first->0, last->100, interior runs evenly spaced between their nearest assigned
//     neighbors, per the spec's own 4-step algorithm). Empty input returns an empty vector (no
//     stops to space).
// PT: O próprio algoritmo de auto-espaçamento da seção 9.2.1 do docs/uix-rcss.md, verbatim, como
//     função PURA de contagem de stop e quais stops carregam posição explícita -- zero geometria
//     de caixa, zero parsing, exatamente a exceção "resolvido apesar de tudo mais ficar simbólico"
//     que a seção 5 nomeia. `explicit_positions_percent[i]` é `std::nullopt` sse o stop `i` não
//     tinha `<position%>` explícito na fonte; o resultado sempre tem
//     `explicit_positions_percent.size()` entradas, todas atribuídas (primeiro->0, último->100,
//     trechos interiores igualmente espaçados entre os próprios vizinhos atribuídos mais próximos,
//     per o próprio algoritmo de 4 passos da spec). Input vazio retorna um vetor vazio (nenhum stop
//     pra espaçar).
std::vector<float> resolve_gradient_stop_positions(
    const std::vector<std::optional<float>>& explicit_positions_percent);

// EN: docs/uix-rcss.md section 9.1's own full grammar, given `box-shadow`'s RAW declaration value
//     text (the whole property value, comma-separated layers included) -- see this file's own
//     .cpp header for the upstream mechanics this traces (`PropertyParserBoxShadow.cpp`, read
//     directly, including a genuine spec-vs-upstream divergence this item found and reports rather
//     than silently working around). `*out` is `"none"` for an empty/`"none"` input (section 9's
//     own empty-list rule), the `|`-joined layer string otherwise. Returns `Invalid` for a layer
//     this module cannot classify (an unrecognised token, fewer than 2 length-shaped tokens, or no
//     color found in a layer -- the last a documented SIMPLIFICATION this item's own delivery notes
//     flag as unverified against `Colourb`'s own default constructor, not a silent guess presented
//     as fact) -- per section 11's own uniform rule, an `Invalid` here means the CALLER drops the
//     whole `box-shadow` declaration, same as any other malformed value.
// PT: A própria gramática completa da seção 9.1 do docs/uix-rcss.md, dado o texto de valor de
//     declaração CRU do `box-shadow` (o valor de propriedade inteiro, camadas separadas-por-vírgula
//     incluídas) -- ver o próprio cabeçalho do .cpp deste arquivo pra mecânica do upstream que isto
//     remonta (`PropertyParserBoxShadow.cpp`, lido direto, incluindo uma divergência genuína
//     spec-vs-upstream que este item achou e reporta em vez de contornar em silêncio). `*out` é
//     `"none"` pra um input vazio/`"none"` (a própria regra de lista-vazia da seção 9), a própria
//     string de camada unida por `|` senão. Retorna `Invalid` pra uma camada que este módulo não
//     consegue classificar (um token não-reconhecido, menos de 2 tokens com forma de comprimento,
//     ou nenhuma cor achada numa camada -- a última uma SIMPLIFICAÇÃO documentada que as próprias
//     notas de entrega deste item sinalizam como não-verificada contra o próprio construtor default
//     do `Colourb`, não um chute silencioso apresentado como fato) -- per a própria regra uniforme
//     da seção 11, um `Invalid` aqui significa que o CHAMADOR derruba a declaração `box-shadow`
//     inteira, igual a qualquer outro valor malformado.
ValueComputeStatus compute_box_shadow(std::string_view raw_value, float dp_ratio, std::string* out);

// EN: docs/uix-rcss.md section 9.2's own `linear-gradient(...)` argument grammar, given the text
//     BETWEEN the outer parentheses (e.g. `"90deg, #FF0000 20%, #00FF00 80%"`) -- produces the
//     `;`-joined args string WITHOUT the `linear-gradient(...)` wrapper (`compute_decorator_list()`
//     below adds that). Exposed standalone (not just via the decorator list) because `polygon()`'s
//     own `<fill>` argument can nest this same grammar recursively (section 9.2's own table).
// PT: A própria gramática de argumento do `linear-gradient(...)` da seção 9.2 do docs/uix-rcss.md,
//     dado o texto ENTRE os parênteses externos (ex. `"90deg, #FF0000 20%, #00FF00 80%"`) --
//     produz a string de args unida por `;` SEM o envoltório `linear-gradient(...)` (o
//     `compute_decorator_list()` abaixo soma isso). Exposta standalone (não só via a lista de
//     decorator) porque o próprio argumento `<fill>` do `polygon()` pode aninhar esta mesma
//     gramática recursivamente (a própria tabela da seção 9.2).
ValueComputeStatus compute_linear_gradient_args(std::string_view inner_args, std::string* out);

// EN: docs/uix-rcss.md section 9.2's own `radial-gradient(...)` argument grammar -- same shape as
//     `compute_linear_gradient_args()` above, plus the `circle at <cx%> <cy%>` center clause
//     (default `50.0000%;50.0000%` when omitted, per section 9.2's own table) and the `ellipse`
//     fail-high case (section 13's own out-of-scope clause).
// PT: A própria gramática de argumento do `radial-gradient(...)` da seção 9.2 do docs/uix-rcss.md
//     -- mesma forma do `compute_linear_gradient_args()` acima, mais a própria cláusula de centro
//     `circle at <cx%> <cy%>` (default `50.0000%;50.0000%` quando omitida, per a própria tabela da
//     seção 9.2) e o próprio caso fail-high de `ellipse` (a própria cláusula fora-de-escopo da
//     seção 13).
ValueComputeStatus compute_radial_gradient_args(std::string_view inner_args, std::string* out);

// EN: docs/uix-rcss.md section 9.2's own full function-LIST grammar, shared by `decorator`/
//     `mask-image`/`filter`/`backdrop-filter`. Dispatches each `name(args)` entry to one of the 10
//     in-scope functions section 9.2's own table names (`image`, `linear-gradient`,
//     `radial-gradient`, `polygon`, `image-tint`, `ripple`, `horizontal-gradient`,
//     `vertical-gradient`, `blur`, `drop-shadow`); `dp_ratio` is only consulted by the functions
//     with a resolved-length argument (`blur`, `drop-shadow`). `raw_value`'s own SOURCE separator
//     between multiple functions differs by property (`decorator`/`mask-image` use `,`;
//     `filter`/`backdrop-filter` use ` `, section 9.2's own `UIX-RCSS-ERRATA-2` correction) -- this
//     function's own internal `scan_function_calls()` scanner is deliberately agnostic to which one
//     it sees (both are skipped identically as inter-call separator), so ONE function correctly
//     serves all four properties without the caller needing to pick a mode. **Fails as a whole**
//     (`ValueComputeStatus::Invalid`, `*out` untouched) the moment ANY entry is unrecognised or
//     malformed -- see header "Fail-high policy" (`UIX-RCSS-ERRATA-2`'s own correction to
//     `Finding C`). `*out` is `"none"` only for a genuinely empty/`"none"` input, with `Ok`.
// PT: A própria gramática de LISTA de função completa da seção 9.2 do docs/uix-rcss.md,
//     compartilhada por `decorator`/`mask-image`/`filter`/`backdrop-filter`. Despacha cada entrada
//     `name(args)` pra uma das 10 funções dentro-de-escopo que a própria tabela da seção 9.2 nomeia
//     (`image`, `linear-gradient`, `radial-gradient`, `polygon`, `image-tint`, `ripple`,
//     `horizontal-gradient`, `vertical-gradient`, `blur`, `drop-shadow`); `dp_ratio` só é consultado
//     pelas funções com argumento de comprimento resolvido (`blur`, `drop-shadow`). O próprio
//     separador de FONTE do `raw_value` entre múltiplas funções difere por propriedade
//     (`decorator`/`mask-image` usam `,`; `filter`/`backdrop-filter` usam ` `, a própria correção
//     `UIX-RCSS-ERRATA-2` da seção 9.2) -- o próprio escaneador interno `scan_function_calls()`
//     desta função é deliberadamente agnóstico a qual dos dois ele vê (os dois são pulados
//     identicamente como separador inter-chamada), então UMA função serve corretamente as quatro
//     propriedades sem o chamador precisar escolher um modo. **Falha como um todo**
//     (`ValueComputeStatus::Invalid`, `*out` intocado) no momento em que QUALQUER entrada é
//     não-reconhecida ou malformada -- ver "Política fail-high" no cabeçalho (a própria correção da
//     `UIX-RCSS-ERRATA-2` ao `Finding C`). `*out` é `"none"` só pra um input genuinamente vazio/
//     `"none"`, com `Ok`.
ValueComputeStatus compute_decorator_list(std::string_view raw_value, float dp_ratio,
                                          std::string* out);

// EN: docs/uix-rcss.md section 9.4's own explicitly-thin `transform` grammar
//     (`translate(<x>;<y>) | scale(<x>;<y>) | rotate(<angle>)`) -- the 2D subset verified against
//     the corpus's own 2 measured instances (both `rotate(...)`), per that section's own stated
//     limit. Same whole-list-fails-together shape as `compute_decorator_list()` above (section 11's
//     own uniform malformed-entry policy applied consistently across every composite-list domain
//     this module implements, not just the two upstream-parser-cited ones).
// PT: A própria gramática explicitamente fina de `transform` da seção 9.4 do docs/uix-rcss.md
//     (`translate(<x>;<y>) | scale(<x>;<y>) | rotate(<angle>)`) -- o subconjunto 2D verificado
//     contra as próprias 2 instâncias medidas do corpus (as duas `rotate(...)`), per o próprio
//     limite declarado daquela seção. Mesma forma de lista-falha-junto do
//     `compute_decorator_list()` acima (a própria política uniforme de entrada-malformada da seção
//     11 aplicada consistentemente em todo domínio de lista composta que este módulo implementa,
//     não só nos dois citados-do-parser-upstream).
ValueComputeStatus compute_transform_list(std::string_view raw_value, float dp_ratio,
                                          std::string* out);

} // namespace glintfx::uix::style
