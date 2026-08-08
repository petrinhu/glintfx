// SPDX-License-Identifier: Apache-2.0
// EN: `UIX-RCSS-DUMP-A` implementation. See rcss_dump.hpp's own header comment for the
//     confinement rationale and the oracle-independence discipline this file follows.
//
//     Value source discipline (docs/uix-rcss.md's own header names `Style::ComputedValues` /
//     `Element::GetProperty` as the sanctioned sources -- this file uses BOTH, per-domain, never
//     guessing which is right by trial and error):
//       - `Style::ComputedValues` (`Element::GetComputedValues()`) is used ONLY where RmlUi's own
//         cascade-time computation does real, non-trivial work this file would otherwise have to
//         re-derive incorrectly: `font-size` (the em/rem ancestor-chain walk, ComputeValues.cpp's
//         own "always do font-size first" special case -- resolving via a raw Property + this
//         element's OWN ResolveLength would be circular for an em-relative font-size), and
//         `line-height`/`vertical-align` (both resolve against a base -- font-size,
//         line-height -- RmlUi's own ComputeProperty.cpp already computes correctly; re-deriving
//         it here would risk the exact "two engines quietly disagree on a formula neither
//         wrote down" class of bug section 5's own opening paragraph warns about).
//       - `Element::GetProperty(name)` (the raw, CASCADED-but-not-yet-instanced Property) is used
//         for every other domain. Two load-bearing, cited discoveries this choice depends on:
//         (1) `PropertyDefinition::GetValue()`'s KEYWORD case (`PropertyDefinition.cpp:97-129`)
//         returns the exact registered keyword string via `Property::ToString()` -- so this file
//         never hand-rolls an enum-to-string table for `keyword`-domain properties (avoids the
//         "second table that can silently drift from the first" version of the same risk).
//         (2) `decorator`/`mask-image`/`filter`/`backdrop-filter` are stored, at the Property
//         level this file reads, as `DecoratorDeclarationList`/`FilterDeclarationList`
//         (`StyleSheetTypes.h:33-48`) -- a list of PARSED-BUT-NOT-YET-INSTANCED sub-property
//         dictionaries, one per function in the comma list, each queryable BY NAME via that
//         function's own `DecoratorInstancer`/`FilterInstancer::GetPropertySpecification()`
//         (`EffectSpecification.h:19`, PUBLIC). This is dramatically more tractable than
//         introspecting the fully-instanced, render-only `Decorator`/`Filter` object graph
//         (which has no public accessor for e.g. a gradient's own stop list) -- confirmed by
//         reading `DecoratorGradient.cpp`/`PropertyParserDecorator.cpp` directly, not assumed.
//
//     🔴 SPEC-VS-UPSTREAM DIVERGENCE -- SETTLED, SECOND PASS, BY THE LÍDER, IN FAVOR OF STRAIGHT
//     ALPHA (reversing this file's own first correction). `docs/uix-rcss.md` section 7.1 claims
//     box-shadow/gradient-`<stop>` colors are dumped straight-alpha. `DecorationTypes.h:9`/`:22`
//     declare `ColourbPremultiplied color;` on both `ColorStop`/`BoxShadow` BY TYPE, so a naive
//     "print the cascade-domain field verbatim" reading (this file's own FIRST attempt at fixing
//     the spec-vs-code mismatch) concluded premultiplied-as-is was the only fact the engine
//     actually holds. That attempt was itself refuted, in TWO independent steps, by the
//     orchestrator's own live audit: (1) the "printing straight would be UB, `alpha=0` divides by
//     zero" argument that justified premultiplied-as-is does not hold -- `Colour.h:105-107`'s own
//     `ToNonPremultiplied()` guards `alpha > 0 ? (red*255)/alpha : 0`, well-defined for every
//     input, no division by zero anywhere; (2) real upstream RmlUi ITSELF un-premultiplies at the
//     exact moment it serializes these two composites to text --
//     `TypeConverter.cpp:223`/`:256` call `ToString(stop.color.ToNonPremultiplied())`/
//     `ToString(shadow.color.ToNonPremultiplied())` respectively. This dump IS a textual
//     serialization of a computed value -- the real engine's own answer to "what text represents
//     this color?" is the straight-alpha one, not the premultiplied storage representation this
//     file's first fix mistook for the canonical fact. **Decision, argued on SCOPE, not
//     aesthetics:** requiring the clean-room engine (`glintfx/src/uix/style/`, the "other side"
//     this dumper's own independence rule forbids reading) to reproduce upstream's internal
//     premultiplied STORAGE representation -- rather than the value upstream itself prints when
//     asked for text -- would be exactly the kind of RmlUi-internal-detail leak `RMLX-2`'s own
//     internalization goal exists to avoid.
//
//     🔴 THE LOSSY STEP IS REAL AND IRREVERSIBLE -- MEASURED, NOT MERELY ASSERTED (the single
//     most likely source of a byte divergence, per the orchestrator's own audit): the printed
//     value is NOT the author's own written value. Truncating-integer-division premultiply at
//     PARSE time (`Colour.h:76-82`) already discarded information before this file's read point;
//     un-premultiplying (`ToNonPremultiplied()`, also truncating) does not recover it. Three
//     measured anchors: `#22D3EE80` (alpha=0x80=128) is stored as `#11697780`
//     (`r'=(0x22*128)/255=17`, `g'=(0xd3*128)/255=105`, `b'=(0xee*128)/255=119`) and this file now
//     prints `#21d1ed80` (`r=(0x11*255)/128=33`, `g=(0x69*255)/128=209`, `b=(0x77*255)/128=237`)
//     -- NOT the author's own `#22d3ee80`; `#C9A24B40` (alpha=0x40=64) is stored as `#32281240`
//     and this file now prints `#c79f4740`; `#22D3EE00` (alpha=0) is stored as `#00000000`
//     (every channel truncates to `(x*0)/255=0` regardless of the original RGB) and prints
//     `#00000000` -- both readings coincide here (`ToNonPremultiplied()`'s own `alpha > 0` guard
//     also returns `0`), the ONE case where the two implementations cannot be told apart by byte
//     output alone. `rcss_dump_worked_examples.cpp`'s own goldens carry this same table, so a
//     second implementer reading only the golden values (not this comment) still has the anchor.
//
//     Two SIBLING findings the orchestrator's live audit reported, confirmed here as ALREADY
//     harmless to this file by construction, not requiring a code change (recorded so a future
//     reader does not re-litigate them against this file specifically): (a) `filter`/
//     `backdrop-filter` are SPACE-separated at the raw-RCSS level (`PropertyParserFilter.cpp:32`,
//     not the comma-list section 9.2's own prose claims for all four composite properties), and
//     (b) one malformed entry inside a `filter`/`backdrop-filter` declaration drops the WHOLE
//     property (`PropertyParserFilter.cpp`'s per-entry `return false`, not a per-entry `continue`
//     section 11's prose implies). Neither affects this file: this dumper never re-splits a raw
//     `filter`/`backdrop-filter` string itself -- it reads `Element::GetProperty("filter")`'s own
//     ALREADY-VALIDATED `FilterDeclarationList` (`filter_list_value()` below), which upstream's
//     own parser has already fully accepted-or-rejected AS A WHOLE, by the real separator and the
//     real malformed-entry consequence, before this file ever sees it. This file's own "drop just
//     this one entry, keep the rest" fallback in `filter_entry_value()`/`decorator_entry_value()`
//     fires ONLY for a function name/shape THIS FILE does not (yet) know how to serialize (an
//     honest scope gap, section 11's fail-high policy applied to the DUMPER's own coverage, not a
//     re-implementation of upstream's malformed-value handling) -- it is never reached for RCSS
//     upstream itself already rejected, because that case never reaches this file as a partial
//     list entry to begin with.
// PT: Implementação da `UIX-RCSS-DUMP-A`. Ver o próprio comentário de cabeçalho de rcss_dump.hpp
//     pro racional de confinamento e a disciplina de independência-de-oráculo que este arquivo
//     segue.
//
//     Disciplina de fonte de valor (o próprio cabeçalho do docs/uix-rcss.md nomeia
//     `Style::ComputedValues`/`Element::GetProperty` como as fontes sancionadas -- este arquivo
//     usa AS DUAS, por domínio, nunca advinhando qual é a certa por tentativa e erro):
//       - `Style::ComputedValues` (`Element::GetComputedValues()`) é usado SÓ onde a própria
//         computação de tempo-de-cascata do RmlUi faz trabalho real, não-trivial, que este
//         arquivo teria que re-derivar incorretamente de outro jeito: `font-size` (a caminhada
//         pela cadeia de ancestrais em em/rem, o próprio caso especial "sempre faz font-size
//         primeiro" do ComputeValues.cpp -- resolver via Property crua + o ResolveLength DESTE
//         PRÓPRIO elemento seria circular pra um font-size relativo-em-em), e
//         `line-height`/`vertical-align` (os dois resolvem contra uma base -- font-size,
//         line-height -- que o próprio ComputeProperty.cpp do RmlUi já computa corretamente;
//         re-derivar aqui arriscaria exatamente a classe de bug "dois motores discordam em
//         silêncio de uma fórmula que nenhum dos dois escreveu por extenso" que o próprio
//         parágrafo de abertura da seção 5 avisa).
//       - `Element::GetProperty(nome)` (a Property crua, CASCATEADA-mas-ainda-não-instanciada) é
//         usado pra todo outro domínio. Duas descobertas citadas, com peso estrutural, das quais
//         esta escolha depende: (1) o caso KEYWORD do `PropertyDefinition::GetValue()`
//         (`PropertyDefinition.cpp:97-129`) devolve a string exata da keyword registrada via
//         `Property::ToString()` -- então este arquivo nunca escreve à mão uma tabela
//         enum-pra-string pras propriedades de domínio `keyword` (evita a versão "segunda tabela
//         que pode divergir da primeira em silêncio" do mesmo risco). (2)
//         `decorator`/`mask-image`/`filter`/`backdrop-filter` são guardados, no nível de Property
//         que este arquivo lê, como `DecoratorDeclarationList`/`FilterDeclarationList`
//         (`StyleSheetTypes.h:33-48`) -- uma lista de dicionários de sub-propriedade JÁ
//         PARSEADOS-MAS-AINDA-NÃO-INSTANCIADOS, um por função da lista separada por vírgula, cada
//         um consultável POR NOME via o próprio
//         `DecoratorInstancer`/`FilterInstancer::GetPropertySpecification()` daquela função
//         (`EffectSpecification.h:19`, PÚBLICO). Isto é dramaticamente mais tratável que
//         introspectar o grafo de objeto `Decorator`/`Filter` já instanciado, só-de-render (que
//         não tem accessor público nenhum pra, ex., a própria lista de stops de um gradiente) --
//         confirmado lendo `DecoratorGradient.cpp`/`PropertyParserDecorator.cpp` direto, não
//         suposto.
//
//     🔴 DIVERGÊNCIA SPEC-VS-UPSTREAM -- FECHADA, SEGUNDA PASSADA, PELO LÍDER, A FAVOR DE ALFA
//     RETO (revertendo a primeira correção deste próprio arquivo). A seção 7.1 do docs/uix-rcss.md
//     afirma que cor de box-shadow/`<stop>` de gradiente é dumpada reta. `DecorationTypes.h:9`/
//     `:22` declaram `ColourbPremultiplied color;` em `ColorStop`/`BoxShadow` POR TIPO, então uma
//     leitura ingênua "imprime o campo de domínio-cascata ao pé da letra" (a PRIMEIRA tentativa
//     deste arquivo de consertar a discrepância spec-vs-código) concluiu que
//     premultiplicado-como-está era o único fato que o motor de fato guarda. Aquela tentativa foi
//     ela mesma refutada, em DOIS passos independentes, pela própria auditoria ao vivo do
//     orquestrador: (1) o argumento "imprimir reto seria UB, `alpha=0` divide por zero" que
//     justificava premultiplicado-como-está não se sustenta -- o próprio `ToNonPremultiplied()`
//     de `Colour.h:105-107` guarda `alpha > 0 ? (red*255)/alpha : 0`, bem-definido pra toda
//     entrada, divisão por zero nenhuma em lugar nenhum; (2) o próprio RmlUi upstream real
//     des-premultiplica no EXATO momento em que serializa estes dois compostos pra texto --
//     `TypeConverter.cpp:223`/`:256` chamam
//     `ToString(stop.color.ToNonPremultiplied())`/`ToString(shadow.color.ToNonPremultiplied())`
//     respectivamente. Este dump É uma serialização textual de valor computado -- a própria
//     resposta do motor real pra "que texto representa esta cor?" é a reta, não a representação
//     de ARMAZENAMENTO premultiplicado que o primeiro conserto deste arquivo confundiu com o fato
//     canônico. **Decisão, argumentada por ESCOPO, não estética:** exigir do motor clean-room
//     (`glintfx/src/uix/style/`, o "outro lado" que a própria regra de independência deste dumper
//     proíbe ler) reproduzir a representação de ARMAZENAMENTO interna premultiplicada do upstream
//     -- em vez do valor que o próprio upstream imprime quando perguntado por texto -- seria
//     exatamente o tipo de vazamento de detalhe-interno-do-RmlUi que o próprio objetivo de
//     internalização da `RMLX-2` existe pra evitar.
//
//     🔴 O PASSO COM PERDA É REAL E IRREVERSÍVEL -- MEDIDO, NÃO MERAMENTE AFIRMADO (a fonte mais
//     provável de uma divergência de byte, pela própria auditoria do orquestrador): o valor
//     impresso NÃO é o valor escrito pelo autor. A divisão inteira truncante de premultiplicar em
//     tempo de PARSE (`Colour.h:76-82`) já descartou informação antes do ponto de leitura deste
//     arquivo; des-premultiplicar (`ToNonPremultiplied()`, também truncante) não a recupera. Três
//     âncoras medidas: `#22D3EE80` (alfa=0x80=128) é guardado como `#11697780`
//     (`r'=(0x22*128)/255=17`, `g'=(0xd3*128)/255=105`, `b'=(0xee*128)/255=119`) e este arquivo
//     agora imprime `#21d1ed80` (`r=(0x11*255)/128=33`, `g=(0x69*255)/128=209`,
//     `b=(0x77*255)/128=237`) -- NÃO o `#22d3ee80` do próprio autor; `#C9A24B40` (alfa=0x40=64) é
//     guardado como `#32281240` e este arquivo agora imprime `#c79f4740`; `#22D3EE00` (alfa=0) é
//     guardado como `#00000000` (todo canal trunca pra `(x*0)/255=0` independente do RGB
//     original) e imprime `#00000000` -- as duas leituras coincidem aqui (a própria guarda
//     `alpha > 0` do `ToNonPremultiplied()` também devolve `0`), o ÚNICO caso em que as duas
//     implementações não podem ser distinguidas só pelo byte de saída. Os próprios goldens do
//     `rcss_dump_worked_examples.cpp` carregam esta mesma tabela, então um segundo implementer
//     lendo só os valores-golden (não este comentário) ainda tem a âncora.
//
//     Dois achados IRMÃOS que a auditoria ao vivo do orquestrador reportou, confirmados aqui como
//     JÁ inofensivos a este arquivo por construção, sem exigir mudança de código (registrado pra
//     um leitor futuro não relitigar contra este arquivo especificamente): (a)
//     `filter`/`backdrop-filter` são separados por ESPAÇO no nível RCSS cru
//     (`PropertyParserFilter.cpp:32`, não a lista-vírgula que a própria prosa da seção 9.2 afirma
//     pras quatro propriedades compostas), e (b) uma entrada malformada dentro de uma declaração
//     `filter`/`backdrop-filter` derruba a propriedade INTEIRA
//     (`PropertyParserFilter.cpp`'s `return false` por-entrada, não um `continue` por-entrada que
//     a prosa da seção 11 sugere). Nenhum dos dois afeta este arquivo: este dumper nunca re-separa
//     uma string `filter`/`backdrop-filter` crua sozinho -- ele lê o próprio
//     `FilterDeclarationList` JÁ VALIDADO do `Element::GetProperty("filter")`
//     (`filter_list_value()` abaixo), que o próprio parser upstream já aceitou-ou-rejeitou POR
//     INTEIRO, pelo separador real e pela consequência real de entrada-malformada, antes deste
//     arquivo sequer ver. O próprio fallback "descarta só esta entrada, mantém o resto" deste
//     arquivo em `filter_entry_value()`/`decorator_entry_value()` só dispara pra um nome/forma de
//     função que ESTE ARQUIVO (ainda) não sabe serializar (uma lacuna honesta de escopo, a
//     política fail-high da seção 11 aplicada à PRÓPRIA cobertura do dumper, não uma
//     reimplementação do tratamento de valor-malformado do upstream) -- nunca é alcançado pra RCSS
//     que o próprio upstream já rejeitou, porque esse caso nunca chega neste arquivo como entrada
//     parcial de lista pra começo de conversa.
// Copyright (c) 2026 Petrus Silva Costa
#include "rcss_dump.hpp"

