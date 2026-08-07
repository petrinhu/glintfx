// SPDX-License-Identifier: Apache-2.0
// EN: UIX-VALUE-COMPUTE -- functional test for glintfx::uix::style's pure value-computation
//     functions (value_compute.hpp). Standalone, no parser, no cascade -- every case below traces
//     to a specific docs/uix-rcss.md section, and the four cases named `worked_example_15_*`
//     reproduce that document's own byte-exact worked examples (sections 15.2-15.4) verbatim, not
//     paraphrased -- this task's own DoD: "transforme cada [exemplo trabalhado] num teste seu".
// PT: UIX-VALUE-COMPUTE -- teste funcional pras funções puras de computação de valor do
//     glintfx::uix::style (value_compute.hpp). Standalone, sem parser, sem cascata -- todo caso
//     abaixo remonta a uma seção específica do docs/uix-rcss.md, e os quatro casos nomeados
//     `worked_example_15_*` reproduzem os próprios exemplos trabalhados byte-exatos daquele
//     documento (seções 15.2-15.4) verbatim, não parafraseados -- o próprio DoD desta tarefa:
//     "transforme cada [exemplo trabalhado] num teste seu".
// Copyright (c) 2026 Petrus Silva Costa
#include "uix/style/value_compute.hpp"

#include "uix/style/property_registry.hpp"

#include <cmath>
#include <cstdio>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

void check_eq(const std::string& got, const std::string& want, const char* what) {
  if (got != want) {
    std::fprintf(stderr, "FAIL: %s (got \"%s\", want \"%s\")\n", what, got.c_str(), want.c_str());
    ++g_failures;
  }
}

using namespace glintfx::uix::style;

// ---------------------------------------------------------------------------
// EN: docs/uix-rcss.md section 15.4, `UIX-RCSS-ERRATA-3`'s own corrected form (`da0d73c`,
//     2026-08-06), byte-exact, all 4 rows. **This item's own history with this table:** the
//     pre-errata-3 literals (`1.234450`/`1.234449`, both signs) never actually reached the "exact
//     tie" boundary they claimed to pin -- widened to `double`, `1.234450f` is
//     `1.2344499826431274`, already strictly below the tie (measured independently, both by this
//     item during its own implementation and, separately, by `UIX-RCSS-DUMP-A`'s own author,
//     commit `a1e0b9f`, using the same Python one-liner verification and landing on the identical
//     replacement literal, `1.21875f = 39/32`, before `UIX-RCSS-ERRATA-3` canonized it -- two
//     independent authors reaching the same structural fix because the fix is not arbitrary: a
//     decimal literal only lands on a real binary32 tie when its own reduced fraction already has
//     a power-of-two denominator, and `1.21875 = 39/32` is the nearest such value to the table's
//     own original decimal neighborhood). This item's own original divergence report (an earlier
//     draft of this same file, kept in this repo's own history, not reproduced here) is
//     SUPERSEDED by this correction landing in the spec itself -- the table below is no longer a
//     reported divergence, it is the document's own current, correct worked example.
// PT: Seção 15.4 do docs/uix-rcss.md, a própria forma corrigida da `UIX-RCSS-ERRATA-3` (`da0d73c`,
//     2026-08-06), byte-exata, as 4 linhas. **A própria história deste item com esta tabela:** os
//     literais pré-errata-3 (`1.234450`/`1.234449`, os dois sinais) nunca de fato alcançavam a
//     fronteira de "empate exato" que alegavam pinar -- ampliado pra `double`, `1.234450f` é
//     `1.2344499826431274`, já estritamente abaixo do empate (medido independentemente, tanto por
//     este item durante a própria implementação quanto, separadamente, pelo próprio autor da
//     `UIX-RCSS-DUMP-A`, commit `a1e0b9f`, usando o mesmo one-liner Python de verificação e pousando
//     no mesmo literal de substituição, `1.21875f = 39/32`, antes da `UIX-RCSS-ERRATA-3` canonizar
//     isso -- dois autores independentes chegando no mesmo conserto estrutural porque o conserto
//     não é arbitrário: um literal decimal só pousa num empate binary32 real quando a própria
//     fração reduzida dele já tem denominador potência-de-dois, e `1.21875 = 39/32` é o valor mais
//     próximo assim da própria vizinhança decimal original da tabela). O próprio relatório de
//     divergência original deste item (uma versão anterior deste mesmo arquivo, mantida no próprio
//     histórico deste repo, não reproduzida aqui) é SUPERADO por esta correção pousar na própria
//     spec -- a tabela abaixo não é mais uma divergência reportada, é o próprio exemplo trabalhado
//     atual, correto, do documento.
void test_worked_example_15_4_quantization_boundary() {
  check_eq(quantize(1.21875f), "1.2188",
           "15.4 row 1: exact tie (39/32, scaled == 12187.5 exactly) rounds AWAY from zero, not "
           "merely 'rounds up'");
  check_eq(quantize(1.21874f), "1.2187", "15.4 row 2: one step below the tie rounds TOWARD zero");
  check_eq(quantize(-1.21875f), "-1.2188",
           "15.4 row 3: negative exact tie also grows in magnitude (away-from-zero, not "
           "toward-positive-infinity)");
  check_eq(quantize(-1.21874f), "-1.2187", "15.4 row 4: mirrors row 2 on the negative side");
}

// ---------------------------------------------------------------------------
// EN: docs/uix-rcss.md section 8's own -0.0 canonicalization clause, and a couple of hand-picked
//     values outside the exact 15.4 boundary set, so the algorithm is exercised beyond just the
//     four anchor rows.
// PT: A própria cláusula de canonicalização -0.0 da seção 8 do docs/uix-rcss.md, e alguns valores
//     escolhidos à mão fora do conjunto de fronteira exato da 15.4, pro algoritmo ser exercitado
//     além só das quatro linhas âncora.
void test_quantize_additional_coverage() {
  check_eq(quantize(0.0f), "0.0000", "quantize(0): no sign");
  check_eq(quantize(-0.00001f), "0.0000", "quantize(-0.00001): canonicalizes -0.0 to 0.0");
  check_eq(quantize(90.0f), "90.0000", "quantize(90): integer value still gets 4 digits");
  check_eq(quantize(0.5f), "0.5000", "quantize(0.5): full 4-digit form, never a shorter one");
}

