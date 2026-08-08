// SPDX-License-Identifier: Apache-2.0
// EN: UIX-VALUE-COMPUTE -- implementation. See value_compute.hpp's own header for the full scope/
//     boundary/fail-high rationale this file holds itself to.
//
//     THE BOX-SHADOW PREMULTIPLY DIVERGENCE this item found reading
//     `examples/RmlUi/Source/Core/PropertyParserBoxShadow.cpp` directly (this task's own "onde a
//     spec e o código real do RmlUi divergirem, o código manda... reporte a divergência" clause),
//     not merely trusting docs/uix-rcss.md section 7.1's own claim: that section states colors
//     dump straight-alpha "consistent for every color-typed field, including colors nested inside
//     a box-shadow layer" -- true of the DUMP FORMAT's own stated intent, but understating what a
//     future side-A implementer (walking real `Style::ComputedValues`) must actually DO to get
//     there. `PropertyParserBoxShadow.cpp:69` calls `.ToPremultiplied()` INSIDE the parser, before
//     the `BoxShadow` struct is ever stored as the property's own cascade value --
//     `shadow.color = prop.Get<Colourb>().ToPremultiplied();`. Unlike a scalar color property
//     (`background-color`, `border-*-color`), where the CASCADE value genuinely stays straight and
//     premultiplication only happens later, at a render-consuming call site the spec's own section
//     7.1 correctly describes -- for `box-shadow` there is no separate straight-alpha value stored
//     ANYWHERE: the premultiplied color IS what `Style::ComputedValues` holds for this property.
//     A side-A dumper that "just reads the field" per section 7.1's own blanket claim would print
//     the PREMULTIPLIED value, not the straight one this document's own worked example 9.1
//     requires -- it must explicitly UN-premultiply (divide each RGB channel by its own straight
//     alpha) first, an extra step no other color-typed field in this registry needs. This module's
//     own `parse_color()` never touches premultiplication at all (it parses straight from RCSS
//     SOURCE TEXT, never from an upstream `Colourb` struct), so it naturally produces the correct
//     straight output for free -- but that is exactly why this divergence would otherwise be
//     INVISIBLE to whoever writes side A by only reading this document's own section 7.1 at face
//     value. Verified by hand-tracing the worked example: source
//     `box-shadow: #22D3EE 0dp 0dp 0dp 1dp inset, #22D3EE26 0dp 0dp 16dp 0dp;` produces
//     `#22d3eeff;...` / `#22d3ee26;...` via this module's own `compute_box_shadow()` below,
//     byte-identical to docs/uix-rcss.md section 9.1's own worked-example line -- proof this
//     module's clean-room path is correct, not merely a claim. Reported here and in this item's
//     own delivery notes, per this task's own "reporte... em vez de escolher sozinho" instruction
//     -- not silently patched into the spec by this item alone.
//
//     THE ANIMATION `delay` GAP -- see value_compute.hpp's own header, "Scope" paragraph, last
//     bullet, for the full citation (`Animation.h:10-18`, `PropertyParserAnimation.cpp:118-204`).
//     Restated here, briefly: this module does NOT implement `compute_animation_list()` at all,
//     because docs/uix-rcss.md section 9.3's own grammar has no field for a real, populated struct
//     member upstream's own parser writes -- implementing against an incomplete contract would be
//     guessing the byte-exact form of a field the spec itself never named, exactly what this
//     document's own header warns a second implementer must not do.
// PT: UIX-VALUE-COMPUTE -- implementação. Ver o próprio cabeçalho do value_compute.hpp pro
//     escopo/fronteira/racional-fail-high completos a que este arquivo se prende.
//
//     A DIVERGÊNCIA DE PREMULTIPLICAÇÃO DO BOX-SHADOW que este item achou lendo direto o
//     `examples/RmlUi/Source/Core/PropertyParserBoxShadow.cpp` (a própria cláusula "onde a spec e
//     o código real do RmlUi divergirem, o código manda... reporte a divergência" desta tarefa),
//     não só confiando na própria alegação da seção 7.1 do docs/uix-rcss.md: aquela seção declara
//     que cores despejam alpha reto "consistente pra todo campo tipo-cor, incluindo cores aninhadas
//     dentro de uma camada de box-shadow" -- verdadeiro sobre a própria intenção declarada do
//     FORMATO de dump, mas subestimando o que um futuro implementer do lado A (percorrendo o
//     `Style::ComputedValues` real) precisa de fato FAZER pra chegar lá.
//     `PropertyParserBoxShadow.cpp:69` chama `.ToPremultiplied()` DENTRO do parser, antes do struct
//     `BoxShadow` ser sequer armazenado como o próprio valor de cascata da propriedade --
//     `shadow.color = prop.Get<Colourb>().ToPremultiplied();`. Diferente de uma propriedade de cor
//     escalar (`background-color`, `border-*-color`), onde o valor de CASCATA genuinamente fica
//     reto e a premultiplicação só acontece depois, num call site consumidor-de-render que a
//     própria seção 7.1 da spec descreve corretamente -- pro `box-shadow` não há valor
//     alpha-reto separado armazenado EM LUGAR NENHUM: a cor premultiplicada É o que o
//     `Style::ComputedValues` guarda pra esta propriedade. Um dumper do lado A que "só lê o campo"
//     per a própria alegação geral da seção 7.1 imprimiria o valor PREMULTIPLICADO, não o reto que
//     o próprio exemplo trabalhado 9.1 deste documento exige -- ele precisa DES-premultiplicar
//     explicitamente (dividir cada canal RGB pelo próprio alpha reto dele) primeiro, um passo extra
//     que nenhum outro campo tipo-cor deste registro precisa. O próprio `parse_color()` deste
//     módulo nunca toca premultiplicação nenhuma (ele parseia direto do TEXTO DE FONTE RCSS, nunca
//     de um struct `Colourb` do upstream), então ele naturalmente produz a saída reta correta de
//     graça -- mas é exatamente por isso que esta divergência ficaria INVISÍVEL pra quem escrever o
//     lado A só lendo a própria seção 7.1 deste documento de cara. Verificado rastreando à mão o
//     exemplo trabalhado: fonte
//     `box-shadow: #22D3EE 0dp 0dp 0dp 1dp inset, #22D3EE26 0dp 0dp 16dp 0dp;` produz
//     `#22d3eeff;...` / `#22d3ee26;...` via o próprio `compute_box_shadow()` deste módulo abaixo,
//     byte-idêntico à própria linha de exemplo trabalhado da seção 9.1 do docs/uix-rcss.md -- prova
//     de que o caminho clean-room deste módulo está correto, não só uma alegação. Reportado aqui e
//     nas próprias notas de entrega deste item, per a própria instrução "reporte... em vez de
//     escolher sozinho" desta tarefa -- não consertado em silêncio na spec por este item sozinho.
//
//     A LACUNA `delay` DO ANIMATION -- ver o próprio cabeçalho do value_compute.hpp, parágrafo
//     "Escopo", último bullet, pra citação completa (`Animation.h:10-18`,
//     `PropertyParserAnimation.cpp:118-204`). Restatado aqui, brevemente: este módulo NÃO
//     implementa `compute_animation_list()` de jeito nenhum, porque a própria gramática da seção
//     9.3 do docs/uix-rcss.md não tem campo pra um membro de struct real, populado, que o próprio
//     parser do upstream escreve -- implementar contra um contrato incompleto seria chutar a forma
//     byte-exata de um campo que a própria spec nunca nomeou, exatamente o que o próprio cabeçalho
//     daquele documento avisa que um segundo implementer não deve fazer.
// Copyright (c) 2026 Petrus Silva Costa
//
// EN: ⚠️ PENDING `UIX-RCSS-ERRATA-2` (in flight at the time this item was delivered, written by a
//     DIFFERENT agent auditing all 68 normative rules of docs/uix-rcss.md against real upstream
//     RmlUi source -- the orchestrator relayed 3 confirmed findings that touch THIS module's own
//     scope while this item was mid-implementation, with an explicit instruction not to refactor
//     around them yet, only to record and report, so the errata and both oracle-side
//     implementers receive the SAME correction at the SAME time). Recorded here, verbatim, as this
//     item's own delivery notes require -- current code below still matches the SPEC TEXT AS READ
//     at the time this item started (`f747ae8`), not yet these 3 findings:
//       1. **Box-shadow AND gradient-stop colors are PREMULTIPLIED, guaranteed by the upstream
//          type system itself** -- `DecorationTypes.h:9` (`ColorStop`) and `:22` (`BoxShadow`)
//          both declare `ColourbPremultiplied color;`, not a straight `Colourb`. This is a
//          STRONGER finding than the box-shadow-only divergence this file's OWN header above
//          already reported (that one said side A needs an extra un-premultiply STEP to reach a
//          straight-alpha dump; this one says gradient stops carry the SAME issue, AND that the
//          CORRECT DUMP OUTPUT itself may need to be premultiplied, not straight, once
//          `UIX-RCSS-ERRATA-2` lands -- undoing the very worked example 9.1 this file's own
//          `compute_box_shadow()` currently reproduces byte-exact). Consequence for THIS module:
//          `parse_color()`/`print_color()` are unchanged (this module parses straight from RCSS
//          SOURCE TEXT, never from an upstream `Colourb`, so it has no premultiplication step to
//          add or remove on its OWN account) -- but `compute_box_shadow()` and the gradient-stop
//          path inside `compute_linear_gradient_args()`/`compute_radial_gradient_args()` may need
//          an explicit PREMULTIPLY step inserted (color channel * alpha / 255, rounded) once the
//          errata's own corrected worked example lands, to match the corrected dump contract. NOT
//          done yet, per explicit instruction -- this module's own test suite still encodes the
//          CURRENT (pre-errata) worked-example text.
//       2. **`filter`/`backdrop-filter` separate by SPACE, not comma** (only `decorator` uses
//          comma) -- `PropertyParserFilter.cpp:32` vs. `PropertyParserDecorator.cpp:55`. Section
//          9.2's own "all four share an identical comma-list shape" claim is false for these two.
//          **No code change needed in this module**: `scan_function_calls()` (this file, `namespace
//          {}` block) was already written comma/whitespace-AGNOSTIC (it treats both as
//          inter-function-call separator, skipped identically) -- a deliberate generality, not a
//          fix aimed at this specific finding, that happens to already cover it. Verified by
//          re-reading this function's own logic, not merely asserted.
//       3. **A malformed entry inside `decorator`/`mask-image`/`filter`/`backdrop-filter` drops
//          the WHOLE property, not just that one entry** -- upstream's own parser loops
//          (`PropertyParserDecorator.cpp` lines 49/60/72/83) `return false` on a bad entry, never
//          `continue`. Section 11's own "that single decorator entry is dropped from its
//          property's list" claim is false. **This DOES contradict this module's own
//          `compute_decorator_list()`/`compute_transform_list()` design** (both currently drop
//          only the failing entry and keep the rest, per section 11's PRE-errata text, and cannot
//          fail as a whole by their own public contract in value_compute.hpp) -- NOT changed yet,
//          per explicit instruction; this is this item's own single largest pending-errata
//          consequence, likely a signature-shape change (from "always succeeds, returns
//          `std::string`" to a `ValueComputeStatus`-returning form) once `UIX-RCSS-ERRATA-2` is
//          published. `compute_box_shadow()` already implements whole-declaration-drop (matches
//          the errata's own expectation) -- no change needed there.
//     None of this changes the quantization algorithm itself (the líder's own round-both-sides-
//     before-printing decision, restated by the orchestrator as untouched by this errata).
// PT: ⚠️ `UIX-RCSS-ERRATA-2` PENDENTE (em voo no momento em que este item foi entregue, escrita por
//     um agente DIFERENTE auditando as 68 regras normativas do docs/uix-rcss.md contra o fonte
//     real do RmlUi upstream -- o orquestrador repassou 3 achados confirmados que tocam o próprio
//     escopo deste módulo enquanto este item estava em implementação, com instrução explícita de
//     não refatorar em torno deles ainda, só registrar e reportar, pra errata e os dois
//     implementers do lado do oráculo receberem a MESMA correção ao MESMO tempo). Registrado aqui,
//     verbatim, per as próprias notas de entrega que este item exige -- o código abaixo ainda casa
//     com o TEXTO DA SPEC COMO LIDO no momento em que este item começou (`f747ae8`), não ainda com
//     estes 3 achados:
//       1. **Cores de box-shadow E de parada de gradiente são PREMULTIPLICADAS, garantido pelo
//          próprio sistema de tipos do upstream** -- `DecorationTypes.h:9` (`ColorStop`) e `:22`
//          (`BoxShadow`) os dois declaram `ColourbPremultiplied color;`, não um `Colourb` reto.
//          Este é um achado MAIS FORTE que a divergência só-de-box-shadow que o PRÓPRIO cabeçalho
//          deste arquivo acima já reportou (aquele dizia que o lado A precisa de um PASSO extra de
//          des-premultiplicar pra chegar num dump alpha-reto; este diz que paradas de gradiente
//          carregam o MESMO problema, E que a PRÓPRIA SAÍDA DE DUMP correta pode precisar ser
//          premultiplicada, não reta, uma vez que a `UIX-RCSS-ERRATA-2` pousar -- desfazendo o
//          próprio exemplo trabalhado 9.1 que o próprio `compute_box_shadow()` deste arquivo
//          reproduz byte-exato hoje). Consequência pra ESTE módulo: `parse_color()`/`print_color()`
//          ficam inalterados (este módulo parseia reto do TEXTO DE FONTE RCSS, nunca de um
//          `Colourb` do upstream, então não tem passo de premultiplicação pra somar ou tirar por
//          conta PRÓPRIA) -- mas `compute_box_shadow()` e o caminho de parada-de-gradiente dentro
//          do `compute_linear_gradient_args()`/`compute_radial_gradient_args()` podem precisar de
//          um passo explícito de PREMULTIPLICAR (canal de cor * alpha / 255, arredondado) uma vez
//          que o exemplo trabalhado corrigido da errata pousar, pra casar com o contrato de dump
//          corrigido. NÃO feito ainda, per instrução explícita -- a própria suíte de teste deste
//          módulo ainda codifica o texto de exemplo trabalhado ATUAL (pré-errata).
//       2. **`filter`/`backdrop-filter` separam por ESPAÇO, não vírgula** (só `decorator` usa
//          vírgula) -- `PropertyParserFilter.cpp:32` vs. `PropertyParserDecorator.cpp:55`. A
//          própria alegação "as quatro compartilham forma idêntica de lista-vírgula" da seção 9.2
//          é falsa pra estas duas. **Nenhuma mudança de código necessária neste módulo**: o
//          `scan_function_calls()` (este arquivo, bloco `namespace {}`) já foi escrito
//          AGNÓSTICO-a-vírgula/whitespace (ele trata os dois como separador inter-chamada-de-
//          função, pulados identicamente) -- uma generalidade deliberada, não um conserto mirado
//          neste achado específico, que calha de já cobrir isso. Verificado relendo a própria
//          lógica desta função, não só alegado.
//       3. **Uma entrada malformada dentro de `decorator`/`mask-image`/`filter`/`backdrop-filter`
//          derruba a PROPRIEDADE INTEIRA, não só aquela entrada** -- os próprios laços de parser do
//          upstream (`PropertyParserDecorator.cpp` linhas 49/60/72/83) fazem `return false` numa
//          entrada ruim, nunca `continue`. A própria alegação "esta entrada de decorator é
//          derrubada da própria lista da propriedade dela" da seção 11 é falsa. **Isto CONTRADIZ o
//          próprio design do `compute_decorator_list()`/`compute_transform_list()` deste módulo**
//          (os dois hoje derrubam só a entrada que falha e mantêm o resto, per o texto PRÉ-errata
//          da seção 11, e não conseguem falhar como um todo pelo próprio contrato público deles no
//          value_compute.hpp) -- NÃO mudado ainda, per instrução explícita; esta é a própria maior
//          consequência-pendente-de-errata deste item, provavelmente uma mudança de forma de
//          assinatura (de "sempre sucede, retorna `std::string`" pra uma forma retornando
//          `ValueComputeStatus`) uma vez que a `UIX-RCSS-ERRATA-2` for publicada.
//          `compute_box_shadow()` já implementa derrubada-da-declaração-inteira (casa com a própria
//          expectativa da errata) -- nenhuma mudança necessária ali.
//     Nada disto muda o próprio algoritmo de quantização (a própria decisão do líder de arredondar
//     os dois lados igual antes de imprimir, restatada pelo orquestrador como intocada por esta
//     errata).
#include "value_compute.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iterator>