#include "dom_dump.hpp"

#include <RmlUi/Core/Animation.h>
#include <RmlUi/Core/Colour.h>
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Decorator.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Filter.h>
#include <RmlUi/Core/NumericValue.h>
#include <RmlUi/Core/Property.h>
#include <RmlUi/Core/PropertyDefinition.h>
#include <RmlUi/Core/PropertyDictionary.h>
#include <RmlUi/Core/PropertySpecification.h>
#include <RmlUi/Core/StyleSheetSpecification.h>
#include <RmlUi/Core/StyleSheetTypes.h>
#include <RmlUi/Core/Transform.h>
#include <RmlUi/Core/TransformPrimitive.h>
#include <RmlUi/Core/Tween.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/Unit.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace glintfx {

// EN: `UIX-QUANTIZE-MAGNITUDE` -- MUST stay byte-identical to side B's own
//     `glintfx::uix::style::kMaxQuantizeMagnitude` (`glintfx/src/uix/style/value_compute.hpp`,
//     that constant's own header comment has the full magnitude-boundary derivation and safety
//     margin). Reproduced here as a local literal, NOT a shared header/include across the two
//     dumper sides -- deliberate, per `ADR-0020`'s own anti-corruption-layer boundary (this file
//     is the RmlUi-hosted side, on its own way OUT of this repo per `UIX-RCSS-DUMP-A`'s own scope;
//     side B is the clean-room engine replacing it). If this value and side B's own ever drift,
//     `UIX-RCSS-ORACULO`'s own differential harness regains exactly the blind spot this item
//     closes (the two sides silently disagreeing on magnitudes no existing fixture exercises).
// PT: `UIX-QUANTIZE-MAGNITUDE` -- TEM que ficar byte-idêntico ao próprio
//     `glintfx::uix::style::kMaxQuantizeMagnitude` do lado B (`glintfx/src/uix/style/
//     value_compute.hpp`, o próprio comentário de cabeçalho daquela constante tem a derivação
//     completa da fronteira de magnitude e a margem de segurança). Reproduzido aqui como literal
//     local, NÃO um header compartilhado entre os dois lados do dumper -- deliberado, per a
//     própria fronteira de anti-corruption-layer do `ADR-0020` (este arquivo é o lado hospedado
//     no RmlUi, no próprio caminho de SAÍDA deste repo per o próprio escopo da
//     `UIX-RCSS-DUMP-A`; o lado B é o motor clean-room que o substitui). Se este valor e o do
//     lado B algum dia divergirem, o próprio harness diferencial da `UIX-RCSS-ORACULO` recupera
//     exatamente o ponto cego que este item fecha (os dois lados discordando em silêncio de
//     magnitudes que nenhuma fixture existente exercita).
inline constexpr double kQuantizeMagnitudeCeiling = 140737488355328.0; // 2^47

std::string rcss_quantize(float x) {
  double d = static_cast<double>(x);
  // EN: Defensive, not in the spec's own algorithm text: a non-finite input (NaN/Inf, reachable
  //     only through a hostile/malformed cascade this dump's own upstream property parsers
  //     should already have rejected) canonicalizes to 0 rather than propagating NaN into the
  //     printed dump -- fail toward a well-formed line, never toward "nan.nnnn" in output text a
  //     downstream diff parser has to special-case.
  // PT: Defensivo, não faz parte do próprio texto do algoritmo da spec: uma entrada não-finita
  //     (NaN/Inf, só alcançável por uma cascata hostil/malformada que os parsers de propriedade
  //     a montante deste dump já deveriam ter rejeitado) canonicaliza pra 0 em vez de propagar
  //     NaN pro texto do dump -- falha pro lado de uma linha bem-formada, nunca pro lado de
  //     "nan.nnnn" no texto de saída que um parser de diff a jusante teria que tratar à parte.
  if (!std::isfinite(d)) d = 0.0;
  // EN: `UIX-QUANTIZE-MAGNITUDE` -- see `kQuantizeMagnitudeCeiling`'s own comment just above.
  //     Saturate BEFORE the `* 10000` scale step, same reasoning and same idiom as side B's own
  //     `quantize()`: no diagnostic channel reaches the stylesheet author from this dump-layer
  //     function either way (this file's own upstream, RmlUi's cascade, has already accepted the
  //     declaration by the time a value reaches here), so saturating to a defined, well-formed
  //     string is this function's own available remedy for a spec-silent numeric edge case, the
  //     exact same footing the non-finite branch just above already stands on.
  // PT: `UIX-QUANTIZE-MAGNITUDE` -- ver o próprio comentário do `kQuantizeMagnitudeCeiling` logo
  //     acima. Satura ANTES do passo de escala `* 10000`, mesmo raciocínio e mesmo idioma do
  //     próprio `quantize()` do lado B: nenhum canal de diagnóstico chega ao autor da folha de
  //     estilo a partir desta função de camada de dump de qualquer jeito (o próprio upstream
  //     deste arquivo, a cascata do RmlUi, já aceitou a declaração no momento em que um valor
  //     chega aqui), então saturar pra uma string definida, bem-formada, é o próprio remédio
  //     disponível desta função pra um caso-limite numérico que a spec silencia, o mesmo patamar
  //     em que o ramo do não-finito logo acima já está.
  if (d > kQuantizeMagnitudeCeiling) {
    d = kQuantizeMagnitudeCeiling;
  } else if (d < -kQuantizeMagnitudeCeiling) {
    d = -kQuantizeMagnitudeCeiling;
  }
  const double scaled = d * 10000.0;
  const double rounded = std::trunc(scaled + std::copysign(0.5, scaled));
  // EN: No separate "-0 -> 0" step needed here (unlike the `double q` this algorithm's own
  //     doc-comment above once described): `long long` has no distinct negative-zero
  //     representation at all, so `static_cast<long long>(rounded)` from a `-0.0`-valued
  //     `rounded` already lands on the single, ordinary integer `0` -- canonicalization is a
  //     property of the TYPE here, not a runtime branch this function has to write (cppcheck's
  //     own `duplicateConditionalAssign` flagged the earlier `if (n == 0) n = 0;` as dead code
  //     for exactly this reason).
  // PT: Nenhum passo separado de "-0 -> 0" precisa aqui (diferente do `double q` que o próprio
  //     comentário deste algoritmo, acima, uma vez descreveu): `long long` não tem representação
  //     de zero-negativo distinta nenhuma, então `static_cast<long long>(rounded)` a partir de um
  //     `rounded` valendo `-0.0` já cai no único inteiro comum `0` -- a canonicalização é
  //     propriedade do TIPO aqui, não um ramo em tempo de execução que esta função precise
  //     escrever (o próprio `duplicateConditionalAssign` do cppcheck sinalizou o antigo
  //     `if (n == 0) n = 0;` como código morto exatamente por isso).
  const long long n = static_cast<long long>(rounded);
  const bool neg = n < 0;
  const unsigned long long absn = neg ? static_cast<unsigned long long>(-n) : static_cast<unsigned long long>(n);
  const unsigned long long intpart = absn / 10000ULL;
  const unsigned long long frac = absn % 10000ULL;
  char frac_buf[8];
  std::snprintf(frac_buf, sizeof(frac_buf), "%04llu", frac);
  std::string out;
  if (neg) out += '-';
  out += std::to_string(intpart);
  out += '.';
  out += frac_buf;
  return out;
}

