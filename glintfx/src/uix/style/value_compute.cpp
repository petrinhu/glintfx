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
                                                  float dp_ratio, int depth, std::string* out);

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

// EN: Plain whitespace tokenizer (no paren awareness needed -- every function this module
//     tokenizes this way, `box-shadow` layers, gradient stops, `radial-gradient`'s own
//     `circle at X% Y%` clause, never nests a parenthesis inside one space-separated token, per
//     this repo's own corpus, docs/uix-rcss.md sections 9.1/9.2.1). Empty tokens are never
//     produced.
// PT: Tokenizador de whitespace puro (sem consciência de parêntese necessária -- toda função que
//     este módulo tokeniza assim, camadas de `box-shadow`, stops de gradiente, a própria cláusula
//     `circle at X% Y%` do `radial-gradient`, nunca aninha um parêntese dentro de um token
//     separado-por-espaço, per o próprio corpus deste repo, seções 9.1/9.2.1 do docs/uix-rcss.md).
//     Tokens vazios nunca são produzidos.
std::vector<std::string_view> split_whitespace(std::string_view s) {
  std::vector<std::string_view> out;
  std::size_t i = 0;
  std::size_t n = s.size();
  while (i < n) {
    while (i < n && is_ws(s[i])) {
      ++i;
    }
    std::size_t start = i;
    while (i < n && !is_ws(s[i])) {
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
} // namespace

ValueComputeStatus parse_color(std::string_view raw, Rgba8* out) {
  if (raw.empty() || raw.size() > kMaxRawValueBytes) {
    return ValueComputeStatus::Invalid;
  }
  if (raw == "white") {
    *out = Rgba8{0xff, 0xff, 0xff, 0xff};
    return ValueComputeStatus::Ok;
  }
  if (raw == "black") {
    *out = Rgba8{0, 0, 0, 0xff};
    return ValueComputeStatus::Ok;
  }
  if (raw == "transparent") {
    *out = Rgba8{0, 0, 0, 0};
    return ValueComputeStatus::Ok;
  }
  if (raw[0] != '#') {
    return ValueComputeStatus::Invalid;
  }
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
  if (ends_with(raw, "px")) {
    float v = 0.0f;
    if (!parse_float_token(raw.substr(0, raw.size() - 2), &v)) {
      return ValueComputeStatus::Invalid;
    }
    *out_value = v;
    *out_unit = LengthUnit::Px;
    return ValueComputeStatus::Ok;
  }
  if (ends_with(raw, "dp")) {
    float v = 0.0f;
    if (!parse_float_token(raw.substr(0, raw.size() - 2), &v)) {
      return ValueComputeStatus::Invalid;
    }
    *out_value = v;
    *out_unit = LengthUnit::Dp;
    return ValueComputeStatus::Ok;
  }
  // EN: Unitless is only accepted for the literal zero (CSS's own zero-length convention) -- see
  //     value_compute.hpp's own header, "Scope", for why every OTHER unit (em/rem/vw/vh) is
  //     `Invalid` rather than guessed.
  // PT: Sem unidade só é aceito pro zero literal (a própria convenção de comprimento-zero do CSS)
  //     -- ver "Escopo" no próprio cabeçalho do value_compute.hpp pro porquê toda OUTRA unidade
  //     (em/rem/vw/vh) é `Invalid` em vez de chutada.
  float v = 0.0f;
  if (parse_float_token(raw, &v) && v == 0.0f) {
    *out_value = 0.0f;
    *out_unit = LengthUnit::Px;
    return ValueComputeStatus::Ok;
  }
  return ValueComputeStatus::Invalid;
}

float resolve_length_px(float value, LengthUnit unit, float dp_ratio) {
  return unit == LengthUnit::Dp ? value * dp_ratio : value;
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
ValueComputeStatus compute_box_shadow(std::string_view raw_value, float dp_ratio,
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
        lengths[length_count++] = resolve_length_px(lv, lu, dp_ratio);
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

ValueComputeStatus compute_polygon(std::string_view inner, float dp_ratio, int depth,
                                   std::string* out) {
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
    if (compute_one_decorator_function(calls[0].name, calls[0].inner, dp_ratio, depth + 1,
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

// EN: Shared by `compute_horizontal_gradient`/`compute_vertical_gradient` -- both reuse upstream's
//     own `PropertyParserColorStopList` (`DecoratorGradient.cpp:186-221`'s own dispatch on the
//     decorator name, the SAME parser every `linear-gradient`/`radial-gradient` stop goes through),
//     so both colors get the SAME `UIX-RCSS-ERRATA-4` lossy round-trip gradient stops do.
// PT: Compartilhado por `compute_horizontal_gradient`/`compute_vertical_gradient` -- os dois
//     reusam o próprio `PropertyParserColorStopList` do upstream (o próprio dispatch por nome de
//     decorator do `DecoratorGradient.cpp:186-221`, o MESMO parser que todo stop de
//     `linear-gradient`/`radial-gradient` atravessa), então as duas cores recebem a MESMA
//     ida-e-volta com perda da `UIX-RCSS-ERRATA-4` que os stops de gradiente recebem.
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
  c0 = dump_box_shadow_or_gradient_stop_color(c0);
  c1 = dump_box_shadow_or_gradient_stop_color(c1);
  std::vector<std::string> parts{print_color(c0), print_color(c1)};
  *out = join(parts, ';');
  return ValueComputeStatus::Ok;
}

ValueComputeStatus compute_drop_shadow(std::string_view inner, float dp_ratio, std::string* out) {
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
    lens[i] = resolve_length_px(v, u, dp_ratio);
  }
  std::vector<std::string> parts{print_color(color), print_length_px(lens[0]),
                                 print_length_px(lens[1]), print_length_px(lens[2])};
  *out = join(parts, ';');
  return ValueComputeStatus::Ok;
}

ValueComputeStatus compute_blur(std::string_view inner, float dp_ratio, std::string* out) {
  std::string_view t = trim(inner);
  float v = 0.0f;
  LengthUnit u = LengthUnit::Px;
  if (parse_length(t, &v, &u) != ValueComputeStatus::Ok) {
    return ValueComputeStatus::Invalid;
  }
  *out = print_length_px(resolve_length_px(v, u, dp_ratio));
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
                                                  float dp_ratio, int depth, std::string* out) {
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
    st = compute_polygon(inner, dp_ratio, depth, &args);
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
    st = compute_blur(inner, dp_ratio, &args);
  } else if (name == "drop-shadow") {
    st = compute_drop_shadow(inner, dp_ratio, &args);
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

ValueComputeStatus compute_decorator_list(std::string_view raw_value, float dp_ratio,
                                          std::string* out) {
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
    if (compute_one_decorator_function(call.name, call.inner, dp_ratio, 0, &one) !=
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

ValueComputeStatus compute_translate(std::string_view inner, float dp_ratio, std::string* out) {
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
    lens[i] = resolve_length_px(v, u, dp_ratio);
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

ValueComputeStatus compute_transform_list(std::string_view raw_value, float dp_ratio,
                                          std::string* out) {
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
      st = compute_translate(call.inner, dp_ratio, &args);
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