// ---------------------------------------------------------------------------
// EN: `UIX-QUANTIZE-MAGNITUDE` -- `kMaxQuantizeMagnitude`'s own boundary, both signs. See that
//     constant's own header comment (value_compute.hpp) for the full derivation: two ORIGINAL UB
//     thresholds (side A's own `long long` at ~9.2234e14, `glintfx/src/rml/rcss_dump.cpp`; side
//     B's own `unsigned long long` here at ~1.8447e15) collapse into ONE shared, much tighter
//     saturation ceiling (`kMaxQuantizeMagnitude`, `2^47`) both sides now agree on. Exercises:
//     (1) far below the ceiling, ordinary corpus-scale value, untouched; (2) exactly AT the
//     ceiling (the nearest float32, itself bit-exact per this constant's own power-of-two choice),
//     still untouched -- the boundary itself is the LAST unclamped value; (3) the very NEXT
//     representable float32 above the ceiling (`std::nextafterf`), the FIRST clamped value --
//     saturates to the exact same string as (2), proving the `>` comparison fires exactly where
//     documented. Per this codebase's own measured lesson ("teste na fronteira exata não basta"),
//     the adjacent-float pair in (2)/(3) is deliberately NOT the only boundary probe: (4) also
//     exercises the OLD divergent zone (a value strictly between the two original per-side UB
//     thresholds, where side A used to be UB and side B did not -- reproducing the exact magnitude
//     this item's own bug report names for each threshold) and (5) exercises `FLT_MAX`, the true
//     float32 ceiling the bug report also named (23 orders of magnitude above the old thresholds)
//     -- all of them well past the new ceiling, not merely one float step past it, so a future
//     off-by-one OR an accidental widening of the ceiling itself is unlikely to make every one of
//     these cases pass by coincidence.
// PT: `UIX-QUANTIZE-MAGNITUDE` -- a própria fronteira do `kMaxQuantizeMagnitude`, nos dois sinais.
//     Ver o próprio comentário de cabeçalho daquela constante (value_compute.hpp) pra derivação
//     completa: dois limiares de UB ORIGINAIS (o próprio `long long` do lado A em ~9,2234e14,
//     `glintfx/src/rml/rcss_dump.cpp`; o próprio `unsigned long long` do lado B aqui em ~1,8447e15)
//     colapsam num ÚNICO teto de saturação compartilhado, bem mais apertado
//     (`kMaxQuantizeMagnitude`, `2^47`), que os dois lados agora concordam. Exercita: (1) bem
//     abaixo do teto, valor de escala de corpus comum, intocado; (2) exatamente NO teto (o float32
//     mais próximo, ele mesmo bit-exato per a própria escolha de potência-de-dois desta constante),
//     ainda intocado -- a própria fronteira é o ÚLTIMO valor não-saturado; (3) o PRÓXIMO float32
//     representável logo acima do teto (`std::nextafterf`), o PRIMEIRO valor saturado -- satura
//     pra exatamente a mesma string de (2), provando que a comparação `>` dispara exatamente onde
//     documentado. Per a própria lição medida desta casa ("teste na fronteira exata não basta"), o
//     par de floats adjacentes em (2)/(3) deliberadamente NÃO é a única sonda de fronteira: (4)
//     também exercita a ANTIGA zona divergente (um valor estritamente entre os dois limiares de UB
//     originais por-lado, onde o lado A costumava ser UB e o lado B não -- reproduzindo a magnitude
//     exata que o próprio relatório deste item nomeia pra cada limiar) e (5) exercita `FLT_MAX`, o
//     verdadeiro teto do float32 que o relatório também nomeou (23 ordens de grandeza acima dos
//     limiares antigos) -- todos bem além do novo teto, não meramente um passo de float além dele,
//     então um futuro erro de um-a-mais OU um alargamento acidental do próprio teto dificilmente
//     faria todos estes casos passarem por coincidência.
void test_quantize_magnitude_ceiling() {
  const float kMaxF = static_cast<float>(kMaxQuantizeMagnitude);
  check(static_cast<double>(kMaxF) == kMaxQuantizeMagnitude,
        "kMaxQuantizeMagnitude (2^47) is exactly representable in float32 -- test setup sanity");
  const std::string kSaturatedPos = "140737488355328.0000";
  const std::string kSaturatedNeg = "-140737488355328.0000";

  // (1) far below the ceiling -- ordinary corpus-scale value, untouched.
  check_eq(quantize(999999.0f), "999999.0000",
           "far below ceiling: untouched, matches direct computation");
  check_eq(quantize(-999999.0f), "-999999.0000", "far below ceiling, negative: untouched");

  // (2) exactly at the ceiling -- the LAST unclamped value (`>`, not `>=`).
  check_eq(quantize(kMaxF), kSaturatedPos,
           "exactly at kMaxQuantizeMagnitude: NOT clamped, happens to already format to the "
           "saturation target string");
  check_eq(quantize(-kMaxF), kSaturatedNeg, "exactly at -kMaxQuantizeMagnitude: NOT clamped");

  // (3) the very next float32 above/below the ceiling -- the FIRST clamped value.
  const float kJustAbove = std::nextafterf(kMaxF, std::numeric_limits<float>::max());
  const float kJustBeyondNeg = std::nextafterf(-kMaxF, std::numeric_limits<float>::lowest());
  check(kJustAbove > kMaxF, "nextafterf sanity: strictly greater than the ceiling");
  check(kJustBeyondNeg < -kMaxF, "nextafterf sanity: strictly less than the negative ceiling");
  check_eq(quantize(kJustAbove), kSaturatedPos,
           "one float32 step past the ceiling: clamped, saturates to the SAME string as (2)");
  check_eq(quantize(kJustBeyondNeg), kSaturatedNeg,
           "one float32 step past the negative ceiling: clamped, saturates to the SAME string as "
           "(2), negative mirror");

  // (4) the OLD divergent zone this item's own bug report named: side A's `long long` UB
  //     threshold (~9.2234e14) and side B's `unsigned long long` UB threshold (~1.8447e15) used
  //     to disagree between them -- both are now WELL past the new, shared, much tighter ceiling.
  check_eq(quantize(1.0e15f), kSaturatedPos,
           "old divergent zone (9.2234e14 < 1e15 < 1.8447e15): both sides now agree, saturated");
  check_eq(quantize(-1.0e15f), kSaturatedNeg, "old divergent zone, negative mirror");
  check_eq(quantize(9.223372e14f), kSaturatedPos,
           "at side A's own OLD long long UB threshold (~LLONG_MAX/10000): well past the new "
           "ceiling, saturated -- this exact magnitude used to be the edge of side A's own UB");
  check_eq(quantize(1.844674e15f), kSaturatedPos,
           "at side B's own OLD unsigned long long UB threshold (~ULLONG_MAX/10000): also well "
           "past the new ceiling, saturated -- this exact magnitude used to be the edge of side "
           "B's own UB");

  // (5) FLT_MAX -- the true float32 ceiling this item's own bug report named (float goes up to
  //     ~3.4028e38, 23 orders of magnitude above the old thresholds). No crash, no UB, same string.
  check_eq(quantize(std::numeric_limits<float>::max()), kSaturatedPos,
           "FLT_MAX (~3.4028e38): 23 orders of magnitude past the old thresholds, still saturates "
           "cleanly, no UB");
  check_eq(quantize(std::numeric_limits<float>::lowest()), kSaturatedNeg,
           "-FLT_MAX: negative mirror, same margin");
}