std::string rcss_color_hex(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
  char buf[10];
  std::snprintf(buf, sizeof(buf), "#%02x%02x%02x%02x", r, g, b, a);
  return std::string(buf);
}

namespace {

// EN: `quantize()` + literal '%' suffix, section 5/7's own symbolic-percent print form.
// PT: `quantize()` + sufixo '%' literal, a própria forma de impressão de porcentagem simbólica
//     das seções 5/7.
std::string quantize_percent(float number) {
  return rcss_quantize(number) + "%";
}

std::string color_hex(const Rml::Colourb& c) {
  return rcss_color_hex(c.red, c.green, c.blue, c.alpha);
}

// EN: `box-shadow`/gradient-`<stop>` colors -- UN-PREMULTIPLIED before printing. See this file's
//     own header comment (the "SPEC-VS-UPSTREAM DIVERGENCE" note) for the full reasoning: real
//     upstream RmlUi un-premultiplies these exact two fields at the moment it serializes them to
//     text (`TypeConverter.cpp:223`/`:256`), so straight alpha IS the real engine's own answer to
//     "what text represents this color?", not the premultiplied storage representation this
//     file's own first attempt printed. The conversion is LOSSY and IRREVERSIBLE (see the header
//     comment's own measured anchors) -- the printed byte is genuinely not the author's own
//     written value for non-`0xff` alpha, and this file does not pretend otherwise.
// PT: Cor de `box-shadow`/`<stop>` de gradiente -- DES-PREMULTIPLICADA antes de imprimir. Ver o
//     próprio comentário de cabeçalho deste arquivo (a nota "DIVERGÊNCIA SPEC-VS-UPSTREAM") pro
//     raciocínio completo: o próprio RmlUi upstream real des-premultiplica exatamente estes dois
//     campos no momento em que os serializa pra texto (`TypeConverter.cpp:223`/`:256`), então
//     alfa reto É a própria resposta do motor real pra "que texto representa esta cor?", não a
//     representação de armazenamento premultiplicado que a primeira tentativa deste arquivo
//     imprimia. A conversão é COM PERDA e IRREVERSÍVEL (ver as próprias âncoras medidas do
//     comentário de cabeçalho) -- o byte impresso genuinamente não é o valor escrito pelo autor
//     pra alfa não-`0xff`, e este arquivo não finge o contrário.
std::string color_hex_straight_from_premultiplied(const Rml::ColourbPremultiplied& p) {
  return color_hex(p.ToNonPremultiplied());
}

std::string angle_bare_degrees(const Rml::NumericValue& nv) {
  float deg = nv.number;
  if (nv.unit == Rml::Unit::RAD) deg = nv.number * (180.0f / 3.14159265358979323846f);
  return rcss_quantize(deg);
}

std::string resolve_length_px(Rml::Element* el, const Rml::NumericValue& nv) {
  const float px = el->ResolveLength(nv);
  return rcss_quantize(px) + "px";
}

// EN: `Element::GetProperty(name)` with a defensive fallback to the property's own registered
//     default (docs/uix-rcss.md section 3: every registry entry always emits a line, "never
//     fewer" -- if the cascade genuinely has nothing, the registry's own default IS the correct
//     computed value, section 6.1). `GetProperty` is not observed to return null in this file's
//     own corpus runs, but the fallback keeps this function total rather than UB-on-null.
// PT: `Element::GetProperty(nome)` com um fallback defensivo pro próprio default registrado da
//     propriedade (seção 3 do docs/uix-rcss.md: toda entrada do registro sempre emite uma linha,
//     "nunca menos" -- se a cascata genuinamente não tem nada, o próprio default do registro É o
//     valor computado correto, seção 6.1). `GetProperty` não é observado devolvendo null nas
//     rodadas de corpus deste próprio arquivo, mas o fallback mantém esta função total em vez de
//     UB-em-null.
const Rml::Property* get_prop(Rml::Element* el, const char* name) {
  if (const Rml::Property* p = el->GetProperty(name)) return p;
  const Rml::PropertyId id = Rml::StyleSheetSpecification::GetPropertyId(name);
  if (id == Rml::PropertyId::Invalid) return nullptr;
  if (const Rml::PropertyDefinition* def = Rml::StyleSheetSpecification::GetProperty(id)) return def->GetDefaultValue();
  return nullptr;
}

// EN: Property-domain dispatch table, section 6.1's own 107-entry registry (`ESC-1` raised it
//     from 72), reproduced in the SAME alphabetical order that table states as this dump's own
//     required PROP order (section 6's own sort rule) -- so this array IS the iteration order, no
//     separate sort step needed.
// PT: Tabela de despacho de domínio-de-propriedade, o próprio registro de 107 entradas da seção
//     6.1 (a `ESC-1` elevou de 72), reproduzido na MESMA ordem alfabética que aquela tabela
//     declara como a própria ordem PROP exigida por este dump (a própria regra de ordenação da
//     seção 6) -- então este array JÁ É a ordem de iteração, sem passo de sort separado.
enum class Domain {
  Keyword,
  Number,
  NumberClamp01,        // opacity
  NumberClampThreshold, // image-tint-threshold, [0, 0.999]
  Color,
  Length,            // always resolves to px, no keyword, no percent
  LengthPercentAuto, // KEYWORD (any registered keyword text) | PERCENT | LENGTH
  String,
  // EN: `ESC-1`/`ESC-3` -- renamed from the single-property `LetterSpacing`/`TextOverflow` (this
  //     TU-local enum's own two-domain shape now serves a SECOND property each, section 6.1's own
  //     `letter-spacing`+`perspective` / `text-overflow`+`nav-down`/`-left`/`-right`/`-up`):
  //     behavior is byte-identical to the two renamed cases' own prior bodies, see `property_
  //     value()`'s own comment at the two case labels below for why the `p == nullptr` fallback
  //     changed from a per-property literal to the domain-`Keyword` case's own `std::string()`.
  // PT: `ESC-1`/`ESC-3` -- renomeados do `LetterSpacing`/`TextOverflow` de propriedade única (a
  //     própria forma de dois-domínio deste enum TU-local agora serve uma SEGUNDA propriedade
  //     cada, `letter-spacing`+`perspective` / `text-overflow`+`nav-down`/`-left`/`-right`/`-up`
  //     da própria seção 6.1): comportamento byte-idêntico ao próprio corpo anterior dos dois
  //     cases renomeados, ver o próprio comentário do `property_value()` nos dois rótulos de case
  //     abaixo pro motivo do fallback `p == nullptr` ter mudado de literal por-propriedade pro
  //     próprio `std::string()` do case de domínio `Keyword`.
  KeywordOrLength, // KEYWORD("normal"|"none") | LENGTH (no percent) -- letter-spacing, perspective
  KeywordOrString, // KEYWORD | STRING -- text-overflow, nav-down/-left/-right/-up
  KeywordOrColor,  // KEYWORD("auto") | COLOR -- caret-color
  KeywordOrNumber, // KEYWORD | NUMBER -- clip, font-weight, z-index
  FontSizeSpecial,
  LineHeightSpecial,
  VerticalAlignSpecial,
  BoxShadowComposite,
  DecoratorListComposite,
  FilterListComposite,
  AnimationComposite,
  TransformComposite,
  // EN: `ESC-1`/`ESC-3` -- section 6.1's own two remaining composite domains, both "(section 9
  //     grammar: none yet -- empty-list echo only)": section 9 defines NO `<transition-value>`/
  //     `<font-effect-value>` grammar today, so a genuinely-declared (non-`none`, non-empty) value
  //     has no spec-conformant text form -- `transition_value()`/`font_effect_value()`'s own
  //     header comments (below) have the full reasoning and the measured call sites this rests on.
  // PT: `ESC-1`/`ESC-3` -- os dois domínios compostos restantes da própria seção 6.1, os dois
  //     "(gramática §9: nenhuma ainda -- só eco de lista-vazia)": a seção 9 não define gramática
  //     `<transition-value>`/`<font-effect-value>` nenhuma hoje, então um valor genuinamente
  //     declarado (não-`none`, não-vazio) não tem forma textual spec-conformante -- os próprios
  //     comentários de cabeçalho do `transition_value()`/`font_effect_value()` (abaixo) têm o
  //     raciocínio completo e os call sites medidos em que isto se apoia.
  TransitionComposite, // owner ESC-23
  FontEffectComposite, // owner ESC-24
};

struct RegistryEntry {
  const char* name;
  Domain domain;
};

// EN: docs/uix-rcss.md section 6.1's table, verbatim order (107 rows -- `ESC-1` added the 35
//     rows marked `NEW` below; every other row and every other row's own domain is unchanged from
//     the prior 72-row table).
// PT: A tabela da seção 6.1 do docs/uix-rcss.md, ordem verbatim (107 linhas -- a `ESC-1` somou as
//     35 linhas marcadas `NEW` abaixo; toda outra linha e o próprio domínio de toda outra linha
//     seguem inalterados da tabela anterior de 72).
constexpr RegistryEntry kRegistry[] = {
    {"-rmlui-direction", Domain::Keyword}, // NEW ESC-1
    {"-rmlui-language", Domain::String},   // NEW ESC-1
    {"align-content", Domain::Keyword},    // NEW ESC-1
    {"align-items", Domain::Keyword},
    {"align-self", Domain::Keyword}, // NEW ESC-1
    {"animation", Domain::AnimationComposite},
    {"backdrop-filter", Domain::FilterListComposite},
    {"background-color", Domain::Color},
    {"border-bottom-color", Domain::Color},
    {"border-bottom-left-radius", Domain::Length},
    {"border-bottom-right-radius", Domain::Length},
    {"border-bottom-width", Domain::Length},
    {"border-left-color", Domain::Color},
    {"border-left-width", Domain::Length},
    {"border-right-color", Domain::Color},
    {"border-right-width", Domain::Length},
    {"border-top-color", Domain::Color},
    {"border-top-left-radius", Domain::Length},
    {"border-top-right-radius", Domain::Length},
    {"border-top-width", Domain::Length},
    {"bottom", Domain::LengthPercentAuto},
    {"box-shadow", Domain::BoxShadowComposite},
    {"box-sizing", Domain::Keyword},
    {"caret-color", Domain::KeywordOrColor}, // NEW ESC-1
    {"clear", Domain::Keyword},              // NEW ESC-1
    {"clip", Domain::KeywordOrNumber},       // NEW ESC-1
    {"color", Domain::Color},
    {"column-gap", Domain::Length},
    {"cursor", Domain::String},
    {"decorator", Domain::DecoratorListComposite},
    {"display", Domain::Keyword},
    {"drag", Domain::Keyword},      // NEW ESC-1
    {"fill-image", Domain::String}, // NEW ESC-1
    {"filter", Domain::FilterListComposite},
    {"flex-basis", Domain::LengthPercentAuto},
    {"flex-direction", Domain::Keyword}, // NEW ESC-1
    {"flex-grow", Domain::Number},
    {"flex-shrink", Domain::Number},
    {"flex-wrap", Domain::Keyword}, // NEW ESC-1
    {"float", Domain::Keyword},     // NEW ESC-1
    {"focus", Domain::Keyword},
    {"font-effect", Domain::FontEffectComposite}, // NEW ESC-1
    {"font-family", Domain::String},
    {"font-kerning", Domain::Keyword}, // NEW ESC-1
    {"font-size", Domain::FontSizeSpecial},
    {"font-style", Domain::Keyword},          // NEW ESC-1
    {"font-weight", Domain::KeywordOrNumber}, // NEW ESC-1
    {"height", Domain::LengthPercentAuto},
    {"image-color", Domain::Color}, // NEW ESC-1
    {"image-tint-color", Domain::Color},
    {"image-tint-mode", Domain::Keyword},
    {"image-tint-threshold", Domain::NumberClampThreshold},
    {"justify-content", Domain::Keyword},
    {"left", Domain::LengthPercentAuto},
    {"letter-spacing", Domain::KeywordOrLength},
    {"line-height", Domain::LineHeightSpecial},
    {"margin-bottom", Domain::LengthPercentAuto},
    {"margin-left", Domain::LengthPercentAuto},
    {"margin-right", Domain::LengthPercentAuto},
    {"margin-top", Domain::LengthPercentAuto},
    {"mask-image", Domain::DecoratorListComposite},
    {"max-height", Domain::LengthPercentAuto},
    {"max-width", Domain::LengthPercentAuto},
    {"min-height", Domain::LengthPercentAuto},
    {"min-width", Domain::LengthPercentAuto},
    {"nav-down", Domain::KeywordOrString},  // NEW ESC-1
    {"nav-left", Domain::KeywordOrString},  // NEW ESC-1
    {"nav-right", Domain::KeywordOrString}, // NEW ESC-1
    {"nav-up", Domain::KeywordOrString},    // NEW ESC-1
    {"opacity", Domain::NumberClamp01},
    {"overflow-x", Domain::Keyword},
    {"overflow-y", Domain::Keyword},
    {"overscroll-behavior", Domain::Keyword}, // NEW ESC-1
    {"padding-bottom", Domain::LengthPercentAuto},
    {"padding-left", Domain::LengthPercentAuto},
    {"padding-right", Domain::LengthPercentAuto},
    {"padding-top", Domain::LengthPercentAuto},
    {"perspective", Domain::KeywordOrLength},            // NEW ESC-1
    {"perspective-origin-x", Domain::LengthPercentAuto}, // NEW ESC-1
    {"perspective-origin-y", Domain::LengthPercentAuto}, // NEW ESC-1
    {"pointer-events", Domain::Keyword},                 // NEW ESC-1
    {"position", Domain::Keyword},
    {"right", Domain::LengthPercentAuto},
    {"ripple-origin-x", Domain::Number},
    {"ripple-origin-y", Domain::Number},
    {"ripple-phase", Domain::Number},
    {"ripple-strength", Domain::Number},
    {"ripple-width", Domain::Number},
    {"row-gap", Domain::Length},
    {"scrollbar-margin", Domain::Length}, // NEW ESC-1
    {"tab-index", Domain::Keyword},
    {"text-align", Domain::Keyword},
    {"text-decoration", Domain::Keyword}, // NEW ESC-1
    {"text-overflow", Domain::KeywordOrString},
    {"text-transform", Domain::Keyword},
    {"top", Domain::LengthPercentAuto},
    {"transform", Domain::TransformComposite},
    {"transform-origin-x", Domain::LengthPercentAuto}, // NEW ESC-1
    {"transform-origin-y", Domain::LengthPercentAuto}, // NEW ESC-1
    {"transform-origin-z", Domain::Length},            // NEW ESC-1
    {"transition", Domain::TransitionComposite},       // NEW ESC-1
    {"vertical-align", Domain::VerticalAlignSpecial},
    {"visibility", Domain::Keyword}, // NEW ESC-1
    {"white-space", Domain::Keyword},
    {"width", Domain::LengthPercentAuto},
    {"word-break", Domain::Keyword},      // NEW ESC-1
    {"z-index", Domain::KeywordOrNumber}, // NEW ESC-1
};
constexpr std::size_t kRegistrySize = sizeof(kRegistry) / sizeof(kRegistry[0]);

float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ---------------------------------------------------------------------------
// EN: Composite grammar helpers (docs/uix-rcss.md section 9). Outer list separator '|', inner
//     argument separator ';' -- section 9's own opening paragraph.
// PT: Auxiliares de gramática composta (seção 9 do docs/uix-rcss.md). Separador de lista externa
//     '|', separador de argumento interno ';' -- o próprio parágrafo de abertura da seção 9.
// ---------------------------------------------------------------------------

std::string box_shadow_value(Rml::Element* el) {
  const Rml::Property* p = get_prop(el, "box-shadow");
  if (!p || p->unit != Rml::Unit::BOXSHADOWLIST) return "none";
  const Rml::BoxShadowList& list = p->value.GetReference<Rml::BoxShadowList>();
  if (list.empty()) return "none";
  std::string out;
  for (std::size_t i = 0; i < list.size(); ++i) {
    const Rml::BoxShadow& s = list[i];
    if (i) out += "|";
    out += color_hex_straight_from_premultiplied(s.color);
    out += ";";
    out += resolve_length_px(el, s.offset_x);
    out += ";";
    out += resolve_length_px(el, s.offset_y);
    out += ";";
    out += resolve_length_px(el, s.blur_radius);
    out += ";";
    out += resolve_length_px(el, s.spread_distance);
    out += ";";
    out += s.inset ? "true" : "false";
  }
  return out;
}

// EN: docs/uix-rcss.md section 9.2.1's auto-spacing algorithm, applied to a vector of
//     already-explicit-or-unknown percent positions (`std::nullopt` == no explicit position).
//     Returns one resolved percent number (0..100 range, NOT yet quantized-to-string) per stop,
//     in the same order.
// PT: O algoritmo de auto-espaçamento da seção 9.2.1 do docs/uix-rcss.md, aplicado a um vetor de
//     posições-percentuais já-explícitas-ou-desconhecidas (`std::nullopt` == sem posição
//     explícita). Devolve um número percentual resolvido (faixa 0..100, AINDA NÃO
//     quantizado-pra-string) por stop, na mesma ordem.
std::vector<float> auto_space_stops(const std::vector<std::optional<float>>& raw) {
  const std::size_t n = raw.size();
  std::vector<float> pos(n, 0.0f);
  std::vector<bool> known(n, false);
  for (std::size_t i = 0; i < n; ++i) {
    if (raw[i].has_value()) {
      pos[i] = *raw[i];
      known[i] = true;
    }
  }
  if (n == 0) return pos;
  if (!known[0]) {
    pos[0] = 0.0f;
    known[0] = true;
  }
  if (!known[n - 1]) {
    pos[n - 1] = 100.0f;
    known[n - 1] = true;
  }
  // EN: `i + 1 < n` (not `i < n`, and no separate `if (i >= n - 1) break;` -- cppcheck's own
  //     `knownConditionTrueFalse` flagged the earlier, redundant two-clause form): index `n - 1`
  //     is always the LAST stop, already forced `known` by the edge-fix step just above, so this
  //     loop only ever needs to visit the strictly-interior indices `1 .. n - 2`.
  // PT: `i + 1 < n` (não `i < n`, e sem o `if (i >= n - 1) break;` separado -- o próprio
  //     `knownConditionTrueFalse` do cppcheck sinalizou a forma anterior, redundante, de duas
  //     cláusulas): o índice `n - 1` é sempre o ÚLTIMO stop, já forçado `known` pelo passo de
  //     conserto-de-borda logo acima, então este laço só precisa visitar os índices
  //     estritamente-interiores `1 .. n - 2`.
  std::size_t i = 1;
  while (i + 1 < n) {
    if (known[i]) {
      ++i;
      continue;
    }
    const std::size_t run_start = i;
    std::size_t j = i;
    while (j < n && !known[j]) ++j;
    const std::size_t K = j - run_start;
    const float p_before = pos[run_start - 1];
    const float p_after = pos[j];
    for (std::size_t k = 0; k < K; ++k) {
      pos[run_start + k] = p_before + static_cast<float>(k + 1) * (p_after - p_before) / static_cast<float>(K + 1);
      known[run_start + k] = true;
    }
    i = j + 1;
  }
  return pos;
}

// EN: Serializes ONE gradient's own `<stop>;<stop>;...` tail (section 9.2.1), given the raw,
//     as-parsed `ColorStopList` (RmlUi's own real, upstream-parsed color-stop list -- straight
//     from the sub-property, before this file's OWN auto-spacing pass, which section 9.2.1
//     requires this file to run itself rather than trust any RmlUi-internal auto-spacing --
//     RmlUi's own `ResolveColorStops` (DecoratorGradient.cpp) additionally applies a render-time
//     min-spacing "nudge" this dump must NOT reproduce, since that nudge depends on the
//     gradient's own pixel length at render time -- a box-geometry-dependent fact section 1
//     places out of this dump's scope; only the geometry-free auto-spacing distribution itself
//     is this dump's job).
// PT: Serializa a cauda `<stop>;<stop>;...` de UM gradiente (seção 9.2.1), dada a
//     `ColorStopList` crua, como-parseada (a própria lista de stops de cor real do RmlUi,
//     parseada upstream -- direto da sub-propriedade, antes da PRÓPRIA passada de
//     auto-espaçamento deste arquivo, que a seção 9.2.1 exige que este arquivo rode sozinho em
//     vez de confiar em qualquer auto-espaçamento interno do RmlUi -- o próprio
//     `ResolveColorStops` do RmlUi (DecoratorGradient.cpp) aplica adicionalmente um "nudge" de
//     espaçamento-mínimo em tempo de render que este dump NÃO pode reproduzir, já que aquele
//     nudge depende do próprio comprimento em pixel do gradiente em tempo de render -- um fato
//     dependente de geometria-de-caixa que a seção 1 põe fora do escopo deste dump; só a própria
//     distribuição de auto-espaçamento, livre de geometria, é trabalho deste dump).
std::string gradient_stops_tail(const Rml::ColorStopList& stops) {
  std::vector<std::optional<float>> raw_positions;
  raw_positions.reserve(stops.size());
  for (const Rml::ColorStop& s : stops) {
    if (s.position.unit == Rml::Unit::PERCENT) {
      raw_positions.emplace_back(s.position.number);
    } else {
      raw_positions.emplace_back(std::nullopt);
    }
  }
  const std::vector<float> resolved = auto_space_stops(raw_positions);
  std::string out;
  for (std::size_t i = 0; i < stops.size(); ++i) {
    if (i) out += ";";
    out += color_hex_straight_from_premultiplied(stops[i].color);
    out += ":";
    out += quantize_percent(resolved[i]);
  }
  return out;
}

// EN: Maps a radial-gradient `position-x`/`position-y` sub-property (keyword-or-length-percent,
//     `DecoratorRadialGradientInstancer`'s own `position-x`/`position-y` registration) to its
//     family-(c) percent number. `center` is the registered default (50%); `left`/`top` are 0%,
//     `right`/`bottom` are 100% -- the only 5 keywords that instancer's own parser accepts.
// PT: Mapeia uma sub-propriedade `position-x`/`position-y` de radial-gradient
//     (keyword-ou-length-percent, o próprio registro `position-x`/`position-y` do
//     `DecoratorRadialGradientInstancer`) pro próprio número percentual da família (c).
//     `center` é o default registrado (50%); `left`/`top` são 0%, `right`/`bottom` são 100% --
//     as únicas 5 keywords que o parser daquele instancer aceita.
std::optional<float> radial_position_percent(const Rml::Property* p) {
  if (!p) return std::nullopt;
  if (p->unit == Rml::Unit::PERCENT) return p->Get<float>();
  if (p->unit == Rml::Unit::KEYWORD) {
    const std::string kw = p->ToString();
    if (kw == "center") return 50.0f;
    if (kw == "left" || kw == "top") return 0.0f;
    if (kw == "right" || kw == "bottom") return 100.0f;
  }
  // EN: LENGTH form (a length offset for the gradient center) is real, RmlUi-native grammar
  //     this file does not attempt -- it is box-geometry-dependent (needs the element's own
  //     render box to convert to a fraction of the gradient's local space), the same "no box
  //     geometry of any kind" line section 1 draws. Not measured in the census, not in any
  //     required worked example -- documented gap, not a silent guess.
  // PT: A forma LENGTH (um offset de comprimento pro centro do gradiente) é gramática real,
  //     nativa do RmlUi, que este arquivo não tenta -- é dependente de geometria-de-caixa
  //     (precisa da própria render box do elemento pra converter numa fração do espaço local do
  //     gradiente), a mesma linha "nenhuma geometria de caixa, de espécie nenhuma" que a seção 1
  //     traça. Não medido no censo, não em exemplo trabalhado nenhum exigido -- lacuna
  //     documentada, não um chute silencioso.
  return std::nullopt;
}

// EN: `ESC-5` -- own, standalone name->color table for THIS file's own independent mini-parser
//     (`parse_color_token`, below), transcribed directly from the pin's own `html_colours` map
//     (`examples/RmlUi/Source/Core/PropertyParserColour.cpp:117-135`, verified byte-identical
//     against the pinned copy the build actually links, `glintfx/build/_deps/rmlui-src/Source/
//     Core/PropertyParserColour.cpp:117-135`) -- deliberately NOT sharing `value_compute.hpp`'s own
//     `kNamedColorTable`: this file's own top-of-file header names Side-A/Side-B oracle
//     independence as the whole reason this dumper exists (`ADR-0020`) -- a shared table would let
//     a single transposed byte corrupt BOTH sides identically, exactly the blind spot the
//     two-independent-implementations design exists to catch. Same 19 entries, same order, same
//     values as `value_compute.cpp`'s own table -- re-derived from the pin a second time, by a
//     second read of the same source, never copy-pasted from the sibling file.
// PT: `ESC-5` -- tabela própria, standalone, nome->cor pro mini-parser independente DESTE arquivo
//     (`parse_color_token`, abaixo), transcrita direto do próprio mapa `html_colours` do pin
//     (`examples/RmlUi/Source/Core/PropertyParserColour.cpp:117-135`, verificado byte-idêntico
//     contra a cópia fixada que o build de fato linka, `glintfx/build/_deps/rmlui-src/Source/Core/
//     PropertyParserColour.cpp:117-135`) -- deliberadamente SEM compartilhar a própria
//     `kNamedColorTable` do value_compute.hpp: o próprio cabeçalho de topo deste arquivo nomeia
//     independência de oráculo Lado-A/Lado-B como toda a razão deste dumper existir (`ADR-0020`) --
//     uma tabela compartilhada deixaria um único byte transposto corromper OS DOIS lados
//     identicamente, exatamente o ponto cego que o desenho de duas-implementações-independentes
//     existe pra pegar. Mesmas 19 entradas, mesma ordem, mesmos valores da própria tabela do
//     value_compute.cpp -- re-derivada do pin uma segunda vez, por uma segunda leitura da mesma
//     fonte, nunca copiada-e-colada do arquivo irmão.
struct NamedColorTokenEntry {
  const char* name;
  Rml::byte r;
  Rml::byte g;
  Rml::byte b;
  Rml::byte a;
};
constexpr NamedColorTokenEntry kNamedColorTokenTable[] = {
    {"black", 0, 0, 0, 255},
    {"silver", 192, 192, 192, 255},
    {"gray", 128, 128, 128, 255},
    {"grey", 128, 128, 128, 255},
    {"white", 255, 255, 255, 255},
    {"maroon", 128, 0, 0, 255},
    {"red", 255, 0, 0, 255},
    {"orange", 255, 165, 0, 255},
    {"purple", 128, 0, 128, 255},
    {"fuchsia", 255, 0, 255, 255},
    {"green", 0, 128, 0, 255},
    {"lime", 0, 255, 0, 255},
    {"olive", 128, 128, 0, 255},
    {"yellow", 255, 255, 0, 255},
    {"navy", 0, 0, 128, 255},
    {"blue", 0, 0, 255, 255},
    {"teal", 0, 128, 128, 255},
    {"aqua", 0, 255, 255, 255},
    {"transparent", 0, 0, 0, 0},
};

// EN: `ESC-5` -- own `#`-vs-name dispatch mirroring the pin's own `ParseColour` shape
//     (`PropertyParserColour.cpp:166-209`): `#` routes to hex (unchanged, below); anything else is
//     lowercased then looked up in `kNamedColorTokenTable` above, mirroring the pin's own
//     `StringUtilities::ToLower(value)` immediately before its own `html_colours.find()`
//     (`:201`). Pre-`ESC-5` this branch only recognized `transparent`/`white`, case-sensitive; now
//     all 19, case-insensitive -- closes the `polygon()` `<fill>` divergence this task's own plan
//     names: the real renderer (`decorator_polygon.cpp:463-477`, `RunNamedParser("color", ...)`) has
//     always accepted all 19 case-insensitively (it IS the pinned `PropertyParserColour`), so
//     leaving this independent mini-parser at 2 names meant a fixture with e.g.
//     `polygon(6, orange)` would compute correctly on Side B and get silently DROPPED here on Side
//     A (`polygon_fill_value`'s own `std::nullopt` fail-high path, below) -- a real divergence, not
//     a hypothetical one (see `uix_esc5_named_colors.rml`'s own `#d` case, caught red before this
//     fix landed).
// PT: `ESC-5` -- próprio despacho `#`-versus-nome espelhando a própria forma do `ParseColour` do pin
//     (`PropertyParserColour.cpp:166-209`): `#` roteia pro hex (inalterado, abaixo); qualquer outra
//     coisa é minusculizada depois procurada na `kNamedColorTokenTable` acima, espelhando o próprio
//     `StringUtilities::ToLower(value)` do pin logo antes da própria chamada `html_colours.find()`
//     dele (`:201`). Pré-`ESC-5` este ramo só reconhecia `transparent`/`white`, case-sensitive;
//     agora as 19, case-insensitive -- fecha a divergência do `<fill>` do `polygon()` que o próprio
//     plano desta tarefa nomeia: o renderer real (`decorator_polygon.cpp:463-477`,
//     `RunNamedParser("color", ...)`) sempre aceitou as 19 case-insensitive (ELE É o
//     `PropertyParserColour` fixado), então deixar este mini-parser independente em 2 nomes
//     significava que uma fixture com ex. `polygon(6, orange)` computava certo no Lado B e era
//     DERRUBADA em silêncio aqui no Lado A (o próprio caminho fail-high `std::nullopt` do
//     `polygon_fill_value`, abaixo) -- uma divergência real, não hipotética (ver o próprio caso
//     `#d` do `uix_esc5_named_colors.rml`, pego vermelho antes deste conserto pousar).
bool parse_color_token(const std::string& raw, Rml::Colourb& out) {
  auto hexval = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    const char l = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (l >= 'a' && l <= 'f') return l - 'a' + 10;
    return -1;
  };
  if (raw.empty()) return false;
  if (raw[0] != '#') {
    std::string lowered = raw;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    // EN: `useStlAlgorithm` (cppcheck) -- `std::find_if`, same reasoning/precedent as this
    //     repo's own sibling fix in `value_compute.cpp`'s own `parse_color()` (`ESC-5`,
    //     identically-shaped independent table lookup).
    // PT: `useStlAlgorithm` (cppcheck) -- `std::find_if`, mesmo racional/precedente do próprio
    //     conserto irmão no `parse_color()` do value_compute.cpp (`ESC-5`, lookup de tabela
    //     independente com a mesma forma).
    const auto* match =
        std::find_if(std::begin(kNamedColorTokenTable), std::end(kNamedColorTokenTable),
                     [&lowered](const NamedColorTokenEntry& entry) { return lowered == entry.name; });
    if (match != std::end(kNamedColorTokenTable)) {
      out = Rml::Colourb(match->r, match->g, match->b, match->a);
      return true;
    }
    return false;
  }
  const std::string hex = raw.substr(1);
  auto digit_pair = [&](std::size_t i) -> int {
    const int hi = hexval(hex[i]);
    const int lo = hexval(hex[i + 1]);
    if (hi < 0 || lo < 0) return -1;
    return hi * 16 + lo;
  };
  if (hex.size() == 3 || hex.size() == 4) {
    int r = hexval(hex[0]), g = hexval(hex[1]), b = hexval(hex[2]);
    int a = (hex.size() == 4) ? hexval(hex[3]) : 15;
    if (r < 0 || g < 0 || b < 0 || a < 0) return false;
    out = Rml::Colourb(static_cast<Rml::byte>(r * 17), static_cast<Rml::byte>(g * 17), static_cast<Rml::byte>(b * 17),
                       static_cast<Rml::byte>(a * 17));
    return true;
  }
  if (hex.size() == 6 || hex.size() == 8) {
    const int r = digit_pair(0), g = digit_pair(2), b = digit_pair(4);
    const int a = (hex.size() == 8) ? digit_pair(6) : 255;
    if (r < 0 || g < 0 || b < 0 || a < 0) return false;
    out = Rml::Colourb(static_cast<Rml::byte>(r), static_cast<Rml::byte>(g), static_cast<Rml::byte>(b), static_cast<Rml::byte>(a));
    return true;
  }
  return false;
}