namespace glintfx::uix::style {

// EN: Forward declaration -- `compute_one_decorator_function` (defined further below, at THIS
//     namespace's own scope, not inside an anonymous namespace) is mutually referenced by
//     `polygon()`'s own recursive `<fill>` argument (a nested `linear-gradient(...)`/
//     `radial-gradient(...)`, docs/uix-rcss.md section 9.2's own table) and by
//     `compute_decorator_list()` (value_compute.hpp's own public function). `depth` is
//     `kMaxNestDepth`-bounded per value_compute.hpp's own header "Teto". Declared HERE, at
//     namespace scope (not inside the anonymous namespace below), so this declaration and its own
//     later definition share the same (external) linkage -- an anonymous-namespace forward
//     declaration of the same signature would be a DIFFERENT, internal-linkage entity, ambiguous
//     against the real one at every call site that can see both.
// PT: Declaração adiantada -- `compute_one_decorator_function` (definida mais abaixo, no PRÓPRIO
//     escopo deste namespace, não dentro de um namespace anônimo) é mutuamente referenciada pelo
//     próprio argumento `<fill>` recursivo do `polygon()` (um `linear-gradient(...)`/
//     `radial-gradient(...)` aninhado, a própria tabela da seção 9.2 do docs/uix-rcss.md) e pelo
//     `compute_decorator_list()` (a própria função pública do value_compute.hpp). `depth` é
//     delimitado por `kMaxNestDepth` per o próprio "Teto" do cabeçalho do value_compute.hpp.
//     Declarada AQUI, em escopo de namespace (não dentro do namespace anônimo abaixo), pra esta
//     declaração e a própria definição posterior dela compartilharem a mesma vinculação (externa)
//     -- uma declaração adiantada em namespace anônimo da mesma assinatura seria uma entidade
//     DIFERENTE, de vinculação interna, ambígua contra a real em todo call site que consiga ver as
//     duas.
ValueComputeStatus compute_one_decorator_function(std::string_view name, std::string_view inner,
                                                  const LengthResolveContext& ctx, int depth,
                                                  std::string* out);

namespace {

// EN: Same 4-character whitespace set lexer.cpp's own `is_whitespace` uses (space, '\t', '\n',
//     '\r') -- see that file's own header for why this exact set, restated here rather than
//     included from it (this module stays a sibling, not a dependent, of lexer.hpp -- see
//     value_compute.hpp's own header, "Scope").
// PT: O mesmo conjunto de 4 caracteres de whitespace que o próprio `is_whitespace` do lexer.cpp usa
//     (espaço, '\t', '\n', '\r') -- ver o próprio cabeçalho daquele arquivo pro porquê deste
//     conjunto exato, restatado aqui em vez de incluído dele (este módulo fica irmão, não
//     dependente, do lexer.hpp -- ver "Escopo" no próprio cabeçalho do value_compute.hpp).
constexpr bool is_ws(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::string_view trim(std::string_view s) {
  std::size_t begin = 0;
  while (begin < s.size() && is_ws(s[begin])) {
    ++begin;
  }
  std::size_t end = s.size();
  while (end > begin && is_ws(s[end - 1])) {
    --end;
  }
  return s.substr(begin, end - begin);
}

std::string to_lower(std::string_view s) {
  std::string out(s);
  std::transform(out.begin(), out.end(), out.begin(), [](char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
  });
  return out;
}

// EN: `UIX-RCSS-ERRATA-4`'s own decision (in flight at the time this item was delivered,
//     REVERSING `UIX-RCSS-ERRATA-2`'s own earlier "print the premultiplied bytes as-is" call, per
//     an explicit correction relayed by the orchestrator, sourced from `TypeConverter.cpp`, read
//     directly): `box-shadow` layer colors and every gradient-stop color (inside
//     `linear-gradient`/`radial-gradient`/`horizontal-gradient`/`vertical-gradient`) are stored
//     PREMULTIPLIED internally by the old engine (`PropertyParserColorStopList.cpp:47`/
//     `PropertyParserBoxShadow.cpp:72`, both `.ToPremultiplied()`, unchanged fact from
//     `UIX-RCSS-ERRATA-2`) -- but the PRINTED/DUMPED value is neither the raw source color nor the
//     raw premultiplied bytes: upstream's own text-serialization path
//     (`TypeConverter.cpp:223`/`:256`) calls `.ToNonPremultiplied()` on the ALREADY-premultiplied
//     stored value before writing it out, so the golden this dump format must match is the LOSSY
//     PREMULTIPLY-THEN-UN-PREMULTIPLY ROUND-TRIP, not either endpoint alone. `alpha=0` IS
//     well-defined here (`Include/RmlUi/Core/Colour.h:105-107`'s own explicit guard,
//     `ColourType(alpha > 0 ? (red*255)/alpha : 0)`) -- this item's own earlier belief that
//     un-premultiplying was undefined at `alpha=0` was WRONG, corrected by the orchestrator's own
//     direct citation, not independently re-derived by this item. Both steps are **integer**
//     multiplication then **integer, truncating** division, matching `ToPremultiplied()`'s
//     `ColourType(red*alpha/255)` and `ToNonPremultiplied()`'s `ColourType((red*255)/alpha)`
//     exactly -- NOT rounded float computation at either step, and the two truncations do NOT
//     cancel out (proven by the orchestrator's own measured table, reverified independently by
//     this item's own `test_box_shadow_color_lossy_roundtrip_orchestrator_table`: `#22d3ee80`
//     round-trips to `#21d1ed80`, neither the source nor the bare-premultiplied intermediate).
//     Every OTHER color-typed field in this registry (`background-color`, `border-*-color`,
//     `color`, `image-tint-color`, `polygon()`'s own solid `<fill>`, `drop-shadow`'s own color)
//     stays straight, untouched by this helper -- called ONLY at the two call sites named above,
//     never made the default path for `parse_color()`/`print_color()` themselves.
// PT: A própria decisão da `UIX-RCSS-ERRATA-4` (em voo no momento em que este item foi entregue,
//     REVERTENDO a própria chamada anterior "imprime os bytes premultiplicados como estão" da
//     `UIX-RCSS-ERRATA-2`, per uma correção explícita repassada pelo orquestrador, com fonte no
//     `TypeConverter.cpp`, lido direto): cores de camada de `box-shadow` e toda cor de stop de
//     gradiente (dentro de `linear-gradient`/`radial-gradient`/`horizontal-gradient`/
//     `vertical-gradient`) são armazenadas PREMULTIPLICADAS internamente pelo motor antigo
//     (`PropertyParserColorStopList.cpp:47`/`PropertyParserBoxShadow.cpp:72`, os dois
//     `.ToPremultiplied()`, fato inalterado da `UIX-RCSS-ERRATA-2`) -- mas o valor
//     IMPRESSO/DESPEJADO não é nem a cor de fonte crua nem os bytes premultiplicados crus: o
//     próprio caminho de serialização-pra-texto do upstream (`TypeConverter.cpp:223`/`:256`)
//     chama `.ToNonPremultiplied()` sobre o valor JÁ premultiplicado armazenado antes de escrevê-lo,
//     então o golden que este formato de dump precisa casar é a IDA-E-VOLTA COM PERDA
//     premultiplicar-depois-des-premultiplicar, não nenhuma das duas pontas sozinha. `alpha=0` É
//     bem-definido aqui (a própria guarda explícita do `Include/RmlUi/Core/Colour.h:105-107`,
//     `ColourType(alpha > 0 ? (red*255)/alpha : 0)`) -- a própria crença anterior deste item de que
//     des-premultiplicar era indefinido em `alpha=0` estava ERRADA, corrigida pela própria citação
//     direta do orquestrador, não re-derivada independentemente por este item. Os dois passos são
//     multiplicação **inteira** depois divisão **inteira, truncando**, casando exatamente com o
//     `ColourType(red*alpha/255)` do `ToPremultiplied()` e o `ColourType((red*255)/alpha)` do
//     `ToNonPremultiplied()` -- NÃO computação float arredondada em nenhum dos dois passos, e as
//     duas truncagens NÃO se cancelam (provado pela própria tabela medida do orquestrador,
//     reverificada independentemente pelo próprio
//     `test_box_shadow_color_lossy_roundtrip_orchestrator_table` deste item: `#22d3ee80` faz
//     ida-e-volta pra `#21d1ed80`, nem a fonte nem o intermediário puramente-premultiplicado).
//     Todo OUTRO campo tipo-cor deste registro (`background-color`, `border-*-color`, `color`,
//     `image-tint-color`, o próprio `<fill>` sólido do `polygon()`, a própria cor do
//     `drop-shadow`) fica reto, intocado por este helper -- chamado SÓ nos dois call sites
//     nomeados acima, nunca virado o caminho default do `parse_color()`/`print_color()` em si.
Rgba8 premultiply(const Rgba8& c) {
  Rgba8 out;
  out.r = static_cast<std::uint8_t>((static_cast<int>(c.r) * c.a) / 255);
  out.g = static_cast<std::uint8_t>((static_cast<int>(c.g) * c.a) / 255);
  out.b = static_cast<std::uint8_t>((static_cast<int>(c.b) * c.a) / 255);
  out.a = c.a;
  return out;
}

Rgba8 non_premultiply(const Rgba8& c) {
  if (c.a == 0) {
    return Rgba8{0, 0, 0, 0};
  }
  Rgba8 out;
  out.r = static_cast<std::uint8_t>((static_cast<int>(c.r) * 255) / c.a);
  out.g = static_cast<std::uint8_t>((static_cast<int>(c.g) * 255) / c.a);
  out.b = static_cast<std::uint8_t>((static_cast<int>(c.b) * 255) / c.a);
  out.a = c.a;
  return out;
}

// EN: The one function every `box-shadow`/gradient-stop color call site should actually call --
//     applies `premultiply()` then `non_premultiply()` in sequence, the exact lossy round-trip
//     upstream's own storage-then-serialize pipeline performs. Named for what it computes, not for
//     either individual step, so a future reader does not have to re-derive "which two calls, in
//     which order" from first principles at every call site.
// PT: A única função que todo call site de cor de `box-shadow`/stop-de-gradiente deveria de fato
//     chamar -- aplica `premultiply()` depois `non_premultiply()` em sequência, a ida-e-volta com
//     perda exata que o próprio pipeline armazenar-depois-serializar do upstream executa. Nomeada
//     pelo que computa, não por nenhum dos dois passos individuais, pra um futuro leitor não
//     precisar re-derivar "quais duas chamadas, em qual ordem" a partir do zero em todo call site.
Rgba8 dump_box_shadow_or_gradient_stop_color(const Rgba8& straight_source_color) {
  return non_premultiply(premultiply(straight_source_color));
}

// EN: Parses `s` as a plain, whole-string float via `std::strtof` -- rejects any leftover byte
//     (trailing garbage), rejects empty input. Returns false on any failure, never partial-parses.
//     `UIX-RCSS-ERRATA-2`'s own closed `Finding J` (docs/uix-rcss.md section 8: "quantize() is
//     defined only for finite x... never printed"): also rejects a non-finite RESULT
//     (`std::isfinite`) -- `strtof` itself successfully parses the C-standard literals `"nan"`/
//     `"inf"`/`"infinity"` (case-insensitive, optionally signed) into a genuine NaN/Infinity
//     `float`, so hostile RCSS text like `"nanpx"`/`"-infdeg"` would otherwise flow through this
//     module's every downstream `parse_length`/`parse_percent`/`parse_angle` call looking like an
//     ordinary, successfully-parsed number. THIS is this module's own enforcement point for
//     `Finding J` -- catching it here, at the one shared numeric-token parser every other parse
//     function in this file funnels through, means no individual caller has to remember to
//     re-check `isfinite` itself.
// PT: Parseia `s` como um float, string-inteira, via `std::strtof` -- rejeita qualquer byte
//     sobrando (lixo à direita), rejeita input vazio. Retorna falso em qualquer falha, nunca
//     parseia parcial. O próprio `Finding J`, fechado pela `UIX-RCSS-ERRATA-2` (seção 8 do
//     docs/uix-rcss.md: "quantize() só é definido pra x finito... nunca impresso"): também rejeita
//     um RESULTADO não-finito (`std::isfinite`) -- o próprio `strtof` parseia com sucesso os
//     literais do padrão C `"nan"`/`"inf"`/`"infinity"` (sem-distinção-de-caixa, opcionalmente
//     assinados) pra um `float` NaN/Infinito genuíno, então texto RCSS hostil tipo `"nanpx"`/
//     `"-infdeg"` senão fluiria por toda chamada `parse_length`/`parse_percent`/`parse_angle`
//     posterior deste módulo parecendo um número comum, parseado com sucesso. ESTE é o próprio
//     ponto de aplicação deste módulo pro `Finding J` -- pegando aqui, no único parser de token
//     numérico compartilhado que toda outra função de parse deste arquivo atravessa, significa que
//     nenhum chamador individual precisa lembrar de reconferir `isfinite` sozinho.
bool parse_float_token(std::string_view s, float* out) {
  if (s.empty() || s.size() > 64) {
    return false;
  }
  char buf[65];
  std::size_t n = s.size();
  for (std::size_t i = 0; i < n; ++i) {
    buf[i] = s[i];
  }
  buf[n] = '\0';
  char* end = nullptr;
  float v = std::strtof(buf, &end);
  if (end != buf + n) {
    return false;
  }
  if (!std::isfinite(v)) {
    return false;
  }
  *out = v;
  return true;
}

// EN: Paren-aware top-level split (mirrors upstream's own
//     `StringUtilities::ExpandString(..., delim, '(', ')')`, cited per-caller in
//     value_compute.hpp) -- `delim` is only recognised at parenthesis depth 0, so a nested
//     function call's own internal commas never split its OUTER argument. Each piece is trimmed.
//     An empty `s` produces a single empty piece (matching `ExpandString`'s own behaviour of never
//     returning zero pieces for non-empty splits) -- callers that need to reject "nothing here" do
//     so themselves by checking the piece for emptiness, same as upstream's own callers do.
// PT: Split de topo-de-nível consciente-de-parênteses (espelha o próprio
//     `StringUtilities::ExpandString(..., delim, '(', ')')` do upstream, citado por-chamador no
//     value_compute.hpp) -- `delim` só é reconhecido em profundidade de parêntese 0, então as
//     próprias vírgulas internas de uma chamada de função aninhada nunca dividem o próprio
//     argumento EXTERNO. Cada pedaço é trimado. Um `s` vazio produz um único pedaço vazio (casando
//     com o próprio comportamento do `ExpandString` de nunca retornar zero pedaços pra splits
//     não-vazios) -- chamadores que precisam rejeitar "nada aqui" fazem isso eles mesmos checando o
//     pedaço por vacuidade, igual aos próprios chamadores do upstream fazem.
std::vector<std::string_view> split_top_level(std::string_view s, char delim) {
  std::vector<std::string_view> out;
  std::size_t start = 0;
  int depth = 0;
  for (std::size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (c == '(') {
      ++depth;
    } else if (c == ')') {
      if (depth > 0) {
        --depth;
      }
    } else if (c == delim && depth == 0) {
      out.push_back(trim(s.substr(start, i - start)));
      start = i + 1;
    }
  }
  out.push_back(trim(s.substr(start)));
  return out;
}

// EN: `ESC-6` -- PAREN-AWARE whitespace tokenizer (upgraded from the pre-`ESC-6` plain version,
//     "no paren awareness needed", which this comment used to claim -- that claim is now FALSE and
//     is corrected in place, not left stale, per this file's own house rule). The pin's own
//     tokenizer for every space-separated grammar this function serves (`box-shadow` layers --
//     `PropertyParserBoxShadow.cpp`'s own `ExpandString(..., ' ', '(', ')')`; gradient stops --
//     `PropertyParserColorStopList.cpp`'s own identical call shape) treats `(`/`)` as its own
//     depth-tracked quote pair, exactly like this file's own `split_top_level()` above already does
//     for its comma-delimited callers -- a space INSIDE a parenthesized argument list never splits
//     the token. Pre-`ESC-6` this function had no such tracking because nothing tokenized by it
//     could contain a `(` at all (a hex color, a bare keyword, a length -- none nest parens); `ESC-6`
//     breaks that premise the moment a functional color form (`rgb(255, 0, 0)`, itself containing
//     BOTH an internal comma-space run) can appear as one of these space-separated tokens (a
//     `box-shadow` color, a gradient-stop color, a `drop-shadow`/straight-gradient color) --
//     `box-shadow: 2px 2px rgb(255, 0, 0)` would otherwise shatter into `rgb(255,`/`0,`/`0)` as three
//     bogus tokens instead of the one color argument it actually is. Depth counter mirrors
//     `split_top_level()`'s own: `(` increments, `)` decrements (never below 0, an unbalanced
//     trailing `)` is simply absorbed rather than underflowing), and whitespace only ends a token at
//     depth 0. Benefits every call site of this function uniformly (`parse_gradient_stop`,
//     `compute_box_shadow`'s own per-layer split, `compute_radial_gradient_args`'s own
//     `circle at X% Y%` clause -- never contains a paren, so unaffected either way --
//     `compute_two_stop_straight_gradient`, `compute_drop_shadow`) with ONE change, not five
//     independent ones. Empty tokens are never produced.
// PT: `ESC-6` -- tokenizador de whitespace CONSCIENTE-DE-PARÊNTESE (alargado da versão pré-`ESC-6`,
//     puramente plana, "sem consciência de parêntese necessária", que este comentário costumava
//     alegar -- essa alegação agora é FALSA e é corrigida no lugar, não deixada obsoleta, pela
//     própria regra da casa deste arquivo). O próprio tokenizador do pin pra toda gramática
//     separada-por-espaço que esta função serve (camadas de `box-shadow` -- o próprio
//     `ExpandString(..., ' ', '(', ')')` do `PropertyParserBoxShadow.cpp`; stops de gradiente -- a
//     própria forma de chamada idêntica do `PropertyParserColorStopList.cpp`) trata `(`/`)` como o
//     próprio par de aspas rastreado-por-profundidade dele, exatamente como o próprio
//     `split_top_level()` deste arquivo acima já faz pros próprios chamadores separados-por-vírgula
//     -- um espaço DENTRO de uma lista de argumento entre parênteses nunca divide o token. Pré-`ESC-6`
//     esta função não tinha rastreamento nenhum assim porque nada tokenizado por ela conseguia conter
//     um `(` de jeito nenhum (uma cor hex, uma palavra-chave crua, um comprimento -- nenhum aninha
//     parêntese); a `ESC-6` quebra essa premissa no momento em que uma forma funcional de cor
//     (`rgb(255, 0, 0)`, ela mesma contendo um trecho de vírgula-espaço interno) pode aparecer como
//     um destes tokens separados-por-espaço (uma cor de `box-shadow`, uma cor de stop de gradiente,
//     uma cor de `drop-shadow`/gradiente reto) -- `box-shadow: 2px 2px rgb(255, 0, 0)` senão
//     estilhaçaria em `rgb(255,`/`0,`/`0)` como três tokens bogus em vez do único argumento de cor
//     que de fato é. Contador de profundidade espelha o próprio `split_top_level()`: `(` incrementa,
//     `)` decrementa (nunca abaixo de 0, um `)` sobrando no final é simplesmente absorvido em vez de
//     estourar por baixo), e whitespace só termina um token em profundidade 0. Beneficia todo call
//     site desta função uniformemente (`parse_gradient_stop`, o próprio split por-camada do
//     `compute_box_shadow`, a própria cláusula `circle at X% Y%` do `compute_radial_gradient_args`
//     -- nunca contém parêntese, então inalterada de qualquer jeito --, `compute_two_stop_straight_
//     gradient`, `compute_drop_shadow`) com UMA mudança, não cinco independentes. Tokens vazios
//     nunca são produzidos.
std::vector<std::string_view> split_whitespace(std::string_view s) {
  std::vector<std::string_view> out;
  std::size_t i = 0;
  std::size_t n = s.size();
  int depth = 0;
  while (i < n) {
    while (i < n && depth == 0 && is_ws(s[i])) {
      ++i;
    }
    std::size_t start = i;
    while (i < n && (depth > 0 || !is_ws(s[i]))) {
      if (s[i] == '(') {
        ++depth;
      } else if (s[i] == ')') {
        if (depth > 0) {
          --depth;
        }
      }
      ++i;
    }
    if (i > start) {
      out.push_back(s.substr(start, i - start));
    }
  }
  return out;
}

bool ends_with(std::string_view s, std::string_view suffix) {
  return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

// EN: `ESC-4` -- declarative suffix->unit table mirroring the pin's own `unit_string_map`
//     (`PropertyParserNumber.cpp:6-24`), restricted to the `Unit::LENGTH` family (`Unit.h:58`) --
//     see value_compute.hpp's own `parse_length()` doc-comment for the full "why a table, not
//     ends_with" rationale. Does NOT include `""`/`"%"`/`"x"`/`"deg"`/`"rad"` (NUMBER/PERCENT/X/
//     DEG/RAD are each some OTHER function's own domain in this module -- see
//     `kResolutionUnitTable` below for `x`'s own separate, single-entry table).
// PT: `ESC-4` -- tabela declarativa sufixo->unidade espelhando o próprio `unit_string_map` do pin
//     (`PropertyParserNumber.cpp:6-24`), restrita à família `Unit::LENGTH` (`Unit.h:58`) -- ver o
//     próprio doc-comment do `parse_length()` no value_compute.hpp pro racional completo "por que
//     uma tabela, não ends_with". NÃO inclui `""`/`"%"`/`"x"`/`"deg"`/`"rad"` (NUMBER/PERCENT/X/DEG/
//     RAD são cada um domínio de OUTRA função deste módulo -- ver `kResolutionUnitTable` abaixo pra
//     própria tabela separada, de entrada única, do `x`).
struct LengthUnitEntry {
  std::string_view suffix;
  LengthUnit unit = LengthUnit::Px;
};
constexpr LengthUnitEntry kLengthUnitTable[] = {
    {"px", LengthUnit::Px},
    {"dp", LengthUnit::Dp},
    {"em", LengthUnit::Em},
    {"rem", LengthUnit::Rem},
    {"vw", LengthUnit::Vw},
    {"vh", LengthUnit::Vh},
    {"in", LengthUnit::In},
    {"cm", LengthUnit::Cm},
    {"mm", LengthUnit::Mm},
    {"pt", LengthUnit::Pt},
    {"pc", LengthUnit::Pc},
};

// EN: `ESC-4` -- reverse scan for the number/unit boundary, transcribing
//     `PropertyParserNumber.cpp:45-55` verbatim (upstream's own comment there: "Find the beginning
//     of the unit string in 'value'"): the byte right after the LAST character (scanning backwards
//     from the end) that is a digit or whitespace. A `raw` with no digit/whitespace at all (e.g.
//     `"nanpx"`) returns 0, matching the pin's own loop-completes-without-writing-`unit_pos`
//     fallthrough -- the "number half" is then empty, which `parse_float_token()`'s own `s.empty()`
//     guard already rejects, so this shape fails downstream, not here, same as upstream's own
//     `strtof("", ...)` failure. `i-- > 0` here (rather than upstream's own bare `i--`, an
//     equivalent post-decrement-while-nonzero idiom for `size_t`) is the same loop, written for
//     clarity over cleverness.
// PT: `ESC-4` -- scan reverso pra fronteira número/unidade, transcrevendo
//     `PropertyParserNumber.cpp:45-55` verbatim (o próprio comentário do upstream ali: "Find the
//     beginning of the unit string in 'value'"): o byte logo depois do ÚLTIMO caractere (escaneando
//     de trás pra frente a partir do fim) que é dígito ou whitespace. Um `raw` sem dígito/whitespace
//     nenhum (ex. `"nanpx"`) retorna 0, casando com o próprio fallthrough do pin de
//     laço-completa-sem-escrever-`unit_pos` -- a "metade número" fica vazia então, que a própria
//     guarda `s.empty()` do `parse_float_token()` já rejeita, então esta forma falha rio-abaixo, não
//     aqui, igual à própria falha `strtof("", ...)` do upstream. `i-- > 0` aqui (em vez do próprio
//     `i--` cru do upstream, um idioma equivalente de decrementa-enquanto-não-zero pra `size_t`) é o
//     mesmo laço, escrito pra clareza em vez de esperteza.
std::size_t find_unit_boundary(std::string_view raw) {
  for (std::size_t i = raw.size(); i-- > 0;) {
    const char c = raw[i];
    if ((c >= '0' && c <= '9') || is_ws(c)) {
      return i + 1;
    }
  }
  return 0;
}

// EN: docs/uix-rcss.md section 9.2.1's own gradient-stop grammar -- a stop is `<color>` alone or
//     `<color>:<position%>` in the DUMP form, but in SOURCE text (this function's own input) it is
//     `<color>` or `<color> <position%>` (space-separated, matching upstream's own
//     `StringUtilities::ExpandString(..., ' ', '(', ')')` shape for gradient-stop tokenizing, the
//     same convention `box-shadow`'s own layer tokens use). Returns false for a stop with more than
//     2 tokens or an unparseable color.
// PT: A própria gramática de stop de gradiente da seção 9.2.1 do docs/uix-rcss.md -- um stop é
//     `<color>` sozinho ou `<color>:<position%>` na forma de DUMP, mas no texto de FONTE (o próprio
//     input desta função) é `<color>` ou `<color> <position%>` (separado-por-espaço, casando com a
//     própria forma `StringUtilities::ExpandString(..., ' ', '(', ')')` do upstream pra
//     tokenização de stop de gradiente, a mesma convenção que os próprios tokens de camada do
//     `box-shadow` usam). Retorna falso pra um stop com mais de 2 tokens ou uma cor não-parseável.
struct ParsedStop {
  Rgba8 color;
  std::optional<float> position_percent;
};

bool parse_gradient_stop(std::string_view stop_text, ParsedStop* out) {
  auto tokens = split_whitespace(stop_text);
  if (tokens.empty() || tokens.size() > 2) {
    return false;
  }
  if (parse_color(tokens[0], &out->color) != ValueComputeStatus::Ok) {
    return false;
  }
  // EN: `UIX-RCSS-ERRATA-4`'s own lossy premultiply/un-premultiply round-trip -- every
  //     gradient-stop color goes through the SAME upstream storage-then-serialize pipeline
  //     `box-shadow` does (`PropertyParserColorStopList.cpp:47` stores premultiplied,
  //     `TypeConverter.cpp:223` un-premultiplies on serialize). See this file's own
  //     `dump_box_shadow_or_gradient_stop_color()` for the exact byte arithmetic and citation.
  // PT: A própria ida-e-volta com perda de premultiplicar/des-premultiplicar da
  //     `UIX-RCSS-ERRATA-4` -- toda cor de stop de gradiente passa pelo MESMO pipeline
  //     armazenar-depois-serializar do upstream que o `box-shadow` passa
  //     (`PropertyParserColorStopList.cpp:47` armazena premultiplicado,
  //     `TypeConverter.cpp:223` des-premultiplica ao serializar). Ver o próprio
  //     `dump_box_shadow_or_gradient_stop_color()` deste arquivo pra aritmética de byte exata e
  //     citação.
  out->color = dump_box_shadow_or_gradient_stop_color(out->color);
  if (tokens.size() == 2) {
    float pct = 0.0f;
    if (parse_percent(tokens[1], &pct) != ValueComputeStatus::Ok) {
      return false;
    }
    out->position_percent = pct;
  } else {
    out->position_percent = std::nullopt;
  }
  return true;
}

// EN: Shared by `compute_linear_gradient_args`/`compute_radial_gradient_args` -- parses a list of
//     stop-shaped top-level pieces (already split on ',') into `ParsedStop`s, applies section
//     9.2.1's own auto-spacing, and appends the `<color>:<position%>` pieces to `*out_parts`.
//     Returns false if ANY stop fails to parse, or fewer than 2 stops are given (a gradient needs
//     at least 2 stops to be a gradient at all -- section 9.2's own table, "then >=2 stops", for
//     both `linear-gradient` and `radial-gradient`).
// PT: Compartilhado por `compute_linear_gradient_args`/`compute_radial_gradient_args` -- parseia
//     uma lista de pedaços de topo-de-nível com forma-de-stop (já divididos em ',') em
//     `ParsedStop`s, aplica o próprio auto-espaçamento da seção 9.2.1, e acrescenta os próprios
//     pedaços `<color>:<position%>` ao `*out_parts`. Retorna falso se QUALQUER stop falhar ao
//     parsear, ou menos de 2 stops forem dados (um gradiente precisa de pelo menos 2 stops pra ser
//     um gradiente de jeito nenhum -- a própria tabela da seção 9.2, "depois >=2 stops", tanto pro
//     `linear-gradient` quanto pro `radial-gradient`).
bool parse_and_space_stops(const std::vector<std::string_view>& stop_pieces,
                           std::vector<std::string>* out_parts) {
  if (stop_pieces.size() < 2) {
    return false;
  }
  std::vector<ParsedStop> stops(stop_pieces.size());
  std::vector<std::optional<float>> explicit_positions(stop_pieces.size());
  for (std::size_t i = 0; i < stop_pieces.size(); ++i) {
    if (!parse_gradient_stop(stop_pieces[i], &stops[i])) {
      return false;
    }
    explicit_positions[i] = stops[i].position_percent;
  }
  std::vector<float> resolved = resolve_gradient_stop_positions(explicit_positions);
  for (std::size_t i = 0; i < stops.size(); ++i) {
    std::string part = print_color(stops[i].color);
    part.push_back(':');
    part += print_percent(resolved[i]);
    out_parts->push_back(std::move(part));
  }
  return true;
}

std::string join(const std::vector<std::string>& parts, char sep) {
  std::string out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      out.push_back(sep);
    }
    out += parts[i];
  }
  return out;
}

// EN: Generic `name(args)` scanner -- finds every `[A-Za-z-]+(...)` run in `s`, matching
//     parentheses tracked by depth (so a nested call inside `args` never terminates the OUTER
//     call early). Whatever lies BETWEEN two calls (whitespace, a comma, both) is skipped as
//     inter-function separator, deliberately tolerant of either convention: `decorator`'s own
//     source separator is a top-level comma (`StringUtilities::ExpandString(..., ',', '(', ')')`,
//     docs/uix-rcss.md section 9.2's own citation) while `transform`'s own multi-function source
//     syntax is ordinary CSS whitespace-adjacency (`translate(...) rotate(...)`, standard CSS
//     transform-list grammar, not separately re-derived from upstream for this item's own
//     explicitly-thin `transform` scope, per that section's own stated limit) -- one scanner
//     serves both call sites in this file without duplicating the same tolerant logic twice. A
//     run of non-identifier bytes with no following `(` is silently skipped (not a function call);
//     an unterminated `(` (unbalanced parens) stops the scan at that point, discarding nothing
//     already found.
// PT: Escaneador genérico de `name(args)` -- acha todo trecho `[A-Za-z-]+(...)` em `s`, com
//     parênteses casados rastreados por profundidade (então uma chamada aninhada dentro de `args`
//     nunca termina a chamada EXTERNA cedo demais). O que fica ENTRE duas chamadas (whitespace,
//     uma vírgula, os dois) é pulado como separador inter-função, deliberadamente tolerante às
//     duas convenções: o próprio separador de fonte do `decorator` é uma vírgula de topo-de-nível
//     (`StringUtilities::ExpandString(..., ',', '(', ')')`, a própria citação da seção 9.2 do
//     docs/uix-rcss.md) enquanto a própria sintaxe de fonte multi-função do `transform` é
//     adjacência-por-whitespace do CSS comum (`translate(...) rotate(...)`, gramática padrão de
//     lista-de-transform do CSS, não re-derivada separadamente do upstream pro próprio escopo
//     explicitamente-fino de `transform` deste item, per o próprio limite declarado daquela seção)
//     -- um escaneador só serve os dois call sites deste arquivo sem duplicar a mesma lógica
//     tolerante duas vezes. Um trecho de bytes não-identificador sem um `(` seguinte é pulado em
//     silêncio (não é uma chamada de função); um `(` não-terminado (parênteses desbalanceados) para
//     o scan naquele ponto, sem descartar nada já achado.
struct FunctionCall {
  std::string_view name;
  std::string_view inner;
};

std::vector<FunctionCall> scan_function_calls(std::string_view s) {
  std::vector<FunctionCall> out;
  std::size_t i = 0;
  std::size_t n = s.size();
  while (i < n) {
    while (i < n && (is_ws(s[i]) || s[i] == ',')) {
      ++i;
    }
    if (i >= n) {
      break;
    }
    std::size_t name_start = i;
    while (i < n && (std::isalnum(static_cast<unsigned char>(s[i])) != 0 || s[i] == '-')) {
      ++i;
    }
    if (i == name_start) {
      // EN: Stray byte that is neither identifier nor whitespace/comma -- skip it, keep scanning.
      // PT: Byte solto que não é nem identificador nem whitespace/vírgula -- pula, continua o scan.
      ++i;
      continue;
    }
    std::size_t name_end = i;
    std::size_t look = i;
    while (look < n && is_ws(s[look])) {
      ++look;
    }
    if (look >= n || s[look] != '(') {
      // EN: Identifier not followed by '(' -- not a function call, skip past it. `i` already
      //     equals `name_end` here (unchanged since that assignment above, only `look` moved) --
      //     no re-assignment needed, matching cppcheck's own redundantAssignment finding.
      // PT: Identificador não seguido de '(' -- não é uma chamada de função, pula além dele. `i`
      //     já vale `name_end` aqui (inalterado desde aquela atribuição acima, só `look` andou)
      //     -- nenhuma reatribuição necessária, casando com o próprio achado
      //     redundantAssignment do cppcheck.
      continue;
    }
    std::size_t paren_start = look + 1;
    std::size_t j = paren_start;
    int depth = 1;
    while (j < n && depth > 0) {
      if (s[j] == '(') {
        ++depth;
      } else if (s[j] == ')') {
        --depth;
        if (depth == 0) {
          break;
        }
      }
      ++j;
    }
    if (depth != 0) {
      break; // unterminated '(' -- stop, keep what was already found
    }
    out.push_back(FunctionCall{s.substr(name_start, name_end - name_start),
                               s.substr(paren_start, j - paren_start)});
    i = j + 1;
  }
  return out;
}

} // namespace

// ===========================================================================
// EN: Section 8 -- quantize().
// PT: Seção 8 -- quantize().
// ===========================================================================
std::string quantize(float x) {
  double d = static_cast<double>(x);
  if (!std::isfinite(d)) {
    // EN: See this file's header, "Non-finite input" note -- the spec's own algorithm text does
    //     not address NaN/Inf; this is a defensive, DECLARED choice, not a silent guess.
    // PT: Ver "Input não-finito" no cabeçalho deste arquivo -- o próprio texto do algoritmo da
    //     spec não endereça NaN/Inf; esta é uma escolha defensiva, DECLARADA, não um chute em
    //     silêncio.
    return "0.0000";
  }
  // EN: `UIX-QUANTIZE-MAGNITUDE` -- see `kMaxQuantizeMagnitude`'s own header comment for the full
  //     derivation. Saturate BEFORE the `* 10000` scale step (not after): `d` here is the raw
  //     value in the CALLER's own unit (px/deg/number/%), so this check is independent of the
  //     internal `10000` scale factor and stays correct even if that factor's own value ever
  //     changes. Clamping (not rejecting) mirrors the non-finite branch just above -- neither has
  //     a `ValueComputeStatus` channel available at this function's own signature.
  // PT: `UIX-QUANTIZE-MAGNITUDE` -- ver o próprio comentário de cabeçalho do `kMaxQuantizeMagnitude`
  //     pra derivação completa. Satura ANTES do passo de escala `* 10000` (não depois): `d` aqui é
  //     o valor cru na própria unidade do CHAMADOR (px/deg/número/%), então esta checagem é
  //     independente do fator de escala interno `10000` e continua correta mesmo se aquele fator
  //     um dia mudar de valor. Saturar (não rejeitar) espelha o ramo do não-finito logo acima --
  //     nenhum dos dois tem canal `ValueComputeStatus` disponível na própria assinatura desta
  //     função.
  if (d > kMaxQuantizeMagnitude) {
    d = kMaxQuantizeMagnitude;
  } else if (d < -kMaxQuantizeMagnitude) {
    d = -kMaxQuantizeMagnitude;
  }
  double scaled = d * 10000.0;
  double rounded = std::trunc(scaled + std::copysign(0.5, scaled));
  bool negative = rounded < 0.0;
  double mag = negative ? -rounded : rounded;
  auto units = static_cast<unsigned long long>(mag);
  if (units == 0) {
    negative = false; // canonicalize -0.0 -> 0.0, no "-0.0000" ever printed (spec's own clause)
  }
  unsigned long long int_part = units / 10000ULL;
  unsigned long long frac_part = units % 10000ULL;
  std::string out;
  if (negative) {
    out.push_back('-');
  }
  out += std::to_string(int_part);
  out.push_back('.');
  char digits[4];
  unsigned long long f = frac_part;
  for (int i = 3; i >= 0; --i) {
    digits[i] = static_cast<char>('0' + (f % 10));
    f /= 10;
  }
  out.append(digits, 4);
  return out;
}

std::string print_number(float x) {
  return quantize(x);
}

std::string print_angle_deg(float degrees) {
  return quantize(degrees);
}

std::string print_percent(float pct) {
  std::string out = quantize(pct);
  out.push_back('%');
  return out;
}

std::string print_length_px(float resolved_px) {
  std::string out = quantize(resolved_px);
  out += "px";
  return out;
}

float degrees_from_radians(float radians) {
  constexpr double kPi = 3.14159265358979323846;
  return static_cast<float>(static_cast<double>(radians) * (180.0 / kPi));
}

std::string print_string(std::string_view raw) {
  std::string_view body = raw;
  if (body.size() >= 2 && (body.front() == '"' || body.front() == '\'') &&
      body.back() == body.front()) {
    body = body.substr(1, body.size() - 2);
  }
  std::string out;
  out.reserve(body.size());
  for (char c : body) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  return out;
}

// ===========================================================================
// EN: Section 7.1 -- color.
// PT: Seção 7.1 -- cor.
// ===========================================================================
namespace {
int hex_digit(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  char lc = (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
  if (lc >= 'a' && lc <= 'f') {
    return lc - 'a' + 10;
  }
  return -1;
}

// EN: `ESC-5` -- declarative name->color table mirroring the pin's own `html_colours` map
//     (`PropertyParserColourData::html_colours`, `examples/RmlUi/Source/Core/
//     PropertyParserColour.cpp:117-135`, verified byte-identical against the actual
//     FetchContent-pinned copy the build links, `glintfx/build/_deps/rmlui-src/Source/Core/
//     PropertyParserColour.cpp:117-135`) -- closes `ADR-0022`'s own measured row ("Named colours |
//     3 (white, black, transparent) | 19 | 16"), `docs/rmlx-subset.md` section 7's own "if the
//     engine being replaced accepts it, ours accepts it" rule. All 19 entries transcribed directly
//     from the pin's own `Colourb(r, g, b[, a])` constructor call ARGUMENTS (decimal, not
//     reinterpreted from a CSS spec's hex table, even though the two agree), IN THE PIN'S OWN
//     ORDER -- `gray`/`grey` intentionally kept as two separate rows (identical value, two
//     spellings the pin itself registers as two distinct map keys), not deduplicated, matching the
//     pin's own map literal exactly rather than a "cleaner" 18-row table this item did not
//     authorize itself to invent. Lookup is linear (19 entries, `raw.size()` already bounded by
//     `kMaxRawValueBytes` above this call site) -- no hash table, matching this file's own
//     `kLengthUnitTable` precedent (`ESC-4`) rather than introducing a second lookup strategy for a
//     table this small.
// PT: `ESC-5` -- tabela declarativa nome->cor espelhando o próprio mapa `html_colours` do pin
//     (`PropertyParserColourData::html_colours`, `examples/RmlUi/Source/Core/
//     PropertyParserColour.cpp:117-135`, verificado byte-idêntico contra a própria cópia fixada via
//     FetchContent que o build linka, `glintfx/build/_deps/rmlui-src/Source/Core/
//     PropertyParserColour.cpp:117-135`) -- fecha a própria linha medida da `ADR-0022` ("Named
//     colours | 3 (white, black, transparent) | 19 | 16"), a própria regra "se o motor que está
//     sendo substituído aceita, o nosso aceita" da seção 7 do `docs/rmlx-subset.md`. As 19 entradas
//     transcritas direto dos próprios ARGUMENTOS de construtor `Colourb(r, g, b[, a])` do pin
//     (decimal, não reinterpretado de uma tabela hex de spec CSS, mesmo as duas concordando) NA
//     PRÓPRIA ORDEM DO PIN -- `gray`/`grey` mantidos de propósito como duas linhas separadas (valor
//     idêntico, duas grafias que o próprio pin registra como duas chaves de mapa distintas), não
//     deduplicadas, casando exatamente com o próprio map literal do pin em vez de uma tabela de 18
//     linhas "mais limpa" que este item não se autorizou a inventar. Lookup é linear (19 entradas,
//     `raw.size()` já delimitado por `kMaxRawValueBytes` acima deste call site) -- sem hash table,
//     casando com o próprio precedente `kLengthUnitTable` deste arquivo (`ESC-4`) em vez de
//     introduzir uma segunda estratégia de lookup pra uma tabela deste tamanho.
struct NamedColorEntry {
  std::string_view name;
  Rgba8 value;
};
constexpr NamedColorEntry kNamedColorTable[] = {
    {"black", Rgba8{0, 0, 0, 0xff}},
    {"silver", Rgba8{192, 192, 192, 0xff}},
    {"gray", Rgba8{128, 128, 128, 0xff}},
    {"grey", Rgba8{128, 128, 128, 0xff}},
    {"white", Rgba8{255, 255, 255, 0xff}},
    {"maroon", Rgba8{128, 0, 0, 0xff}},
    {"red", Rgba8{255, 0, 0, 0xff}},
    {"orange", Rgba8{255, 165, 0, 0xff}},
    {"purple", Rgba8{128, 0, 128, 0xff}},
    {"fuchsia", Rgba8{255, 0, 255, 0xff}},
    {"green", Rgba8{0, 128, 0, 0xff}},
    {"lime", Rgba8{0, 255, 0, 0xff}},
    {"olive", Rgba8{128, 128, 0, 0xff}},
    {"yellow", Rgba8{255, 255, 0, 0xff}},
    {"navy", Rgba8{0, 0, 128, 0xff}},
    {"blue", Rgba8{0, 0, 255, 0xff}},
    {"teal", Rgba8{0, 128, 128, 0xff}},
    {"aqua", Rgba8{0, 255, 255, 0xff}},
    {"transparent", Rgba8{0, 0, 0, 0}},
};
} // namespace

// ===========================================================================
// EN: `ESC-6` -- the 8 functional color forms (`rgb()`, `rgba()`, `hsl()`, `hsla()`, `lab()`,
//     `lch()`, `oklab()`, `oklch()`). Every helper below is named to mirror the pin's OWN function
//     name (`examples/RmlUi/Source/Core/PropertyParserColour.cpp`, read in full before writing any
//     of this), transcribed literal-for-literal, operation-for-operation, in the SAME order -- not
//     an "equivalent" CSS-spec reimplementation. Two house rules this whole block holds itself to,
//     stated once here rather than repeated at every function:
//       (1) **`atof`/`atoi` leniency is DELIBERATE, confined to these 8 forms' own component
//       tokens.** The pin's own `ParseRGBColour`/`ParseHSLColour`/`ParseCIELABColour`/
//       `ParseOklabColour` call C's `atof`/`atoi` DIRECTLY on each already-tokenized component
//       (`:281`/`:284`/`:317`/`:366`/etc.) -- partial-parse-tolerant, trailing-garbage-ignoring,
//       "no valid conversion -> 0" semantics, the EXACT OPPOSITE of this file's own house
//       `parse_float_token()` discipline (whole-string match, rejects trailing garbage,
//       `parse_length`/`parse_percent`/`parse_angle`'s own shared funnel). Reusing
//       `parse_float_token()` here would silently NARROW what the pin itself accepts --
//       `rgb(1.9,0,0)`'s own "1.9" component truncates to 1 via `atoi`, and `rgb(abc,def,ghi)` ->
//       `#000000ff` (garbage -> 0), both documented, tested anchors this task's own delivery notes
//       name explicitly. `pin_atof`/`pin_atoi` below are this file's OWN, narrow, confined
//       reimplementation of that exact leniency -- see their own comment for why a NUL-terminated
//       COPY, never `sv.data()` directly.
//       (2) **Float32 throughout, zero `double` promotion, because the differential oracle compares
//       against the REAL compiled pin byte-for-byte.** `color`/`box-shadow`/a gradient stop all read
//       the pin's own already-parsed `Property::Get<Colourb>()` on side A of `UIX-RCSS-ORACULO`
//       (`glintfx/src/rml/rcss_dump_differential_oracle.cpp`) -- meaning a functional-color fixture
//       this item's own `.rml` adds is checked against the ACTUAL upstream engine, not merely against
//       this file's own Python-independent-oracle-derived unit tests. Every literal below is a
//       `float` (`f` suffix), every intermediate is `float`, and `Math::DegreesToRadians`'s own
//       FLOAT32 `RMLUI_PI` (`examples/RmlUi/Include/RmlUi/Core/Math.h:23`, `3.141592653f`) is
//       transcribed as its OWN, separate constant (`radians_from_degrees_pin()` below) rather than
//       reusing this file's own higher-precision `degrees_from_radians()` (section 8.2's own
//       DOUBLE-precision `kPi`, a DIFFERENT axis this file does not claim pin-parity for at all) --
//       see that function's own comment for the measured reason this distinction is load-bearing,
//       not pedantry.
// PT: `ESC-6` -- as 8 formas funcionais de cor (`rgb()`, `rgba()`, `hsl()`, `hsla()`, `lab()`,
//     `lch()`, `oklab()`, `oklch()`). Todo helper abaixo é nomeado pra espelhar o próprio nome de
//     função do pin (`examples/RmlUi/Source/Core/PropertyParserColour.cpp`, lido INTEIRO antes de
//     escrever qualquer coisa deste bloco), transcrito literal-por-literal, operação-por-operação,
//     na MESMA ordem -- não uma reimplementação "equivalente" de spec CSS. Duas regras da casa que
//     este bloco inteiro se prende, ditas uma vez aqui em vez de repetidas em toda função:
//       (1) **A leniência `atof`/`atoi` é DELIBERADA, confinada aos próprios tokens de componente
//       destas 8 formas.** O próprio `ParseRGBColour`/`ParseHSLColour`/`ParseCIELABColour`/
//       `ParseOklabColour` do pin chama `atof`/`atoi` DIRETO em cada componente já-tokenizado
//       (`:281`/`:284`/`:317`/`:366`/etc.) -- semântica tolerante-a-parse-parcial, ignora-lixo-à-
//       direita, "nenhuma conversão válida -> 0", o OPOSTO EXATO da própria disciplina da casa
//       `parse_float_token()` deste arquivo (casamento de string inteira, rejeita lixo à direita, o
//       próprio funil compartilhado do `parse_length`/`parse_percent`/`parse_angle`). Reusar o
//       `parse_float_token()` aqui estreitaria em silêncio o que o próprio pin aceita -- o próprio
//       "1.9" do componente de `rgb(1.9,0,0)` trunca pra 1 via `atoi`, e `rgb(abc,def,ghi)` ->
//       `#000000ff` (lixo -> 0), as duas âncoras documentadas, testadas, que as próprias notas de
//       entrega desta tarefa nomeiam explicitamente. `pin_atof`/`pin_atoi` abaixo são a própria
//       reimplementação estreita, confinada, deste arquivo dessa exata leniência -- ver o próprio
//       comentário delas pro porquê de uma CÓPIA terminada em NUL, nunca `sv.data()` direto.
//       (2) **Float32 do início ao fim, zero promoção pra `double`, porque o oráculo diferencial
//       compara contra o PRÓPRIO pin compilado byte-a-byte.** `color`/`box-shadow`/um stop de
//       gradiente todos leem o próprio `Property::Get<Colourb>()` já-parseado do pin no lado A do
//       `UIX-RCSS-ORACULO` (`glintfx/src/rml/rcss_dump_differential_oracle.cpp`) -- significando que
//       uma fixture de cor funcional que o próprio `.rml` deste item soma é checada contra o
//       MOTOR upstream de fato, não só contra os próprios testes unitários deste arquivo
//       derivados-do-oráculo-Python-independente. Todo literal abaixo é `float` (sufixo `f`), todo
//       intermediário é `float`, e o próprio `RMLUI_PI` FLOAT32 do `Math::DegreesToRadians`
//       (`examples/RmlUi/Include/RmlUi/Core/Math.h:23`, `3.141592653f`) é transcrito como a própria
//       constante separada dele (`radians_from_degrees_pin()` abaixo) em vez de reusar o próprio
//       `degrees_from_radians()` de maior precisão deste arquivo (o próprio `kPi` de precisão DUPLA
//       da seção 8.2, um eixo DIFERENTE pro qual este arquivo não alega paridade nenhuma com o pin)
//       -- ver o próprio comentário daquela função pro motivo medido desta distinção ser
//       load-bearing, não pedantismo.
namespace {

// EN: Mirrors C's `atof`/`atoi` semantics EXACTLY (leading whitespace skipped, optional sign,
//     digits, trailing GARBAGE silently ignored, "no valid conversion" -> 0) -- see this section's
//     own top header, rule (1), for why this file needs its OWN narrow copy of that leniency rather
//     than reusing `parse_float_token()`. Implemented via `std::strtof`/`std::strtol` on a
//     NUL-terminated COPY -- never `sv.data()` directly: a `string_view` slice produced by
//     `expand_color_function_values()` below is NOT guaranteed NUL-terminated at its own logical
//     end (it is a slice of a larger buffer, e.g. `"255"` inside `"rgb(255,0,0)"`), and `atof`/
//     `atoi` require a real C string -- calling them on `sv.data()` directly would read PAST the
//     intended token boundary into whatever bytes happen to follow in the caller's buffer, a
//     genuine out-of-bounds read this file's own `kMaxRawValueBytes`-bounded, fail-high discipline
//     does not tolerate elsewhere and must not silently reintroduce here. The `end` output pointer
//     of `strtof`/`strtol` is DELIBERATELY discarded (never checked) -- that discard IS the
//     "ignore trailing garbage" behavior; checking it would turn this back into `parse_float_token`.
// PT: Espelha a semântica de `atof`/`atoi` do C EXATAMENTE (whitespace à esquerda pulado, sinal
//     opcional, dígitos, LIXO à direita ignorado em silêncio, "nenhuma conversão válida" -> 0) --
//     ver o próprio cabeçalho de topo desta seção, regra (1), pro porquê deste arquivo precisar da
//     própria cópia estreita dessa leniência em vez de reusar o `parse_float_token()`. Implementado
//     via `std::strtof`/`std::strtol` numa CÓPIA terminada em NUL -- nunca `sv.data()` direto: uma
//     fatia `string_view` produzida pelo `expand_color_function_values()` abaixo NÃO é garantida
//     terminada em NUL no próprio fim lógico dela (é uma fatia de um buffer maior, ex. `"255"`
//     dentro de `"rgb(255,0,0)"`), e `atof`/`atoi` exigem uma C string de verdade -- chamá-las em
//     `sv.data()` direto leria PASSANDO da própria fronteira de token pretendida pra dentro de
//     quaisquer bytes que calhem de vir a seguir no buffer do chamador, uma leitura fora-dos-limites
//     genuína que a própria disciplina fail-high, delimitada-por-`kMaxRawValueBytes`, deste arquivo
//     não tolera em lugar nenhum mais e não pode reintroduzir em silêncio aqui. O ponteiro de saída
//     `end` do `strtof`/`strtol` é DELIBERADAMENTE descartado (nunca conferido) -- esse descarte É o
//     comportamento "ignora lixo à direita"; conferi-lo viraria isto de volta num
//     `parse_float_token`.
float pin_atof(std::string_view s) {
  std::string buf(s);
  return std::strtof(buf.c_str(), nullptr);
}

int pin_atoi(std::string_view s) {
  std::string buf(s);
  return static_cast<int>(std::strtol(buf.c_str(), nullptr, 10));
}

// EN: Transcribes `StringUtilities::ExpandString`'s own 4-arg overload
//     (`examples/RmlUi/Source/Core/StringUtilities.cpp:242-291`) as a state machine over
//     `std::string_view`, PLUS the paren-extraction `GetColourFunctionValues()` itself does
//     (`PropertyParserColour.cpp:534-546`) -- one function doing both steps, matching this
//     section's own naming. Two callers, two shapes: `rgb()`/`hsl()` use `,` with
//     `ignore_repeated_delimiters=false` (a repeated OR leading comma DOES produce an empty entry
//     -- `rgb(255,,0)`'s own documented, tested `""` middle token, `atoi("")` -> 0);
//     `lab()`/`lch()`/`oklab()`/`oklch()` use ` ` with `ignore_repeated_delimiters=true` (a run of
//     spaces, or a leading one, produces NO entry at all, not even an empty one). Quote semantics
//     (`'`/`"` open a quote ONLY when the immediately preceding character was itself a delimiter,
//     or `i==0` -- `last_char_delimiter` below; a matching, unescaped quote character closes it,
//     `s[i-1] != '\\'`) are transcribed too, including the upstream state machine's own quirk that
//     an isolated `''`/`""` pair (nothing between the two quote characters) contributes NOTHING to
//     the token list, not even an empty one, because neither quote character itself ever sets
//     `has_start` -- reproduced here deliberately, not smoothed over, because a "corrected"
//     transliteration would silently diverge from the pin on an input this module cannot prove the
//     corpus never contains. `GetColourFunctionValues`'s own missing-`)`-is-ACCEPTED tolerance
//     (`rgb(255,0,0` with no closing paren still parses -- a documented, tested anchor) is
//     reproduced by the SAME mechanism the pin itself relies on: `raw.rfind(')')` returning `npos`
//     (or, degenerately, landing before `begin_values`) makes `last_paren - begin_values` underflow
//     into a huge `size_t`, which `std::string_view::substr`'s own `count` parameter then silently
//     CLAMPS to "rest of the string" -- documented, standard-mandated clamping behavior, not UB,
//     the exact same clamp `std::string::substr` performs for the pin's own identical-shaped
//     expression. Returns `false` only when `raw` has no `(` at all, matching
//     `GetColourFunctionValues`'s own single failure case.
// PT: Transcreve o próprio overload de 4 argumentos do `StringUtilities::ExpandString`
//     (`examples/RmlUi/Source/Core/StringUtilities.cpp:242-291`) como uma máquina de estados sobre
//     `std::string_view`, MAIS a própria extração de parêntese que o `GetColourFunctionValues()` faz
//     (`PropertyParserColour.cpp:534-546`) -- uma função só fazendo os dois passos, casando com o
//     próprio nome desta seção. Dois chamadores, duas formas: `rgb()`/`hsl()` usam `,` com
//     `ignore_repeated_delimiters=false` (uma vírgula repetida OU inicial DE FATO produz uma entrada
//     vazia -- o próprio `""` do token do meio de `rgb(255,,0)`, documentado, testado, `atoi("")`
//     -> 0); `lab()`/`lch()`/`oklab()`/`oklch()` usam ` ` com `ignore_repeated_delimiters=true` (um
//     trecho de espaços, ou um inicial, não produz entrada nenhuma, nem vazia). Semântica de aspas
//     (`'`/`"` abrem uma quote SÓ quando o caractere imediatamente anterior era ele mesmo um
//     delimitador, ou `i==0` -- `last_char_delimiter` abaixo; uma aspas casada, não-escapada, fecha,
//     `s[i-1] != '\\'`) também é transcrita, incluindo o próprio quirk da máquina de estados do
//     upstream de que um par isolado `''`/`""` (nada entre as duas aspas) não contribui NADA pra
//     lista de tokens, nem vazio, porque nenhuma das duas aspas em si nunca seta `has_start` --
//     reproduzido aqui de propósito, não suavizado, porque uma transliteração "corrigida" divergiria
//     em silêncio do pin num input que este módulo não consegue provar que o corpus nunca contém. A
//     própria tolerância "`)` ausente é ACEITO" do `GetColourFunctionValues` (`rgb(255,0,0` sem `)`
//     de fechamento ainda parseia -- uma âncora documentada, testada) é reproduzida pelo MESMO
//     mecanismo que o próprio pin depende: `raw.rfind(')')` retornando `npos` (ou, degeneradamente,
//     caindo antes de `begin_values`) faz `last_paren - begin_values` estourar por baixo pra um
//     `size_t` enorme, que o próprio parâmetro `count` do `std::string_view::substr` então SATURA em
//     silêncio pra "resto da string" -- comportamento de saturação documentado, mandado pela spec,
//     não UB, exatamente o mesmo clamp que o `std::string::substr` faz pra própria expressão de
//     forma idêntica do pin. Retorna `false` só quando `raw` não tem `(` nenhum, casando com o único
//     caso de falha do próprio `GetColourFunctionValues`.
bool expand_color_function_values(std::string_view raw, bool is_comma_separated,
                                  std::vector<std::string_view>* out_values) {
  std::size_t first_paren = raw.find('(');
  if (first_paren == std::string_view::npos) {
    return false;
  }
  std::size_t begin_values = first_paren + 1;
  std::size_t last_paren = raw.rfind(')');
  std::size_t count = last_paren - begin_values; // intentional size_t underflow when ')' missing
                                                 // or before '(' -- see comment above
  std::string_view inner = raw.substr(begin_values, count);

  const char delimiter = is_comma_separated ? ',' : ' ';
  const bool ignore_repeated_delimiters = !is_comma_separated;
  char quote = 0;
  bool last_char_delimiter = true;
  bool has_start = false;
  std::size_t start_idx = 0;
  std::size_t end_idx = 0;
  for (std::size_t i = 0; i < inner.size(); ++i) {
    const char ch = inner[i];
    if (last_char_delimiter && quote == 0 && (ch == '"' || ch == '\'')) {
      quote = ch;
    } else if (quote != 0 && ch == quote && (i == 0 || inner[i - 1] != '\\')) {
      quote = 0;
    } else if (ch == delimiter && quote == 0) {
      if (has_start) {
        out_values->push_back(inner.substr(start_idx, end_idx - start_idx + 1));
      } else if (!ignore_repeated_delimiters) {
        out_values->push_back(std::string_view{});
      }
      last_char_delimiter = true;
      has_start = false;
    } else if (!is_ws(ch) || quote != 0) {
      if (!has_start) {
        start_idx = i;
        has_start = true;
      }
      end_idx = i;
      last_char_delimiter = false;
    }
    // EN/PT: implicit no-op else (whitespace outside a quote, not a delimiter, not a quote char) --
    // matches upstream's own implicit final branch, nothing changes.
  }
  if (has_start) {
    out_values->push_back(inner.substr(start_idx, end_idx - start_idx + 1));
  }
  return true;
}

// EN: `HSL_f` (`PropertyParserColour.cpp:11-16`) -- Wikipedia's own "HSL to RGB alternative"
//     formula the pin itself cites (`:18`,
//     https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB_alternative).
// PT: `HSL_f` (`PropertyParserColour.cpp:11-16`) -- a própria fórmula "HSL to RGB alternative" da
//     Wikipedia que o próprio pin cita (`:18`,
//     https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB_alternative).
float hsl_f(float h, float s, float l, float n) {
  float k = std::fmod(n + h * (1.0f / 30.0f), 12.0f);
  float a = s * std::min(l, 1.0f - l);
  return l - a * std::max(-1.0f, std::min({k - 3.0f, 9.0f - k, 1.0f}));
}

// EN: `HSLAToRGBA` (`PropertyParserColour.cpp:19-36`) -- takes/writes H,S,L in `vals[0..2]` in
//     place, REPLACING them with R,G,B; `vals[3]` (alpha) is left UNTOUCHED here, exactly like the
//     pin -- alpha's own byte conversion happens later, in the SAME uniform `*255` loop every other
//     channel goes through (`ParseHSLColour`'s own closing `for`, `:326-327`).
// PT: `HSLAToRGBA` (`PropertyParserColour.cpp:19-36`) -- lê/escreve H,S,L em `vals[0..2]` no lugar,
//     SUBSTITUINDO por R,G,B; `vals[3]` (alpha) fica INTOCADO aqui, exatamente como o pin -- a
//     própria conversão pra byte do alpha acontece depois, no MESMO laço `*255` uniforme que todo
//     outro canal atravessa (o próprio `for` de fechamento do `ParseHSLColour`, `:326-327`).
void hsla_to_rgba(float vals[4]) {
  if (vals[1] == 0.0f) {
    vals[0] = vals[1] = vals[2];
  } else {
    float h = std::fmod(vals[0], 360.0f);
    if (h < 0) {
      h += 360.0f;
    }
    float s = vals[1];
    float l = vals[2];
    vals[0] = hsl_f(h, s, l, 0.0f);
    vals[1] = hsl_f(h, s, l, 8.0f);
    vals[2] = hsl_f(h, s, l, 4.0f);
  }
}

// EN: `InverseSRGBNonlinearTransfer` (`PropertyParserColour.cpp:39-42`) -- linear-light to
//     gamma-encoded sRGB, the pin's own cited reference
//     https://en.wikipedia.org/wiki/SRGB#Definition. Shared by both `cielab_to_rgba()` and
//     `oklab_to_rgba()` below, matching the pin's own single free function shared by
//     `CIELABToRGBA`/`OklabToRGBA`.
// PT: `InverseSRGBNonlinearTransfer` (`PropertyParserColour.cpp:39-42`) -- luz-linear pra sRGB
//     codificado em gama, a própria referência citada pelo pin
//     https://en.wikipedia.org/wiki/SRGB#Definition. Compartilhada por `cielab_to_rgba()` e
//     `oklab_to_rgba()` abaixo, casando com a própria função livre única do pin compartilhada por
//     `CIELABToRGBA`/`OklabToRGBA`.
float inverse_srgb_nonlinear_transfer(float channel) {
  return channel > 0.0031308f ? 1.055f * std::pow(channel, 1.0f / 2.4f) - 0.055f : 12.92f * channel;
}

// EN: `Math::DegreesToRadians` (`examples/RmlUi/Include/RmlUi/Core/Math.h:128` declared,
//     `Source/Core/Math.cpp:83-86` defined: `return angle * (RMLUI_PI / 180.0f);`) -- used ONLY by
//     `parse_cielab_function`/`parse_oklab_function` below, for `lch()`/`oklch()`'s own own
//     hue->Cartesian step (`Math::Cos(Math::DegreesToRadians(hue))`/`Math::Sin(...)`,
//     `PropertyParserColour.cpp:424-425`/`:523-524`). ⚠️ Deliberately NOT this file's own existing
//     `degrees_from_radians()` run backwards: that function's own `kPi` is DOUBLE-precision
//     (`3.14159265358979323846`, this file's own choice for section 8.2's angle UNIT conversion, an
//     axis the pin does not define byte-parity for at all) -- the WRONG constant here. The pin's
//     own `RMLUI_PI` (`Math.h:23`) is a FLOAT32 literal, `3.141592653f`, and the whole expression is
//     float32 arithmetic throughout (`float * (float / float)`, zero double promotion) --
//     transcribed bit-for-bit rather than reusing this file's own higher-precision constant,
//     because `lch()`/`oklch()`'s own hue feeds a Cartesian conversion the differential oracle
//     compares against the REAL pin byte-for-byte (this section's own top header, rule 2) -- a
//     precision mismatch here is not a rounding nicety, it is a MEASURABLE divergence at a
//     non-axis-aligned hue.
// PT: `Math::DegreesToRadians` (`examples/RmlUi/Include/RmlUi/Core/Math.h:128` declarado,
//     `Source/Core/Math.cpp:83-86` definido: `return angle * (RMLUI_PI / 180.0f);`) -- usado SÓ
//     pelo `parse_cielab_function`/`parse_oklab_function` abaixo, pro próprio passo hue->Cartesiano
//     do lch()/oklch() (`Math::Cos(Math::DegreesToRadians(hue))`/`Math::Sin(...)`,
//     `PropertyParserColour.cpp:424-425`/`:523-524`). ⚠️ Deliberadamente NÃO o próprio
//     `degrees_from_radians()` deste arquivo rodado ao contrário: o próprio `kPi` daquela função é
//     de precisão DUPLA (`3.14159265358979323846`, escolha própria deste arquivo pra conversão de
//     UNIDADE de ângulo da seção 8.2, um eixo pro qual o pin não define paridade de byte nenhuma) --
//     a constante ERRADA aqui. O próprio `RMLUI_PI` do pin (`Math.h:23`) é um literal FLOAT32,
//     `3.141592653f`, e a expressão inteira é aritmética float32 do início ao fim (`float * (float /
//     float)`, zero promoção pra double) -- transcrita bit-a-bit em vez de reusar a própria
//     constante de maior precisão deste arquivo, porque o próprio hue do lch()/oklch() alimenta uma
//     conversão Cartesiana que o oráculo diferencial compara contra o PRÓPRIO pin byte-a-byte (a
//     própria regra 2 do cabeçalho de topo desta seção) -- um descompasso de precisão aqui não é um
//     capricho de arredondamento, é uma divergência MENSURÁVEL num hue não-alinhado-a-eixo.
float radians_from_degrees_pin(float degrees) {
  constexpr float kRmluiPi = 3.141592653f;
  return degrees * (kRmluiPi / 180.0f);
}

// EN: `CIELABToRGBA` (`PropertyParserColour.cpp:45-77`) -- CIELAB -> CIE XYZ (D65) -> linear sRGB
//     -> gamma sRGB, the pin's own cited reference
//     https://en.wikipedia.org/wiki/CIELAB_color_space#Converting_between_CIELAB_and_CIE_XYZ_coordinates.
//     ⚠️ The f-inverse threshold/divisor (`0.008856f`/`7.787f`) are the LEGACY CIE constants, NOT
//     the δ=6/29-derived ones the current CSS Color spec text uses -- transcribed from the pin
//     verbatim, per this task's own "onde a spec e o código real divergirem, o código manda" rule
//     (this module's own general house discipline, already applied by this file's own top-of-file
//     header for the box-shadow-premultiply finding). The XYZ->sRGB matrix (`:64-68`) is the pin's
//     OWN exact literal digits (differing from the IEC 61966-2-1 reference matrix in the last 1-2
//     decimal digits of a few entries) -- transcribed as printed in the pin, not "corrected" against
//     an external spec: this module targets byte-for-byte parity with the ENGINE BEING REPLACED, not
//     a from-scratch CSS-spec-conformant reimplementation. `values[3]` (alpha) passes through
//     UNTOUCHED -- alpha never goes through color-space conversion, matching the pin exactly
//     (`CIELABToRGBA` only ever writes `values[0..2]`). The `t*t*t` cube is computed ONCE per axis
//     and reused for both the threshold comparison and the true-branch value (the pin's own source
//     text repeats the SAME expression twice at each call site, `:51-56`) -- behaviourally IDENTICAL
//     (IEEE-754 float multiplication is deterministic: the same operands, same operation, always
//     produce the same bits, whether evaluated once and reused or literally re-evaluated), NOT the
//     kind of reciprocal-vs-division rewrite `resolve_length_px()`'s own header warns is NOT
//     bit-safe -- there is no operation-SHAPE change here, only a redundant-recomputation removal.
// PT: `CIELABToRGBA` (`PropertyParserColour.cpp:45-77`) -- CIELAB -> CIE XYZ (D65) -> sRGB linear ->
//     sRGB gama, a própria referência citada pelo pin
//     https://en.wikipedia.org/wiki/CIELAB_color_space#Converting_between_CIELAB_and_CIE_XYZ_coordinates.
//     ⚠️ O limiar/divisor da f-inversa (`0.008856f`/`7.787f`) são as constantes CIE LEGADAS, NÃO as
//     derivadas de δ=6/29 que o texto atual da spec CSS Color usa -- transcritas do pin verbatim,
//     per a própria regra "onde a spec e o código real divergirem, o código manda" desta tarefa (a
//     própria disciplina geral da casa deste módulo, já aplicada pelo próprio cabeçalho de topo
//     deste arquivo pro achado de premultiplicação do box-shadow). A matriz XYZ->sRGB (`:64-68`) são
//     os próprios dígitos literais EXATOS do pin (diferindo da matriz de referência IEC 61966-2-1
//     nos últimos 1-2 dígitos decimais de algumas entradas) -- transcrita como impressa no pin, não
//     "corrigida" contra uma spec externa: este módulo mira paridade byte-a-byte com o MOTOR SENDO
//     SUBSTITUÍDO, não uma reimplementação conformante-à-spec-CSS do zero. `values[3]` (alpha) passa
//     INTOCADO -- alpha nunca passa por conversão de espaço de cor, casando com o pin exatamente (o
//     `CIELABToRGBA` só nunca escreve `values[0..2]`). O cubo `t*t*t` é computado UMA VEZ por eixo e
//     reusado tanto pra comparação de limiar quanto pro valor do ramo verdadeiro (o próprio texto de
//     fonte do pin repete a MESMA expressão duas vezes em cada call site, `:51-56`) --
//     comportamentalmente IDÊNTICO (multiplicação float IEEE-754 é determinística: os mesmos
//     operandos, a mesma operação, sempre produzem os mesmos bits, seja avaliada uma vez e reusada
//     ou literalmente reavaliada), NÃO o tipo de reescrita recíproco-versus-divisão que o próprio
//     cabeçalho do `resolve_length_px()` avisa NÃO ser bit-seguro -- não há mudança de FORMA de
//     operação aqui, só remoção de recomputação redundante.
void cielab_to_rgba(float values[4]) {
  float y_double_prime = (values[0] + 16.0f) / 116.0f;
  float x_double_prime = (values[1] / 500.0f) + y_double_prime;
  float z_double_prime = y_double_prime - (values[2] / 200.0f);

  auto f_inverse = [](float t) {
    float t3 = t * t * t;
    return t3 > 0.008856f ? t3 : (t - (16.0f / 116.0f)) / 7.787f;
  };
  float x_prime = f_inverse(x_double_prime);
  float y_prime = f_inverse(y_double_prime);
  float z_prime = f_inverse(z_double_prime);

  constexpr float kIlluminantD65X = 0.95047f;
  constexpr float kIlluminantD65Y = 1.0f;
  constexpr float kIlluminantD65Z = 1.08883f;

  float x = x_prime * kIlluminantD65X;
  float y = y_prime * kIlluminantD65Y;
  float z = z_prime * kIlluminantD65Z;

  constexpr float kXyzToSrgb[3][3] = {
      {+3.2404548f, -1.5371389f, -0.4985315f},
      {-0.9692664f, +1.8760109f, +0.0415561f},
      {+0.0556434f, -0.2040259f, +1.0572252f},
  };

  float r = kXyzToSrgb[0][0] * x + kXyzToSrgb[0][1] * y + kXyzToSrgb[0][2] * z;
  float g = kXyzToSrgb[1][0] * x + kXyzToSrgb[1][1] * y + kXyzToSrgb[1][2] * z;
  float b = kXyzToSrgb[2][0] * x + kXyzToSrgb[2][1] * y + kXyzToSrgb[2][2] * z;

  values[0] = std::clamp(inverse_srgb_nonlinear_transfer(r), 0.0f, 1.0f);
  values[1] = std::clamp(inverse_srgb_nonlinear_transfer(g), 0.0f, 1.0f);
  values[2] = std::clamp(inverse_srgb_nonlinear_transfer(b), 0.0f, 1.0f);
}

// EN: `OklabToRGBA` (`PropertyParserColour.cpp:80-113`) -- Oklab -> LMS' (cubed -> LMS) -> linear
//     sRGB -> gamma sRGB, the pin's own cited references
//     https://en.wikipedia.org/wiki/Oklab_color_space#Conversions_between_color_spaces and
//     https://bottosson.github.io/posts/oklab/. Both matrices are Ottosson's own literal
//     coefficients, 10 decimal digits, transcribed exactly (`:82-86`/`:100-104`). `values[3]`
//     (alpha) passes through UNTOUCHED, same reasoning as `cielab_to_rgba()` above.
// PT: `OklabToRGBA` (`PropertyParserColour.cpp:80-113`) -- Oklab -> LMS' (ao cubo -> LMS) -> sRGB
//     linear -> sRGB gama, as próprias referências citadas pelo pin
//     https://en.wikipedia.org/wiki/Oklab_color_space#Conversions_between_color_spaces e
//     https://bottosson.github.io/posts/oklab/. As duas matrizes são os próprios coeficientes
//     literais do Ottosson, 10 dígitos decimais, transcritos exatos (`:82-86`/`:100-104`).
//     `values[3]` (alpha) passa INTOCADO, mesmo raciocínio do `cielab_to_rgba()` acima.
void oklab_to_rgba(float values[4]) {
  constexpr float kOklabToLmsPrime[3][3] = {
      {+1.0f, +0.3963377774f, +0.2158037573f},
      {+1.0f, -0.1055613458f, -0.0638541728f},
      {+1.0f, -0.0894841775f, -1.2914855480f},
  };
  float lightness = values[0];
  float a_axis = values[1];
  float b_axis = values[2];
  float l_prime = kOklabToLmsPrime[0][0] * lightness + kOklabToLmsPrime[0][1] * a_axis +
                  kOklabToLmsPrime[0][2] * b_axis;
  float m_prime = kOklabToLmsPrime[1][0] * lightness + kOklabToLmsPrime[1][1] * a_axis +
                  kOklabToLmsPrime[1][2] * b_axis;
  float s_prime = kOklabToLmsPrime[2][0] * lightness + kOklabToLmsPrime[2][1] * a_axis +
                  kOklabToLmsPrime[2][2] * b_axis;

  float l = l_prime * l_prime * l_prime;
  float m = m_prime * m_prime * m_prime;
  float s = s_prime * s_prime * s_prime;

  constexpr float kLmsToSrgb[3][3] = {
      {+4.0767416621f, -3.3077115913f, +0.2309699292f},
      {-1.2684380046f, +2.6097574011f, -0.3413193965f},
      {-0.0041960863f, -0.7034186147f, +1.7076147010f},
  };
  float r = kLmsToSrgb[0][0] * l + kLmsToSrgb[0][1] * m + kLmsToSrgb[0][2] * s;
  float g = kLmsToSrgb[1][0] * l + kLmsToSrgb[1][1] * m + kLmsToSrgb[1][2] * s;
  float b = kLmsToSrgb[2][0] * l + kLmsToSrgb[2][1] * m + kLmsToSrgb[2][2] * s;

  values[0] = std::clamp(inverse_srgb_nonlinear_transfer(r), 0.0f, 1.0f);
  values[1] = std::clamp(inverse_srgb_nonlinear_transfer(g), 0.0f, 1.0f);
  values[2] = std::clamp(inverse_srgb_nonlinear_transfer(b), 0.0f, 1.0f);
}

// EN: `ParseRGBColour` (`PropertyParserColour.cpp:253-290`) -- `raw[3]=='a'` is the ONLY aridade
//     check (`rgbx(1,2,3)` is accepted as 3-arg `rgb`, matching the pin's own loose prefix, this
//     section's own top header). `%` truncates via a C-style `int(...)` cast (never rounds -- `50%`
//     -> `int(50.0f * 2.55f)` = `int(127.5f)` = `127`, NOT `128`); a bare integer/decimal token goes
//     through `pin_atoi()` (`"1.9"` -> `1`, garbage -> `0`). `Math::Clamp` SATURATES into `[0,255]`
//     AFTER the truncating cast, never before -- `rgb(300,-5,0)` clamps to `#ff0000ff`, not
//     rejected.
// PT: `ParseRGBColour` (`PropertyParserColour.cpp:253-290`) -- `raw[3]=='a'` é a ÚNICA checagem de
//     aridade (`rgbx(1,2,3)` é aceito como `rgb` de 3 argumentos, casando com o próprio prefixo
//     frouxo do pin, o próprio cabeçalho de topo desta seção). `%` trunca via um cast `int(...)`
//     estilo C (nunca arredonda -- `50%` -> `int(50.0f * 2.55f)` = `int(127.5f)` = `127`, NÃO
//     `128`); um token inteiro/decimal cru passa pelo `pin_atoi()` (`"1.9"` -> `1`, lixo -> `0`). O
//     `Math::Clamp` SATURA pra `[0,255]` DEPOIS do cast truncador, nunca antes -- `rgb(300,-5,0)`
//     satura pra `#ff0000ff`, não é rejeitado.
bool parse_rgb_function(std::string_view raw, Rgba8* out) {
  std::vector<std::string_view> values;
  if (!expand_color_function_values(raw, /*is_comma_separated=*/true, &values)) {
    return false;
  }
  const bool is_rgba = raw.size() > 3 && raw[3] == 'a';
  if (is_rgba) {
    if (values.size() != 4) {
      return false;
    }
  } else {
    if (values.size() != 3) {
      return false;
    }
    values.push_back("255");
  }
  std::uint8_t bytes[4];
  for (int i = 0; i < 4; ++i) {
    std::string_view v = values[static_cast<std::size_t>(i)];
    int component;
    if (!v.empty() && v.back() == '%') {
      float pct = pin_atof(v.substr(0, v.size() - 1));
      component = static_cast<int>(pct * (255.0f / 100.0f));
    } else {
      component = pin_atoi(v);
    }
    bytes[i] = static_cast<std::uint8_t>(std::clamp(component, 0, 255));
  }
  out->r = bytes[0];
  out->g = bytes[1];
  out->b = bytes[2];
  out->a = bytes[3];
  return true;
}

// EN: `ParseHSLColour` (`PropertyParserColour.cpp:292-330`) -- `raw[3]=='a'` is the ONLY aridade
//     check, same shape as RGB above. H/alpha (`{0,3}`) parse via plain `pin_atof()`, no `%`
//     requirement, no clamp before `HSLAToRGBA` -- H wraps (`fmod` + negative correction, INSIDE
//     `hsla_to_rgba()`), alpha is clamped only implicitly by the closing `*255` byte loop. ⚠️ S/L
//     (`{1,2}`) REQUIRE a trailing `%` -- `hsl(120,0.5,0.5)` (bare fraction, no `%`) is `Invalid`,
//     never silently reinterpreted as already-normalized `[0,1]`.
// PT: `ParseHSLColour` (`PropertyParserColour.cpp:292-330`) -- `raw[3]=='a'` é a ÚNICA checagem de
//     aridade, mesma forma do RGB acima. H/alpha (`{0,3}`) parseiam via `pin_atof()` cru, sem
//     exigência de `%`, sem clamp antes do `HSLAToRGBA` -- H dá a volta (`fmod` + correção de
//     negativo, DENTRO do `hsla_to_rgba()`), alpha só é clampado implicitamente pelo laço de byte
//     `*255` de fechamento. ⚠️ S/L (`{1,2}`) EXIGEM um `%` à direita -- `hsl(120,0.5,0.5)` (fração
//     crua, sem `%`) é `Invalid`, nunca reinterpretado em silêncio como já-normalizado `[0,1]`.
bool parse_hsl_function(std::string_view raw, Rgba8* out) {
  std::vector<std::string_view> values;
  if (!expand_color_function_values(raw, /*is_comma_separated=*/true, &values)) {
    return false;
  }
  const bool is_hsla = raw.size() > 3 && raw[3] == 'a';
  if (is_hsla) {
    if (values.size() != 4) {
      return false;
    }
  } else {
    if (values.size() != 3) {
      return false;
    }
    values.push_back("1.0");
  }
  float vals[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  for (int idx : {0, 3}) {
    vals[idx] = pin_atof(values[static_cast<std::size_t>(idx)]);
  }
  for (int idx : {1, 2}) {
    std::string_view v = values[static_cast<std::size_t>(idx)];
    if (!v.empty() && v.back() == '%') {
      vals[idx] = pin_atof(v.substr(0, v.size() - 1)) * (1.0f / 100.0f);
    } else {
      return false;
    }
  }
  hsla_to_rgba(vals);
  std::uint8_t bytes[4];
  for (int i = 0; i < 4; ++i) {
    bytes[i] = static_cast<std::uint8_t>(std::clamp(static_cast<int>(vals[i] * 255.0f), 0, 255));
  }
  out->r = bytes[0];
  out->g = bytes[1];
  out->b = bytes[2];
  out->a = bytes[3];
  return true;
}

// EN: `ParseCIELABColour` (`PropertyParserColour.cpp:332-433`) -- handles BOTH `lab()` and `lch()`,
//     same as the pin's own single function, branching on `raw.substr(0,3)=="lab"` (`:377`).
//     Space-tokenized (`expand_color_function_values(..., false, ...)`); a 5-token result requires
//     `values[3]=="/"` as an ISOLATED token (`lab(50 40 60/0.5)`'s own missing spaces around `/`
//     never produces that isolated token -- `Invalid`, not a lenient re-split); 3 tokens means no
//     alpha, defaulted to `"1.0"`. Lightness/alpha (`{0,3}`): `none`->0, `%`->direct-no-divide for L
//     (0%-100% IS 0.0-100.0, no `/100`) but `/100` FOR ALPHA specifically, plain number otherwise;
//     clamped to `[0,100]`/`[0,1]`. `lab()`'s own a/b axes (`{1,2}`): `%` maps ±100% to ±125.0
//     (`kCielabAxisPercentageBound`), clamped to `±160` (`kCielabAxisBoundLimit`). `lch()`'s own
//     chroma: `%` maps 100% to 150.0 (`kCielchMaximumPercentageChroma`), clamped `[0,230]`
//     (`kCielchMaximumChroma`); hue is a plain-degrees `pin_atof()`, UNCLAMPED, fed through
//     `radians_from_degrees_pin()` (this section's own float32-exact pi, NOT `degrees_from_radians`)
//     to `std::cos`/`std::sin` for the polar->Cartesian step.
// PT: `ParseCIELABColour` (`PropertyParserColour.cpp:332-433`) -- trata TANTO `lab()` quanto
//     `lch()`, igual à própria função única do pin, ramificando em `raw.substr(0,3)=="lab"`
//     (`:377`). Tokenizado por espaço (`expand_color_function_values(..., false, ...)`); um
//     resultado de 5 tokens exige `values[3]=="/"` como token ISOLADO (os próprios espaços ausentes
//     ao redor do `/` de `lab(50 40 60/0.5)` nunca produzem aquele token isolado -- `Invalid`, não
//     um re-split tolerante); 3 tokens significa sem alpha, default `"1.0"`. Luminosidade/alpha
//     (`{0,3}`): `none`->0, `%`->direto-sem-dividir pra L (0%-100% É 0.0-100.0, sem `/100`) mas
//     `/100` PRO ALPHA especificamente, número cru senão; clampado pra `[0,100]`/`[0,1]`. Os
//     próprios eixos a/b do `lab()` (`{1,2}`): `%` mapeia ±100% pra ±125.0
//     (`kCielabAxisPercentageBound`), clampado pra `±160` (`kCielabAxisBoundLimit`). A própria
//     chroma do `lch()`: `%` mapeia 100% pra 150.0 (`kCielchMaximumPercentageChroma`), clampado
//     `[0,230]` (`kCielchMaximumChroma`); hue é um `pin_atof()` de graus cru, NÃO-CLAMPADO,
//     alimentado pelo `radians_from_degrees_pin()` (o próprio pi float32-exato desta seção, NÃO o
//     `degrees_from_radians`) pro `std::cos`/`std::sin` do próprio passo polar->Cartesiano.
bool parse_cielab_function(std::string_view raw, Rgba8* out) {
  std::vector<std::string_view> values;
  if (!expand_color_function_values(raw, /*is_comma_separated=*/false, &values)) {
    return false;
  }
  if (values.size() == 5) {
    if (values[3] != "/") {
      return false;
    }
    values[3] = values[4];
    values.pop_back();
  } else {
    if (values.size() != 3) {
      return false;
    }
    values.push_back("1.0");
  }

  float lab_values[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  for (int idx : {0, 3}) {
    std::string_view v = values[static_cast<std::size_t>(idx)];
    if (v == "none") {
      lab_values[idx] = 0.0f;
    } else if (!v.empty() && v.back() == '%') {
      lab_values[idx] = pin_atof(v.substr(0, v.size() - 1));
      if (idx == 3) {
        lab_values[idx] /= 100.0f;
      }
    } else {
      lab_values[idx] = pin_atof(v);
    }
    lab_values[idx] = std::clamp(lab_values[idx], 0.0f, idx == 0 ? 100.0f : 1.0f);
  }

  const bool is_lab = raw.substr(0, 3) == "lab";
  if (is_lab) {
    for (int idx : {1, 2}) {
      std::string_view v = values[static_cast<std::size_t>(idx)];
      if (v == "none") {
        lab_values[idx] = 0.0f;
      } else if (!v.empty() && v.back() == '%') {
        constexpr float kCielabAxisPercentageBound = 125.0f;
        lab_values[idx] = pin_atof(v.substr(0, v.size() - 1)) / 100.0f * kCielabAxisPercentageBound;
      } else {
        lab_values[idx] = pin_atof(v);
      }
      constexpr float kCielabAxisBoundLimit = 160.0f;
      lab_values[idx] = std::clamp(lab_values[idx], -kCielabAxisBoundLimit, +kCielabAxisBoundLimit);
    }
  } else {
    std::string_view chroma_tok = values[1];
    float chroma;
    if (chroma_tok == "none") {
      chroma = 0.0f;
    } else if (!chroma_tok.empty() && chroma_tok.back() == '%') {
      constexpr float kCielchMaximumPercentageChroma = 150.0f;
      chroma =
          pin_atof(chroma_tok.substr(0, chroma_tok.size() - 1)) / 100.0f * kCielchMaximumPercentageChroma;
    } else {
      chroma = pin_atof(chroma_tok);
    }
    constexpr float kCielchMaximumChroma = 230.0f;
    chroma = std::clamp(chroma, 0.0f, kCielchMaximumChroma);

    std::string_view hue_tok = values[2];
    float hue = hue_tok == "none" ? 0.0f : pin_atof(hue_tok);

    lab_values[1] = chroma * std::cos(radians_from_degrees_pin(hue));
    lab_values[2] = chroma * std::sin(radians_from_degrees_pin(hue));
  }

  cielab_to_rgba(lab_values);
  std::uint8_t bytes[4];
  for (int i = 0; i < 4; ++i) {
    bytes[i] =
        static_cast<std::uint8_t>(std::clamp(static_cast<int>(lab_values[i] * 255.0f), 0, 255));
  }
  out->r = bytes[0];
  out->g = bytes[1];
  out->b = bytes[2];
  out->a = bytes[3];
  return true;
}

// EN: `ParseOklabColour` (`PropertyParserColour.cpp:435-532`) -- handles BOTH `oklab()` and
//     `oklch()`, branching on `raw.substr(0,5)=="oklab"` (`:476`). Same 5-token
//     `/`-isolated-alpha/3-token-no-alpha shape as CIELAB above. ⚠️ Lightness is `[0,1]` here
//     (`%`->`/100`), NOT `[0,100]` like `lab()` -- the one place this function's own clamp shape
//     genuinely differs from `parse_cielab_function()`'s. `oklab()`'s own a/b axes: `%` maps ±100%
//     to ±0.4 (`kOklabAxisPercentageBound`), clamped `±0.5` (`kOklabAxisBoundLimit`). `oklch()`'s
//     own chroma: `%` maps 100% to 0.4 (`kOklchMaximumPercentageChroma`), clamped `[0,0.5]`
//     (`kOklchMaximumChroma`); hue same polar->Cartesian step as CIELCh above.
// PT: `ParseOklabColour` (`PropertyParserColour.cpp:435-532`) -- trata TANTO `oklab()` quanto
//     `oklch()`, ramificando em `raw.substr(0,5)=="oklab"` (`:476`). Mesma forma de 5-tokens-com-
//     alpha-isolado-por-`/`/3-tokens-sem-alpha do CIELAB acima. ⚠️ Luminosidade é `[0,1]` aqui
//     (`%`->`/100`), NÃO `[0,100]` como o `lab()` -- o único lugar onde a própria forma de clamp
//     desta função genuinamente diverge do `parse_cielab_function()`. Os próprios eixos a/b do
//     `oklab()`: `%` mapeia ±100% pra ±0.4 (`kOklabAxisPercentageBound`), clampado `±0.5`
//     (`kOklabAxisBoundLimit`). A própria chroma do `oklch()`: `%` mapeia 100% pra 0.4
//     (`kOklchMaximumPercentageChroma`), clampado `[0,0.5]` (`kOklchMaximumChroma`); hue mesmo passo
//     polar->Cartesiano do CIELCh acima.
bool parse_oklab_function(std::string_view raw, Rgba8* out) {
  std::vector<std::string_view> values;
  if (!expand_color_function_values(raw, /*is_comma_separated=*/false, &values)) {
    return false;
  }
  if (values.size() == 5) {
    if (values[3] != "/") {
      return false;
    }
    values[3] = values[4];
    values.pop_back();
  } else {
    if (values.size() != 3) {
      return false;
    }
    values.push_back("1.0");
  }

  float oklab_values[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  for (int idx : {0, 3}) {
    std::string_view v = values[static_cast<std::size_t>(idx)];
    if (v == "none") {
      oklab_values[idx] = 0.0f;
    } else if (!v.empty() && v.back() == '%') {
      oklab_values[idx] = pin_atof(v.substr(0, v.size() - 1)) / 100.0f;
    } else {
      oklab_values[idx] = pin_atof(v);
    }
    oklab_values[idx] = std::clamp(oklab_values[idx], 0.0f, 1.0f);
  }

  const bool is_oklab = raw.substr(0, 5) == "oklab";
  if (is_oklab) {
    for (int idx : {1, 2}) {
      std::string_view v = values[static_cast<std::size_t>(idx)];
      if (v == "none") {
        oklab_values[idx] = 0.0f;
      } else if (!v.empty() && v.back() == '%') {
        constexpr float kOklabAxisPercentageBound = 0.4f;
        oklab_values[idx] =
            pin_atof(v.substr(0, v.size() - 1)) / 100.0f * kOklabAxisPercentageBound;
      } else {
        oklab_values[idx] = pin_atof(v);
      }
      constexpr float kOklabAxisBoundLimit = 0.5f;
      oklab_values[idx] = std::clamp(oklab_values[idx], -kOklabAxisBoundLimit, +kOklabAxisBoundLimit);
    }
  } else {
    std::string_view chroma_tok = values[1];
    float chroma;
    if (chroma_tok == "none") {
      chroma = 0.0f;
    } else if (!chroma_tok.empty() && chroma_tok.back() == '%') {
      constexpr float kOklchMaximumPercentageChroma = 0.4f;
      chroma = pin_atof(chroma_tok.substr(0, chroma_tok.size() - 1)) / 100.0f *
               kOklchMaximumPercentageChroma;
    } else {
      chroma = pin_atof(chroma_tok);
    }
    constexpr float kOklchMaximumChroma = 0.5f;
    chroma = std::clamp(chroma, 0.0f, kOklchMaximumChroma);

    std::string_view hue_tok = values[2];
    float hue = hue_tok == "none" ? 0.0f : pin_atof(hue_tok);

    oklab_values[1] = chroma * std::cos(radians_from_degrees_pin(hue));
    oklab_values[2] = chroma * std::sin(radians_from_degrees_pin(hue));
  }

  oklab_to_rgba(oklab_values);
  std::uint8_t bytes[4];
  for (int i = 0; i < 4; ++i) {
    bytes[i] =
        static_cast<std::uint8_t>(std::clamp(static_cast<int>(oklab_values[i] * 255.0f), 0, 255));
  }
  out->r = bytes[0];
  out->g = bytes[1];
  out->b = bytes[2];
  out->a = bytes[3];
  return true;
}

} // namespace

// EN: `ESC-5`/`ESC-6` -- mirrors the pin's own `ParseColour` dispatch shape (`examples/RmlUi/Source/
//     Core/PropertyParserColour.cpp:166-209`, verified byte-identical against the pinned
//     `glintfx/build/_deps/rmlui-src` copy the build actually links): `#` routes to hex (unchanged,
//     below); then, as of `ESC-6`, the 8 functional-form prefixes (`rgb`/`hsl`/`lab`/`lch`/`oklab`/
//     `oklch`), case-SENSITIVE on the RAW text -- `RGB(...)` does NOT match `"rgb"` here, exactly
//     like the pin's own `value.substr(0,3) == "rgb"` (a plain `==`, no `ToLower` anywhere in this
//     branch of the pin); a functional branch's own parse FAILURE returns `Invalid` DIRECTLY --
//     it NEVER falls through to the name lookup below, matching the pin's own `if
//     (!ParseRGBColour(...)) return false;` shape at every one of its 4 branches (`:178-197`), not a
//     `continue`-to-next-check chain (`labrador` is intercepted, and fails, at the `lab` prefix
//     branch -- it never reaches the name table at all, "prefix steals" the input); only when NONE
//     of `#`/8-functional-prefixes match does this function fall to `to_lower()` (this file's own
//     section-2 helper, already shared by `compute_box_shadow`/`parse_gradient_stop`) + the
//     `kNamedColorTable` lookup, mirroring the pin's own `StringUtilities::ToLower(value)`
//     immediately before its own `html_colours.find()` call (`:201`).
// PT: `ESC-5`/`ESC-6` -- espelha a própria forma de despacho do `ParseColour` do pin
//     (`examples/RmlUi/Source/Core/PropertyParserColour.cpp:166-209`, verificado byte-idêntico
//     contra a própria cópia fixada `glintfx/build/_deps/rmlui-src` que o build de fato linka): `#`
//     roteia pro hex (inalterado, abaixo); depois, desde a `ESC-6`, os 8 prefixos de forma funcional
//     (`rgb`/`hsl`/`lab`/`lch`/`oklab`/`oklch`), case-SENSITIVE sobre o texto CRU -- `RGB(...)` NÃO
//     casa com `"rgb"` aqui, exatamente como o próprio `value.substr(0,3) == "rgb"` do pin (um `==`
//     puro, sem `ToLower` nenhum neste ramo do pin); a própria falha de parse de um ramo funcional
//     retorna `Invalid` DIRETO -- NUNCA cai no lookup de nome abaixo, casando com a própria forma
//     `if (!ParseRGBColour(...)) return false;` do pin em cada uma das 4 ramificações dele
//     (`:178-197`), não uma cadeia `continue`-pra-próxima-checagem (`labrador` é interceptado, e
//     falha, no próprio ramo de prefixo `lab` -- nunca chega na tabela de nome de jeito nenhum, o
//     prefixo "rouba" o input); só quando NENHUM dos `#`/8-prefixos-funcionais casa é que esta
//     função cai pro `to_lower()` (o próprio helper da seção 2 deste arquivo, já compartilhado por
//     `compute_box_shadow`/`parse_gradient_stop`) + o lookup do `kNamedColorTable`, espelhando o
//     próprio `StringUtilities::ToLower(value)` do pin logo antes da própria chamada
//     `html_colours.find()` dele (`:201`).
ValueComputeStatus parse_color(std::string_view raw, Rgba8* out) {
  if (raw.empty() || raw.size() > kMaxRawValueBytes) {
    return ValueComputeStatus::Invalid;
  }
  if (raw[0] == '#') {
    std::string_view hex = raw.substr(1);
    if (hex.size() == 3 || hex.size() == 4) {
      int vals[4] = {0, 0, 0, 0xf};
      for (std::size_t i = 0; i < hex.size(); ++i) {
        int v = hex_digit(hex[i]);
        if (v < 0) {
          return ValueComputeStatus::Invalid;
        }
        vals[i] = v;
      }
      out->r = static_cast<std::uint8_t>(vals[0] * 16 + vals[0]);
      out->g = static_cast<std::uint8_t>(vals[1] * 16 + vals[1]);
      out->b = static_cast<std::uint8_t>(vals[2] * 16 + vals[2]);
      out->a = hex.size() == 4 ? static_cast<std::uint8_t>(vals[3] * 16 + vals[3])
                               : static_cast<std::uint8_t>(0xff);
      return ValueComputeStatus::Ok;
    }
    if (hex.size() == 6 || hex.size() == 8) {
      auto byte_at = [&](std::size_t i) -> int {
        int hi = hex_digit(hex[i]);
        int lo = hex_digit(hex[i + 1]);
        if (hi < 0 || lo < 0) {
          return -1;
        }
        return hi * 16 + lo;
      };
      int r = byte_at(0);
      int g = byte_at(2);
      int b = byte_at(4);
      if (r < 0 || g < 0 || b < 0) {
        return ValueComputeStatus::Invalid;
      }
      int a = hex.size() == 8 ? byte_at(6) : 0xff;
      if (a < 0) {
        return ValueComputeStatus::Invalid;
      }
      out->r = static_cast<std::uint8_t>(r);
      out->g = static_cast<std::uint8_t>(g);
      out->b = static_cast<std::uint8_t>(b);
      out->a = static_cast<std::uint8_t>(a);
      return ValueComputeStatus::Ok;
    }
    return ValueComputeStatus::Invalid;
  }
  // EN: `ESC-6` -- functional color forms, case-SENSITIVE prefix dispatch on `raw` (the ORIGINAL
  //     text, never lowered) -- see this function's own doc-comment above for the full "prefix
  //     steals the input, failure never falls through to the name table" rationale.
  // PT: `ESC-6` -- formas funcionais de cor, despacho de prefixo case-SENSITIVE sobre `raw` (o
  //     próprio texto ORIGINAL, nunca minusculizado) -- ver o próprio doc-comment desta função
  //     acima pro racional completo "o prefixo rouba o input, falha nunca cai no lookup de nome".
  if (raw.substr(0, 3) == "rgb") {
    return parse_rgb_function(raw, out) ? ValueComputeStatus::Ok : ValueComputeStatus::Invalid;
  }
  if (raw.substr(0, 3) == "hsl") {
    return parse_hsl_function(raw, out) ? ValueComputeStatus::Ok : ValueComputeStatus::Invalid;
  }
  if (raw.substr(0, 3) == "lab" || raw.substr(0, 3) == "lch") {
    return parse_cielab_function(raw, out) ? ValueComputeStatus::Ok : ValueComputeStatus::Invalid;
  }
  if (raw.substr(0, 5) == "oklab" || raw.substr(0, 5) == "oklch") {
    return parse_oklab_function(raw, out) ? ValueComputeStatus::Ok : ValueComputeStatus::Invalid;
  }
  std::string lowered = to_lower(raw);
  // EN: `useStlAlgorithm` (cppcheck) -- `std::find_if` instead of a raw hand-rolled loop, same
  //     fix `gamepad_mapping.cpp`/`shorthand.cpp`/`property_registry.cpp` already apply to their
  //     own identically-shaped "linear scan a small declarative table" lookups in this repo.
  // PT: `useStlAlgorithm` (cppcheck) -- `std::find_if` em vez de um laço escrito à mão, o mesmo
  //     conserto que `gamepad_mapping.cpp`/`shorthand.cpp`/`property_registry.cpp` já aplicam
  //     pros próprios lookups de "varredura linear de uma tabela declarativa pequena" com a
  //     mesma forma, neste repo.
  const auto* match = std::find_if(std::begin(kNamedColorTable), std::end(kNamedColorTable),
                                   [&lowered](const NamedColorEntry& entry) { return lowered == entry.name; });
  if (match != std::end(kNamedColorTable)) {
    *out = match->value;
    return ValueComputeStatus::Ok;
  }
  return ValueComputeStatus::Invalid;
}

std::string print_color(const Rgba8& c) {
  static const char kHex[] = "0123456789abcdef";
  std::string s;
  s.reserve(9);
  s.push_back('#');
  auto app = [&](std::uint8_t v) {
    s.push_back(kHex[v >> 4]);
    s.push_back(kHex[v & 0xf]);
  };
  app(c.r);
  app(c.g);
  app(c.b);
  app(c.a);
  return s;
}

// ===========================================================================
// EN: Section 8.1 -- length; section 5 -- symbolic percent; section 8.2 -- angle parse.
// PT: Seção 8.1 -- comprimento; seção 5 -- porcentagem simbólica; seção 8.2 -- parse de ângulo.
// ===========================================================================
ValueComputeStatus parse_length(std::string_view raw, float* out_value, LengthUnit* out_unit) {
  if (raw.empty() || raw.size() > kMaxRawValueBytes) {
    return ValueComputeStatus::Invalid;
  }
  const std::size_t unit_pos = find_unit_boundary(raw);
  const std::string_view number_part = raw.substr(0, unit_pos);
  const std::string_view unit_part = raw.substr(unit_pos);

  if (unit_part.empty()) {
    // EN: Unitless is only accepted for the literal zero (CSS's own zero-length convention) --
    //     unchanged by `ESC-4` -- see value_compute.hpp's own header, "Scope", for why (a policy
    //     this file, not the pin, chooses -- the pin has no unitless-zero exception for LENGTH at
    //     all, `PropertyParserNumber::ParseValue`'s own zero-exception only fires when
    //     `zero_unit != Unit::UNKNOWN`, which the real `length`/`length-percent` parser
    //     registrations never set).
    // PT: Sem unidade só é aceito pro zero literal (a própria convenção de comprimento-zero do
    //     CSS) -- inalterado pela `ESC-4` -- ver "Escopo" no próprio cabeçalho do
    //     value_compute.hpp pro porquê (uma política deste arquivo, não do pin -- o pin não tem
    //     exceção nenhuma de zero-sem-unidade pra LENGTH, a própria exceção-zero do
    //     `PropertyParserNumber::ParseValue` só dispara quando `zero_unit != Unit::UNKNOWN`, que os
    //     próprios registros reais de parser `length`/`length-percent` nunca setam).
    float v = 0.0f;
    if (parse_float_token(number_part, &v) && v == 0.0f) {
      *out_value = 0.0f;
      *out_unit = LengthUnit::Px;
      return ValueComputeStatus::Ok;
    }
    return ValueComputeStatus::Invalid;
  }

  const std::string unit_lower = to_lower(unit_part);
  for (const LengthUnitEntry& entry : kLengthUnitTable) {
    if (unit_lower == entry.suffix) {
      float v = 0.0f;
      if (!parse_float_token(number_part, &v)) {
        return ValueComputeStatus::Invalid;
      }
      *out_value = v;
      *out_unit = entry.unit;
      return ValueComputeStatus::Ok;
    }
  }
  return ValueComputeStatus::Invalid;
}

float resolve_length_px(float value, LengthUnit unit, const LengthResolveContext& ctx) {
  // EN: PPI_UNIT family first (mirrors the pin's own `ComputeLength`'s own `Any(value.unit &
  //     Unit::PPI_UNIT)` check delegating to `ComputePPILength` before its own switch,
  //     `ComputeProperty.cpp:54-55`) -- see value_compute.hpp's own doc-comment for the exact
  //     formula and why multiplication-by-reciprocal, not division.
  // PT: Família PPI_UNIT primeiro (espelha a própria checagem `Any(value.unit & Unit::PPI_UNIT)`
  //     do `ComputeLength` do pin, delegando pro `ComputePPILength` antes do próprio switch dele,
  //     `ComputeProperty.cpp:54-55`) -- ver o próprio doc-comment do value_compute.hpp pra fórmula
  //     exata e por que multiplicação-pelo-recíproco, não divisão.
  if (unit == LengthUnit::In || unit == LengthUnit::Cm || unit == LengthUnit::Mm ||
      unit == LengthUnit::Pt || unit == LengthUnit::Pc) {
    const float inch = value * 96.0f * ctx.dp_ratio;
    switch (unit) {
      case LengthUnit::In:
        return inch;
      case LengthUnit::Cm:
        return inch * (1.0f / 2.54f);
      case LengthUnit::Mm:
        return inch * (1.0f / 25.4f);
      case LengthUnit::Pt:
        return inch * (1.0f / 72.0f);
      case LengthUnit::Pc:
        return inch * (1.0f / 6.0f);
      default:
        break; // unreachable -- the outer `if` already narrowed to these 5.
    }
  }
  switch (unit) {
    case LengthUnit::Px:
      return value;
    case LengthUnit::Dp:
      return value * ctx.dp_ratio;
    case LengthUnit::Em:
      return value * ctx.font_size_px;
    case LengthUnit::Rem:
      return value * ctx.document_font_size_px;
    case LengthUnit::Vw:
      return value * ctx.vp_w_px * 0.01f;
    case LengthUnit::Vh:
      return value * ctx.vp_h_px * 0.01f;
    default:
      return value; // unreachable -- PPI_UNIT already handled above.
  }
}

// EN: `UIX-EM-UNIT`/`ESC-4` -- see value_compute.hpp's own header comment at this function's own
//     declaration for the full "why this stays a separate, narrow function even though
//     parse_length/resolve_length_px are now unit-complete" rationale. Delegates to ONE
//     `parse_length()` call for suffix recognition (unified table, `ESC-4`'s own widened function),
//     then switches on the resulting `LengthUnit`: `Em` reads the EXPLICIT `parent_font_size_px`
//     parameter (this function's own one deliberate exception to the general funnel); `Rem` reads
//     `ctx.document_font_size_px` (the SAME field the general funnel's own `Rem` case already
//     reads -- no exception needed here); every OTHER unit (`Px`/`Dp`/`Vw`/`Vh`/the 5 `PPI_UNIT`
//     members) delegates straight to `resolve_length_px(value, unit, ctx)` unchanged, the pin's own
//     "font-relative lengths handled above, other lengths handled as normal" fallthrough
//     (`ComputeProperty.cpp:116-117`). Both `Em`/`Rem` guard their own ancestor value with
//     `std::isfinite` before multiplying -- a caller-supplied ancestor this function cannot itself
//     validate the shape of must not silently propagate a NaN/Inf product.
// PT: `UIX-EM-UNIT`/`ESC-4` -- ver o próprio comentário de cabeçalho do value_compute.hpp na
//     própria declaração desta função pro racional completo "por que isto continua sendo uma
//     função separada, estreita, mesmo com parse_length/resolve_length_px agora completos em
//     unidade". Delega pra UMA chamada `parse_length()` pro próprio reconhecimento de sufixo
//     (tabela unificada, a própria função alargada da `ESC-4`), depois faz switch no `LengthUnit`
//     resultante: `Em` lê o próprio parâmetro EXPLÍCITO `parent_font_size_px` (a única exceção
//     deliberada desta função ao funil geral); `Rem` lê `ctx.document_font_size_px` (o MESMO campo
//     que o próprio caso `Rem` do funil geral já lê -- nenhuma exceção precisada aqui); toda OUTRA
//     unidade (`Px`/`Dp`/`Vw`/`Vh`/os 5 membros `PPI_UNIT`) delega direto pro
//     `resolve_length_px(value, unit, ctx)` inalterado, o próprio fallthrough "font-relative
//     lengths handled above, other lengths handled as normal" do pin
//     (`ComputeProperty.cpp:116-117`). `Em`/`Rem` os dois guardam o próprio valor ancestral com
//     `std::isfinite` antes de multiplicar -- um valor ancestral fornecido-pelo-chamador que esta
//     função não consegue validar a própria forma sozinha não pode propagar em silêncio um produto
//     NaN/Inf.
ValueComputeStatus parse_font_size(std::string_view raw, float parent_font_size_px,
                                   const LengthResolveContext& ctx, float* out_px) {
  if (raw.empty() || raw.size() > kMaxRawValueBytes) {
    return ValueComputeStatus::Invalid;
  }
  float value = 0.0f;
  LengthUnit unit = LengthUnit::Px;
  if (parse_length(raw, &value, &unit) != ValueComputeStatus::Ok) {
    return ValueComputeStatus::Invalid;
  }
  switch (unit) {
    case LengthUnit::Em:
      if (!std::isfinite(parent_font_size_px)) {
        return ValueComputeStatus::Invalid;
      }
      *out_px = value * parent_font_size_px;
      return ValueComputeStatus::Ok;
    case LengthUnit::Rem:
      if (!std::isfinite(ctx.document_font_size_px)) {
        return ValueComputeStatus::Invalid;
      }
      *out_px = value * ctx.document_font_size_px;
      return ValueComputeStatus::Ok;
    default:
      *out_px = resolve_length_px(value, unit, ctx);
      return ValueComputeStatus::Ok;
  }
}

// EN: `ESC-4` -- `x`/resolution, `Unit::X`. See value_compute.hpp's own `parse_resolution()`
//     doc-comment for the full rationale (deliberately NOT part of `LengthUnit`, its only real
//     pin-side consumer is `@spritesheet`'s `resolution: <n>x`, not implemented yet). Reuses
//     `find_unit_boundary()` for the same reverse-scan mechanics `parse_length()` uses, but matches
//     the unit half against the single literal `"x"` rather than a table -- one entry does not
//     warrant `kLengthUnitTable`'s own array-of-struct shape.
// PT: `ESC-4` -- `x`/resolution, `Unit::X`. Ver o próprio doc-comment do `parse_resolution()` no
//     value_compute.hpp pro racional completo (deliberadamente NÃO parte do `LengthUnit`, o único
//     consumidor real dele no pin é o `resolution: <n>x` do `@spritesheet`, ainda não implementado).
//     Reusa o `find_unit_boundary()` pra mesma mecânica de scan-reverso que o `parse_length()` usa,
//     mas casa a metade unidade contra o literal único `"x"` em vez de uma tabela -- uma entrada só
//     não justifica a própria forma array-de-struct do `kLengthUnitTable`.
ValueComputeStatus parse_resolution(std::string_view raw, float* out_value) {
  if (raw.empty() || raw.size() > kMaxRawValueBytes) {
    return ValueComputeStatus::Invalid;
  }
  const std::size_t unit_pos = find_unit_boundary(raw);
  const std::string_view number_part = raw.substr(0, unit_pos);
  const std::string_view unit_part = raw.substr(unit_pos);
  if (to_lower(unit_part) != "x") {
    return ValueComputeStatus::Invalid;
  }
  float v = 0.0f;
  if (!parse_float_token(number_part, &v)) {
    return ValueComputeStatus::Invalid;
  }
  *out_value = v;
  return ValueComputeStatus::Ok;
}

ValueComputeStatus parse_percent(std::string_view raw, float* out_percent) {
  if (raw.empty() || raw.back() != '%' || raw.size() > kMaxRawValueBytes) {
    return ValueComputeStatus::Invalid;
  }
  float v = 0.0f;
  if (!parse_float_token(raw.substr(0, raw.size() - 1), &v)) {
    return ValueComputeStatus::Invalid;
  }
  *out_percent = v;
  return ValueComputeStatus::Ok;
}

ValueComputeStatus parse_angle(std::string_view raw, float* out_deg) {
  if (raw.empty() || raw.size() > kMaxRawValueBytes) {
    return ValueComputeStatus::Invalid;
  }
  if (ends_with(raw, "deg")) {
    float v = 0.0f;
    if (!parse_float_token(raw.substr(0, raw.size() - 3), &v)) {
      return ValueComputeStatus::Invalid;
    }
    *out_deg = v;
    return ValueComputeStatus::Ok;
  }
  if (ends_with(raw, "rad")) {
    float v = 0.0f;
    if (!parse_float_token(raw.substr(0, raw.size() - 3), &v)) {
      return ValueComputeStatus::Invalid;
    }
    *out_deg = degrees_from_radians(v);
    return ValueComputeStatus::Ok;
  }
  return ValueComputeStatus::Invalid;
}

// ===========================================================================
// EN: Section 9.2.1 -- gradient stop auto-spacing.
// PT: Seção 9.2.1 -- auto-espaçamento de stop de gradiente.
// ===========================================================================
std::vector<float> resolve_gradient_stop_positions(
    const std::vector<std::optional<float>>& explicit_positions_percent) {
  std::size_t n = explicit_positions_percent.size();
  std::vector<float> result(n, 0.0f);
  if (n == 0) {
    return result;
  }
  std::vector<bool> assigned(n, false);
  for (std::size_t i = 0; i < n; ++i) {
    if (explicit_positions_percent[i].has_value()) {
      result[i] = *explicit_positions_percent[i];
      assigned[i] = true;
    }
  }
  // EN: Step 2/3 of the spec's own algorithm -- first/last default to 0%/100% when unpositioned.
  // PT: Passo 2/3 do próprio algoritmo da spec -- primeiro/último default pra 0%/100% quando
  //     sem posição.
  if (!assigned[0]) {
    result[0] = 0.0f;
    assigned[0] = true;
  }
  if (!assigned[n - 1]) {
    result[n - 1] = 100.0f;
    assigned[n - 1] = true;
  }
  // EN: Step 4 -- every remaining run of K consecutive unpositioned stops between two assigned
  //     neighbors gets evenly-spaced positions, verbatim from the spec's own formula.
  // PT: Passo 4 -- todo trecho restante de K stops sem-posição consecutivos entre dois vizinhos
  //     atribuídos recebe posições igualmente espaçadas, verbatim da própria fórmula da spec.
  std::size_t i = 0;
  while (i < n) {
    if (assigned[i]) {
      ++i;
      continue;
    }
    std::size_t run_start = i;
    std::size_t j = i;
    while (j < n && !assigned[j]) {
      ++j;
    }
    float p_before = result[run_start - 1];
    float p_after = result[j];
    auto k_count = static_cast<float>(j - run_start);
    for (std::size_t k = run_start; k < j; ++k) {
      auto step = static_cast<float>(k - run_start + 1);
      result[k] = p_before + step * (p_after - p_before) / (k_count + 1.0f);
      assigned[k] = true;
    }
    i = j;
  }
  return result;
}

// ===========================================================================
// EN: Section 9.1 -- box-shadow.
// PT: Seção 9.1 -- box-shadow.
// ===========================================================================
ValueComputeStatus compute_box_shadow(std::string_view raw_value, const LengthResolveContext& ctx,
                                      std::string* out) {
  out->clear();
  if (raw_value.size() > kMaxRawValueBytes) {
    return ValueComputeStatus::Invalid;
  }
  std::string_view trimmed = trim(raw_value);
  if (trimmed.empty() || trimmed == "none") {
    *out = "none";
    return ValueComputeStatus::Ok;
  }
  // EN: Upstream lowercases the WHOLE value before parsing (`ToLower(value)`,
  //     PropertyParserBoxShadow.cpp -- see this file's own top-of-file header for the citation) --
  //     mirrored here so `INSET`/`Inset` match the same as `inset`, matching real upstream
  //     behaviour rather than a narrower guess.
  // PT: O upstream lowercasa o valor INTEIRO antes de parsear (`ToLower(value)`,
  //     PropertyParserBoxShadow.cpp -- ver o próprio cabeçalho no topo deste arquivo pra citação)
  //     -- espelhado aqui pra `INSET`/`Inset` casarem igual a `inset`, casando com o comportamento
  //     real do upstream em vez de um chute mais estreito.
  std::string lowered = to_lower(trimmed);
  auto layer_pieces = split_top_level(lowered, ',');
  std::vector<std::string> layer_strs;
  for (std::string_view layer_sv : layer_pieces) {
    auto tokens = split_whitespace(layer_sv);
    if (tokens.empty()) {
      return ValueComputeStatus::Invalid;
    }
    bool inset = false;
    bool has_color = false;
    Rgba8 color{};
    float lengths[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    int length_count = 0;
    for (std::string_view tok : tokens) {
      float lv = 0.0f;
      LengthUnit lu = LengthUnit::Px;
      if (parse_length(tok, &lv, &lu) == ValueComputeStatus::Ok) {
        if (length_count >= 4) {
          return ValueComputeStatus::Invalid; // matches upstream's own "default: return false"
        }
        lengths[length_count++] = resolve_length_px(lv, lu, ctx);
      } else if (tok == "inset") {
        inset = true;
      } else if (parse_color(tok, &color) == ValueComputeStatus::Ok) {
        has_color = true;
      } else {
        return ValueComputeStatus::Invalid;
      }
    }
    if (length_count < 2) {
      return ValueComputeStatus::Invalid; // matches upstream's own "length_argument_index < 2"
    }
    if (!has_color) {
      // EN: Documented simplification -- see value_compute.hpp's own header, this function's own
      //     doc-comment, for why this is a deliberate, declared choice, not upstream-verified.
      // PT: Simplificação documentada -- ver o próprio cabeçalho do value_compute.hpp, o próprio
      //     doc-comment desta função, pro porquê disto ser uma escolha deliberada, declarada, não
      //     verificada-contra-upstream.
      return ValueComputeStatus::Invalid;
    }
    // EN: `UIX-RCSS-ERRATA-4`'s own lossy round-trip -- see this file's own
    //     `dump_box_shadow_or_gradient_stop_color()` for the exact byte arithmetic and citation.
    // PT: A própria ida-e-volta com perda da `UIX-RCSS-ERRATA-4` -- ver o próprio
    //     `dump_box_shadow_or_gradient_stop_color()` deste arquivo pra aritmética de byte exata e
    //     citação.
    color = dump_box_shadow_or_gradient_stop_color(color);
    std::string layer;
    layer += print_color(color);
    layer.push_back(';');
    layer += print_length_px(lengths[0]);
    layer.push_back(';');
    layer += print_length_px(lengths[1]);
    layer.push_back(';');
    layer += print_length_px(length_count > 2 ? lengths[2] : 0.0f);
    layer.push_back(';');
    layer += print_length_px(length_count > 3 ? lengths[3] : 0.0f);
    layer.push_back(';');
    layer += inset ? "true" : "false";
    layer_strs.push_back(std::move(layer));
  }
  *out = join(layer_strs, '|');
  return ValueComputeStatus::Ok;
}

// ===========================================================================
// EN: Section 9.2 -- linear-gradient / radial-gradient argument grammars.
// PT: Seção 9.2 -- gramáticas de argumento de linear-gradient / radial-gradient.
// ===========================================================================
ValueComputeStatus compute_linear_gradient_args(std::string_view inner_args, std::string* out) {
  out->clear();
  if (inner_args.size() > kMaxRawValueBytes) {
    return ValueComputeStatus::Invalid;
  }
  auto pieces = split_top_level(inner_args, ',');
  if (pieces.size() < 3) { // angle + at least 2 stops
    return ValueComputeStatus::Invalid;
  }
  float angle_deg = 0.0f;
  if (parse_angle(pieces[0], &angle_deg) != ValueComputeStatus::Ok) {
    return ValueComputeStatus::Invalid;
  }
  std::vector<std::string_view> stop_pieces(pieces.begin() + 1, pieces.end());
  std::vector<std::string> parts;
  parts.push_back(print_angle_deg(angle_deg));
  if (!parse_and_space_stops(stop_pieces, &parts)) {
    return ValueComputeStatus::Invalid;
  }
  *out = join(parts, ';');
  return ValueComputeStatus::Ok;
}

ValueComputeStatus compute_radial_gradient_args(std::string_view inner_args, std::string* out) {
  out->clear();
  if (inner_args.size() > kMaxRawValueBytes) {
    return ValueComputeStatus::Invalid;
  }
  // EN: `split_top_level()` always returns at least one piece (even for empty input, per its
  //     own header comment) -- an emptiness check here would be dead code; the `size() < 2`
  //     guard inside `parse_and_space_stops()` below already rejects too-few-stops, including
  //     the single-empty-piece case an empty `inner_args` produces.
  // PT: `split_top_level()` sempre retorna pelo menos um pedaço (mesmo pra input vazio, per o
  //     próprio comentário de cabeçalho dela) -- uma checagem de vacuidade aqui seria código
  //     morto; a própria guarda `size() < 2` dentro do `parse_and_space_stops()` abaixo já
  //     rejeita stops-de-menos, incluindo o caso de peça-única-vazia que um `inner_args` vazio
  //     produz.
  auto pieces = split_top_level(inner_args, ',');
  float cx = 50.0f;
  float cy = 50.0f;
  std::size_t stops_begin = 0;
  std::string_view first = pieces[0];
  if (first.substr(0, 7) == "ellipse") {
    // EN: docs/uix-rcss.md section 13's own out-of-scope clause -- `ellipse` is fail-high, never
    //     silently coerced to `circle`.
    // PT: A própria cláusula fora-de-escopo da seção 13 do docs/uix-rcss.md -- `ellipse` é
    //     fail-high, nunca coagido em silêncio pra `circle`.
    return ValueComputeStatus::Invalid;
  }
  if (first.substr(0, 6) == "circle") {
    auto tokens = split_whitespace(first);
    if (tokens.size() != 4 || tokens[0] != "circle" || tokens[1] != "at") {
      return ValueComputeStatus::Invalid;
    }
    if (parse_percent(tokens[2], &cx) != ValueComputeStatus::Ok ||
        parse_percent(tokens[3], &cy) != ValueComputeStatus::Ok) {
      return ValueComputeStatus::Invalid;
    }
    stops_begin = 1;
  }
  std::vector<std::string_view> stop_pieces(pieces.begin() + static_cast<long>(stops_begin),
                                            pieces.end());
  std::vector<std::string> parts;
  parts.push_back(print_percent(cx));
  parts.push_back(print_percent(cy));
  if (!parse_and_space_stops(stop_pieces, &parts)) {
    return ValueComputeStatus::Invalid;
  }
  *out = join(parts, ';');
  return ValueComputeStatus::Ok;
}

// ===========================================================================
// EN: Section 9.2 -- the remaining single-purpose decorator/filter functions, plus the dispatcher
//     and the list-level `compute_decorator_list()`.
// PT: Seção 9.2 -- as funções restantes de decorator/filter de propósito único, mais o
//     despachante e o `compute_decorator_list()` de nível-lista.
// ===========================================================================
namespace {

ValueComputeStatus compute_polygon(std::string_view inner, const LengthResolveContext& ctx,
                                   int depth, std::string* out) {
  auto pieces = split_top_level(inner, ',');
  if (pieces.size() < 2 || pieces.size() > 3) {
    return ValueComputeStatus::Invalid;
  }
  float sides_f = 0.0f;
  if (!parse_float_token(pieces[0], &sides_f)) {
    return ValueComputeStatus::Invalid;
  }
  if (std::trunc(sides_f) != sides_f || sides_f < 3.0f || sides_f > 1024.0f) {
    return ValueComputeStatus::Invalid; // docs/effects.md's own [3, 1024] range, fail-high
  }
  std::string fill_str;
  std::string_view fill = pieces[1];
  if (fill.substr(0, 15) == "linear-gradient" || fill.substr(0, 15) == "radial-gradient") {
    auto calls = scan_function_calls(fill);
    if (calls.size() != 1) {
      return ValueComputeStatus::Invalid;
    }
    if (compute_one_decorator_function(calls[0].name, calls[0].inner, ctx, depth + 1,
                                       &fill_str) != ValueComputeStatus::Ok) {
      return ValueComputeStatus::Invalid;
    }
  } else {
    Rgba8 color{};
    if (parse_color(fill, &color) != ValueComputeStatus::Ok) {
      return ValueComputeStatus::Invalid;
    }
    fill_str = print_color(color);
  }
  float rotation_deg = 0.0f;
  if (pieces.size() == 3 && !pieces[2].empty()) {
    if (parse_angle(pieces[2], &rotation_deg) != ValueComputeStatus::Ok) {
      return ValueComputeStatus::Invalid;
    }
  }
  std::vector<std::string> parts;
  parts.push_back(print_number(sides_f));
  parts.push_back(fill_str);
  parts.push_back(print_angle_deg(rotation_deg));
  *out = join(parts, ';');
  return ValueComputeStatus::Ok;
}

// EN: `UIX-GRADIENT-ALFA` -- shared by `compute_horizontal_gradient`/`compute_vertical_gradient`.
//     ⚠️ CORRECTED (this item): the previous version of this comment claimed both reuse upstream's
//     own `PropertyParserColorStopList`, "the SAME parser every `linear-gradient`/`radial-gradient`
//     stop goes through" -- that claim was FALSE, inherited from `docs/uix-rcss.md`'s own
//     `UIX-RCSS-ERRATA-4` text without independent re-verification, and it drove this function to
//     apply `dump_box_shadow_or_gradient_stop_color()`'s own lossy premultiply-round-trip where it
//     never belonged, corrupting the RGB of any low-alpha 8-digit hex stop
//     (`UIX-ORACLE-MEDICAO`'s own residuo C, `system_menu__config_controles_tabela.rml:469`).
//     Verified by reading `examples/RmlUi/Source/Core/DecoratorGradient.h`/`.cpp` directly:
//     `DecoratorStraightGradient` (the type `horizontal-gradient`/`vertical-gradient` actually
//     instantiate) declares plain `Colourb start, stop;` (`DecoratorGradient.h:34`) -- NOT
//     `ColourbPremultiplied`, structurally different from `BoxShadow`'s/`ColorStop`'s own
//     `ColourbPremultiplied color;` (`DecorationTypes.h:9`/`:22`) -- and
//     `DecoratorStraightGradientInstancer::InstanceDecorator` (`DecoratorGradient.cpp:196-219`)
//     fetches `properties_.GetProperty(ids.start)->Get<Colourb>()`, never calling
//     `.ToPremultiplied()` anywhere, a completely different code path from
//     `PropertyParserColorStopList.cpp:47`/`PropertyParserBoxShadow.cpp:72`'s own
//     `.ToPremultiplied()` calls. These two colors therefore print STRAIGHT, exactly like
//     `background-color`/`border-*-color`/every other non-premultiplied color-typed field this
//     registry has -- `parse_color()`/`print_color()` alone, no round-trip step. Full derivation,
//     TDD proof, and the routing note for the spec's own errata (this item does not edit
//     `docs/uix-rcss.md` itself) are in this file's own test suite,
//     `test_gradient_alpha_roundtrip_matches_upstream_storage_type`
//     (`tests/uix_style/value_compute_sanity.cpp`).
// PT: `UIX-GRADIENT-ALFA` -- compartilhado por `compute_horizontal_gradient`/
//     `compute_vertical_gradient`. ⚠️ CORRIGIDO (este item): a versão anterior deste comentário
//     alegava que os dois reusam o próprio `PropertyParserColorStopList` do upstream, "o MESMO
//     parser que todo stop de `linear-gradient`/`radial-gradient` atravessa" -- essa alegação era
//     FALSA, herdada do próprio texto da `UIX-RCSS-ERRATA-4` do docs/uix-rcss.md sem
//     re-verificação independente, e levou esta função a aplicar a própria ida-e-volta com perda
//     de premultiplicação do `dump_box_shadow_or_gradient_stop_color()` onde ela nunca pertencia,
//     corrompendo o RGB de todo stop hex de 8 dígitos em alfa baixo (o próprio resíduo C da
//     `UIX-ORACLE-MEDICAO`, `system_menu__config_controles_tabela.rml:469`). Verificado lendo
//     direto o `examples/RmlUi/Source/Core/DecoratorGradient.h`/`.cpp`: o
//     `DecoratorStraightGradient` (o tipo que `horizontal-gradient`/`vertical-gradient` de fato
//     instanciam) declara `Colourb start, stop;` plano (`DecoratorGradient.h:34`) -- NÃO
//     `ColourbPremultiplied`, estruturalmente diferente do próprio `ColourbPremultiplied color;`
//     do `BoxShadow`/`ColorStop` (`DecorationTypes.h:9`/`:22`) -- e o próprio
//     `DecoratorStraightGradientInstancer::InstanceDecorator` (`DecoratorGradient.cpp:196-219`)
//     busca `properties_.GetProperty(ids.start)->Get<Colourb>()`, nunca chamando
//     `.ToPremultiplied()` em lugar nenhum, um caminho de código completamente diferente das
//     próprias chamadas `.ToPremultiplied()` do `PropertyParserColorStopList.cpp:47`/
//     `PropertyParserBoxShadow.cpp:72`. Estas duas cores portanto imprimem RETAS, exatamente como
//     `background-color`/`border-*-color`/todo outro campo tipo-cor não-premultiplicado deste
//     registro -- só `parse_color()`/`print_color()`, sem passo de ida-e-volta. Derivação
//     completa, prova TDD, e a nota de roteamento pra própria errata da spec (este item não edita
//     o docs/uix-rcss.md sozinho) estão na própria suíte de teste deste arquivo,
//     `test_gradient_alpha_roundtrip_matches_upstream_storage_type`
//     (`tests/uix_style/value_compute_sanity.cpp`).
ValueComputeStatus compute_two_stop_straight_gradient(std::string_view inner, std::string* out) {
  auto tokens = split_whitespace(inner);
  if (tokens.size() != 2) {
    return ValueComputeStatus::Invalid;
  }
  Rgba8 c0{};
  Rgba8 c1{};
  if (parse_color(tokens[0], &c0) != ValueComputeStatus::Ok ||
      parse_color(tokens[1], &c1) != ValueComputeStatus::Ok) {
    return ValueComputeStatus::Invalid;
  }
  std::vector<std::string> parts{print_color(c0), print_color(c1)};
  *out = join(parts, ';');
  return ValueComputeStatus::Ok;
}

ValueComputeStatus compute_drop_shadow(std::string_view inner, const LengthResolveContext& ctx,
                                       std::string* out) {
  auto tokens = split_whitespace(inner);
  if (tokens.size() != 4) {
    return ValueComputeStatus::Invalid;
  }
  Rgba8 color{};
  if (parse_color(tokens[0], &color) != ValueComputeStatus::Ok) {
    return ValueComputeStatus::Invalid;
  }
  float lens[3] = {0.0f, 0.0f, 0.0f};
  for (int i = 0; i < 3; ++i) {
    float v = 0.0f;
    LengthUnit u = LengthUnit::Px;
    if (parse_length(tokens[static_cast<std::size_t>(i) + 1], &v, &u) != ValueComputeStatus::Ok) {
      return ValueComputeStatus::Invalid;
    }
    lens[i] = resolve_length_px(v, u, ctx);
  }
  std::vector<std::string> parts{print_color(color), print_length_px(lens[0]),
                                 print_length_px(lens[1]), print_length_px(lens[2])};
  *out = join(parts, ';');
  return ValueComputeStatus::Ok;
}

ValueComputeStatus compute_blur(std::string_view inner, const LengthResolveContext& ctx,
                                std::string* out) {
  std::string_view t = trim(inner);
  float v = 0.0f;
  LengthUnit u = LengthUnit::Px;
  if (parse_length(t, &v, &u) != ValueComputeStatus::Ok) {
    return ValueComputeStatus::Invalid;
  }
  *out = print_length_px(resolve_length_px(v, u, ctx));
  return ValueComputeStatus::Ok;
}

ValueComputeStatus compute_ripple(std::string_view inner, std::string* out) {
  std::string_view t = trim(inner);
  float v = 0.0f;
  if (t.empty()) {
    v = 0.0f; // docs/effects.md's own default: omitted/empty means auto (0)
  } else if (!parse_float_token(t, &v)) {
    return ValueComputeStatus::Invalid;
  }
  *out = print_number(v);
  return ValueComputeStatus::Ok;
}

ValueComputeStatus compute_url_function(std::string_view inner, std::string* out) {
  std::string_view t = trim(inner);
  if (t.empty()) {
    return ValueComputeStatus::Invalid;
  }
  *out = print_string(t);
  return ValueComputeStatus::Ok;
}

} // namespace

ValueComputeStatus compute_one_decorator_function(std::string_view name, std::string_view inner,
                                                  const LengthResolveContext& ctx, int depth,
                                                  std::string* out) {
  if (depth > kMaxNestDepth) {
    return ValueComputeStatus::Invalid;
  }
  std::string args;
  ValueComputeStatus st = ValueComputeStatus::Invalid;
  if (name == "image") {
    st = compute_url_function(inner, &args);
  } else if (name == "image-tint") {
    st = compute_url_function(inner, &args);
  } else if (name == "linear-gradient") {
    st = compute_linear_gradient_args(inner, &args);
  } else if (name == "radial-gradient") {
    st = compute_radial_gradient_args(inner, &args);
  } else if (name == "polygon") {
    st = compute_polygon(inner, ctx, depth, &args);
  } else if (name == "ripple") {
    st = compute_ripple(inner, &args);
  } else if (name == "horizontal-gradient" || name == "vertical-gradient") {
    // EN: `UIX-RCSS-ERRATA-2`'s own added row -- 107 corpus occurrences, the single most-used
    //     decorator function in the corpus. Same `DecoratorStraightGradientInstancer`, same
    //     2-color grammar as `horizontal-gradient` (docs/uix-rcss.md section 9.2's own table) --
    //     the function NAME alone picks the axis at render time, this dump format's own grammar
    //     for the two is byte-identical.
    // PT: A própria linha acrescentada da `UIX-RCSS-ERRATA-2` -- 107 ocorrências no corpus, a
    //     função de decorator mais usada do corpus inteiro. Mesmo `DecoratorStraightGradientInstancer`,
    //     mesma gramática de 2 cores do `horizontal-gradient` (a própria tabela da seção 9.2 do
    //     docs/uix-rcss.md) -- só o NOME da função escolhe o eixo em tempo de render, a própria
    //     gramática deste formato de dump pras duas é byte-idêntica.
    st = compute_two_stop_straight_gradient(inner, &args);
  } else if (name == "blur") {
    st = compute_blur(inner, ctx, &args);
  } else if (name == "drop-shadow") {
    st = compute_drop_shadow(inner, ctx, &args);
  } else {
    return ValueComputeStatus::Invalid; // unknown function name -- section 11's own fail-high case
  }
  if (st != ValueComputeStatus::Ok) {
    return ValueComputeStatus::Invalid;
  }
  out->clear();
  *out += name;
  out->push_back('(');
  *out += args;
  out->push_back(')');
  return ValueComputeStatus::Ok;
}

ValueComputeStatus compute_decorator_list(std::string_view raw_value,
                                          const LengthResolveContext& ctx, std::string* out) {
  out->clear();
  std::string_view trimmed = trim(raw_value);
  if (trimmed.empty() || trimmed == "none") {
    *out = "none";
    return ValueComputeStatus::Ok;
  }
  if (trimmed.size() > kMaxRawValueBytes) {
    return ValueComputeStatus::Invalid;
  }
  auto calls = scan_function_calls(trimmed);
  if (calls.empty()) {
    // EN: Non-empty, non-`"none"` text that does not scan into even ONE `name(args)` call --
    //     genuinely malformed input, not a valid empty resolution. Returning `Ok`+`"none"` here
    //     would be indistinguishable from a real empty/`"none"` declaration, hiding a fail-high
    //     case behind a value that looks legitimate.
    // PT: Texto não-vazio, não-`"none"`, que não escaneia em nem UMA chamada `name(args)` --
    //     input genuinamente malformado, não uma resolução vazia válida. Retornar `Ok`+`"none"`
    //     aqui seria indistinguível de uma declaração `"none"`/vazia real, escondendo um caso
    //     fail-high atrás de um valor que parece legítimo.
    return ValueComputeStatus::Invalid;
  }
  std::vector<std::string> results;
  for (const FunctionCall& call : calls) {
    std::string one;
    if (compute_one_decorator_function(call.name, call.inner, ctx, 0, &one) !=
        ValueComputeStatus::Ok) {
      // EN: `UIX-RCSS-ERRATA-2`'s own correction to `Finding C` -- a single unrecognised/malformed
      //     entry aborts the WHOLE property, matching upstream's own `return false` on the FIRST
      //     bad entry (`PropertyParserDecorator::ParseValue`/`PropertyParserFilter::ParseValue`,
      //     `property.value` never assigned) -- no partial list survives.
      // PT: A própria correção da `UIX-RCSS-ERRATA-2` ao `Finding C` -- uma única entrada
      //     não-reconhecida/malformada derruba a PROPRIEDADE INTEIRA, casando com o próprio
      //     `return false` do upstream na PRIMEIRA entrada ruim
      //     (`PropertyParserDecorator::ParseValue`/`PropertyParserFilter::ParseValue`, o próprio
      //     `property.value` nunca atribuído) -- nenhuma lista parcial sobrevive.
      return ValueComputeStatus::Invalid;
    }
    results.push_back(std::move(one));
  }
  if (results.empty()) {
    *out = "none";
    return ValueComputeStatus::Ok;
  }
  *out = join(results, '|');
  return ValueComputeStatus::Ok;
}

// ===========================================================================
// EN: Section 9.4 -- transform (2D subset).
// PT: Seção 9.4 -- transform (subconjunto 2D).
// ===========================================================================
namespace {

ValueComputeStatus compute_translate(std::string_view inner, const LengthResolveContext& ctx,
                                     std::string* out) {
  auto pieces = split_top_level(inner, ',');
  if (pieces.size() != 2) {
    return ValueComputeStatus::Invalid;
  }
  float lens[2] = {0.0f, 0.0f};
  for (int i = 0; i < 2; ++i) {
    float v = 0.0f;
    LengthUnit u = LengthUnit::Px;
    if (parse_length(pieces[static_cast<std::size_t>(i)], &v, &u) != ValueComputeStatus::Ok) {
      return ValueComputeStatus::Invalid;
    }
    lens[i] = resolve_length_px(v, u, ctx);
  }
  std::vector<std::string> parts{print_length_px(lens[0]), print_length_px(lens[1])};
  *out = join(parts, ';');
  return ValueComputeStatus::Ok;
}

ValueComputeStatus compute_scale(std::string_view inner, std::string* out) {
  auto pieces = split_top_level(inner, ',');
  if (pieces.size() != 2) {
    return ValueComputeStatus::Invalid;
  }
  float nums[2] = {0.0f, 0.0f};
  for (int i = 0; i < 2; ++i) {
    if (!parse_float_token(pieces[static_cast<std::size_t>(i)], &nums[i])) {
      return ValueComputeStatus::Invalid;
    }
  }
  std::vector<std::string> parts{print_number(nums[0]), print_number(nums[1])};
  *out = join(parts, ';');
  return ValueComputeStatus::Ok;
}

ValueComputeStatus compute_rotate(std::string_view inner, std::string* out) {
  std::string_view t = trim(inner);
  float deg = 0.0f;
  if (parse_angle(t, &deg) != ValueComputeStatus::Ok) {
    return ValueComputeStatus::Invalid;
  }
  *out = print_angle_deg(deg);
  return ValueComputeStatus::Ok;
}

} // namespace

ValueComputeStatus compute_transform_list(std::string_view raw_value,
                                          const LengthResolveContext& ctx, std::string* out) {
  out->clear();
  std::string_view trimmed = trim(raw_value);
  if (trimmed.empty() || trimmed == "none") {
    *out = "none";
    return ValueComputeStatus::Ok;
  }
  if (trimmed.size() > kMaxRawValueBytes) {
    return ValueComputeStatus::Invalid;
  }
  auto calls = scan_function_calls(trimmed);
  if (calls.empty()) {
    // EN: Same reasoning as `compute_decorator_list()`'s own identical guard -- non-empty,
    //     non-`"none"` text with zero scannable function calls is malformed, never a silent
    //     `"none"`.
    // PT: Mesmo raciocínio da própria guarda idêntica do `compute_decorator_list()` -- texto
    //     não-vazio, não-`"none"`, com zero chamadas de função escaneáveis é malformado, nunca um
    //     `"none"` em silêncio.
    return ValueComputeStatus::Invalid;
  }
  std::vector<std::string> results;
  for (const FunctionCall& call : calls) {
    std::string args;
    ValueComputeStatus st = ValueComputeStatus::Invalid;
    if (call.name == "translate") {
      st = compute_translate(call.inner, ctx, &args);
    } else if (call.name == "scale") {
      st = compute_scale(call.inner, &args);
    } else if (call.name == "rotate") {
      st = compute_rotate(call.inner, &args);
    }
    if (st != ValueComputeStatus::Ok) {
      // EN: Section 11's own uniform malformed-entry policy, applied consistently to `transform`
      //     too (this module's own decision, since `transform`'s own scope is explicitly thin and
      //     not verified against a real upstream parser loop the way `decorator`/`box-shadow` are
      //     -- see value_compute.hpp's own header for `transform`'s own stated limit).
      // PT: A própria política uniforme de entrada-malformada da seção 11, aplicada
      //     consistentemente pro `transform` também (decisão própria deste módulo, já que o
      //     próprio escopo de `transform` é explicitamente fino e não verificado contra um laço de
      //     parser upstream real do jeito que `decorator`/`box-shadow` são -- ver o próprio
      //     cabeçalho do value_compute.hpp pro próprio limite declarado de `transform`).
      return ValueComputeStatus::Invalid;
    }
    std::string one;
    one += call.name;
    one.push_back('(');
    one += args;
    one.push_back(')');
    results.push_back(std::move(one));
  }
  *out = join(results, '|');
  return ValueComputeStatus::Ok;
}

} // namespace glintfx::uix::style