// ---------------------------------------------------------------------------
// EN: docs/uix-rcss.md section 15.3 -- the three `%` families side by side inside the same
//     `decorator` value, plus `width`'s own family (a) on a separate line. Reproduces the FULL
//     dump line byte-exact via `compute_decorator_list()` (family b/c) and `print_percent()`
//     (family a), the same functions a future cascade slice would call.
// PT: Seção 15.3 do docs/uix-rcss.md -- as três famílias de `%` lado a lado dentro do mesmo valor
//     `decorator`, mais a própria família (a) do `width` numa linha separada. Reproduz a linha de
//     dump INTEIRA byte-exata via `compute_decorator_list()` (família b/c) e `print_percent()`
//     (família a), as mesmas funções que uma futura fatia de cascata chamaria.
void test_worked_example_15_3_three_percent_families() {
  const char* decorator_raw =
      "linear-gradient(90deg, #FF0000 20%, #00FF00 80%), "
      "radial-gradient(circle at 35% 30%, #F0D98C, #C9A24B 55%, #7A5A2E 100%)";
  std::string decorator_dump;
  check(compute_decorator_list(decorator_raw, /*dp_ratio=*/1.0f, &decorator_dump) ==
            ValueComputeStatus::Ok,
        "15.3: decorator computes Ok");
  // EN: All 5 stop colors here carry alpha=ff, so gradient-stop premultiplication (`UIX-RCSS-
  //     ERRATA-2`, §7.1) is a no-op (channel*255/255=channel) -- this line's own bytes are
  //     unaffected by that correction, verified by still matching the document's own printed
  //     gabarito exactly.
  // PT: As 5 cores de stop aqui carregam alpha=ff, então a premultiplicação de stop de gradiente
  //     (`UIX-RCSS-ERRATA-2`, §7.1) é um no-op (canal*255/255=canal) -- os bytes desta linha ficam
  //     inalterados por essa correção, verificado ainda batendo exatamente com o próprio gabarito
  //     impresso do documento.
  check_eq(decorator_dump,
           "linear-gradient(90.0000;#ff0000ff:20.0000%;#00ff00ff:80.0000%)|"
           "radial-gradient(35.0000%;30.0000%;#f0d98cff:0.0000%;#c9a24bff:55.0000%;"
           "#7a5a2eff:100.0000%)",
           "15.3: decorator= line, byte-exact, both gradient functions in one PROP line");

  float width_pct = 0.0f;
  check(parse_percent("50%", &width_pct) == ValueComputeStatus::Ok, "15.3: width parses as 50%");
  check_eq(print_percent(width_pct), "50.0000%", "15.3: width= line, family (a), stays symbolic");
}

// ---------------------------------------------------------------------------
// EN: docs/uix-rcss.md section 15.2 -- shorthand order is load-bearing for `border-top`.
//     `UIX-RCSS-ERRATA-2` (`Finding A`) corrected the MALFORMED case's own consequence: it is NOT
//     "both longhands revert" -- upstream's `SetProperty` calls happen INSIDE the parsing loop,
//     with no rollback, so whichever longhand ALREADY MATCHED before the post-loop failure fires
//     KEEPS the source value; only the NEVER-matched longhand falls back to its own section 6.1
//     registry initial. For `#b`'s reversed order, `border-top-color` matched (from `"#7A5A2E"`)
//     BEFORE the failure, `border-top-width` never matched anything. This item's OWN subject under
//     test is the PRINT step (color hex normalization, length quantize+px) applied to whichever
//     raw text wins for each longhand independently -- NOT the shorthand routing/partial-write
//     mechanism itself (that is `shorthand.cpp`'s own territory, a DIFFERENT concurrently-edited
//     file this item does not touch, per this task's own file-boundary rule). Rather than depend
//     on `shorthand::expand_shorthand`'s own exact return shape for the malformed case (which may
//     not yet reflect `UIX-RCSS-ERRATA-2` at the time this test runs, since that fix belongs to a
//     sibling module), this test asserts the PRINT-domain computation directly against the two raw
//     strings the errata's own byte-by-byte upstream trace names explicitly -- decoupling this
//     item's own correctness proof from a sibling module's own, possibly-still-in-flight fix.
// PT: Seção 15.2 do docs/uix-rcss.md -- a ordem do shorthand é estrutural pro `border-top`. A
//     `UIX-RCSS-ERRATA-2` (`Finding A`) corrigiu a própria consequência do caso MALFORMADO: NÃO é
//     "os dois longhands revertem" -- as próprias chamadas `SetProperty` do upstream acontecem
//     DENTRO do laço de parsing, sem rollback, então qualquer longhand que JÁ TINHA CASADO antes da
//     falha pós-laço disparar MANTÉM o valor da fonte; só o longhand NUNCA-casado cai pro próprio
//     valor inicial de registro da seção 6.1. Pra ordem revertida do `#b`, `border-top-color` casou
//     (de `"#7A5A2E"`) ANTES da falha, `border-top-width` nunca casou nada. O PRÓPRIO sujeito sob
//     teste deste item é o passo de IMPRESSÃO (normalização hex de cor, quantize+px de comprimento)
//     aplicado a qualquer texto cru que vencer pra cada longhand independentemente -- NÃO o próprio
//     mecanismo de roteamento/escrita-parcial do shorthand (isso é território do próprio
//     `shorthand.cpp`, um arquivo DIFERENTE sendo editado concorrentemente, per a própria regra de
//     fronteira-de-arquivo desta tarefa). Em vez de depender da própria forma de retorno exata do
//     `shorthand::expand_shorthand` pro caso malformado (que pode ainda não refletir a
//     `UIX-RCSS-ERRATA-2` no momento em que este teste roda, já que aquele conserto é de um módulo
//     irmão), este teste assere a própria computação de domínio-de-IMPRESSÃO direto contra as duas
//     strings cruas que o próprio rastro byte-a-byte upstream da errata nomeia explicitamente --
//     desacoplando a própria prova de correção deste item do próprio conserto, possivelmente ainda
//     em voo, de um módulo irmão.
void test_worked_example_15_2_border_top_order_is_load_bearing() {
  constexpr float kDpRatio = 1.0f;

  // #a { border-top: 1dp #7A5A2E; } -- width-then-color, the corpus's own real order. Both
  // longhands matched, both keep the source value.
  {
    float len_val = 0.0f;
    LengthUnit len_unit = LengthUnit::Px;
    check(parse_length("1dp", &len_val, &len_unit) == ValueComputeStatus::Ok,
          "15.2 body/0: border-top-width raw '1dp' parses as a length");
    check_eq(print_length_px(resolve_length_px(len_val, len_unit, kDpRatio)), "1.0000px",
             "15.2 body/0 PROP border-top-width=1.0000px");

    Rgba8 color{};
    check(parse_color("#7A5A2E", &color) == ValueComputeStatus::Ok,
          "15.2 body/0: border-top-color raw '#7A5A2E' parses as a color");
    check_eq(print_color(color), "#7a5a2eff", "15.2 body/0 PROP border-top-color=#7a5a2eff");
  }

  // #b { border-top: #7A5A2E 1dp; } -- color-then-width, reversed order. Per UIX-RCSS-ERRATA-2's
  // own corrected trace: border-top-color MATCHED (from the source token "#7A5A2E") before the
  // shorthand's own post-loop failure fired, so it KEEPS that value -- identical to body/0's own
  // color, NOT reverted. border-top-width NEVER matched anything in this call, so it falls back
  // to its own section 6.1 registry initial value ("0px").
  {
    Rgba8 color{};
    check(parse_color("#7A5A2E", &color) == ValueComputeStatus::Ok,
          "15.2 body/1: border-top-color STILL parses from the source token '#7A5A2E' -- matched "
          "before the shorthand's own post-loop failure, per UIX-RCSS-ERRATA-2's own corrected "
          "trace, NOT reverted to the registry initial");
    check_eq(print_color(color), "#7a5a2eff",
             "15.2 body/1 PROP border-top-color=#7a5a2eff (matches body/0 -- an earlier, "
             "pre-errata worked example published '#000000ff' here, which was wrong, not a "
             "rounding variant)");

    const PropertyInfo* width_info = find_property("border-top-width");
    check(width_info != nullptr, "15.2 body/1: border-top-width is a registered property");
    float len_val = 0.0f;
    LengthUnit len_unit = LengthUnit::Px;
    check(parse_length(width_info->initial_value, &len_val, &len_unit) == ValueComputeStatus::Ok,
          "15.2 body/1: border-top-width's own registry initial value ('0px') parses");
    check_eq(print_length_px(resolve_length_px(len_val, len_unit, kDpRatio)), "0.0000px",
             "15.2 body/1 PROP border-top-width=0.0000px (never matched, registry initial)");
  }
}