std::string trim(const std::string& s) {
  std::size_t b = 0, e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

bool starts_with(const std::string& s, const char* prefix) {
  const std::size_t n = std::string(prefix).size();
  return s.size() >= n && s.compare(0, n, prefix) == 0;
}

// EN: `polygon()`'s own `fill` argument -- either a plain color token (this function's own,
//     independent hex/named-color mini-parser, section 7.1's in-scope grammar: the 4 hex forms
//     `#rgb`/`#rgba`/`#rrggbb`/`#rrggbbaa` plus, as of `ESC-5`, all 19 of the pin's own named
//     colors, case-insensitive -- `parse_color_token()`'s own doc-comment above) or a NESTED
//     `linear-gradient(...)`/`radial-gradient(...)` (`docs/uix-rcss.md` section 9.2's own table:
//     `fill` "either a <color> ..., or a nested linear-gradient(...)/radial-gradient(...) using
//     this same grammar recursively"). The nested-gradient case is a DECLARED, BOUNDED gap in
//     this file, not a silent omission: `polygon()`'s own `fill` sub-property is stored as an
//     OPAQUE, UNPARSED string by its own instancer (`decorator_polygon.cpp:786`, parser
//     "string", not "color") -- unlike every top-level decorator function this file handles via
//     a `PropertyDictionary` of ALREADY-PARSED sub-properties, `fill`'s nested-gradient case has
//     no such structured form available from public API; parsing it would mean writing a SECOND,
//     from-scratch composite-value parser operating on raw source text, duplicating exactly the
//     kind of machinery `src/uix/style/` (the forbidden other side) exists to own. Not exercised
//     by any of this document's own section 15 required worked examples. Returns `std::nullopt`
//     for the nested-gradient case (caller drops the whole `polygon()` entry, fail-high per
//     section 11 -- "this dumper does not attempt X" is reported honestly as absence, not
//     invented as a wrong present value).
// PT: O próprio argumento `fill` do `polygon()` -- ou um token de cor plana (mini-parser próprio
//     e independente deste arquivo, só a gramática dentro-do-escopo da seção 7.1:
//     `#rgb`/`#rgba`/`#rrggbb`/`#rrggbbaa`, `transparent`, `white`) ou um `linear-gradient(...)`/
//     `radial-gradient(...)` ANINHADO (a própria tabela da seção 9.2 do docs/uix-rcss.md: `fill`
//     "ou um <color> ..., ou um linear-gradient(...)/radial-gradient(...) ANINHADO usando essa
//     mesma gramática recursivamente"). O caso de gradiente-aninhado é uma lacuna DECLARADA,
//     LIMITADA deste arquivo, não uma omissão silenciosa: a própria sub-propriedade `fill` do
//     `polygon()` é guardada como string OPACA, NÃO-PARSEADA pelo próprio instancer dela
//     (`decorator_polygon.cpp:786`, parser "string", não "color") -- diferente de toda função de
//     decorator de topo-de-nível que este arquivo trata via um `PropertyDictionary` de
//     sub-propriedades JÁ-PARSEADAS, o caso de gradiente-aninhado do `fill` não tem forma
//     estruturada nenhuma disponível via API pública; parseá-lo significaria escrever um
//     SEGUNDO parser de valor-composto do zero, operando sobre texto-fonte cru, duplicando
//     exatamente o tipo de maquinaria que o `src/uix/style/` (o outro lado, proibido) existe pra
//     possuir. Não exercitado por nenhum dos exemplos trabalhados exigidos da própria seção 15
//     deste documento. Devolve `std::nullopt` pro caso de gradiente-aninhado (o chamador descarta
//     a entrada `polygon()` inteira, fail-high pela seção 11 -- "este dumper não tenta X" é
//     reportado honestamente como ausência, não inventado como um valor presente errado).
std::optional<std::string> polygon_fill_value(const std::string& raw_fill) {
  const std::string t = trim(raw_fill);
  if (starts_with(t, "linear-gradient(") || starts_with(t, "radial-gradient(")) return std::nullopt;
  Rml::Colourb c;
  if (!parse_color_token(t, c)) return std::nullopt;
  return color_hex(c);
}

// EN: One decorator/mask-image LIST entry, section 9.2's own per-function table. Returns
//     `std::nullopt` for a function this dumper does not (yet) serialize, or one that fails its
//     own declared range/shape validation -- caller drops the entry, section 11's fail-high
//     policy (logged to stderr, never silently invented).
// PT: Uma entrada de LISTA decorator/mask-image, a própria tabela por-função da seção 9.2.
//     Devolve `std::nullopt` pra uma função que este dumper (ainda) não serializa, ou uma que
//     falha a própria validação declarada de faixa/forma -- o chamador descarta a entrada,
//     política fail-high da seção 11 (logada em stderr, nunca inventada em silêncio).
std::optional<std::string> decorator_entry_value(Rml::Element* el, const Rml::DecoratorDeclaration& decl) {
  if (!decl.instancer) return std::nullopt; // @decorator name reference -- out of this dump's scope
  const Rml::PropertySpecification& spec = decl.instancer->GetPropertySpecification();
  auto get_sub = [&](const char* name) -> const Rml::Property* {
    const Rml::PropertyDefinition* def = spec.GetProperty(name);
    if (!def) return nullptr;
    return decl.properties.GetProperty(def->GetId());
  };

  if (decl.type == "image") {
    const Rml::Property* src = get_sub("image-src");
    if (!src) return std::nullopt;
    return "image(" + dom_dump_escape(src->Get<Rml::String>()) + ")";
  }
  if (decl.type == "image-tint") {
    const Rml::Property* src = get_sub("src");
    if (!src) return std::nullopt;
    return "image-tint(" + dom_dump_escape(src->Get<Rml::String>()) + ")";
  }
  if (decl.type == "ripple") {
    const Rml::Property* max_radius = get_sub("max-radius");
    if (!max_radius) return std::nullopt;
    return "ripple(" + rcss_quantize(max_radius->Get<float>()) + ")";
  }
  if (decl.type == "horizontal-gradient" || decl.type == "vertical-gradient") {
    // EN: `UIX-RCSS-ERRATA-2` (relayed by the orchestrator mid-task): `vertical-gradient` was
    //     missing from section 9.2's own function table despite being the corpus's single
    //     most-used decorator function (107 occurrences / 16 files, more than `polygon`) --
    //     same `DecoratorStraightGradientInstancer`/`start-color`/`stop-color` sub-properties as
    //     `horizontal-gradient`, only the paint AXIS differs (render-time concern, out of this
    //     dump's own cascade-value scope) -- so the same code path serves both, keyed only by
    //     which literal function name won the cascade.
    // PT: `UIX-RCSS-ERRATA-2` (repassada pelo orquestrador no meio da tarefa): `vertical-gradient`
    //     estava faltando na própria tabela de função da seção 9.2 apesar de ser a função de
    //     decorator MAIS usada do corpus (107 ocorrências / 16 arquivos, mais que `polygon`) --
    //     mesmas sub-propriedades `start-color`/`stop-color` do
    //     `DecoratorStraightGradientInstancer` de `horizontal-gradient`, só o EIXO de pintura
    //     difere (preocupação de tempo-de-render, fora do próprio escopo de valor-de-cascata
    //     deste dump) -- então o mesmo caminho de código serve os dois, chaveado só por qual
    //     nome de função literal venceu a cascata.
    const Rml::Property* start = get_sub("start-color");
    const Rml::Property* stop = get_sub("stop-color");
    if (!start || !stop) return std::nullopt;
    return decl.type + "(" + color_hex(start->Get<Rml::Colourb>()) + ";" + color_hex(stop->Get<Rml::Colourb>()) + ")";
  }
  if (decl.type == "linear-gradient") {
    const Rml::Property* angle = get_sub("angle");
    const Rml::Property* stops = get_sub("color-stops");
    if (!angle || !stops || stops->unit != Rml::Unit::COLORSTOPLIST) return std::nullopt;
    const Rml::ColorStopList& list = stops->value.GetReference<Rml::ColorStopList>();
    if (list.empty()) return std::nullopt;
    return "linear-gradient(" + angle_bare_degrees(angle->GetNumericValue()) + ";" + gradient_stops_tail(list) + ")";
  }
  if (decl.type == "radial-gradient") {
    const Rml::Property* shape = get_sub("ending-shape");
    if (shape && shape->unit == Rml::Unit::KEYWORD && shape->ToString() == "ellipse") {
      return std::nullopt; // fail-high per docs/effects.md: only circle is in scope
    }
    const Rml::Property* px = get_sub("position-x");
    const Rml::Property* py = get_sub("position-y");
    const Rml::Property* stops = get_sub("color-stops");
    if (!stops || stops->unit != Rml::Unit::COLORSTOPLIST) return std::nullopt;
    const std::optional<float> cx = radial_position_percent(px);
    const std::optional<float> cy = radial_position_percent(py);
    if (!cx.has_value() || !cy.has_value()) return std::nullopt;
    const Rml::ColorStopList& list = stops->value.GetReference<Rml::ColorStopList>();
    if (list.empty()) return std::nullopt;
    return "radial-gradient(" + quantize_percent(*cx) + ";" + quantize_percent(*cy) + ";" + gradient_stops_tail(list) + ")";
  }
  if (decl.type == "polygon") {
    const Rml::Property* sides = get_sub("sides");
    const Rml::Property* fill = get_sub("fill");
    const Rml::Property* rotation = get_sub("rotation");
    if (!sides || !fill || !rotation) return std::nullopt;
    // EN: `UIX-QUANTIZE-MAGNITUDE`'s own auditoria-dominó sibling, found while auditing this file's
    //     other float-to-integer conversion (`rcss_quantize()` above): `std::lround()` on a
    //     `float` whose magnitude exceeds what `long` can represent is a domain error, an
    //     unspecified result per the C standard -- the SAME missing-guard shape as this file's own
    //     `rcss_quantize()` bug, one function away. Side B's own `polygon()` sides parse
    //     (`glintfx/src/uix/style/value_compute.cpp`) never had this exposure: it keeps the value
    //     `float` end to end and range-checks it directly, no intermediate integer cast at all --
    //     this side-A-only artifact exists because `sides` here is read back out of an
    //     already-parsed RmlUi `Property` as `float`, and `long` is only introduced locally to
    //     compare against the `[3,1024]` range below. Fail-high BEFORE the conversion (same
    //     "section 11 fail-high: range violation" the line below already documents) rather than
    //     relying on `std::lround()`'s own unspecified-but-in-practice-huge-negative return for an
    //     out-of-range input to accidentally fall outside `[3,1024]` anyway.
    // PT: O próprio sibling de auditoria-dominó da `UIX-QUANTIZE-MAGNITUDE`, achado ao auditar a
    //     outra conversão float-para-inteiro deste arquivo (`rcss_quantize()` acima): `std::lround()`
    //     sobre um `float` cuja magnitude excede o que `long` consegue representar é um erro de
    //     domínio, resultado não-especificado per o padrão C -- a MESMA forma de guarda faltando
    //     do próprio bug do `rcss_quantize()` deste arquivo, uma função adiante. O próprio parse de
    //     sides do `polygon()` do lado B (`glintfx/src/uix/style/value_compute.cpp`) nunca teve
    //     essa exposição: mantém o valor `float` do início ao fim e faz o range-check direto nele,
    //     sem cast intermediário pra inteiro nenhum -- este artefato é exclusivo do lado A porque
    //     `sides` aqui é lido de volta de uma `Property` já parseada do RmlUi como `float`, e
    //     `long` só é introduzido localmente pra comparar contra o range `[3,1024]` abaixo.
    //     Fail-high ANTES da conversão (o mesmo "section 11 fail-high: range violation" que a
    //     linha abaixo já documenta) em vez de depender do retorno não-especificado-mas-na-prática-
    //     enorme-e-negativo do `std::lround()` pra um input fora de faixa acidentalmente cair fora
    //     de `[3,1024]` de qualquer jeito.
    const float sides_raw = sides->Get<float>();
    if (!std::isfinite(sides_raw) || sides_raw < -1e6f || sides_raw > 1e6f) {
      return std::nullopt; // fail-high: magnitude guard before std::lround(), see comment above
    }
    const long sides_int = std::lround(sides_raw);
    if (sides_int < 3 || sides_int > 1024) return std::nullopt; // section 11 fail-high: range violation
    const std::optional<std::string> fill_str = polygon_fill_value(fill->Get<Rml::String>());
    if (!fill_str.has_value()) return std::nullopt;
    return "polygon(" + rcss_quantize(static_cast<float>(sides_int)) + ";" + *fill_str + ";" +
           angle_bare_degrees(rotation->GetNumericValue()) + ")";
  }
  (void)el;
  return std::nullopt; // unrecognized function name -- section 11 fail-high
}

std::string decorator_list_value(Rml::Element* el, const char* property_name) {
  const Rml::Property* p = get_prop(el, property_name);
  if (!p || p->unit != Rml::Unit::DECORATOR) return "none";
  const Rml::DecoratorsPtr decorators = p->Get<Rml::DecoratorsPtr>();
  if (!decorators || decorators->list.empty()) return "none";
  std::string out;
  bool first = true;
  for (const Rml::DecoratorDeclaration& decl : decorators->list) {
    const std::optional<std::string> entry = decorator_entry_value(el, decl);
    if (!entry.has_value()) {
      std::fprintf(stderr, "rcss_dump: %s decorator entry type '%s' dropped (fail-high, section 11)\n", property_name, decl.type.c_str());
      continue;
    }
    if (!first) out += "|";
    out += *entry;
    first = false;
  }
  return out.empty() ? "none" : out;
}

std::optional<std::string> filter_entry_value(Rml::Element* el, const Rml::FilterDeclaration& decl) {
  if (!decl.instancer) return std::nullopt;
  const Rml::PropertySpecification& spec = decl.instancer->GetPropertySpecification();
  auto get_sub = [&](const char* name) -> const Rml::Property* {
    const Rml::PropertyDefinition* def = spec.GetProperty(name);
    if (!def) return nullptr;
    return decl.properties.GetProperty(def->GetId());
  };
  if (decl.type == "blur") {
    const Rml::Property* sigma = get_sub("sigma");
    if (!sigma) return std::nullopt;
    return "blur(" + resolve_length_px(el, sigma->GetNumericValue()) + ")";
  }
  if (decl.type == "drop-shadow") {
    const Rml::Property* color = get_sub("color");
    const Rml::Property* ox = get_sub("offset-x");
    const Rml::Property* oy = get_sub("offset-y");
    const Rml::Property* sigma = get_sub("sigma");
    if (!color || !ox || !oy || !sigma) return std::nullopt;
    return "drop-shadow(" + color_hex(color->Get<Rml::Colourb>()) + ";" + resolve_length_px(el, ox->GetNumericValue()) + ";" +
           resolve_length_px(el, oy->GetNumericValue()) + ";" + resolve_length_px(el, sigma->GetNumericValue()) + ")";
  }
  return std::nullopt;
}

std::string filter_list_value(Rml::Element* el, const char* property_name) {
  const Rml::Property* p = get_prop(el, property_name);
  if (!p || p->unit != Rml::Unit::FILTER) return "none";
  const Rml::FiltersPtr filters = p->Get<Rml::FiltersPtr>();
  if (!filters || filters->list.empty()) return "none";
  std::string out;
  bool first = true;
  for (const Rml::FilterDeclaration& decl : filters->list) {
    const std::optional<std::string> entry = filter_entry_value(el, decl);
    if (!entry.has_value()) {
      std::fprintf(stderr, "rcss_dump: %s filter entry type '%s' dropped (fail-high, section 11)\n", property_name, decl.type.c_str());
      continue;
    }
    if (!first) out += "|";
    out += *entry;
    first = false;
  }
  return out.empty() ? "none" : out;
}

// EN: `Tween::to_string()` (`Tween.h:26`, PUBLIC, `Tween.cpp:137-166`) already reconstructs the
//     EXACT keyword string upstream's own `PropertyParserAnimationData::keywords` table
//     registers -- `Tween` does not expose the `type`/`direction` fields a naive reading of its
//     own two-argument constructor might suggest (they are private; the constructor folds them
//     into `type_in`/`type_out` instead, `Tween.cpp:90-96`), so this file calls the library's own
//     accessor rather than re-deriving the same string from a private representation it cannot
//     see. This ALSO resolves the "what does a defaulted tween print" question this file's
//     research first thought was an open ambiguity: `Animation::tween`'s own default member
//     (`Tween tween;`, `Animation.h:12`) constructs via `Tween(Type type = Linear, Direction
//     direction = Out)`'s DEFAULT ARGUMENTS -- `type_in` stays `None`, `type_out` becomes
//     `Linear` -- so `to_string()` deterministically returns `"linear-out"` for a `<timing-
//     keyword>` the source never wrote, not an arbitrary placeholder this file would have had to
//     invent. Not exercised by any required worked example, but no longer an open question.
// PT: `Tween::to_string()` (`Tween.h:26`, PÚBLICO, `Tween.cpp:137-166`) já reconstrói a string de
//     keyword EXATA que a própria tabela `PropertyParserAnimationData::keywords` do upstream
//     registra -- `Tween` não expõe os campos `type`/`direction` que uma leitura ingênua do
//     próprio construtor de dois argumentos poderia sugerir (são privados; o construtor os
//     dobra em `type_in`/`type_out`, `Tween.cpp:90-96`), então este arquivo chama o próprio
//     accessor da lib em vez de re-derivar a mesma string de uma representação privada que não
//     consegue ver. Isto TAMBÉM resolve a pergunta "o que um tween default imprime" que a
//     pesquisa deste arquivo achou de início ser uma ambiguidade em aberto: o próprio membro
//     default de `Animation::tween` (`Tween tween;`, `Animation.h:12`) constrói via os
//     ARGUMENTOS DEFAULT de `Tween(Type type = Linear, Direction direction = Out)` -- `type_in`
//     fica `None`, `type_out` vira `Linear` -- então `to_string()` devolve deterministicamente
//     `"linear-out"` pra uma `<timing-keyword>` que a fonte nunca escreveu, não um placeholder
//     arbitrário que este arquivo teria tido que inventar. Não exercitada por exemplo trabalhado
//     exigido nenhum, mas deixa de ser pergunta em aberto.
std::string animation_value(Rml::Element* el) {
  const Rml::Property* p = get_prop(el, "animation");
  if (!p || p->unit != Rml::Unit::ANIMATION) return "none";
  const Rml::AnimationList& list = p->value.GetReference<Rml::AnimationList>();
  if (list.empty()) return "none";
  std::string out;
  for (std::size_t i = 0; i < list.size(); ++i) {
    const Rml::Animation& a = list[i];
    if (i) out += "|";
    out += "animation(";
    out += dom_dump_escape(a.name);
    out += ";";
    out += rcss_quantize(a.duration);
    out += ";";
    out += a.tween.to_string();
    out += ";";
    out += (a.num_iterations == -1) ? "infinite" : std::to_string(a.num_iterations);
    out += ";";
    out += a.alternate ? "true" : "false";
    out += ";";
    out += a.paused ? "true" : "false";
    out += ")";
  }
  (void)el;
  return out;
}

// EN: §9.4's own deliberately-thin 2D subset: `translate(x;y) | scale(x;y) | rotate(angle)`.
//     Any OTHER primitive type (3D, matrix, single-axis translate/scale/rotate) is dropped from
//     the list, fail-high, section 11 -- section 9.4's own header names this teto explicitly.
// PT: O próprio subconjunto 2D deliberadamente fino da §9.4: `translate(x;y) | scale(x;y) |
//     rotate(angle)`. Qualquer OUTRO tipo de primitiva (3D, matriz, translate/scale/rotate de
//     eixo único) é descartado da lista, fail-high, seção 11 -- o próprio cabeçalho da seção 9.4
//     nomeia este teto explicitamente.
std::optional<std::string> transform_primitive_value(Rml::Element* el, const Rml::TransformPrimitive& prim) {
  using Rml::TransformPrimitive;
  switch (prim.type) {
    case TransformPrimitive::TRANSLATE2D: {
      const Rml::Transforms::Translate2D& t = prim.translate_2d;
      auto axis = [&](const Rml::NumericValue& nv) -> std::string {
        if (nv.unit == Rml::Unit::PERCENT) return quantize_percent(nv.number);
        return resolve_length_px(el, nv);
      };
      return "translate(" + axis(t.values[0]) + ";" + axis(t.values[1]) + ")";
    }
    case TransformPrimitive::SCALE2D: {
      const Rml::Transforms::Scale2D& s = prim.scale_2d;
      return "scale(" + rcss_quantize(s.values[0]) + ";" + rcss_quantize(s.values[1]) + ")";
    }
    case TransformPrimitive::ROTATE2D: {
      const Rml::Transforms::Rotate2D& r = prim.rotate_2d;
      // EN: Rotate2D::values[0] is stored resolved to RADIANS (TransformPrimitive.cpp:125-126,
      //     `ResolvedPrimitive(values, {Unit::RAD})`) -- convert to degrees per §8.2 before
      //     quantizing, same conversion factor as `angle_bare_degrees`.
      // PT: Rotate2D::values[0] é guardado resolvido em RADIANOS
      //     (TransformPrimitive.cpp:125-126, `ResolvedPrimitive(values, {Unit::RAD})`) --
      //     converte pra graus pela §8.2 antes de quantizar, mesmo fator de conversão de
      //     `angle_bare_degrees`.
      const float deg = r.values[0] * (180.0f / 3.14159265358979323846f);
      return "rotate(" + rcss_quantize(deg) + ")";
    }
    default:
      return std::nullopt;
  }
}

std::string transform_value(Rml::Element* el) {
  const Rml::Property* p = get_prop(el, "transform");
  if (!p || p->unit != Rml::Unit::TRANSFORM) return "none";
  const Rml::TransformPtr transform = p->Get<Rml::TransformPtr>();
  if (!transform || transform->GetNumPrimitives() == 0) return "none";
  std::string out;
  bool first = true;
  for (int i = 0; i < transform->GetNumPrimitives(); ++i) {
    const std::optional<std::string> entry = transform_primitive_value(el, transform->GetPrimitive(i));
    if (!entry.has_value()) {
      std::fprintf(stderr, "rcss_dump: transform primitive #%d dropped (out of the 2D subset, section 9.4/11)\n", i);
      continue;
    }
    if (!first) out += "|";
    out += *entry;
    first = false;
  }
  return out.empty() ? "none" : out;
}

// EN: `transition` -- docs/uix-rcss.md section 6.1's own composite domain, `(section 9 grammar:
//     none yet -- empty-list echo only, owner ESC-23)`: section 9 has no `<transition-value>`
//     grammar defined today, so there is no spec-conformant text form for a NON-`none`,
//     NON-empty `TransitionList` (`none` keyword, OR the registry's own empty-list default,
//     `Animation.h:29-36`'s own `TransitionList() {}` -- `none=true` either way, confirmed at
//     `PropertyParserAnimation.cpp:247`/`:220` for the two respective paths). A genuinely
//     declared transition (the `all` keyword, or one-or-more explicit `<property> <duration>
//     ...` entries -- `ParseTransition()`'s own `transitions` vector non-empty either way) is a
//     DECLARED, BOUNDED gap: section 11's fail-high policy applied to the dumper's own missing
//     grammar, not a silent wrong-value invention -- logs to stderr and echoes `none`, the same
//     shape `filter_entry_value()`'s own unknown-function branch already uses one section up.
// PT: `transition` -- o próprio domínio composto da seção 6.1 do docs/uix-rcss.md, `(gramática
//     §9: nenhuma ainda -- só eco de lista-vazia, dona ESC-23)`: a seção 9 não tem gramática
//     `<transition-value>` definida hoje, então não existe forma textual spec-conformante pra
//     uma `TransitionList` NÃO-`none`, NÃO-vazia (a keyword `none`, OU o próprio default de
//     lista-vazia do registro, o próprio `TransitionList() {}` de `Animation.h:29-36` --
//     `none=true` nos dois casos, confirmado em `PropertyParserAnimation.cpp:247`/`:220` pros
//     dois caminhos respectivos). Uma transição genuinamente declarada (a keyword `all`, ou uma-
//     ou-mais entradas explícitas `<propriedade> <duração> ...` -- o próprio vetor `transitions`
//     do `ParseTransition()` não-vazio nos dois casos) é uma lacuna DECLARADA, LIMITADA: a
//     política fail-high da seção 11 aplicada à própria gramática faltante do dumper, não uma
//     invenção silenciosa de valor errado -- loga em stderr e ecoa `none`, a mesma forma que o
//     próprio ramo de função-desconhecida do `filter_entry_value()` já usa uma seção acima.
std::string transition_value(Rml::Element* el) {
  const Rml::Property* p = get_prop(el, "transition");
  if (!p || p->unit != Rml::Unit::TRANSITION) return "none";
  const Rml::TransitionList& list = p->value.GetReference<Rml::TransitionList>();
  if (list.none || list.transitions.empty()) return "none";
  std::fprintf(stderr,
               "rcss_dump: transition value declared but section 9's own composite grammar for "
               "'transition' does not exist yet (owner ESC-23) -- echoing 'none' (fail-high, section 11)\n");
  return "none";
}

// EN: `font-effect` -- docs/uix-rcss.md section 6.1's own composite domain, `(section 9 grammar:
//     none yet -- empty-list echo only, owner ESC-24)`: mirrors `transition_value()`'s own
//     reasoning one property up -- no `<font-effect-value>` grammar in section 9 today. The
//     registry's own empty-string default does not even reach `Unit::FONTEFFECT`:
//     `PropertyParserFontEffect.cpp:23-28`'s own empty-or-"none" branch sets
//     `property.unit = Unit::UNKNOWN` (not a null `Property*`, and not `FONTEFFECT` either) --
//     measured directly at that call site, not assumed from this domain's own name symmetry with
//     `decorator`/`filter`/`mask-image`. The `p->unit != Rml::Unit::FONTEFFECT` guard below
//     already covers this measured fact (falls to `"none"`) without needing a special case.
// PT: `font-effect` -- o próprio domínio composto da seção 6.1 do docs/uix-rcss.md, `(gramática
//     §9: nenhuma ainda -- só eco de lista-vazia, dona ESC-24)`: espelha o próprio raciocínio do
//     `transition_value()` uma propriedade acima -- nenhuma gramática `<font-effect-value>` na
//     seção 9 hoje. O próprio default de string-vazia do registro nem chega a `Unit::FONTEFFECT`:
//     o próprio ramo vazio-ou-"none" de `PropertyParserFontEffect.cpp:23-28` seta
//     `property.unit = Unit::UNKNOWN` (não um `Property*` nulo, e também não `FONTEFFECT`) --
//     medido direto naquele call site, não suposto pela própria simetria de nome deste domínio
//     com `decorator`/`filter`/`mask-image`. A guarda `p->unit != Rml::Unit::FONTEFFECT` abaixo
//     já cobre este fato medido (cai pra `"none"`) sem precisar de caso especial.
std::string font_effect_value(Rml::Element* el) {
  const Rml::Property* p = get_prop(el, "font-effect");
  if (!p || p->unit != Rml::Unit::FONTEFFECT) return "none";
  const Rml::FontEffectsPtr effects = p->Get<Rml::FontEffectsPtr>();
  if (!effects || effects->list.empty()) return "none";
  std::fprintf(stderr,
               "rcss_dump: font-effect value declared but section 9's own composite grammar for "
               "'font-effect' does not exist yet (owner ESC-24) -- echoing 'none' (fail-high, section 11)\n");
  return "none";
}

// EN: `Style::VerticalAlign::Type`'s own keyword names -- `StyleTypes.h`'s own enum order/names,
//     hand-mapped here because, unlike a plain RCSS `keyword`-domain property,
//     `vertical-align`'s ALREADY-RESOLVED-percent form (section below, `VerticalAlignSpecial`)
//     comes from `Style::ComputedValues`, not a raw `Property` -- so `Property::ToString()`'s own
//     KEYWORD-branch trick (this file's main keyword-domain strategy) is not reachable here; a
//     small, closed, 9-entry table is the direct alternative.
// PT: Os próprios nomes de keyword de `Style::VerticalAlign::Type` -- a própria ordem/nomes de
//     enum do `StyleTypes.h`, mapeados à mão aqui porque, diferente de uma propriedade RCSS
//     comum de domínio `keyword`, a forma JÁ-RESOLVIDA-em-percentual do `vertical-align` (abaixo,
//     `VerticalAlignSpecial`) vem do `Style::ComputedValues`, não de uma `Property` crua -- então
//     o próprio truque do ramo KEYWORD de `Property::ToString()` (a estratégia principal deste
//     arquivo pra domínio `keyword`) não é alcançável aqui; uma tabela pequena, fechada, de 9
//     entradas é a alternativa direta.
const char* vertical_align_keyword(Rml::Style::VerticalAlign::Type type) {
  using VA = Rml::Style::VerticalAlign;
  switch (type) {
    case VA::Baseline:
      return "baseline";
    case VA::Middle:
      return "middle";
    case VA::Sub:
      return "sub";
    case VA::Super:
      return "super";
    case VA::TextTop:
      return "text-top";
    case VA::TextBottom:
      return "text-bottom";
    case VA::Top:
      return "top";
    case VA::Center:
      return "center";
    case VA::Bottom:
      return "bottom";
    default:
      return "baseline";
  }
}

std::string property_value(Rml::Element* el, const RegistryEntry& entry) {
  switch (entry.domain) {
    case Domain::Keyword: {
      const Rml::Property* p = get_prop(el, entry.name);
      return p ? p->ToString() : std::string();
    }
    case Domain::Number: {
      const Rml::Property* p = get_prop(el, entry.name);
      return rcss_quantize(p ? p->Get<float>() : 0.0f);
    }
    case Domain::NumberClamp01: {
      const Rml::Property* p = get_prop(el, entry.name);
      return rcss_quantize(clampf(p ? p->Get<float>() : 1.0f, 0.0f, 1.0f));
    }
    case Domain::NumberClampThreshold: {
      const Rml::Property* p = get_prop(el, entry.name);
      return rcss_quantize(clampf(p ? p->Get<float>() : 0.55f, 0.0f, 0.999f));
    }
    case Domain::Color: {
      const Rml::Property* p = get_prop(el, entry.name);
      return p ? color_hex(p->Get<Rml::Colourb>()) : color_hex(Rml::Colourb(0, 0, 0, 0));
    }
    case Domain::Length: {
      const Rml::Property* p = get_prop(el, entry.name);
      return resolve_length_px(el, p ? p->GetNumericValue() : Rml::NumericValue());
    }
    case Domain::LengthPercentAuto: {
      const Rml::Property* p = get_prop(el, entry.name);
      if (!p) return "0.0000px";
      if (p->unit == Rml::Unit::KEYWORD) return p->ToString();
      if (p->unit == Rml::Unit::PERCENT) return quantize_percent(p->Get<float>());
      return resolve_length_px(el, p->GetNumericValue());
    }
    case Domain::String: {
      const Rml::Property* p = get_prop(el, entry.name);
      return dom_dump_escape(p ? p->Get<Rml::String>() : Rml::String());
    }
    // EN: `ESC-3` -- `p == nullptr` fallback is `std::string()` here (not the per-property literal
    //     `"normal"`/`"clip"` the pre-`ESC-3` `LetterSpacing`/`TextOverflow` cases each hard-coded
    //     on their own single property): the fallback is unreachable BY CONSTRUCTION -- `get_prop()`
    //     (above) already falls back to the property's own registered default for every property
    //     that exists in the pinned RmlUi build, the same "never fewer" guarantee section 3 of
    //     docs/uix-rcss.md states for every registry entry -- and a single literal would no longer
    //     be correct once a domain serves TWO properties with two different defaults
    //     (`letter-spacing`'s own `"normal"` vs. `perspective`'s own `"none"`;
    //     `text-overflow`'s own `"clip"` vs. `nav-*`'s own `"none"`). Same idiom the plain
    //     `Domain::Keyword` case (above) already uses.
    // PT: `ESC-3` -- o fallback de `p == nullptr` é `std::string()` aqui (não o literal
    //     por-propriedade `"normal"`/`"clip"` que os cases `LetterSpacing`/`TextOverflow`
    //     pré-`ESC-3` cravavam cada um na própria propriedade única): o fallback é inalcançável
    //     POR CONSTRUÇÃO -- o `get_prop()` (acima) já cai pro próprio default registrado da
    //     propriedade pra toda propriedade que existe no build pinado do RmlUi, a mesma garantia
    //     "nunca menos" que a seção 3 do docs/uix-rcss.md declara pra toda entrada do registro --
    //     e um literal único deixaria de ser correto assim que um domínio passa a servir DUAS
    //     propriedades com dois defaults diferentes (o próprio `"normal"` de `letter-spacing`
    //     contra o próprio `"none"` de `perspective`; o próprio `"clip"` de `text-overflow` contra
    //     o próprio `"none"` dos `nav-*`). Mesmo idioma que o case `Domain::Keyword` puro (acima)
    //     já usa.
    case Domain::KeywordOrLength: {
      const Rml::Property* p = get_prop(el, entry.name);
      if (!p) return std::string();
      if (p->unit == Rml::Unit::KEYWORD) return p->ToString();
      return resolve_length_px(el, p->GetNumericValue());
    }
    case Domain::KeywordOrString: {
      const Rml::Property* p = get_prop(el, entry.name);
      if (!p) return std::string();
      if (p->unit == Rml::Unit::STRING) return dom_dump_escape(p->Get<Rml::String>());
      return p->ToString();
    }
    case Domain::KeywordOrColor: {
      const Rml::Property* p = get_prop(el, entry.name);
      if (!p) return std::string();
      if (p->unit == Rml::Unit::KEYWORD) return p->ToString();
      return color_hex(p->Get<Rml::Colourb>());
    }
    // EN: `ESC-1`'s own `font-weight` note (registered `"normal=400, bold=700"`, `StyleSheetSpeci
    //     fication.cpp:355`) needs no special case here: the KEYWORD branch already reconstructs
    //     the registered keyword text via `Property::ToString()` (this file's own header comment,
    //     lines 19-22 -- `PropertyDefinition::GetValue()`'s KEYWORD case reverse-maps the stored
    //     int through the SAME parser parameter map `AddParser("keyword", "normal=400, bold=700")`
    //     built, `PropertyDefinition.cpp:97-129`), so `font-weight: normal` (the registry default)
    //     prints `normal`, never the numeric alias -- no hand-rolled enum-to-string table, same
    //     discipline the header comment already names.
    // PT: A própria nota de `font-weight` da `ESC-1` (registrado `"normal=400, bold=700"`,
    //     `StyleSheetSpecification.cpp:355`) não precisa de caso especial aqui: o ramo KEYWORD já
    //     reconstrói o texto de keyword registrado via `Property::ToString()` (o próprio comentário
    //     de cabeçalho deste arquivo, linhas 19-22 -- o caso KEYWORD do
    //     `PropertyDefinition::GetValue()` reverse-mapeia o int guardado pelo MESMO mapa de
    //     parâmetro de parser que `AddParser("keyword", "normal=400, bold=700")` constrói,
    //     `PropertyDefinition.cpp:97-129`), então `font-weight: normal` (o default do registro)
    //     imprime `normal`, nunca o alias numérico -- nenhuma tabela enum-pra-string à mão, a
    //     mesma disciplina que o comentário de cabeçalho já nomeia.
    case Domain::KeywordOrNumber: {
      const Rml::Property* p = get_prop(el, entry.name);
      if (!p) return std::string();
      if (p->unit == Rml::Unit::KEYWORD) return p->ToString();
      return rcss_quantize(p->Get<float>());
    }
    case Domain::FontSizeSpecial: {
      return resolve_length_px(el, Rml::NumericValue(el->GetComputedValues().font_size(), Rml::Unit::PX));
    }
    case Domain::LineHeightSpecial: {
      const Rml::Style::LineHeight lh = el->GetComputedValues().line_height();
      if (lh.inherit_type == Rml::Style::LineHeight::Number) return rcss_quantize(lh.inherit_value);
      return rcss_quantize(lh.value) + "px";
    }
    case Domain::VerticalAlignSpecial: {
      const Rml::Style::VerticalAlign va = el->GetComputedValues().vertical_align();
      if (va.type == Rml::Style::VerticalAlign::Length) return rcss_quantize(va.value) + "px";
      return vertical_align_keyword(va.type);
    }
    case Domain::BoxShadowComposite:
      return box_shadow_value(el);
    case Domain::DecoratorListComposite:
      return decorator_list_value(el, entry.name);
    case Domain::FilterListComposite:
      return filter_list_value(el, entry.name);
    case Domain::AnimationComposite:
      return animation_value(el);
    case Domain::TransformComposite:
      return transform_value(el);
    case Domain::TransitionComposite:
      return transition_value(el);
    case Domain::FontEffectComposite:
      return font_effect_value(el);
  }
  return std::string();
}

// EN: Pre-order depth-first, ONE call per ELEMENT node (docs/uix-rcss.md section 2: text nodes
//     carry no `PROP` records at all -- this walk simply never calls this function for one,
//     mirroring `dom_dump.cpp`'s own `dump_node` traversal shape exactly, minus its TEXT/ELEM/ID/
//     CLASS/ATTR record emission, which is `docs/uix-dom.md`'s job, not this document's).
// PT: Pré-ordem profundidade-primeiro, UMA chamada por nó ELEMENTO (seção 2 do
//     docs/uix-rcss.md: nó de texto não carrega registro `PROP` nenhum -- esta travessia
//     simplesmente nunca chama esta função pra um -- espelha a própria forma de travessia
//     `dump_node` do dom_dump.cpp exatamente, menos a emissão de registro TEXT/ELEM/ID/CLASS/ATTR
//     dela, que é trabalho do docs/uix-dom.md, não deste documento).
void dump_element_props(Rml::Element* el, const std::string& path, std::string& out) {
  out += path;
  out += " PROPS ";
  out += std::to_string(kRegistrySize);
  out += "\n";
  for (const RegistryEntry& entry : kRegistry) {
    out += path;
    out += " PROP ";
    out += entry.name;
    out += "=";
    out += property_value(el, entry);
    out += "\n";
  }
}

void walk_state(Rml::Element* el, const std::string& path, std::string& out) {
  if (el->GetTagName() == "#text") return; // section 2: no PROP block for text nodes
  dump_element_props(el, path, out);
  const int n = el->GetNumChildren(false);
  for (int i = 0; i < n; ++i) {
    walk_state(el->GetChild(i), path + "/" + std::to_string(i), out);
  }
}

// EN: Recursively forces (or un-forces) `:hover` on `el` and every descendant -- section 4's own
//     "globally, for every element simultaneously" definition of `hover-all`. Walks ALL children
//     including text nodes (SetPseudoClass on a text-node-typed Element is a a no-op per RmlUi's
//     own selector-matching semantics -- text nodes are never `:hover`-matchable in the first
//     place -- so calling it unconditionally here is harmless, and simpler than special-casing
//     the tag name a second time in a function whose only job is the recursive walk).
// PT: Força (ou desfaz a força de) `:hover` recursivamente em `el` e todo descendente -- a
//     própria definição "globalmente, em todo elemento simultaneamente" do `hover-all` da seção
//     4. Percorre TODO filho, inclusive nó de texto (`SetPseudoClass` num Element tipado
//     nó-de-texto é no-op pela própria semântica de casamento-de-seletor do RmlUi -- nó de texto
//     nunca é casável por `:hover` de jeito nenhum) -- então chamar sem condição aqui é inofensivo,
//     e mais simples que tratar o nome de tag à parte de novo numa função cujo único trabalho é a
//     travessia recursiva.
void force_hover_recursive(Rml::Element* el, bool activate) {
  el->SetPseudoClass("hover", activate);
  const int n = el->GetNumChildren(false);
  for (int i = 0; i < n; ++i) {
    force_hover_recursive(el->GetChild(i), activate);
  }
}

} // namespace

std::string rcss_dump_document(Rml::ElementDocument* doc) {
  assert(doc != nullptr &&
         "rcss_dump_document: doc must not be null (caller must check Load()/"
         "LoadDocumentFromMemory() before calling, same discipline as dom_dump_document())");

  std::string out;

  out += "STATE none\n";
  walk_state(doc, "body", out);

  force_hover_recursive(doc, true);
  if (Rml::Context* ctx = doc->GetContext()) ctx->Update();

  out += "STATE hover-all\n";
  walk_state(doc, "body", out);

  force_hover_recursive(doc, false);
  if (Rml::Context* ctx = doc->GetContext()) ctx->Update();

  return out;
}

} // namespace glintfx