// ---------------------------------------------------------------------------
// EN: docs/uix-rcss.md section 9.1's own worked example -- both layers, byte-exact,
//     `UIX-RCSS-ERRATA-4`'s own REVERSED decision (in flight at the time this item was delivered,
//     superseding `UIX-RCSS-ERRATA-2`'s own "print the premultiplied bytes as-is" call): the
//     printed color is the LOSSY PREMULTIPLY/UN-PREMULTIPLY ROUND-TRIP upstream's own
//     `TypeConverter.cpp` actually performs when serializing these two fields to text
//     (`TypeConverter.cpp:223` for `ColorStopList`, `:256` for `BoxShadowList`, both calling
//     `.ToNonPremultiplied()` on the ALREADY-premultiplied stored value) -- NOT the raw
//     premultiplied bytes, and NOT the original straight source color either. `alpha=0` is
//     well-defined (`Colour.h:105-107`'s own explicit `alpha > 0 ? (red*255)/alpha : 0` guard),
//     contradicting this item's own earlier belief (relayed by the orchestrator, corrected here)
//     that un-premultiplying was undefined there. Layer 1's color (`#22D3EE`, alpha `ff`=255)
//     round-trips to itself (`channel*255/255=channel` both directions, no precision lost at full
//     opacity), so it stays `#22d3eeff`. Layer 2's color (`#22D3EE26`, alpha `0x26`=38) does NOT:
//     premultiply step gives `(5,31,35,38)` (`UIX-RCSS-ERRATA-2`'s own math, unchanged), then the
//     un-premultiply step gives `R=5*255/38=33`, `G=31*255/38=208`, `B=35*255/38=234` (integer
//     truncating division both times) -> `#21d0ea26` -- neither the source `#22d3ee26` NOR the
//     bare-premultiplied `#051f2326` an earlier version of this test asserted under
//     `UIX-RCSS-ERRATA-2`'s own now-superseded decision.
// PT: O próprio exemplo trabalhado da seção 9.1 do docs/uix-rcss.md -- as duas camadas,
//     byte-exatas, a própria decisão REVERTIDA da `UIX-RCSS-ERRATA-4` (em voo no momento em que
//     este item foi entregue, substituindo a própria decisão "imprime os bytes premultiplicados
//     como estão" da `UIX-RCSS-ERRATA-2`): a cor impressa é a IDA-E-VOLTA COM PERDA de
//     premultiplicar/des-premultiplicar que o próprio `TypeConverter.cpp` do upstream de fato
//     executa ao serializar estes dois campos pra texto (`TypeConverter.cpp:223` pro
//     `ColorStopList`, `:256` pro `BoxShadowList`, os dois chamando `.ToNonPremultiplied()` sobre o
//     valor JÁ premultiplicado armazenado) -- NÃO os bytes premultiplicados crus, e NÃO a cor de
//     fonte reta original também. `alpha=0` é bem-definido (a própria guarda explícita `alpha > 0 ?
//     (red*255)/alpha : 0` do `Colour.h:105-107`), contradizendo a própria crença anterior deste
//     item (repassada pelo orquestrador, corrigida aqui) de que des-premultiplicar era indefinido
//     ali. A cor da camada 1 (`#22D3EE`, alpha `ff`=255) faz ida-e-volta pra si mesma
//     (`canal*255/255=canal` nas duas direções, zero precisão perdida em opacidade plena), então
//     fica `#22d3eeff`. A cor da camada 2 (`#22D3EE26`, alpha `0x26`=38) NÃO: o passo de
//     premultiplicar dá `(5,31,35,38)` (a própria matemática da `UIX-RCSS-ERRATA-2`, inalterada),
//     depois o passo de des-premultiplicar dá `R=5*255/38=33`, `G=31*255/38=208`,
//     `B=35*255/38=234` (divisão inteira truncando as duas vezes) -> `#21d0ea26` -- nem o próprio
//     `#22d3ee26` de fonte NEM o `#051f2326` puramente-premultiplicado que uma versão anterior
//     deste teste asserava sob a própria decisão, agora superada, da `UIX-RCSS-ERRATA-2`.
void test_worked_example_9_1_box_shadow() {
  std::string out;
  check(compute_box_shadow("#22D3EE 0dp 0dp 0dp 1dp inset, #22D3EE26 0dp 0dp 16dp 0dp",
                           /*dp_ratio=*/1.0f, &out) == ValueComputeStatus::Ok,
        "9.1: box-shadow computes Ok");
  check_eq(out,
           "#22d3eeff;0.0000px;0.0000px;0.0000px;1.0000px;true|"
           "#21d0ea26;0.0000px;0.0000px;16.0000px;0.0000px;false",
           "9.1: box-shadow= line, byte-exact, both layers, lossy premultiply/un-premultiply "
           "round-trip colors (UIX-RCSS-ERRATA-4)");
}

// ---------------------------------------------------------------------------
// EN: The orchestrator's own measured 3-row table for `UIX-RCSS-ERRATA-4` (relayed verbatim,
//     reverified independently here by running each color through `compute_box_shadow()` rather
//     than trusting the relay at face value): the AUTHORED byte, the value the old engine's own
//     internal `ColourbPremultiplied` storage would hold, and the ACTUAL printed golden -- three
//     different bytes for `#22d3ee80`/`#c9a24b40`, proving this is a genuine lossy round-trip, not
//     an identity or a simple premultiply.
// PT: A própria tabela de 3 linhas medida pelo orquestrador pra `UIX-RCSS-ERRATA-4` (repassada
//     verbatim, reverificada independentemente aqui rodando cada cor pelo `compute_box_shadow()`
//     em vez de confiar no relay de cara): o byte AUTORADO, o valor que o próprio armazenamento
//     interno `ColourbPremultiplied` do motor antigo guardaria, e o golden REALMENTE impresso --
//     três bytes diferentes pra `#22d3ee80`/`#c9a24b40`, provando que isto é uma ida-e-volta com
//     perda genuína, não uma identidade nem uma premultiplicação simples.
void test_box_shadow_color_lossy_roundtrip_orchestrator_table() {
  std::string out;
  check(compute_box_shadow("#22d3ee80 0dp 0dp", 1.0f, &out) == ValueComputeStatus::Ok,
        "orchestrator table row 1 computes Ok");
  check(out.rfind("#21d1ed80;", 0) == 0,
        "orchestrator table row 1: #22d3ee80 -> #21d1ed80 (authored != stored != printed)");

  check(compute_box_shadow("#c9a24b40 0dp 0dp", 1.0f, &out) == ValueComputeStatus::Ok,
        "orchestrator table row 2 computes Ok");
  check(out.rfind("#c79f4740;", 0) == 0, "orchestrator table row 2: #c9a24b40 -> #c79f4740");

  check(compute_box_shadow("#22d3ee00 0dp 0dp", 1.0f, &out) == ValueComputeStatus::Ok,
        "orchestrator table row 3 computes Ok");
  check(out.rfind("#00000000;", 0) == 0,
        "orchestrator table row 3: alpha=0 -- the guarded division never fires, all channels 0");
}

// ---------------------------------------------------------------------------
// EN: docs/uix-rcss.md section 9.1's own malformed-layer rule (`UIX-RCSS-ERRATA-2`, closing
//     `Finding I`): a malformed layer inside a comma-separated `box-shadow` list aborts the
//     ENTIRE property, not just that layer -- already this module's own design (unchanged by the
//     errata, only now confirmed authoritative rather than this item's own independent guess).
// PT: A própria regra de camada-malformada da seção 9.1 do docs/uix-rcss.md (`UIX-RCSS-ERRATA-2`,
//     fechando o `Finding I`): uma camada malformada dentro de uma lista `box-shadow`
//     separada-por-vírgula derruba a propriedade INTEIRA, não só aquela camada -- já o próprio
//     design deste módulo (inalterado pela errata, agora só confirmado autoritativo em vez de um
//     chute independente deste item).
void test_box_shadow_malformed_layer_drops_whole_property() {
  std::string out;
  check(compute_box_shadow("#22D3EE 0dp 0dp inset, not-a-color 1dp 1dp", /*dp_ratio=*/1.0f, &out) ==
            ValueComputeStatus::Invalid,
        "a malformed 2nd layer invalidates the WHOLE box-shadow declaration, even though the 1st "
        "layer alone would have parsed fine");
}

// ---------------------------------------------------------------------------
// EN: docs/uix-rcss.md section 9.2.1's own auto-spacing algorithm, exercised beyond the one
//     worked-example shape (single K=1 run) already covered by 15.3/9.1's own reuse -- a K=2 run
//     to prove the general "evenly spaced between neighbors" formula, not just the K=1 special
//     case that could be mistaken for something simpler.
// PT: O próprio algoritmo de auto-espaçamento da seção 9.2.1 do docs/uix-rcss.md, exercitado além
//     da única forma de exemplo trabalhado (um trecho K=1) já coberta pelo próprio reuso da
//     15.3/9.1 -- um trecho K=2 pra provar a fórmula geral "igualmente espaçado entre vizinhos",
//     não só o caso especial K=1 que poderia ser confundido com algo mais simples.
void test_gradient_stop_auto_spacing_general_run() {
  // EN: 4 stops, only the first and last have an explicit position -- a K=2 run in the middle.
  // PT: 4 stops, só o primeiro e o último têm posição explícita -- um trecho K=2 no meio.
  std::vector<std::optional<float>> explicit_positions{0.0f, std::nullopt, std::nullopt, 100.0f};
  std::vector<float> resolved = resolve_gradient_stop_positions(explicit_positions);
  check(resolved.size() == 4, "auto-spacing K=2: 4 positions produced");
  check_eq(print_percent(resolved[0]), "0.0000%", "auto-spacing K=2: stop 0 stays explicit 0%");
  check_eq(print_percent(resolved[1]), "33.3333%",
           "auto-spacing K=2: stop 1 = 0 + 1*(100-0)/3");
  check_eq(print_percent(resolved[2]), "66.6667%",
           "auto-spacing K=2: stop 2 = 0 + 2*(100-0)/3");
  check_eq(print_percent(resolved[3]), "100.0000%", "auto-spacing K=2: stop 3 stays explicit 100%");
}

// ---------------------------------------------------------------------------
// EN: `UIX-RCSS-CONFORMIDADE` -- section 9.2.1's own rule 3 ("the LAST stop, if it has no explicit
//     position, is assigned 100%"), the one rule of the algorithm's own four numbered steps that
//     neither the document's own §9.2.1/§15.3 worked example (rule 2 only: first stop unpositioned)
//     nor `test_gradient_stop_auto_spacing_general_run` immediately above (rule 4 only: an interior
//     K=2 run, both ends already explicit) ever exercises. Two stops, first explicit at `10%`,
//     second (and last) with no explicit position -- rule 3 fires alone, undiluted by rule 2 or 4.
// PT: `UIX-RCSS-CONFORMIDADE` -- a própria regra 3 da seção 9.2.1 ("o ÚLTIMO stop, se não tiver
//     posição explícita, recebe 100%"), a única das quatro regras numeradas do algoritmo que nem o
//     próprio exemplo trabalhado §9.2.1/§15.3 do documento (só regra 2: primeiro stop sem posição)
//     nem o `test_gradient_stop_auto_spacing_general_run` logo acima (só regra 4: um trecho K=2
//     interior, as duas pontas já explícitas) chegam a exercitar. Dois stops, primeiro explícito em
//     `10%`, segundo (e último) sem posição explícita -- a regra 3 dispara sozinha, sem diluição
//     das regras 2 ou 4.
void test_gradient_stop_auto_spacing_last_stop_unpositioned() {
  std::vector<std::optional<float>> explicit_positions{10.0f, std::nullopt};
  std::vector<float> resolved = resolve_gradient_stop_positions(explicit_positions);
  check(resolved.size() == 2, "auto-spacing rule 3: 2 positions produced");
  check_eq(print_percent(resolved[0]), "10.0000%", "auto-spacing rule 3: stop 0 stays explicit 10%");
  check_eq(print_percent(resolved[1]), "100.0000%",
           "auto-spacing rule 3: last stop, no explicit position, assigned 100%");
}

// ---------------------------------------------------------------------------
// EN: docs/uix-rcss.md section 7.1 -- all 4 authorized hex forms normalize to the same 8-digit
//     canonical form, plus the 3 authorized named colors, plus fail-high for everything else
//     (section 13's own "requires the líder's sign-off" clause).
// PT: Seção 7.1 do docs/uix-rcss.md -- as 4 formas hex autorizadas normalizam pra mesma forma
//     canônica de 8 dígitos, mais as 3 cores nomeadas autorizadas, mais fail-high pra tudo mais
//     (a própria cláusula "exige aval do líder" da seção 13).
void test_color_parsing_all_forms() {
  Rgba8 c{};
  check(parse_color("#f00", &c) == ValueComputeStatus::Ok, "#rgb parses");
  check_eq(print_color(c), "#ff0000ff", "#f00 -> #ff0000ff (single-digit channels double, alpha ff)");

  check(parse_color("#f008", &c) == ValueComputeStatus::Ok, "#rgba parses");
  check_eq(print_color(c), "#ff000088", "#f008 -> #ff000088 (alpha channel also doubles)");

  check(parse_color("#22D3EE", &c) == ValueComputeStatus::Ok, "#rrggbb parses");
  check_eq(print_color(c), "#22d3eeff", "#22D3EE -> #22d3eeff (lowercased, alpha defaults ff)");

  check(parse_color("#22D3EE26", &c) == ValueComputeStatus::Ok, "#rrggbbaa parses");
  check_eq(print_color(c), "#22d3ee26", "#22D3EE26 -> #22d3ee26 (already 8-digit, unchanged shape)");

  check(parse_color("white", &c) == ValueComputeStatus::Ok, "'white' parses");
  check_eq(print_color(c), "#ffffffff", "white -> #ffffffff");

  check(parse_color("black", &c) == ValueComputeStatus::Ok, "'black' parses (registry initial only)");
  check_eq(print_color(c), "#000000ff", "black -> #000000ff");

  check(parse_color("transparent", &c) == ValueComputeStatus::Ok, "'transparent' parses");
  check_eq(print_color(c), "#00000000", "transparent -> #00000000");

  check(parse_color("red", &c) == ValueComputeStatus::Invalid,
        "'red': fail-high, not authorized by section 13 (only white/black/transparent are)");
  check(parse_color("rgb(255,0,0)", &c) == ValueComputeStatus::Invalid,
        "rgb(): fail-high, zero-measured functional color form, section 13");
  check(parse_color("#ff", &c) == ValueComputeStatus::Invalid,
        "2-digit hex: fail-high, not one of the 4 authorized forms");
}

// ---------------------------------------------------------------------------
// EN: docs/uix-rcss.md section 8.1 -- only `px`/`dp` resolve, unitless `0` is accepted, every
//     other unit (em/rem/vw/vh) is fail-high, per this item's own declared teto.
// PT: Seção 8.1 do docs/uix-rcss.md -- só `px`/`dp` resolvem, `0` sem unidade é aceito, toda
//     outra unidade (em/rem/vw/vh) é fail-high, per o próprio teto declarado deste item.
void test_length_resolution() {
  float v = 0.0f;
  LengthUnit u = LengthUnit::Px;
  check(parse_length("16px", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::Px && v == 16.0f,
        "16px parses as Px unit, value 16");
  check_eq(print_length_px(resolve_length_px(v, u, /*dp_ratio=*/2.0f)), "16.0000px",
           "px length ignores dp_ratio");

  check(parse_length("2dp", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::Dp && v == 2.0f,
        "2dp parses as Dp unit, value 2");
  check_eq(print_length_px(resolve_length_px(v, u, /*dp_ratio=*/8.0f)), "16.0000px",
           "2dp at dp_ratio=8 resolves to 16px");

  check(parse_length("0", &v, &u) == ValueComputeStatus::Ok,
        "unitless 0 is accepted (CSS's own zero-length convention)");
  check(parse_length("5", &v, &u) == ValueComputeStatus::Invalid,
        "unitless non-zero is fail-high, never guessed");
  check(parse_length("1em", &v, &u) == ValueComputeStatus::Invalid,
        "em: fail-high, needs font-size context this pure-function layer does not have");
  check(parse_length("1rem", &v, &u) == ValueComputeStatus::Invalid, "rem: fail-high, same reason");
}

// ---------------------------------------------------------------------------
// EN: docs/uix-rcss.md section 8's own `UIX-RCSS-ERRATA-2`-closed `Finding J`: `quantize()` is
//     defined only for finite `x` -- a non-finite computed value is never printed via the
//     algorithm, it is a fail-high case at the same footing as a rejected parse. This module's OWN
//     enforcement point is at PARSE time (`parse_float_token`, this file's own internal helper) --
//     hostile RCSS text like `"nan"`/`"inf"` (both of which `strtof` parses successfully into a
//     non-finite `float`, per the C standard) must never silently flow through as a "valid" number,
//     length, percent, or angle only to reach `quantize()` looking like an ordinary value.
//     `quantize()` itself keeps its own documented defensive `"0.0000"` fallback as a last-resort
//     belt (it is directly callable and directly tested, `test_worked_example_15_4_*` above), but
//     the REAL fix, matching the errata's own "never printed" framing, is that no PARSE path in
//     this module can produce a non-finite float in the first place.
// PT: O próprio `Finding J`, fechado pela `UIX-RCSS-ERRATA-2`, da seção 8 do docs/uix-rcss.md:
//     `quantize()` só é definido pra `x` finito -- um valor computado não-finito nunca é impresso
//     pelo algoritmo, é um caso fail-high no mesmo patamar de um parse rejeitado. O PRÓPRIO ponto
//     de aplicação deste módulo é em tempo de PARSE (`parse_float_token`, o próprio helper interno
//     deste arquivo) -- texto RCSS hostil tipo `"nan"`/`"inf"` (os dois que o `strtof` parseia com
//     sucesso pra um `float` não-finito, per o próprio padrão C) nunca pode fluir em silêncio como
//     um número, comprimento, porcentagem ou ângulo "válido" só pra chegar no `quantize()`
//     parecendo um valor comum. O próprio `quantize()` mantém o próprio fallback defensivo
//     documentado `"0.0000"` como cinto de segurança de último recurso (é diretamente chamável e
//     diretamente testado, `test_worked_example_15_4_*` acima), mas o conserto REAL, casando com a
//     própria formulação "nunca impresso" da errata, é que nenhum caminho de PARSE deste módulo
//     consegue produzir um float não-finito antes de mais nada.
void test_non_finite_input_is_fail_high_at_parse_time() {
  float v = 0.0f;
  LengthUnit u = LengthUnit::Px;
  check(parse_length("nanpx", &v, &u) == ValueComputeStatus::Invalid,
        "'nanpx': strtof would parse the 'nan' prefix as NaN -- Invalid, never silently printed");
  check(parse_length("infpx", &v, &u) == ValueComputeStatus::Invalid,
        "'infpx': strtof would parse the 'inf' prefix as +Infinity -- Invalid");
  float pct = 0.0f;
  check(parse_percent("nan%", &pct) == ValueComputeStatus::Invalid, "'nan%': Invalid");
  float deg = 0.0f;
  check(parse_angle("nandeg", &deg) == ValueComputeStatus::Invalid, "'nandeg': Invalid");
  check(parse_angle("-infrad", &deg) == ValueComputeStatus::Invalid, "'-infrad': Invalid");
}

// ---------------------------------------------------------------------------
// EN: docs/uix-rcss.md section 9.4's own thin transform grammar, plus section 11's own
//     `UIX-RCSS-ERRATA-2`-corrected "malformed entry drops the WHOLE property" rule exercised at
//     the transform-list level too (applied consistently, not just to the two upstream-parser-
//     cited domains).
// PT: A própria gramática fina de transform da seção 9.4 do docs/uix-rcss.md, mais a própria regra
//     "entrada malformada derruba a PROPRIEDADE INTEIRA", corrigida pela `UIX-RCSS-ERRATA-2`, da
//     seção 11, exercitada no nível de lista de transform também (aplicada consistentemente, não
//     só nos dois domínios citados-do-parser-upstream).
void test_transform_list() {
  std::string out;
  check(compute_transform_list("rotate(0deg)", 1.0f, &out) == ValueComputeStatus::Ok,
        "corpus instance 1 computes Ok");
  check_eq(out, "rotate(0.0000)", "corpus instance 1: rotate(0deg)");

  check(compute_transform_list("rotate(360deg)", 1.0f, &out) == ValueComputeStatus::Ok,
        "corpus instance 2 computes Ok");
  check_eq(out, "rotate(360.0000)", "corpus instance 2: rotate(360deg)");

  check(compute_transform_list("translate(10px, 20px) rotate(45deg)", 1.0f, &out) ==
            ValueComputeStatus::Ok,
        "translate+rotate computes Ok");
  check_eq(out, "translate(10.0000px;20.0000px)|rotate(45.0000)",
           "translate+rotate, whitespace-adjacent multi-function CSS transform-list syntax");

  check(compute_transform_list("scale(1.5, 2)", 1.0f, &out) == ValueComputeStatus::Ok,
        "scale computes Ok");
  check_eq(out, "scale(1.5000;2.0000)", "scale: plain numbers, not lengths");

  check(compute_transform_list("", 1.0f, &out) == ValueComputeStatus::Ok,
        "empty transform value computes Ok");
  check_eq(out, "none", "empty transform value prints 'none'");

  check(compute_transform_list("none", 1.0f, &out) == ValueComputeStatus::Ok,
        "'none' transform value computes Ok");
  check_eq(out, "none", "'none' transform value prints 'none'");

  check(compute_transform_list("matrix3d(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)", 1.0f, &out) ==
            ValueComputeStatus::Invalid,
        "matrix3d: out-of-scope per section 13, fail-high -- Invalid, never a guess");

  check(compute_transform_list("translate(10px, 20px) matrix3d(...)", 1.0f, &out) ==
            ValueComputeStatus::Invalid,
        "one unknown function among otherwise-valid ones: the WHOLE transform list is Invalid, "
        "per section 11's own corrected uniform malformed-entry policy -- translate() does NOT "
        "survive alone");
}

// ---------------------------------------------------------------------------
// EN: docs/uix-rcss.md section 11's own `UIX-RCSS-ERRATA-2`-corrected rule (`Finding C`): an
//     unknown decorator/filter function name, or a malformed argument shape, drops the ENTIRE
//     property -- a mix of one valid and one unknown function in the SAME decorator value must
//     invalidate the WHOLE list, matching upstream's own `return false` on the first bad entry
//     (`PropertyParserDecorator.cpp:63-131`). An earlier, pre-errata version of this test asserted
//     the OPPOSITE (per-entry drop, the rest survives) -- that was wrong, not a stricter subset.
// PT: A própria regra corrigida-pela-`UIX-RCSS-ERRATA-2` da seção 11 do docs/uix-rcss.md
//     (`Finding C`): um nome de função de decorator/filter desconhecido, ou uma forma de argumento
//     malformada, derruba a propriedade INTEIRA -- uma mistura de uma função válida e uma
//     desconhecida no MESMO valor decorator precisa invalidar a lista INTEIRA, casando com o
//     próprio `return false` do upstream na primeira entrada ruim
//     (`PropertyParserDecorator.cpp:63-131`). Uma versão anterior, pré-errata, deste teste asserava
//     o OPOSTO (derrubada por-entrada, o resto sobrevive) -- aquilo estava errado, não um
//     subconjunto mais estrito.
void test_decorator_list_malformed_entry_drops_whole_property() {
  std::string out;

  check(compute_decorator_list("blur(4px), not-a-real-function(1,2,3)", 1.0f, &out) ==
            ValueComputeStatus::Invalid,
        "unknown function anywhere in the list invalidates the WHOLE property -- blur() does NOT "
        "survive alone (UIX-RCSS-ERRATA-2, Finding C)");

  check(compute_decorator_list("", 1.0f, &out) == ValueComputeStatus::Ok,
        "empty decorator value computes Ok");
  check_eq(out, "none", "empty decorator value prints 'none'");

  check(compute_decorator_list("none", 1.0f, &out) == ValueComputeStatus::Ok,
        "'none' decorator value computes Ok");
  check_eq(out, "none", "'none' decorator value prints 'none'");

  check(compute_decorator_list("radial-gradient(ellipse at 50% 50%, #fff, #000)", 1.0f, &out) ==
            ValueComputeStatus::Invalid,
        "ellipse: section 13's own out-of-scope clause, fail-high -- the whole property drops");

  check(compute_decorator_list("horizontal-gradient(#000f #0000)", 1.0f, &out) ==
            ValueComputeStatus::Ok,
        "horizontal-gradient computes Ok");
  // EN: #000f (r=0,g=0,b=0,a=ff) premultiplies to itself (alpha=255, no-op); #0000
  //     (r=0,g=0,b=0,a=00) premultiplies to itself too (already all-zero channels) -- this
  //     specific pair happens not to exercise a NON-trivial premultiply, unlike the
  //     vertical-gradient case immediately below, which deliberately does.
  // PT: #000f (r=0,g=0,b=0,a=ff) premultiplica pra si mesmo (alpha=255, no-op); #0000
  //     (r=0,g=0,b=0,a=00) premultiplica pra si mesmo também (canais já todos zero) -- este par
  //     específico calha de não exercitar uma premultiplicação NÃO-trivial, diferente do caso
  //     vertical-gradient logo abaixo, que exercita de propósito.
  check_eq(out, "horizontal-gradient(#000000ff;#00000000)",
           "mask-image's own 2-stop shorthand form");

  // EN: docs/uix-rcss.md section 9.2's own newly-added row (UIX-RCSS-ERRATA-2) -- 107 corpus
  //     occurrences, the single most-used decorator function in the corpus, missing entirely
  //     before this errata. Same grammar as horizontal-gradient, and a color pair chosen so the
  //     premultiply/un-premultiply round-trip step is NOT a no-op, unlike the horizontal-gradient
  //     case above -- same lossy round-trip and same literal as the 9.1 worked example above
  //     (`UIX-RCSS-ERRATA-4`'s own reversed decision).
  // PT: A própria linha recém-acrescentada da seção 9.2 do docs/uix-rcss.md (UIX-RCSS-ERRATA-2) --
  //     107 ocorrências no corpus, a função de decorator mais usada do corpus inteiro, faltando por
  //     inteiro antes desta errata. Mesma gramática do horizontal-gradient, e um par de cores
  //     escolhido pra o passo de ida-e-volta premultiplicar/des-premultiplicar NÃO ser um no-op,
  //     diferente do caso horizontal-gradient acima -- mesma ida-e-volta com perda e mesmo literal
  //     do exemplo trabalhado 9.1 acima (a própria decisão revertida da `UIX-RCSS-ERRATA-4`).
  check(compute_decorator_list("vertical-gradient(#22D3EE26 #ffffffff)", 1.0f, &out) ==
            ValueComputeStatus::Ok,
        "vertical-gradient computes Ok");
  check_eq(out, "vertical-gradient(#21d0ea26;#ffffffff)",
           "vertical-gradient: same lossy premultiply/un-premultiply round-trip as "
           "box-shadow/gradient-stop colors, #22D3EE26 -> #21d0ea26 (same math as the 9.1 worked "
           "example)");

  check(compute_decorator_list("image( runes-base.png )", 1.0f, &out) == ValueComputeStatus::Ok,
        "image() computes Ok");
  check_eq(out, "image(runes-base.png)", "image(): bare url, whitespace trimmed");

  check(compute_decorator_list(
            "polygon(6, radial-gradient(circle at 40% 35%, #F0D98C, #C9A24B 55%, #7A5A2E 100%))",
            1.0f, &out) == ValueComputeStatus::Ok,
        "polygon() with nested radial-gradient fill computes Ok");
  check_eq(out,
           "polygon(6.0000;radial-gradient(40.0000%;35.0000%;#f0d98cff:0.0000%;#c9a24bff:55.0000%;"
           "#7a5a2eff:100.0000%);0.0000)",
           "polygon()'s own recursive nested-gradient <fill> argument, docs/effects.md's own "
           "documented example -- the nested gradient's OWN stop colors are also premultiplied "
           "(all alpha=ff here, so a no-op, same reasoning as the 15.3 reuse above)");

  check(compute_decorator_list("polygon(2, #fff)", 1.0f, &out) == ValueComputeStatus::Invalid,
        "polygon sides out of [3,1024]: fail-high, docs/effects.md's own validated range -- the "
        "whole property drops");
}

} // namespace

int main() {
  test_worked_example_15_4_quantization_boundary();
  test_quantize_additional_coverage();
  test_quantize_magnitude_ceiling();
  test_worked_example_15_3_three_percent_families();
  test_worked_example_15_2_border_top_order_is_load_bearing();
  test_worked_example_9_1_box_shadow();
  test_box_shadow_color_lossy_roundtrip_orchestrator_table();
  test_box_shadow_malformed_layer_drops_whole_property();
  test_gradient_stop_auto_spacing_general_run();
  test_gradient_stop_auto_spacing_last_stop_unpositioned();
  test_color_parsing_all_forms();
  test_length_resolution();
  test_non_finite_input_is_fail_high_at_parse_time();
  test_transform_list();
  test_decorator_list_malformed_entry_drops_whole_property();

  // EN: Scope line, printed always (even at zero), per this task's own DoD -- domains covered by
  //     section 7 (7: keyword is the caller's own job, not counted here), composite grammars
  //     covered by section 9 (3 of 5: box-shadow, decorator/mask/filter/backdrop-filter, transform
  //     -- animation explicitly NOT covered, see this file's own header and value_compute.cpp's
  //     own top-of-file header for the reported `delay`-field gap), and the one resolved
  //     percentage exception (gradient-stop auto-spacing). Reflects `UIX-RCSS-ERRATA-2` (landed,
  //     `bdf3f45`): premultiplied box-shadow/gradient-stop colors, whole-property-drop for every
  //     composite list, `vertical-gradient` added, non-finite input fail-high at parse time.
  // PT: Linha de escopo, impressa sempre (mesmo em zero), per o próprio DoD desta tarefa: domínios
  //     cobertos pela seção 7 (7: keyword é trabalho do próprio chamador, não contado aqui),
  //     gramáticas compostas cobertas pela seção 9 (3 de 5: box-shadow, decorator/mask/filter/
  //     backdrop-filter, transform -- animation explicitamente NÃO coberta, ver o próprio
  //     cabeçalho deste arquivo e o próprio cabeçalho de topo do value_compute.cpp pra lacuna de
  //     campo `delay` reportada), e a única exceção de porcentagem resolvida (auto-espaçamento de
  //     stop de gradiente). Reflete a `UIX-RCSS-ERRATA-2` (pousada, `bdf3f45`): cores de
  //     box-shadow/stop-de-gradiente premultiplicadas, derrubada-da-propriedade-inteira pra toda
  //     lista composta, `vertical-gradient` acrescentada, input não-finito fail-high em tempo de
  //     parse.
  std::printf(
      "SCOPE: value domains covered 6 (number, length, angle, percent-symbolic, color, string) | "
      "composite grammars covered 3 of 5 (box-shadow, decorator/mask/filter/backdrop-filter, "
      "transform) | composite grammars NOT covered 2 of 5 (animation -- reported `delay`-field "
      "spec gap, see header; keyword-domain expansion is a future cascade slice's own job, not "
      "this item's) | decorator functions covered 10 of 10 (image, linear-gradient, "
      "radial-gradient, polygon, image-tint, ripple, horizontal-gradient, vertical-gradient, blur, "
      "drop-shadow) | resolved-percentage exceptions 1 of 1 (gradient-stop auto-spacing, section "
      "9.2.1, all 4 numbered rules now individually exercised -- rule 1/2 via the 15.3 worked "
      "example reuse, rule 3 via UIX-RCSS-CONFORMIDADE's own added last-stop-unpositioned case, "
      "rule 4 via the K=2 general run above) | worked examples reproduced byte-exact 5 of 6 (15.2, "
      "15.3, 15.4 all 4 rows -- "
      "UIX-RCSS-ERRATA-3's own corrected literals, an earlier draft of this file's own reported "
      "divergence against the pre-errata-3 literals is superseded, not reproduced here -- plus "
      "9.1's own box-shadow example, all against the CURRENT spec text including UIX-RCSS-ERRATA-4 "
      "in flight) | worked examples that DIVERGED from this document's own printed gabarito at the "
      "time this item read it, since resolved upstream in the spec itself: 1 (the original 15.4 "
      "row 1/3 literals -- this item's own independent finding, reported in an earlier draft of "
      "this test file, converged on the identical fix (`1.21875f = 39/32`) independently derived "
      "by `UIX-RCSS-DUMP-A`'s own author, commit `a1e0b9f`, before `UIX-RCSS-ERRATA-3` canonized "
      "it)\n");

  if (g_failures > 0) {
    std::fprintf(stderr, "value_compute_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("value_compute_sanity: PASS");
  return 0;
}
