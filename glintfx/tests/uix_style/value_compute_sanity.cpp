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
  check(compute_decorator_list(decorator_raw, LengthResolveContext{.dp_ratio = 1.0f}, &decorator_dump) ==
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
    check_eq(print_length_px(resolve_length_px(len_val, len_unit, LengthResolveContext{.dp_ratio = kDpRatio})), "1.0000px",
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
    check_eq(print_length_px(resolve_length_px(len_val, len_unit, LengthResolveContext{.dp_ratio = kDpRatio})), "0.0000px",
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
                           LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
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
  check(compute_box_shadow("#22d3ee80 0dp 0dp", LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
        "orchestrator table row 1 computes Ok");
  check(out.rfind("#21d1ed80;", 0) == 0,
        "orchestrator table row 1: #22d3ee80 -> #21d1ed80 (authored != stored != printed)");

  check(compute_box_shadow("#c9a24b40 0dp 0dp", LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
        "orchestrator table row 2 computes Ok");
  check(out.rfind("#c79f4740;", 0) == 0, "orchestrator table row 2: #c9a24b40 -> #c79f4740");

  check(compute_box_shadow("#22d3ee00 0dp 0dp", LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
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
  check(compute_box_shadow("#22D3EE 0dp 0dp inset, not-a-color 1dp 1dp",
                           LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
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
// EN: `UIX-GRADIENT-ALFA` -- fixes the residuo C of `UIX-ORACLE-MEDICAO`:
//     `vertical-gradient()`/`horizontal-gradient()` with an 8-digit hex stop at low alpha
//     corrupted the RGB while preserving the alpha byte exactly, measured against
//     `system_menu__config_controles_tabela.rml:469` (`decorator: vertical-gradient( #C9A24B24
//     #C9A24B0a );`) -- side A (real RmlUi) prints `#c9a24b24;#c9a24b0a` (STRAIGHT, unchanged
//     except lowercasing), this module printed `#c69b4624;#b299330a` before this item.
//
//     EXHAUSTIVE SWEEP, DECLARED DENOMINATOR: `dump_box_shadow_or_gradient_stop_color()` (this
//     file's own lossy premultiply-then-un-premultiply round-trip, `UIX-RCSS-ERRATA-4`) has
//     exactly **3** call sites in this file (`grep`-confirmed, not assumed) -- `N=3`, `M=3`
//     examined, `K=1` affected: `compute_box_shadow()` (1 call), `parse_gradient_stop()` (1 call,
//     shared by `linear-gradient`/`radial-gradient` -- both route every stop through this SAME
//     function via `parse_and_space_stops()`), and `compute_two_stop_straight_gradient()` (2
//     calls, `c0`/`c1`, shared by `horizontal-gradient`/`vertical-gradient` -- both dispatch to
//     this SAME function by name in `compute_one_decorator_function()`). 3 call sites, 5
//     RCSS-facing property/decorator paths (`box-shadow` + the 4 gradient functions) between them.
//
//     ROOT CAUSE, CONFIRMED BY READING THE REAL UPSTREAM SOURCE DIRECTLY (this task's own "onde a
//     spec e o código real do RmlUi divergirem, o código manda" clause) -- NOT the mechanism this
//     item's own brief hypothesized (a stray un-premultiply step left over from the
//     `UIX-RCSS-ERRATA-2`->`ERRATA-4` reversal): the round-trip arithmetic ITSELF is correct
//     (proven by `test_box_shadow_color_lossy_roundtrip_orchestrator_table` above, unchanged,
//     still green) -- the bug is that `compute_two_stop_straight_gradient()` applies it to a
//     decorator type that was NEVER premultiplied by upstream in the first place.
//     `examples/RmlUi/Source/Core/DecoratorGradient.h` declares `Colourb start, stop;` for
//     `DecoratorStraightGradient` (line ~34) -- a PLAIN, non-premultiplied `Colourb`, structurally
//     DIFFERENT from `BoxShadow`'s/`ColorStop`'s own `ColourbPremultiplied color;`
//     (`DecorationTypes.h:9`/`:22`, the fact `UIX-RCSS-ERRATA-4`'s own text correctly cites for
//     THOSE two). `DecoratorStraightGradientInstancer::InstanceDecorator`
//     (`DecoratorGradient.cpp:196-219`, read directly) fetches
//     `properties_.GetProperty(ids.start)->Get<Colourb>()` -- **never** `.ToPremultiplied()`
//     anywhere in this instancer, a COMPLETELY DIFFERENT code path from
//     `PropertyParserColorStopList.cpp:47`/`PropertyParserBoxShadow.cpp:72`'s own
//     `.ToPremultiplied()` calls. `docs/uix-rcss.md`'s own `UIX-RCSS-ERRATA-4` text asserts
//     `horizontal-gradient`/`vertical-gradient` "reuse upstream's own `PropertyParserColorStopList`
//     ... the SAME parser every `linear-gradient`/`radial-gradient` stop goes through" -- this
//     claim is FALSE for these two specifically, per the direct reading above; this file's own
//     (now-corrected) comment at `compute_two_stop_straight_gradient()`'s own declaration
//     previously repeated the same false claim, inherited from the same source, not independently
//     re-verified. Reported for routing to the spec's own owner (this item does not edit
//     `docs/uix-rcss.md` itself, per this task's own file-boundary rule) -- not silently patched
//     into the doc by this item alone.
//
//     Side A empirically confirms the same conclusion two different ways: (1) the residuo C
//     measurement itself (straight, unchanged RGB); (2) the SECOND corpus occurrence in the same
//     fixture (`#22D3EE1a #22D3EE0a`, line 472) that `UIX-ORACLE-MEDICAO`'s own report flagged as
//     "not verified, never bound to an element in that oracle run" -- exercised here directly,
//     proving the fix is not a one-RGB-value coincidence.
// PT: `UIX-GRADIENT-ALFA` -- conserta o resíduo C da `UIX-ORACLE-MEDICAO`:
//     `vertical-gradient()`/`horizontal-gradient()` com um stop hex de 8 dígitos em alfa baixo
//     corrompia o RGB preservando o byte de alfa exato, medido contra
//     `system_menu__config_controles_tabela.rml:469` (`decorator: vertical-gradient( #C9A24B24
//     #C9A24B0a );`) -- o lado A (RmlUi real) imprime `#c9a24b24;#c9a24b0a` (RETO, inalterado
//     exceto lowercase), este módulo imprimia `#c69b4624;#b299330a` antes deste item.
//
//     VARREDURA EXAUSTIVA, DENOMINADOR DECLARADO: o `dump_box_shadow_or_gradient_stop_color()`
//     (a própria ida-e-volta com perda de premultiplicar-depois-des-premultiplicar deste arquivo,
//     `UIX-RCSS-ERRATA-4`) tem exatamente **3** call sites neste arquivo (confirmado por `grep`,
//     não suposto) -- `N=3`, `M=3` examinados, `K=1` afetado: `compute_box_shadow()` (1 chamada),
//     `parse_gradient_stop()` (1 chamada, compartilhada por `linear-gradient`/`radial-gradient` --
//     os dois roteiam todo stop por esta MESMA função via `parse_and_space_stops()`), e
//     `compute_two_stop_straight_gradient()` (2 chamadas, `c0`/`c1`, compartilhada por
//     `horizontal-gradient`/`vertical-gradient` -- os dois despacham pra esta MESMA função por nome
//     no `compute_one_decorator_function()`). 3 call sites, 5 caminhos RCSS-facing (propriedade/
//     decorator) entre eles (`box-shadow` mais as 4 funções de gradiente).
//
//     CAUSA RAIZ, CONFIRMADA LENDO O PRÓPRIO FONTE UPSTREAM DIRETO (a própria cláusula "onde a
//     spec e o código real do RmlUi divergirem, o código manda" desta tarefa) -- NÃO o mecanismo
//     que o próprio briefing deste item hipotetizou (um passo de des-premultiplicar sobrando da
//     reversão `UIX-RCSS-ERRATA-2`->`ERRATA-4`): a própria aritmética da ida-e-volta está correta
//     (provado pelo `test_box_shadow_color_lossy_roundtrip_orchestrator_table` acima, inalterado,
//     ainda verde) -- o bug é que `compute_two_stop_straight_gradient()` a aplica a um tipo de
//     decorator que NUNCA foi premultiplicado pelo upstream, de saída.
//     `examples/RmlUi/Source/Core/DecoratorGradient.h` declara `Colourb start, stop;` pro
//     `DecoratorStraightGradient` (linha ~34) -- um `Colourb` PLANO, não-premultiplicado,
//     estruturalmente DIFERENTE do próprio `ColourbPremultiplied color;` do `BoxShadow`/`ColorStop`
//     (`DecorationTypes.h:9`/`:22`, o fato que o próprio texto da `UIX-RCSS-ERRATA-4` cita
//     corretamente pra ESSES dois). O `DecoratorStraightGradientInstancer::InstanceDecorator`
//     (`DecoratorGradient.cpp:196-219`, lido direto) busca
//     `properties_.GetProperty(ids.start)->Get<Colourb>()` -- **nunca** `.ToPremultiplied()` em
//     lugar nenhum deste instancer, um caminho de código COMPLETAMENTE DIFERENTE das próprias
//     chamadas `.ToPremultiplied()` do `PropertyParserColorStopList.cpp:47`/
//     `PropertyParserBoxShadow.cpp:72`. O próprio texto da `UIX-RCSS-ERRATA-4` do
//     `docs/uix-rcss.md` afirma que `horizontal-gradient`/`vertical-gradient` "reusam o próprio
//     `PropertyParserColorStopList` do upstream... o MESMO parser que todo stop de
//     `linear-gradient`/`radial-gradient` atravessa" -- esta alegação é FALSA especificamente pra
//     estes dois, per a leitura direta acima; o próprio comentário deste arquivo na declaração do
//     `compute_two_stop_straight_gradient()` (agora corrigido) repetia a mesma alegação falsa,
//     herdada da mesma fonte, não re-verificada independentemente. Reportado pro roteamento ao
//     próprio dono da spec (este item não edita o `docs/uix-rcss.md` sozinho, per a própria regra
//     de fronteira-de-arquivo desta tarefa) -- não remendado em silêncio na doc por este item
//     sozinho.
//
//     O lado A confirma empiricamente a mesma conclusão de duas formas diferentes: (1) a própria
//     medição do resíduo C (RGB reto, inalterado); (2) a SEGUNDA ocorrência do corpus na mesma
//     fixture (`#22D3EE1a #22D3EE0a`, linha 472) que o próprio relatório da `UIX-ORACLE-MEDICAO`
//     sinalizou como "não verificada, nunca vinculada a um elemento naquela corrida do oráculo" --
//     exercitada aqui direto, provando que o conserto não é coincidência de um único valor de RGB.
void test_gradient_alpha_roundtrip_matches_upstream_storage_type() {
  std::string out;

  // linear-gradient / radial-gradient stops -- ColorStop, ColourbPremultiplied upstream
  // (DecorationTypes.h:9) -- KEEP the lossy round-trip, same byte math as box-shadow's own
  // orchestrator-table row 2 (test_box_shadow_color_lossy_roundtrip_orchestrator_table above:
  // #c9a24b40 -> #c79f4740) -- different alpha here (0x24/0x0a), independently reconfirmed.
  check(compute_decorator_list("linear-gradient(90deg, #C9A24B24, #C9A24B0a)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
            ValueComputeStatus::Ok,
        "linear-gradient with low-alpha 8-digit hex stops computes Ok");
  check_eq(out, "linear-gradient(90.0000;#c69b4624:0.0000%;#b299330a:100.0000%)",
           "linear-gradient KEEPS the lossy round-trip -- ColorStop IS ColourbPremultiplied "
           "upstream, this is CORRECT behaviour, not the bug this item fixes");

  check(compute_decorator_list("radial-gradient(#C9A24B24, #C9A24B0a)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
            ValueComputeStatus::Ok,
        "radial-gradient with low-alpha 8-digit hex stops computes Ok");
  check_eq(out, "radial-gradient(50.0000%;50.0000%;#c69b4624:0.0000%;#b299330a:100.0000%)",
           "radial-gradient KEEPS the lossy round-trip too -- same ColorStop storage, same "
           "parse_gradient_stop() call site as linear-gradient");

  // horizontal-gradient / vertical-gradient -- DecoratorStraightGradient, plain Colourb upstream
  // (DecoratorGradient.h) -- NEVER round-trip. This IS the fix: byte-exact match to the oracle's
  // own measured side-A line for system_menu__config_controles_tabela.rml:469.
  check(compute_decorator_list("horizontal-gradient(#C9A24B24 #C9A24B0a)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
            ValueComputeStatus::Ok,
        "horizontal-gradient with low-alpha 8-digit hex computes Ok");
  check_eq(out, "horizontal-gradient(#c9a24b24;#c9a24b0a)",
           "horizontal-gradient NEVER round-trips -- start-color/stop-color are plain Colourb "
           "upstream, straight passthrough, never the corrupted #c69b4624;#b299330a this bug used "
           "to produce");

  check(compute_decorator_list("vertical-gradient(#C9A24B24 #C9A24B0a)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
            ValueComputeStatus::Ok,
        "vertical-gradient with low-alpha 8-digit hex computes Ok");
  check_eq(out, "vertical-gradient(#c9a24b24;#c9a24b0a)",
           "vertical-gradient NEVER round-trips either -- this IS "
           "system_menu__config_controles_tabela.rml:469's own exact value, byte-exact match to "
           "the oracle's own measured side-A line, never the #c69b4624;#b299330a residuo C "
           "reported");

  // The second corpus occurrence UIX-ORACLE-MEDICAO's own report explicitly flagged as
  // unverified ("did not appear in the diff for this fixture", never bound to an element in that
  // run) -- exercised directly here to prove the fix generalizes past one RGB/alpha pair.
  check(compute_decorator_list("vertical-gradient(#22D3EE1a #22D3EE0a)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
            ValueComputeStatus::Ok,
        "vertical-gradient second corpus occurrence (config_controles_tabela.rml:472) computes Ok");
  check_eq(out, "vertical-gradient(#22d3ee1a;#22d3ee0a)",
           "second corpus occurrence also stays straight -- not a one-fixture, one-RGB coincidence");
}

// ---------------------------------------------------------------------------
// EN: docs/uix-rcss.md section 7.1 -- all 4 authorized hex forms normalize to the same 8-digit
//     canonical form, plus fail-high for a syntax neither the census nor the pinned RmlUi build
//     accepts (section 13's own "real zero is a real cut" clause, restricted by `ADR-0022`/
//     `docs/rmlx-subset.md` section 7 to what the pin itself does not have). `ESC-5` widened the
//     named-color side from 3 to the pin's own full 19-entry `html_colours` table -- this function
//     keeps its own original 3 individual checks (`white`/`black`/`transparent`, still passing
//     unmodified) plus INVERTS its own former `'red'` fail-high assertion below (the pin DOES have
//     `red` -- `PropertyParserColour.cpp:123`) for a red-to-green signal with zero new test
//     scaffolding; the full 19-entry enumeration (transcribed independently from the pin, not from
//     this file's own `kNamedColorTable`), case-insensitivity, and the fail-high boundary against
//     real-but-pin-absent CSS names live in `test_color_parsing_esc5_named_color_parity()`
//     immediately below, mirroring how `ESC-4` added `test_length_resolution_esc4_full_unit_parity()`
//     as a sibling rather than folding everything into this one function.
// PT: Seção 7.1 do docs/uix-rcss.md -- as 4 formas hex autorizadas normalizam pra mesma forma
//     canônica de 8 dígitos, mais fail-high pra uma sintaxe que nem o censo nem o build fixado do
//     RmlUi aceitam (a própria cláusula "zero real é corte real" da seção 13, restrita pela
//     `ADR-0022`/seção 7 do `docs/rmlx-subset.md` ao que o próprio pin não tem). A `ESC-5` alargou o
//     lado de cor nomeada de 3 pra própria tabela `html_colours` completa de 19 entradas do pin --
//     esta função mantém os 3 checks individuais originais dela (`white`/`black`/`transparent`,
//     ainda passando sem mudança) mais INVERTE a própria asserção fail-high anterior de `'red'`
//     abaixo (o pin TEM `red` -- `PropertyParserColour.cpp:123`) pra um sinal vermelho-pra-verde com
//     zero andaime de teste novo; a enumeração completa das 19 (transcrita independente do pin, não
//     da própria `kNamedColorTable` deste arquivo), a case-insensitivity, e a fronteira fail-high
//     contra nomes CSS reais-porém-ausentes-do-pin moram no
//     `test_color_parsing_esc5_named_color_parity()` logo abaixo, espelhando como a `ESC-4` somou o
//     `test_length_resolution_esc4_full_unit_parity()` como irmã em vez de dobrar tudo nesta função.
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

  check(parse_color("red", &c) == ValueComputeStatus::Ok,
        "'red': ESC-5 -- now authorized, section 13's own set widened to the pin's full 19 "
        "(was fail-high pre-ESC-5)");
  check_eq(print_color(c), "#ff0000ff", "red -> #ff0000ff");
  check(parse_color("rgb(255,0,0)", &c) == ValueComputeStatus::Ok,
        "rgb(): ESC-6 -- now authorized, functional color forms delivered (was fail-high "
        "pre-ESC-6, RED anchor this task's own delivery notes name explicitly)");
  check_eq(print_color(c), "#ff0000ff", "rgb(255,0,0) -> #ff0000ff");
  check(parse_color("#ff", &c) == ValueComputeStatus::Invalid,
        "2-digit hex: fail-high, not one of the 4 authorized forms");
}

// ---------------------------------------------------------------------------
// EN: `ESC-5` -- docs/uix-rcss.md section 7.1's own full named-color set, all 19 of the pin's own
//     `html_colours` map (`examples/RmlUi/Source/Core/PropertyParserColour.cpp:117-135`), plus the
//     case-insensitivity the pin's own `ParseColour` applies via `StringUtilities::ToLower(value)`
//     immediately before its own lookup (`:201`), plus the fail-high boundary against real CSS
//     names the pin does NOT register. **Every `name`/`want_hex` pair below is transcribed directly
//     from the pin's own source, by hand, NOT by iterating `value_compute.cpp`'s own
//     `kNamedColorTable`** -- this is this task's own explicit "independent oracle" requirement: a
//     bug shared by both tables (a transposed digit, a swapped row) would hide behind agreement if
//     this test merely re-read the production table instead of re-deriving the same 19 answers on
//     its own.
// PT: `ESC-5` -- o próprio conjunto completo de cor nomeada da seção 7.1 do docs/uix-rcss.md, as 19
//     do próprio mapa `html_colours` do pin (`examples/RmlUi/Source/Core/
//     PropertyParserColour.cpp:117-135`), mais a case-insensitivity que o próprio `ParseColour` do
//     pin aplica via `StringUtilities::ToLower(value)` logo antes do próprio lookup (`:201`), mais a
//     fronteira fail-high contra nomes CSS reais que o pin NÃO registra. **Todo par
//     `name`/`want_hex` abaixo é transcrito direto da própria fonte do pin, à mão, NÃO iterando a
//     própria `kNamedColorTable` do value_compute.cpp** -- este é o próprio requisito explícito
//     "oráculo independente" desta tarefa: um bug compartilhado pelas duas tabelas (um dígito
//     transposto, uma linha trocada) se esconderia atrás da concordância se este teste só relesse a
//     tabela de produção em vez de re-derivar as mesmas 19 respostas sozinho.
void test_color_parsing_esc5_named_color_parity() {
  struct NamedColorCase {
    const char* name;
    const char* want_hex;
  };
  // EN: Pin's own `Colourb(r, g, b[, a])` constructor arguments (decimal), converted to the
  //     canonical 8-digit hex form by hand and cross-checked against `ADR-0022`'s own measured
  //     table (`docs/adr/0022-paridade-total-com-o-motor-substituido.md`, "Named colours" row) --
  //     not copied from that table either, both were derived independently from the same pin
  //     source and happen to agree, which is the point.
  // PT: Os próprios argumentos de construtor `Colourb(r, g, b[, a])` do pin (decimal), convertidos
  //     pra forma hex canônica de 8 dígitos à mão e cruzados contra a própria tabela medida da
  //     `ADR-0022` (`docs/adr/0022-paridade-total-com-o-motor-substituido.md`, linha "Named
  //     colours") -- também não copiados daquela tabela, as duas foram derivadas independentemente
  //     da mesma fonte do pin e calham de concordar, que é o ponto.
  static const NamedColorCase kPinColors[] = {
      {"black", "#000000ff"},
      {"silver", "#c0c0c0ff"},
      {"gray", "#808080ff"},
      {"grey", "#808080ff"},
      {"white", "#ffffffff"},
      {"maroon", "#800000ff"},
      {"red", "#ff0000ff"},
      {"orange", "#ffa500ff"},
      {"purple", "#800080ff"},
      {"fuchsia", "#ff00ffff"},
      {"green", "#008000ff"},
      {"lime", "#00ff00ff"},
      {"olive", "#808000ff"},
      {"yellow", "#ffff00ff"},
      {"navy", "#000080ff"},
      {"blue", "#0000ffff"},
      {"teal", "#008080ff"},
      {"aqua", "#00ffffff"},
      {"transparent", "#00000000"},
  };
  check(sizeof(kPinColors) / sizeof(kPinColors[0]) == 19,
        "pin's own html_colours table has exactly 19 entries");
  for (const NamedColorCase& tc : kPinColors) {
    Rgba8 got{};
    check(parse_color(tc.name, &got) == ValueComputeStatus::Ok, tc.name);
    check_eq(print_color(got), tc.want_hex, tc.name);
  }

  // EN: Case-insensitivity -- mirrors the pin's own `ToLower()` call, `PropertyParserColour.cpp:201`.
  // PT: Case-insensitivity -- espelha a própria chamada `ToLower()` do pin,
  //     `PropertyParserColour.cpp:201`.
  Rgba8 c{};
  check(parse_color("Red", &c) == ValueComputeStatus::Ok, "'Red' (mixed case) parses");
  check_eq(print_color(c), "#ff0000ff", "Red -> #ff0000ff");
  check(parse_color("RED", &c) == ValueComputeStatus::Ok, "'RED' (all caps) parses");
  check_eq(print_color(c), "#ff0000ff", "RED -> #ff0000ff");
  check(parse_color("White", &c) == ValueComputeStatus::Ok, "'White' (mixed case) parses");
  check_eq(print_color(c), "#ffffffff", "White -> #ffffffff");
  check(parse_color("TRANSPARENT", &c) == ValueComputeStatus::Ok, "'TRANSPARENT' (all caps) parses");
  check_eq(print_color(c), "#00000000", "TRANSPARENT -> #00000000");

  // EN: Fail-high boundary preserved -- real, extended CSS named colors (X11-derived CSS Color
  //     Module keywords) the pin's own 19-entry table does NOT register, plus one arbitrary
  //     non-color identifier. Neither the census nor the pinned RmlUi build accepts these -- the
  //     one case section 13 still fails high on, per `ADR-0022`'s own unchanged fail-high policy.
  // PT: Fronteira fail-high preservada -- cores CSS reais, estendidas (keywords do CSS Color
  //     Module, derivadas do X11) que a própria tabela de 19 do pin NÃO registra, mais um
  //     identificador não-cor qualquer. Nem o censo nem o build fixado do RmlUi aceitam essas -- o
  //     único caso em que a seção 13 ainda falha alto, per a própria política fail-high inalterada
  //     da `ADR-0022`.
  check(parse_color("rebeccapurple", &c) == ValueComputeStatus::Invalid,
        "'rebeccapurple': extended CSS name, NOT in the pin's 19-entry table, fail-high");
  check(parse_color("cornflowerblue", &c) == ValueComputeStatus::Invalid,
        "'cornflowerblue': extended CSS name, NOT in the pin's 19-entry table, fail-high");
  check(parse_color("notacolor", &c) == ValueComputeStatus::Invalid,
        "'notacolor': arbitrary non-color identifier, fail-high");
}

// ---------------------------------------------------------------------------
// EN: `ESC-6` -- docs/uix-rcss.md section 7.1's own 8 functional color forms (`rgb()`, `rgba()`,
//     `hsl()`, `hsla()`, `lab()`, `lch()`, `oklab()`, `oklch()`), closing `ADR-0022`'s own measured
//     row ("Colour functional forms | -- | 8 | --"). Anchors marked "task-given" below are
//     transcribed verbatim from this task's own delivery notes (derived by hand from the pin's real
//     algorithm, `PropertyParserColour.cpp`, read in full). Anchors marked "independent Python
//     oracle" are derived by a SEPARATE, from-scratch transcription of the pin's CIELAB/Oklab math
//     in `numpy.float32` (never by running this task's own new C++ and copying its output -- that
//     would be a tautological test) -- script kept in this task's own delivery notes, not
//     reproduced in this repo (this file's own house style: the ANCHOR is the deliverable, not the
//     generator).
//
//     ⚠️ TWO CORRECTIONS TO THIS TASK'S OWN BRIEFING, discovered by that independent oracle and
//     VERIFIED against this item's own actually-compiled implementation across `-O0`/`-O1`/`-O2`/
//     `-O3`/no-flag (this repo's own default) before being pinned here -- rule 9 of this task's own
//     briefing ("se o pin divergir do que este plano afirma, o PIN ganha"):
//       1. **`oklab(1 0 0)` is `#fefefeff`, NOT `#ffffffff`** (the briefing's own stated anchor).
//       Not a rounding nicety, a deterministic, platform-independent IEEE-754 fact, unrelated to any
//       FMA-contraction risk: `OklabToRGBA(1,0,0)` produces `l'=m'=s'=1.0f` EXACTLY (matrix times a
//       zero vector, no rounding possible), `l=m=s=1.0f` EXACTLY (cubing exactly 1.0), `pow(1.0f,
//       y)=1.0f` EXACTLY (an IEEE-754/C++-mandated special case for ANY exponent, including this
//       function's own `1.0f/2.4f`) -- so `InverseSRGBNonlinearTransfer(1.0f)` reduces to
//       `1.055f * 1.0f - 0.055f` = `1.055f - 0.055f`, and `1.055f - 0.055f != 1.0f` in binary32 (it
//       is `0.99999994f`, exactly 1 ULP below -- measured directly, bit pattern `0x3f7fffff` vs
//       `1.0f`'s own `0x3f800000`). `0.99999994f * 255.0f = 254.99998...`, which the pin's own
//       `(int)`-cast TRUNCATES (never rounds) to `254`, not `255`. This is the SAME phenomenon this
//       document's own section 15's `lab(100 0 0)` warning already names for CIELAB (below) --
//       discovered independently here for Oklab's own identity/white point, which the briefing
//       assumed (without measuring) would be exact.
//       2. **`lab(50 40 60/0.5)` (the briefing's own literal example for "`/` not isolated") is
//       `Ok`, `#c35600ff` -- NOT `Invalid`.** With `/` glued to BOTH neighbors (no space on either
//       side), the space-delimited tokenizer produces only 3 tokens (`"50"`,`"40"`,`"60/0.5"` -- the
//       slash is swallowed into the SAME token as `"60"`, never its own token), so `ParseCIELABColour`
//       takes its OWN no-alpha branch (`values.size()==3`) and never reaches the `values[3]=="/"`
//       check at all; `pin_atof("60/0.5")` then stops at the `/` (this section's own documented
//       `atof` leniency) and returns `60.0f`, silently discarding `/0.5` as trailing garbage -- the
//       B-axis becomes `60`, alpha defaults to `1.0`. The test THIS task's own brief actually
//       intended -- "the `/` isolation requirement is enforced" -- needs PARTIAL spacing (one side
//       only), which genuinely IS `Invalid` (below): a real, previously-undiscovered boundary this
//       independent verification pass found while confirming the briefing's own literal string.
// PT: `ESC-6` -- as próprias 8 formas funcionais de cor da seção 7.1 do docs/uix-rcss.md (`rgb()`,
//     `rgba()`, `hsl()`, `hsla()`, `lab()`, `lch()`, `oklab()`, `oklch()`), fechando a própria linha
//     medida da `ADR-0022` ("Colour functional forms | -- | 8 | --"). Âncoras marcadas "dada pela
//     tarefa" abaixo são transcritas verbatim das próprias notas de entrega desta tarefa (derivadas
//     à mão do próprio algoritmo real do pin, `PropertyParserColour.cpp`, lido inteiro). Âncoras
//     marcadas "oráculo Python independente" são derivadas por uma transcrição SEPARADA, do zero, da
//     própria matemática CIELAB/Oklab do pin em `numpy.float32` (nunca rodando o C++ novo desta
//     tarefa e copiando a própria saída dele -- isso seria um teste tautológico) -- script mantido
//     nas próprias notas de entrega desta tarefa, não reproduzido neste repo (o próprio estilo da
//     casa deste arquivo: a ÂNCORA é o entregável, não o gerador).
//
//     ⚠️ DUAS CORREÇÕES AO PRÓPRIO BRIEFING desta tarefa, descobertas por aquele oráculo
//     independente e VERIFICADAS contra a própria implementação JÁ COMPILADA deste item em
//     `-O0`/`-O1`/`-O2`/`-O3`/sem-flag (o próprio default deste repo) antes de serem pinadas aqui --
//     regra 9 do próprio briefing desta tarefa ("se o pin divergir do que este plano afirma, o PIN
//     ganha"):
//       1. **`oklab(1 0 0)` é `#fefefeff`, NÃO `#ffffffff`** (a própria âncora declarada do
//       briefing). Não é capricho de arredondamento, é um fato IEEE-754 determinístico,
//       independente de plataforma, sem relação com risco nenhum de contração-FMA:
//       `OklabToRGBA(1,0,0)` produz `l'=m'=s'=1.0f` EXATO (matriz vezes vetor zero, nenhum
//       arredondamento possível), `l=m=s=1.0f` EXATO (elevar exatamente 1.0 ao cubo), `pow(1.0f,
//       y)=1.0f` EXATO (um caso especial mandado pela IEEE-754/C++ pra QUALQUER expoente, inclusive
//       o próprio `1.0f/2.4f` desta função) -- então `InverseSRGBNonlinearTransfer(1.0f)` se reduz a
//       `1.055f * 1.0f - 0.055f` = `1.055f - 0.055f`, e `1.055f - 0.055f != 1.0f` em binary32 (é
//       `0.99999994f`, exatamente 1 ULP abaixo -- medido direto, padrão de bits `0x3f7fffff` contra
//       o próprio `0x3f800000` do `1.0f`). `0.99999994f * 255.0f = 254.99998...`, que o próprio
//       cast `(int)` do pin TRUNCA (nunca arredonda) pra `254`, não `255`. Este é o MESMO fenômeno
//       que o próprio aviso do `lab(100 0 0)` da seção 15 deste documento já nomeia pro CIELAB
//       (abaixo) -- descoberto aqui independentemente pro próprio ponto-branco/identidade do Oklab,
//       que o briefing supôs (sem medir) que seria exato.
//       2. **`lab(50 40 60/0.5)` (o próprio exemplo literal do briefing pro "`/` não isolado") é
//       `Ok`, `#c35600ff` -- NÃO `Invalid`.** Com `/` colado nos DOIS vizinhos (sem espaço de
//       nenhum lado), o tokenizador separado-por-espaço produz só 3 tokens (`"50"`,`"40"`,
//       `"60/0.5"` -- a barra é engolida pro MESMO token que "60", nunca o próprio token dela), então
//       o `ParseCIELABColour` toma o próprio ramo sem-alpha dele (`values.size()==3`) e nunca chega
//       a checar `values[3]=="/"` sequer; `pin_atof("60/0.5")` então para no `/` (a própria
//       leniência `atof` documentada desta seção) e retorna `60.0f`, descartando `/0.5` em silêncio
//       como lixo à direita -- o eixo B vira `60`, alpha default `1.0`. O teste que o próprio
//       briefing desta tarefa de fato pretendia -- "a exigência de isolamento do `/` é aplicada" --
//       precisa de espaçamento PARCIAL (um lado só), que genuinamente É `Invalid` (abaixo): uma
//       fronteira real, antes não-descoberta, que esta passada de verificação independente achou
//       enquanto confirmava a própria string literal do briefing.
void test_color_parsing_esc6_functional_forms() {
  Rgba8 c{};

  // --- Anchors (task-given), rgb()/rgba() -----------------------------------------------------
  check(parse_color("rgb(255,0,0)", &c) == ValueComputeStatus::Ok, "rgb(255,0,0): Ok");
  check_eq(print_color(c), "#ff0000ff", "rgb(255,0,0) -> #ff0000ff");

  check(parse_color("rgb(100%,0%,50%)", &c) == ValueComputeStatus::Ok, "rgb(100%,0%,50%): Ok");
  check_eq(print_color(c), "#ff007fff",
           "rgb(100%,0%,50%) -> #ff007fff -- 50% truncates via int(50*2.55)=int(127.5)=127 (0x7f), "
           "NEVER rounds to 128");

  check(parse_color("rgba(0,0,0,128)", &c) == ValueComputeStatus::Ok, "rgba(0,0,0,128): Ok");
  check_eq(print_color(c), "#00000080", "rgba(0,0,0,128) -> #00000080 -- alpha is a plain 0-255 int");

  check(parse_color("rgba(0,0,0,0.5)", &c) == ValueComputeStatus::Ok, "rgba(0,0,0,0.5): Ok");
  check_eq(print_color(c), "#00000000",
           "rgba(0,0,0,0.5) -> #00000000 -- alpha is atoi-parsed, NOT 0-1 float: atoi(\"0.5\") stops "
           "at '.', giving 0 -- rgba's own alpha is 0-255 int OR %, never a 0-1 fraction");

  // --- Anchors (task-given), hsl()/hsla(), including hue wrap -----------------------------------
  check(parse_color("hsl(120,100%,50%)", &c) == ValueComputeStatus::Ok, "hsl(120,100%,50%): Ok");
  check_eq(print_color(c), "#00ff00ff", "hsl(120,100%,50%) -> #00ff00ff (pure green)");

  check(parse_color("hsl(480,100%,50%)", &c) == ValueComputeStatus::Ok, "hsl(480,100%,50%): Ok");
  check_eq(print_color(c), "#00ff00ff",
           "hsl(480,...) -> same as hsl(120,...) -- fmod(480,360)=120, hue wraps positive");

  check(parse_color("hsl(-240,100%,50%)", &c) == ValueComputeStatus::Ok, "hsl(-240,100%,50%): Ok");
  check_eq(print_color(c), "#00ff00ff",
           "hsl(-240,...) -> same as hsl(120,...) -- fmod(-240,360)=-240, +360=120, hue wraps "
           "negative too");

  check(parse_color("hsla(0,0%,100%,0.5)", &c) == ValueComputeStatus::Ok, "hsla(0,0%,100%,0.5): Ok");
  check_eq(print_color(c), "#ffffff7f",
           "hsla(0,0%,100%,0.5) -> #ffffff7f -- s=0 collapses to grayscale (r=g=b=l=white), alpha "
           "here IS 0-1 float (asymmetry with rgba's own 0-255 int/%% alpha, both documented "
           "verbatim per this task's own briefing)");

  // --- Anchors (task-given), oklab() identity/zero cases -----------------------------------------
  check(parse_color("oklab(1 0 0)", &c) == ValueComputeStatus::Ok, "oklab(1 0 0): Ok");
  check_eq(print_color(c), "#fefefeff",
           "oklab(1 0 0) -> #fefefeff, NOT #ffffffff -- CORRECTED anchor, see this function's own "
           "header for the measured, platform-independent IEEE-754 reason (1.055f-0.055f != 1.0f)");

  check(parse_color("oklab(0 0 0)", &c) == ValueComputeStatus::Ok, "oklab(0 0 0): Ok");
  check_eq(print_color(c), "#000000ff", "oklab(0 0 0) -> #000000ff (exact -- 0*anything=0 exactly)");

  check(parse_color("oklab(none none none)", &c) == ValueComputeStatus::Ok,
        "oklab(none none none): Ok");
  check_eq(print_color(c), "#000000ff",
           "oklab(none none none) -> #000000ff, same as (0 0 0) -- 'none' means 0.0 for L/a/b, "
           "alpha still defaults to 1.0 (3 tokens, no explicit alpha)");

  // --- Non-trivial lab()/lch()/oklch(), independent Python oracle -------------------------------
  check(parse_color("lab(55.5 23.75 -40.25 / 0.8)", &c) == ValueComputeStatus::Ok,
        "lab(55.5 23.75 -40.25 / 0.8): Ok -- plain numbers (no clamp), alpha via isolated '/'");
  check_eq(print_color(c), "#8779cacc", "lab non-trivial, independent Python oracle");

  check(parse_color("lch(42.5 63.25 275.5)", &c) == ValueComputeStatus::Ok,
        "lch(42.5 63.25 275.5): Ok -- plain L/chroma (no clamp), no alpha (defaults 1.0)");
  check_eq(print_color(c), "#0068cdff",
           "lch non-trivial, independent Python oracle -- R=0 is a ROBUST clamp (pre-clamp linear "
           "r ~= -0.92, nowhere near the 0.0 boundary itself, verified in this task's own delivery "
           "notes), not a fragile arithmetic coincidence");

  check(parse_color("oklch(0.65 0.12 145.5)", &c) == ValueComputeStatus::Ok,
        "oklch(0.65 0.12 145.5): Ok");
  check_eq(print_color(c), "#5ba260ff", "oklch non-trivial, independent Python oracle");

  // EN: The Ottosson post's own worked reference (https://bottosson.github.io/posts/oklab/):
  //     sRGB red is approximately `oklch(0.62796 0.25768 29.23)` -- the decimal literals are
  //     themselves an APPROXIMATION (Ottosson's own post rounds the true irrational coordinates),
  //     so this is NOT expected to be bit-exact sRGB red for the SAME reason `lab(100 0 0)` below
  //     is not bit-exact white -- measured, not assumed: R clamps to 1.0 ROBUSTLY (pre-clamp linear
  //     r ~= 1.0000032, about 60 ULP above 1.0, per this task's own delivery notes), G/B are tiny
  //     positive residuals (~3e-5/~3e-4) that truncate to 0 regardless of any sub-ULP wobble.
  // PT: A própria referência trabalhada do post do Ottosson
  //     (https://bottosson.github.io/posts/oklab/): sRGB red é aproximadamente
  //     `oklch(0.62796 0.25768 29.23)` -- os próprios literais decimais já são uma APROXIMAÇÃO (o
  //     próprio post do Ottosson arredonda as coordenadas irracionais verdadeiras), então isto NÃO é
  //     esperado ser sRGB red bit-exato pelo MESMO motivo que o `lab(100 0 0)` abaixo não é branco
  //     bit-exato -- medido, não suposto: R clampa pra 1.0 DE FORMA ROBUSTA (r linear pré-clamp ~=
  //     1,0000032, cerca de 60 ULP acima de 1.0, per as próprias notas de entrega desta tarefa),
  //     G/B são resíduos positivos minúsculos (~3e-5/~3e-4) que truncam pra 0 independente de
  //     qualquer oscilação sub-ULP.
  check(parse_color("oklch(0.62796 0.25768 29.23)", &c) == ValueComputeStatus::Ok,
        "oklch(0.62796 0.25768 29.23) [Ottosson reference]: Ok");
  check_eq(print_color(c), "#ff0000ff",
           "Ottosson reference approximates sRGB red -- #ff0000ff, R clamped robustly, G/B "
           "truncate-to-0 robustly (both far from any boundary, independent Python oracle)");

  // --- Clamp boundaries actually observed post-conversion, independent Python oracle -------------
  check(parse_color("lab(50 200 60)", &c) == ValueComputeStatus::Ok,
        "lab(50 200 60): Ok -- a=200 clamps to +160 (kCielabAxisBoundLimit) BEFORE the CIELAB "
        "matrix, not rejected");
  check_eq(print_color(c), "#ff0028ff", "lab a-clamp result, independent Python oracle");
  {
    // EN: Contrast check -- the UNCLAMPED a=200 value would print a DIFFERENT byte (#ff0032ff, this
    //     task's own delivery notes), proving the clamp is actually load-bearing here, not a no-op.
    // PT: Checagem de contraste -- o próprio valor a=200 NÃO-CLAMPADO imprimiria um byte DIFERENTE
    //     (#ff0032ff, as próprias notas de entrega desta tarefa), provando que o clamp de fato
    //     importa aqui, não é um no-op.
    check(print_color(c) != "#ff0032ff",
          "lab a-clamp: the clamped result must differ from the unclamped-would-be result");
  }

  check(parse_color("lch(50% 500 90)", &c) == ValueComputeStatus::Ok,
        "lch(50% 500 90): Ok -- L from '50%' is DIRECT 50.0 (no /100 for lightness), chroma=500 "
        "clamps to 230 (kCielchMaximumChroma)");
  check_eq(print_color(c), "#9c7300ff", "lch chroma-clamp result, independent Python oracle");

  check(parse_color("oklab(0.5 0.9 -0.2)", &c) == ValueComputeStatus::Ok,
        "oklab(0.5 0.9 -0.2): Ok -- a=0.9 clamps to +0.5 (kOklabAxisBoundLimit)");
  check_eq(print_color(c), "#f400c3ff", "oklab a-clamp result, independent Python oracle");

  check(parse_color("oklch(0.5 0.9 150)", &c) == ValueComputeStatus::Ok,
        "oklch(0.5 0.9 150): Ok -- chroma=0.9 clamps to 0.5 (kOklchMaximumChroma)");
  check_eq(print_color(c), "#009700ff", "oklch chroma-clamp result, independent Python oracle");

  // EN: docs/uix-rcss.md section 15's own explicit warning, reproduced as a real test (not merely
  //     cited): CIELAB's D65 round-trip is NOT bit-exact white for L=100 -- the compound rounding
  //     through `f_inverse`/the XYZ matrix/`InverseSRGBNonlinearTransfer` lands 1 byte below 255 on
  //     every channel. Verified independently by this item's own Python oracle AND by this item's
  //     own actually-compiled build (see this function's own header, the `oklab(1 0 0)` correction,
  //     for the SAME phenomenon's simpler, fully-traced root cause).
  // PT: O próprio aviso explícito da seção 15 do docs/uix-rcss.md, reproduzido como teste de
  //     verdade (não só citado): a ida-e-volta D65 do CIELAB NÃO é branco bit-exato pra L=100 -- o
  //     arredondamento composto através do `f_inverse`/matriz XYZ/`InverseSRGBNonlinearTransfer`
  //     pousa 1 byte abaixo de 255 em todo canal. Verificado independentemente pelo próprio oráculo
  //     Python deste item E pelo próprio build já compilado deste item (ver o próprio cabeçalho
  //     desta função, a correção do `oklab(1 0 0)`, pra causa raiz mais simples, totalmente
  //     rastreada, do MESMO fenômeno).
  check(parse_color("lab(100 0 0)", &c) == ValueComputeStatus::Ok, "lab(100 0 0): Ok");
  check_eq(print_color(c), "#fefefeff",
           "lab(100 0 0) -> #fefefeff, NOT pure white #ffffffff -- docs/uix-rcss.md section 15's "
           "own explicit warning, now a real assertion");

  // --- Fronteira/fail-high -----------------------------------------------------------------------
  check(parse_color("hsl(120,0.5,0.5)", &c) == ValueComputeStatus::Invalid,
        "hsl(120,0.5,0.5): Invalid -- S/L require a trailing '%', a bare fraction is NOT silently "
        "reinterpreted as already-normalized [0,1]");
  check(parse_color("rgb(1,2)", &c) == ValueComputeStatus::Invalid,
        "rgb(1,2): Invalid -- 2 values, rgb (3-arg, no alpha suffix) requires exactly 3");
  check(parse_color("rgba(1,2,3)", &c) == ValueComputeStatus::Invalid,
        "rgba(1,2,3): Invalid -- 3 values, rgba (4-arg) requires exactly 4");

  check(parse_color("rgb(300,-5,0)", &c) == ValueComputeStatus::Ok,
        "rgb(300,-5,0): Ok -- out-of-range components SATURATE, never rejected");
  check_eq(print_color(c), "#ff0000ff",
           "rgb(300,-5,0) -> #ff0000ff -- 300 clamps to 255, -5 clamps to 0");

  check(parse_color("RGB(255,0,0)", &c) == ValueComputeStatus::Invalid,
        "RGB(255,0,0): Invalid -- top-level dispatch is case-SENSITIVE ('RGB' != 'rgb'), and the "
        "lowered fallback 'rgb(255,0,0)' is not a valid color NAME either");
  check(parse_color("cmyk(0,0,0,0)", &c) == ValueComputeStatus::Invalid,
        "cmyk(0,0,0,0): Invalid -- not a recognised prefix, not a color name");
  check(parse_color("rgb", &c) == ValueComputeStatus::Invalid,
        "'rgb' bareword: Invalid -- prefix matches, but there is no '(' at all -- the prefix STEALS "
        "the input from the name table (GetColourFunctionValues's own single failure case), never "
        "silently falls back to 'is this a color name'");
  check(parse_color("labrador", &c) == ValueComputeStatus::Invalid,
        "'labrador': Invalid -- 'lab' prefix steals this too (substr(0,3)==\"lab\"), same reasoning "
        "as bare 'rgb' -- never reaches the name table despite obviously not being a color function");

  // EN: `rgbx(1,2,3)` -- the pin's own prefix match is LOOSE (`value.substr(0,3)=="rgb"`, not an
  //     exact `=="rgb("` or a word-boundary check), and aridade is detected by `raw[3]=='a'` alone
  //     -- `raw[3]` here is `'x'`, not `'a'`, so this is accepted as the 3-arg `rgb` form, per this
  //     task's own briefing narrative (not one of the required test-list bullets, added for
  //     completeness since it is exactly the kind of surprising, documented consequence this whole
  //     exercise exists to pin down).
  // PT: `rgbx(1,2,3)` -- o próprio casamento de prefixo do pin é FROUXO (`value.substr(0,3)=="rgb"`,
  //     não um `=="rgb("` exato nem checagem de fronteira-de-palavra), e a aridade é detectada só
  //     por `raw[3]=='a'` -- `raw[3]` aqui é `'x'`, não `'a'`, então isto é aceito como a própria
  //     forma `rgb` de 3 argumentos, per a própria narrativa do briefing desta tarefa (não um dos
  //     bullets obrigatórios da lista de teste, somado pra completude já que é exatamente o tipo de
  //     consequência surpreendente, documentada, que este exercício inteiro existe pra pinar).
  check(parse_color("rgbx(1,2,3)", &c) == ValueComputeStatus::Ok,
        "rgbx(1,2,3): Ok -- prefix match is loose ('rgb' is the first 3 chars), aridade check is "
        "only raw[3]=='a', and 'x' != 'a' means this is treated as 3-arg rgb");
  check_eq(print_color(c), "#010203ff", "rgbx(1,2,3) -> #010203ff (r=1,g=2,b=3, alpha default 255)");

  // --- '/' isolation boundary (this item's own independent finding, see function header) ---------
  check(parse_color("lab(50 40 60/0.5)", &c) == ValueComputeStatus::Ok,
        "lab(50 40 60/0.5) [task's own literal string]: Ok -- CORRECTED from the briefing's own "
        "assumed Invalid, see this function's own header correction 2");
  check_eq(print_color(c), "#c35600ff",
           "lab(50 40 60/0.5) -> #c35600ff -- '/' glued both sides is swallowed into the SAME token "
           "as '60' (space-delimited tokenizer never splits it out), atof(\"60/0.5\") stops at '/' "
           "giving B=60, alpha silently defaults to 1.0 (the '0.5' is never read as alpha at all)");

  check(parse_color("lab(50 40 60/ 0.5)", &c) == ValueComputeStatus::Invalid,
        "lab(50 40 60/ 0.5) ['/' glued LEFT only]: Invalid -- 4 tokens ('60/' and '0.5' split by "
        "the space, but '/' stays glued to '60'), matches NEITHER the 3-token (no alpha) NOR "
        "5-token (isolated '/') shape -- this IS the real '/'-isolation failure the briefing's own "
        "literal string did not actually exercise");
  check(parse_color("lab(50 40 60 /0.5)", &c) == ValueComputeStatus::Invalid,
        "lab(50 40 60 /0.5) ['/' glued RIGHT only]: Invalid -- same reasoning, mirrored ('60' and "
        "'/0.5' split by the space, '/' stays glued to '0.5')");
  check(parse_color("lab(50 40 60 / 0.5)", &c) == ValueComputeStatus::Ok,
        "lab(50 40 60 / 0.5) ['/' fully isolated]: Ok -- 5 tokens, values[3]==\"/\" holds, alpha IS "
        "read this time");
  check_eq(print_color(c), "#c356007f",
           "lab(50 40 60 / 0.5) -> #c356007f -- SAME RGB as the glued-both-sides case above "
           "(#c35600), DIFFERENT alpha (0x7f = int(0.5*255) = 127, not the silent 0xff default) -- "
           "proves the '/' isolation actually gates whether alpha is read at all, not just whether "
           "parsing succeeds");

  // --- Documental (this task's own briefing narrative, verbatim behaviour) -----------------------
  check(parse_color("rgb(255,0,0", &c) == ValueComputeStatus::Ok,
        "rgb(255,0,0 [no closing paren]: Ok -- GetColourFunctionValues's own rfind(')')==npos "
        "underflow clamps to 'rest of string' via substr's own documented saturation, not UB");
  check_eq(print_color(c), "#ff0000ff", "missing ')' still parses correctly to #ff0000ff");

  check(parse_color("rgb(abc,def,ghi)", &c) == ValueComputeStatus::Ok,
        "rgb(abc,def,ghi): Ok -- pin_atoi leniency, garbage -> 0 for every component");
  check_eq(print_color(c), "#000000ff", "rgb(abc,def,ghi) -> #000000ff (all garbage -> 0)");

  check(parse_color("rgb(255,,0)", &c) == ValueComputeStatus::Ok,
        "rgb(255,,0): Ok -- repeated comma produces an empty middle token (ignore_repeated_"
        "delimiters=false for comma-separated rgb/hsl), pin_atoi(\"\") -> 0");
  check_eq(print_color(c), "#ff0000ff", "rgb(255,,0) -> #ff0000ff (empty component -> 0)");
}

// ---------------------------------------------------------------------------
// EN: `ESC-6` -- the `split_whitespace()` paren-aware upgrade (value_compute.cpp's own comment at
//     that function's own definition), exercised end to end through every one of its 5 call sites
//     that can now receive a functional color as one of its own space-separated tokens: gradient
//     stops (`parse_gradient_stop`, via the PUBLIC `compute_linear_gradient_args`), `box-shadow`
//     layers (`compute_box_shadow`, PUBLIC), `drop-shadow()` and 2-color straight gradients
//     (`compute_drop_shadow`/`compute_two_stop_straight_gradient`, both INTERNAL-linkage --
//     exercised here via the PUBLIC `compute_decorator_list()` dispatcher instead, the only way this
//     test file can reach them at all). `compute_radial_gradient_args`'s own `circle at X% Y%`
//     clause is the one call site this upgrade does NOT need to prove anything new for (it never
//     contains a paren) -- not re-tested here, already covered by this file's own pre-existing
//     `circle at` assertions elsewhere.
//
//     Also exercises the SIBLING, deliberately asymmetric case-folding rule (this section's own
//     "Fidelidade de caixa por contexto" note, this task's own briefing): `box-shadow` lowercases
//     its own WHOLE raw value before parsing (`PropertyParserBoxShadow.cpp:24`, this file's own
//     `compute_box_shadow`, unchanged by `ESC-6`) -- so an uppercase `RGB()` INSIDE a `box-shadow`
//     declaration IS accepted, folded to lowercase before `parse_color()` ever inspects the prefix.
//     A gradient stop does NOT lowercase (`compute_linear_gradient_args`/
//     `parse_and_space_stops`/`parse_gradient_stop`, none of them call `to_lower()`) -- so the SAME
//     uppercase `RGB()` inside a gradient stop is `Invalid`, the case-sensitive top-level dispatch
//     rule applying with full force. Two contexts, two outcomes, from the SAME uppercase text --
//     this is a real, measured asymmetry of the PIN itself, not an inconsistency this module
//     introduces.
// PT: `ESC-6` -- o próprio alargamento consciente-de-parêntese do `split_whitespace()` (o próprio
//     comentário do value_compute.cpp na própria definição daquela função), exercitado ponta-a-ponta
//     por cada um dos 5 call sites dela que agora conseguem receber uma cor funcional como um dos
//     próprios tokens separados-por-espaço: stops de gradiente (`parse_gradient_stop`, via o
//     `compute_linear_gradient_args` PÚBLICO), camadas de `box-shadow` (`compute_box_shadow`,
//     PÚBLICO), `drop-shadow()` e gradientes retos de 2 cores
//     (`compute_drop_shadow`/`compute_two_stop_straight_gradient`, os dois de vinculação INTERNA --
//     exercitados aqui via o despachante `compute_decorator_list()` PÚBLICO em vez disso, o único
//     jeito deste arquivo de teste alcançá-los sequer). A própria cláusula `circle at X% Y%` do
//     `compute_radial_gradient_args` é o único call site que este alargamento NÃO precisa provar
//     nada novo (nunca contém parêntese) -- não re-testada aqui, já coberta pelas próprias
//     asserções `circle at` pré-existentes deste arquivo em outro lugar.
//
//     Também exercita a regra IRMÃ, deliberadamente assimétrica, de dobra-de-caixa (a própria nota
//     "Fidelidade de caixa por contexto" desta seção, o próprio briefing desta tarefa): o
//     `box-shadow` minusculiza o PRÓPRIO valor cru INTEIRO antes de parsear
//     (`PropertyParserBoxShadow.cpp:24`, o próprio `compute_box_shadow` deste arquivo, inalterado
//     pela `ESC-6`) -- então um `RGB()` maiúsculo DENTRO de uma declaração `box-shadow` É aceito,
//     dobrado pra minúsculo antes do `parse_color()` sequer inspecionar o prefixo. Um stop de
//     gradiente NÃO minusculiza (`compute_linear_gradient_args`/`parse_and_space_stops`/
//     `parse_gradient_stop`, nenhum deles chama `to_lower()`) -- então o MESMO `RGB()` maiúsculo
//     dentro de um stop de gradiente é `Invalid`, a própria regra de despacho case-sensitive de
//     nível-superior valendo com força total. Dois contextos, dois resultados, do MESMO texto
//     maiúsculo -- isto é uma assimetria real, medida, do PRÓPRIO pin, não uma inconsistência que
//     este módulo introduz.
void test_color_functional_forms_paren_aware_split_esc6() {
  std::string out;

  // --- box-shadow: lowercase rgb() embedded, paren-aware split required --------------------------
  check(compute_box_shadow("2px 2px rgb(255, 0, 0)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
            ValueComputeStatus::Ok,
        "box-shadow layer with an embedded rgb() color (internal comma+space) computes Ok -- "
        "requires the paren-aware split_whitespace() fix, else 'rgb(255,'/'0,'/'0)' would shatter "
        "into 3 bogus extra tokens instead of the one color argument it actually is");
  check_eq(out, "#ff0000ff;2.0000px;2.0000px;0.0000px;0.0000px;false",
           "box-shadow layer with functional color parses byte-exact (2 length tokens, no "
           "blur/spread, not inset)");

  // --- box-shadow: UPPERCASE RGB() -- accepted, box-shadow lowercases the WHOLE value first ------
  check(compute_box_shadow("2px 2px RGB(255, 0, 0)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
            ValueComputeStatus::Ok,
        "box-shadow lowercases its OWN whole value before parsing (PropertyParserBoxShadow.cpp's "
        "own ToLower(value)) -- uppercase RGB() inside box-shadow IS accepted, unlike top-level "
        "RGB() outside any box-shadow context (test_color_parsing_esc6_functional_forms's own "
        "RGB(255,0,0)==Invalid case)");
  check_eq(out, "#ff0000ff;2.0000px;2.0000px;0.0000px;0.0000px;false",
           "same byte-exact result as the lowercase rgb() case -- case-folded before parse_color() "
           "ever sees the prefix");

  // --- gradient stop: lowercase rgb()/hsl() embedded, paren-aware split required ------------------
  check(compute_linear_gradient_args("90deg, rgb(255, 0, 0) 0%, rgb(0, 0, 255) 100%", &out) ==
            ValueComputeStatus::Ok,
        "linear-gradient stops with embedded rgb() colors compute Ok -- same paren-aware "
        "split_whitespace() fix, exercised via parse_gradient_stop()'s own 2-token "
        "<color> <position%> tokenization");
  check_eq(out, "90.0000;#ff0000ff:0.0000%;#0000ffff:100.0000%",
           "gradient args byte-exact -- both explicit-position stops parsed and (losslessly, full "
           "opacity) round-tripped through the box-shadow/gradient-stop premultiply pair "
           "(UIX-RCSS-ERRATA-4)");

  // --- gradient stop: UPPERCASE RGB() -- Invalid, gradient stops do NOT lowercase ------------------
  check(compute_linear_gradient_args("90deg, RGB(255, 0, 0) 0%, rgb(0, 0, 255) 100%", &out) ==
            ValueComputeStatus::Invalid,
        "gradient stops do NOT lowercase (unlike box-shadow) -- uppercase RGB() inside a stop "
        "fails parse_color()'s own case-sensitive prefix check AND the lowered name-table fallback "
        "('rgb(255, 0, 0)' is not a valid color name) -- the WHOLE gradient declaration drops, "
        "section 11's uniform malformed-entry policy");

  // --- decorator list: drop-shadow() with embedded rgb(), paren-aware split required, via the -----
  //     PUBLIC compute_decorator_list() dispatcher (compute_drop_shadow itself has internal linkage)
  check(compute_decorator_list("drop-shadow(rgb(255, 0, 0) 2dp 2dp 4dp)",
                               LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
        "drop-shadow() with an embedded rgb() color computes Ok -- same paren-aware "
        "split_whitespace() fix, exercised via compute_drop_shadow()'s own 4-token "
        "<color> <len> <len> <len> tokenization (reached only through compute_decorator_list(), "
        "compute_drop_shadow itself is anonymous-namespace, not directly callable from this file)");
  check_eq(out, "drop-shadow(#ff0000ff;2.0000px;2.0000px;4.0000px)",
           "drop-shadow byte-exact with functional color -- this color is NOT premultiply-"
           "round-tripped (drop-shadow's own color stays straight, this file's own documented "
           "scope)");

  // --- decorator list: horizontal-gradient() (2-stop straight gradient) with embedded rgb() -------
  check(compute_decorator_list("horizontal-gradient(rgb(255, 0, 0) rgb(0, 255, 0))",
                               LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
        "horizontal-gradient() with embedded rgb() colors computes Ok -- same paren-aware "
        "split_whitespace() fix, exercised via compute_two_stop_straight_gradient()'s own 2-token "
        "tokenization");
  check_eq(out, "horizontal-gradient(#ff0000ff;#00ff00ff)",
           "horizontal-gradient byte-exact -- UIX-GRADIENT-ALFA's own finding: straight-gradient "
           "colors are NOT premultiply-round-tripped either, stay exactly as parsed");
}

// ---------------------------------------------------------------------------
// EN: `ESC-4` -- docs/uix-rcss.md section 8.1's own `px`/`dp` cases, UNCHANGED by this item (the
//     unitless-`0`/non-zero-unitless fail-high rule is unchanged too). The pre-`ESC-4` version of
//     this test also asserted `1em`/`1rem` were `Invalid` here -- that assertion is now FALSE
//     (`parse_length`'s own domain widened to the full 11-member `LENGTH` family, see
//     value_compute.hpp's own updated header) and is replaced by
//     `test_length_resolution_esc4_full_unit_parity()` immediately below, which proves the
//     opposite for all 9 new units plus the widened suffix-recognition mechanics (case-
//     insensitivity, `x` staying excluded, `"10 px"`'s own documented divergence).
// PT: `ESC-4` -- os próprios casos `px`/`dp` da seção 8.1 do docs/uix-rcss.md, INALTERADOS por este
//     item (a regra fail-high de zero-sem-unidade/não-zero-sem-unidade também inalterada). A versão
//     pré-`ESC-4` deste teste também asserava `1em`/`1rem` como `Invalid` aqui -- essa asserção
//     agora é FALSA (o próprio domínio do `parse_length` alargou pra família `LENGTH` completa de
//     11 membros, ver o próprio cabeçalho atualizado do value_compute.hpp) e é substituída pelo
//     `test_length_resolution_esc4_full_unit_parity()` logo abaixo, que prova o oposto pros 9
//     unidades novas mais a própria mecânica alargada de reconhecimento de sufixo
//     (case-insensitivity, `x` continuando excluído, a própria divergência documentada do
//     `"10 px"`).
void test_length_resolution() {
  float v = 0.0f;
  LengthUnit u = LengthUnit::Px;
  check(parse_length("16px", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::Px && v == 16.0f,
        "16px parses as Px unit, value 16");
  check_eq(print_length_px(resolve_length_px(v, u, LengthResolveContext{.dp_ratio = 2.0f})),
           "16.0000px", "px length ignores dp_ratio");

  check(parse_length("2dp", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::Dp && v == 2.0f,
        "2dp parses as Dp unit, value 2");
  check_eq(print_length_px(resolve_length_px(v, u, LengthResolveContext{.dp_ratio = 8.0f})),
           "16.0000px", "2dp at dp_ratio=8 resolves to 16px");

  check(parse_length("0", &v, &u) == ValueComputeStatus::Ok,
        "unitless 0 is accepted (CSS's own zero-length convention)");
  check(parse_length("5", &v, &u) == ValueComputeStatus::Invalid,
        "unitless non-zero is fail-high, never guessed");
}

// ---------------------------------------------------------------------------
// EN: `ESC-4` -- docs/uix-rcss.md section 8.1's own full `LENGTH` unit family (11 members),
//     closing `rmlx-subset.md` section 6.3/section 7's "full parity with the substituted engine"
//     decision for this axis (2026-08-06/2026-08-07). Every resolved value below transcribes
//     `ComputeLength`/`ComputePPILength` verbatim
//     (`glintfx/build/_deps/rmlui-src/Source/Core/ComputeProperty.cpp:29-70`) -- see
//     `value_compute.hpp`'s own `resolve_length_px()` doc-comment for the exact per-unit formula
//     this test pins. `em`/`rem` here exercise the GENERAL, ancestor-blind funnel (any property
//     OTHER than `font-size`) -- `test_font_size_em_resolution()` below is the SEPARATE, narrower
//     `parse_font_size()` exception for the font-size property's own parent-vs-self/root-vs-
//     document rule, which this test does not touch.
// PT: `ESC-4` -- a própria família de unidade `LENGTH` completa (11 membros) da seção 8.1 do
//     docs/uix-rcss.md, fechando a decisão "paridade completa com o motor substituído" da seção
//     6.3/seção 7 do `rmlx-subset.md` pra este eixo (2026-08-06/2026-08-07). Todo valor resolvido
//     abaixo transcreve `ComputeLength`/`ComputePPILength` verbatim
//     (`glintfx/build/_deps/rmlui-src/Source/Core/ComputeProperty.cpp:29-70`) -- ver o próprio
//     doc-comment do `resolve_length_px()` no value_compute.hpp pra fórmula exata por unidade que
//     este teste pina. `em`/`rem` aqui exercitam o funil GERAL, cego-a-ancestral (qualquer
//     propriedade que NÃO seja `font-size`) -- o `test_font_size_em_resolution()` abaixo é a
//     exceção SEPARADA, mais estreita, do `parse_font_size()` pra própria regra pai-vs-
//     elemento/raiz-vs-documento da propriedade font-size, que este teste não toca.
void test_length_resolution_esc4_full_unit_parity() {
  float v = 0.0f;
  LengthUnit u = LengthUnit::Px;

  // (1) em/rem, GENERAL funnel -- ctx.font_size_px (em's own base) and ctx.document_font_size_px
  //     (rem's own base) are DELIBERATELY different values below, so a swap of the two fields
  //     inside resolve_length_px() would fail this test rather than pass by coincidence.
  check(parse_length("2em", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::Em && v == 2.0f,
        "ESC-4: '2em' parses as Em unit, value 2 -- the pre-ESC-4 Invalid case is now Ok");
  check_eq(print_length_px(resolve_length_px(
               v, u, LengthResolveContext{.font_size_px = 10.0f, .document_font_size_px = 999.0f})),
           "20.0000px",
           "ESC-4: 2em * ctx.font_size_px(10) = 20px -- reads font_size_px, NEVER "
           "document_font_size_px(999), proving em is SELF-relative in the general funnel");

  check(parse_length("1.5rem", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::Rem,
        "ESC-4: '1.5rem' parses as Rem unit");
  check_eq(print_length_px(resolve_length_px(
               v, u, LengthResolveContext{.font_size_px = 999.0f, .document_font_size_px = 20.0f})),
           "30.0000px",
           "ESC-4: 1.5rem * ctx.document_font_size_px(20) = 30px -- reads document_font_size_px, "
           "NEVER font_size_px(999), proving rem is DOCUMENT-relative, not self-relative");

  // (2) vw/vh -- viewport-relative, docs/uix-rcss.md section 1's own "viewport as a parameter"
  //     clause (ESC-4's own addition): 320x240 mirrors this repo's own real oracle viewport
  //     (rcss_dump_differential_oracle.cpp's own `engine.attach(&clock, 320, 240)`).
  check(parse_length("50vw", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::Vw,
        "ESC-4: '50vw' parses as Vw unit");
  check_eq(print_length_px(resolve_length_px(v, u, LengthResolveContext{.vp_w_px = 320.0f})),
           "160.0000px", "ESC-4: 50vw * 320 * 0.01 = 160px");

  check(parse_length("50vh", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::Vh,
        "ESC-4: '50vh' parses as Vh unit");
  check_eq(print_length_px(resolve_length_px(v, u, LengthResolveContext{.vp_h_px = 240.0f})),
           "120.0000px", "ESC-4: 50vh * 240 * 0.01 = 120px");

  // (3) Physical (PPI_UNIT) -- `rmlx-subset.md` section 6.3's own ⚠️ note: these ALSO scale with
  //     `dp_ratio` (`Unit.h:62`'s own `DP_SCALABLE_LENGTH = DP | PPI_UNIT`), NOT a fixed CSS 96dpi.
  //     `dp_ratio=1.0` first (the identity case), then the delta test at `dp_ratio=2.0` (`1in` ->
  //     `192px`, not `96px`) -- the one assertion that actually FALSIFIES "CSS 96dpi fixed" rather
  //     than merely being consistent with it (this codebase's own house rule: a boundary needs the
  //     case that would catch the wrong implementation, not just the case the right one produces).
  check(parse_length("1in", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::In,
        "ESC-4: '1in' parses as In unit");
  check_eq(print_length_px(resolve_length_px(v, u, LengthResolveContext{.dp_ratio = 1.0f})),
           "96.0000px", "ESC-4: 1in at dp_ratio=1.0 -> 96px (PixelsPerInch)");
  check_eq(print_length_px(resolve_length_px(v, u, LengthResolveContext{.dp_ratio = 2.0f})),
           "192.0000px",
           "ESC-4: 1in at dp_ratio=2.0 -> 192px, NOT 96px -- physical units scale with dp_ratio "
           "too, falsifying a fixed-96dpi implementation rather than merely being consistent with "
           "one");

  check(parse_length("1cm", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::Cm,
        "ESC-4: '1cm' parses as Cm unit");
  check_eq(print_length_px(resolve_length_px(v, u, LengthResolveContext{.dp_ratio = 1.0f})),
           "37.7953px", "ESC-4: 1cm at dp_ratio=1.0 -> 96 * (1/2.54)");

  check(parse_length("1mm", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::Mm,
        "ESC-4: '1mm' parses as Mm unit");
  check_eq(print_length_px(resolve_length_px(v, u, LengthResolveContext{.dp_ratio = 1.0f})),
           "3.7795px", "ESC-4: 1mm at dp_ratio=1.0 -> 96 * (1/25.4)");

  check(parse_length("1pt", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::Pt,
        "ESC-4: '1pt' parses as Pt unit");
  check_eq(print_length_px(resolve_length_px(v, u, LengthResolveContext{.dp_ratio = 1.0f})),
           "1.3333px", "ESC-4: 1pt at dp_ratio=1.0 -> 96 * (1/72)");

  check(parse_length("1pc", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::Pc,
        "ESC-4: '1pc' parses as Pc unit");
  check_eq(print_length_px(resolve_length_px(v, u, LengthResolveContext{.dp_ratio = 1.0f})),
           "16.0000px", "ESC-4: 1pc at dp_ratio=1.0 -> 96 * (1/6)");

  // (4) Case-insensitivity -- ESC-4's own measured parity side effect: the pin's own
  //     `StringUtilities::ToLower` on the unit half is unconditional (`PropertyParserNumber.cpp:58`).
  //     Pre-ESC-4 this function was case-SENSITIVE (an `ends_with`-per-unit chain), rejecting both.
  check(parse_length("10PX", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::Px && v == 10.0f,
        "ESC-4: '10PX' (uppercase suffix) now accepted, case-insensitive per the pin");
  check(parse_length("1IN", &v, &u) == ValueComputeStatus::Ok && u == LengthUnit::In && v == 1.0f,
        "ESC-4: '1IN' (uppercase suffix) now accepted too");

  // (5) Fail-high, unchanged discipline extended to the 9 new units.
  check(parse_length("10q", &v, &u) == ValueComputeStatus::Invalid,
        "ESC-4: '10q' -- unrecognised suffix, Invalid, never guessed");
  check(parse_length("10 px", &v, &u) == ValueComputeStatus::Invalid,
        "ESC-4: '10 px' (space before the unit) -- Invalid, a DOCUMENTED divergence from the pin "
        "(which accepts it -- strtof does not require a whole-string match); this module's own "
        "parse_float_token() does, per value_compute.hpp's own parse_length() doc-comment, and "
        "this item does not change that");
  check(parse_length("10x", &v, &u) == ValueComputeStatus::Invalid,
        "ESC-4: '10x' -- Unit::X is NOT part of Unit::LENGTH (Unit.h:58), so parse_length() must "
        "never accept it; see parse_resolution() below for x's own separate, narrow home");

  // (6) `x`/resolution -- kept OUT of parse_length()/LengthUnit entirely (Unit::X is not LENGTH), a
  //     standalone function whose only real pin-side consumer is `@spritesheet`'s own
  //     `resolution: <n>x` (not implemented yet, owned by ESC-14) -- passthrough, no scaling, and
  //     (unlike parse_length) NO unitless-zero exception, because the pin's own
  //     `PropertyParserNumber(Unit::X)` constructor call passes no `zero_unit` argument.
  float res = 0.0f;
  check(parse_resolution("2x", &res) == ValueComputeStatus::Ok && res == 2.0f,
        "ESC-4: parse_resolution('2x') -- Ok, passthrough, value 2.0");
  check(parse_resolution("2X", &res) == ValueComputeStatus::Ok && res == 2.0f,
        "ESC-4: parse_resolution('2X') -- case-insensitive too, same as parse_length()'s own rule");
  check(parse_resolution("10px", &res) == ValueComputeStatus::Invalid,
        "ESC-4: parse_resolution('10px') -- Invalid, 'px' is not the resolution unit");
  check(parse_resolution("2", &res) == ValueComputeStatus::Invalid,
        "ESC-4: parse_resolution('2') -- Invalid, no unitless exception for resolution (the pin's "
        "own PropertyParserNumber(Unit::X) constructor passes no zero_unit, PropertyParserNumber.h:14)");
  check(parse_resolution("", &res) == ValueComputeStatus::Invalid,
        "ESC-4: parse_resolution('') -- Invalid, empty input");
}

// ---------------------------------------------------------------------------
// EN: `UIX-EM-UNIT` -- `font-size`'s own `em` resolution. `UIX-ORACLE-MEDICAO`'s own residuo B
//     measured this LIVE against `fonteng_sup_scene.rml` (`.sup{font-size:0.7em}` over
//     `body{font-size:64px}`): side A (real RmlUi) prints `44.8000px` (`0.7 * 64`, exact), side B
//     (this module, before this item) fell back to the registry's own `12.0000px` initial value --
//     NOT a coincidence that happened to look plausible, the documented `Invalid`-from-`parse_length`
//     fail-high path `canonical_print`'s own one-shot retry always takes for a value shape this
//     module could not resolve at all. `parse_font_size()` closes that specific gap WITHOUT widening
//     `parse_length`/`resolve_length_px`'s own general, ancestor-blind signature (the test just
//     above still asserts `parse_length("1em", ...)` stays `Invalid` -- unchanged, zero ripple to
//     `box-shadow`/`drop-shadow`/`blur`/`transform`'s own length arguments, which the corpus's own
//     census, `docs/uix-rcss-censo.md`, never exercises with `em` -- the ONE real corpus occurrence
//     of `em` is this exact `font-size` declaration). Chained case (`parent 200px -> child 0.5em ->
//     grandchild 0.25em`) exercises the property this bug hid behind: each level's OWN resolved px
//     becomes the NEXT level's `parent_font_size_px` argument -- calling `parse_font_size()` twice in
//     a row, feeding the first call's own output into the second call's own input, is this test's
//     own proof that inheritance chains correctly rather than each level re-reading some fixed
//     ancestor.
// PT: `UIX-EM-UNIT` -- resolução do próprio `em` do `font-size`. O próprio resíduo B da
//     `UIX-ORACLE-MEDICAO` mediu isto AO VIVO contra o `fonteng_sup_scene.rml`
//     (`.sup{font-size:0.7em}` sobre `body{font-size:64px}`): o lado A (RmlUi real) imprime
//     `44.8000px` (`0.7 * 64`, exato), o lado B (este módulo, antes deste item) caía pro próprio
//     `12.0000px` de valor inicial de registro -- NÃO uma coincidência que calhou de parecer
//     plausível, é o próprio caminho fail-high `Invalid`-vindo-do-`parse_length` que o próprio retry
//     de um-tiro do `canonical_print` sempre toma pra uma forma de valor que este módulo não
//     conseguia resolver de jeito nenhum. `parse_font_size()` fecha exatamente essa lacuna SEM
//     alargar a própria assinatura geral, cega-a-ancestral, do `parse_length`/`resolve_length_px` (o
//     teste logo acima ainda afirma que `parse_length("1em", ...)` fica `Invalid` -- inalterado, zero
//     ondulação pros próprios argumentos de comprimento de `box-shadow`/`drop-shadow`/`blur`/
//     `transform`, que o próprio censo do corpus, `docs/uix-rcss-censo.md`, nunca exercita com `em`
//     -- a ÚNICA ocorrência real de `em` no corpus é exatamente esta declaração de `font-size`).
//     Caso encadeado (`pai 200px -> filho 0.5em -> neto 0.25em`) exercita exatamente a propriedade
//     que este bug escondia: o próprio px resolvido de CADA nível vira o argumento
//     `parent_font_size_px` do PRÓXIMO nível -- chamar `parse_font_size()` duas vezes seguidas,
//     alimentando a própria saída da primeira chamada na própria entrada da segunda, é a própria
//     prova deste teste de que a herança encadeia corretamente em vez de cada nível reler algum
//     ancestral fixo.
void test_font_size_em_resolution() {
  float px = 0.0f;
  constexpr LengthResolveContext kCtx1{}; // dp_ratio=1.0, others 0 -- irrelevant to em/px/dp cases.

  // (1) The exact oracle-measured case: parent 64px, child 0.7em -> 44.8px, never the 12px fallback.
  check(parse_font_size("0.7em", /*parent_font_size_px=*/64.0f, kCtx1, &px) ==
            ValueComputeStatus::Ok,
        "0.7em over a 64px parent resolves Ok, not Invalid");
  check_eq(print_length_px(px), "44.8000px",
           "0.7em over a 64px parent is 44.8px, matching UIX-ORACLE-MEDICAO's own measured side-A "
           "value byte-exact -- never 12.0000px, the registry-initial coincidence this bug produced");

  // (2) Chained: parent 200px (absolute) -> child 0.5em (=100px) -> grandchild 0.25em relative to
  //     the CHILD's own resolved 100px (=25px), never relative to the 200px grandparent or to some
  //     fixed context -- this is "where inheritance goes wrong hides", per this item's own brief.
  float child_px = 0.0f;
  check(parse_font_size("0.5em", /*parent_font_size_px=*/200.0f, kCtx1, &child_px) ==
            ValueComputeStatus::Ok,
        "chained step 1: 0.5em over a 200px parent resolves Ok");
  check_eq(print_length_px(child_px), "100.0000px", "chained step 1: 0.5 * 200 = 100px");

  float grandchild_px = 0.0f;
  check(parse_font_size("0.25em", /*parent_font_size_px=*/child_px, kCtx1, &grandchild_px) ==
            ValueComputeStatus::Ok,
        "chained step 2: 0.25em resolves Ok against the PREVIOUS step's own resolved px, not the "
        "original 200px grandparent");
  check_eq(print_length_px(grandchild_px), "25.0000px",
           "chained step 2: 0.25 * 100 = 25px -- if this read 50px (0.25 * 200), the chain would be "
           "silently skipping a generation, exactly the inheritance bug this test is designed to "
           "catch");

  // (3) Absolute units still resolve exactly as parse_length()/resolve_length_px() already do --
  //     parse_font_size() delegates to them for px/dp/unitless-zero, never re-deriving that logic.
  //     `parent_font_size_px` is irrelevant here (a hostile/nonsensical value proves it is ignored).
  check(parse_font_size("16px", /*parent_font_size_px=*/999.0f,
                        LengthResolveContext{.dp_ratio = 2.0f}, &px) == ValueComputeStatus::Ok,
        "16px (absolute) resolves Ok regardless of parent_font_size_px");
  check_eq(print_length_px(px), "16.0000px", "16px ignores both parent_font_size_px and dp_ratio");

  check(parse_font_size("2dp", /*parent_font_size_px=*/999.0f,
                        LengthResolveContext{.dp_ratio = 8.0f}, &px) == ValueComputeStatus::Ok,
        "2dp (dp_ratio-relative, not font-size-relative) resolves Ok");
  check_eq(print_length_px(px), "16.0000px", "2dp at dp_ratio=8 resolves to 16px, parent ignored");

  check(parse_font_size("0", /*parent_font_size_px=*/999.0f, kCtx1, &px) == ValueComputeStatus::Ok,
        "unitless 0 still accepted (CSS's own zero-length convention), parent ignored");

  // (4) `ESC-4` -- `rem`: the SAME hole `em` had before this repo's own `UIX-EM-UNIT` item, now
  //     CLOSED (not merely diagnosed) by this item, per `rmlx-subset.md` section 6.3's own "full
  //     parity, not the measured minimum" decision (zero corpus `rem` occurrences was never a
  //     reason to skip it -- see `docs/rmlx-subset.md` section 7, the líder's amendment
  //     generalizing that precedent to every axis this project scopes). `rem` reads
  //     `ctx.document_font_size_px`, a DIFFERENT field than `em`'s own `parent_font_size_px`
  //     PARAMETER -- the two values below are deliberately UNEQUAL, so a copy-paste bug that reused
  //     `parent_font_size_px` for `rem` too would fail this specific assertion rather than pass by
  //     coincidence (`ComputeFontsize`'s own real split, `ComputeProperty.cpp:100-111`: `em` uses
  //     `parent_values->font_size()`, `rem` uses `document_values->font_size()`, two different
  //     `Style::ComputedValues*` sources).
  // PT: `ESC-4` -- `rem`: o MESMO buraco que o `em` tinha antes do próprio item `UIX-EM-UNIT` deste
  //     repo, agora FECHADO (não só diagnosticado) por este item, pela própria decisão "paridade
  //     completa, não o mínimo medido" da seção 6.3 do `rmlx-subset.md` (zero ocorrências de `rem`
  //     no corpus nunca foi motivo pra pular -- ver a seção 7 do `docs/rmlx-subset.md`, a própria
  //     emenda do líder generalizando esse precedente pra todo eixo que este projeto escopa). `rem`
  //     lê `ctx.document_font_size_px`, um campo DIFERENTE do próprio PARÂMETRO
  //     `parent_font_size_px` do `em` -- os dois valores abaixo são deliberadamente DESIGUAIS, então
  //     um bug de copiar-colar que reusasse `parent_font_size_px` também pro `rem` falharia esta
  //     asserção específica em vez de passar por coincidência (a própria separação real do
  //     `ComputeFontsize`, `ComputeProperty.cpp:100-111`: `em` usa `parent_values->font_size()`,
  //     `rem` usa `document_values->font_size()`, duas fontes `Style::ComputedValues*` diferentes).
  check(parse_font_size("1rem", /*parent_font_size_px=*/999.0f,
                        LengthResolveContext{.document_font_size_px = 64.0f}, &px) ==
            ValueComputeStatus::Ok,
        "ESC-4: 1rem now resolves Ok (was Invalid pre-ESC-4) -- reads document_font_size_px, "
        "never parent_font_size_px");
  check_eq(print_length_px(px), "64.0000px",
           "ESC-4: 1rem * document_font_size_px(64) = 64px, NOT 999px (parent_font_size_px, "
           "deliberately different from document_font_size_px above to catch a field mix-up)");
  check(parse_font_size("0.7rem", /*parent_font_size_px=*/999.0f,
                        LengthResolveContext{.document_font_size_px = 64.0f}, &px) ==
            ValueComputeStatus::Ok,
        "ESC-4: 0.7rem also resolves Ok, not misparsed as '0.7r' + implicit em -- the reverse-scan "
        "suffix table matches the WHOLE 'rem' string exactly, never a leftover 'r' from an em match");
  check_eq(print_length_px(px), "44.8000px", "ESC-4: 0.7rem * 64 = 44.8px");

  // `ESC-4` -- non-finite `ctx.document_font_size_px` is fail-high, mirroring the pre-existing
  // `isfinite(parent_font_size_px)` guard the `em` branch already had (this module's own house
  // discipline: a caller-supplied ancestor value this function cannot itself validate the shape of
  // must still not silently propagate a NaN/Inf multiplication result).
  check(parse_font_size(
            "1rem", /*parent_font_size_px=*/64.0f,
            LengthResolveContext{.document_font_size_px = std::numeric_limits<float>::quiet_NaN()},
            &px) == ValueComputeStatus::Invalid,
        "ESC-4: 1rem with a non-finite ctx.document_font_size_px: Invalid, never a silent NaN "
        "propagated into the caller");

  // `ESC-4` -- the widened `parse_length()` now recognises `vw`/`vh`/physical suffixes too, so
  // `parse_font_size()`'s own delegation to `resolve_length_px()` for every non-em/rem unit (the
  // pin's own "font-relative lengths handled above, other lengths handled as normal" fallthrough,
  // `ComputeFontsize`'s own closing comment, `ComputeProperty.cpp:116-117`) now covers `font-size:
  // 50vw`/`font-size: 1in`, etc, not just px/dp as before this item -- `parent_font_size_px` stays
  // irrelevant for these too, same reasoning as the px/dp cases in (3) above.
  check(parse_font_size("50vw", /*parent_font_size_px=*/999.0f,
                        LengthResolveContext{.vp_w_px = 320.0f}, &px) == ValueComputeStatus::Ok,
        "ESC-4: font-size: 50vw now resolves Ok (delegates to the widened resolve_length_px)");
  check_eq(print_length_px(px), "160.0000px", "ESC-4: 50vw * 320 * 0.01 = 160px");
  check(parse_font_size("1in", /*parent_font_size_px=*/999.0f,
                        LengthResolveContext{.dp_ratio = 2.0f}, &px) == ValueComputeStatus::Ok,
        "ESC-4: font-size: 1in now resolves Ok too");
  check_eq(print_length_px(px), "192.0000px",
           "ESC-4: 1in at dp_ratio=2.0 -> 192px, same PPI_UNIT formula as the general funnel");

  // (5) Malformed shapes stay Invalid -- same fail-high discipline as parse_length(). `%` stays
  //     OUT OF SCOPE for this item on purpose (docs/rmlx-subset.md section 6.3's own closing note:
  //     "font-size é LEN sem %" -- registry domain, not widened here).
  check(parse_font_size("", /*parent_font_size_px=*/64.0f, kCtx1, &px) ==
            ValueComputeStatus::Invalid,
        "empty raw text: Invalid");
  check(parse_font_size("em", /*parent_font_size_px=*/64.0f, kCtx1, &px) ==
            ValueComputeStatus::Invalid,
        "bare 'em' with no leading number: Invalid, never silently 0 or 1");
  check(parse_font_size("bogus", /*parent_font_size_px=*/64.0f, kCtx1, &px) ==
            ValueComputeStatus::Invalid,
        "unrecognised unit: Invalid");
  check(parse_font_size("50%", /*parent_font_size_px=*/64.0f, kCtx1, &px) ==
            ValueComputeStatus::Invalid,
        "ESC-4: '50%' -- Invalid, out of THIS item's own scope by design (font-size's own registry "
        "domain is LEN, not LengthPercent; rmlx-subset.md section 6.3's own closing note)");
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
  check(compute_transform_list("rotate(0deg)", LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
        "corpus instance 1 computes Ok");
  check_eq(out, "rotate(0.0000)", "corpus instance 1: rotate(0deg)");

  check(compute_transform_list("rotate(360deg)", LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
        "corpus instance 2 computes Ok");
  check_eq(out, "rotate(360.0000)", "corpus instance 2: rotate(360deg)");

  check(compute_transform_list("translate(10px, 20px) rotate(45deg)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
            ValueComputeStatus::Ok,
        "translate+rotate computes Ok");
  check_eq(out, "translate(10.0000px;20.0000px)|rotate(45.0000)",
           "translate+rotate, whitespace-adjacent multi-function CSS transform-list syntax");

  check(compute_transform_list("scale(1.5, 2)", LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
        "scale computes Ok");
  check_eq(out, "scale(1.5000;2.0000)", "scale: plain numbers, not lengths");

  check(compute_transform_list("", LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
        "empty transform value computes Ok");
  check_eq(out, "none", "empty transform value prints 'none'");

  check(compute_transform_list("none", LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
        "'none' transform value computes Ok");
  check_eq(out, "none", "'none' transform value prints 'none'");

  check(compute_transform_list("matrix3d(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
            ValueComputeStatus::Invalid,
        "matrix3d: out-of-scope per section 13, fail-high -- Invalid, never a guess");

  check(compute_transform_list("translate(10px, 20px) matrix3d(...)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
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

  check(compute_decorator_list("blur(4px), not-a-real-function(1,2,3)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
            ValueComputeStatus::Invalid,
        "unknown function anywhere in the list invalidates the WHOLE property -- blur() does NOT "
        "survive alone (UIX-RCSS-ERRATA-2, Finding C)");

  check(compute_decorator_list("", LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
        "empty decorator value computes Ok");
  check_eq(out, "none", "empty decorator value prints 'none'");

  check(compute_decorator_list("none", LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
        "'none' decorator value computes Ok");
  check_eq(out, "none", "'none' decorator value prints 'none'");

  check(compute_decorator_list("radial-gradient(ellipse at 50% 50%, #fff, #000)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
            ValueComputeStatus::Invalid,
        "ellipse: section 13's own out-of-scope clause, fail-high -- the whole property drops");

  check(compute_decorator_list("horizontal-gradient(#000f #0000)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
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
  //     before this errata. Same grammar as horizontal-gradient.
  //     ⚠️ CORRECTED by `UIX-GRADIENT-ALFA` (this item): this check used to assert a lossy
  //     premultiply/un-premultiply round-trip here (`#22D3EE26` -> `#21d0ea26`), reasoning by false
  //     analogy to box-shadow/gradient-stop colors -- that assertion itself encoded the residuo C
  //     bug `UIX-ORACLE-MEDICAO` measured, not correct behaviour. Verified by reading
  //     `DecoratorGradient.h`/`.cpp` directly (see `compute_two_stop_straight_gradient()`'s own
  //     header comment in value_compute.cpp for the full derivation): `horizontal-gradient`/
  //     `vertical-gradient` use plain `Colourb`, never premultiplied by upstream -- straight
  //     passthrough is correct, `#22D3EE26` prints as `#22d3ee26` (lowercased, unchanged), never
  //     `#21d0ea26`.
  // PT: A própria linha recém-acrescentada da seção 9.2 do docs/uix-rcss.md (UIX-RCSS-ERRATA-2) --
  //     107 ocorrências no corpus, a função de decorator mais usada do corpus inteiro, faltando por
  //     inteiro antes desta errata. Mesma gramática do horizontal-gradient.
  //     ⚠️ CORRIGIDO pela `UIX-GRADIENT-ALFA` (este item): esta checagem alegava uma ida-e-volta
  //     com perda de premultiplicar/des-premultiplicar aqui (`#22D3EE26` -> `#21d0ea26`),
  //     raciocinando por falsa analogia com cores de box-shadow/stop-de-gradiente -- a própria
  //     asserção codificava o bug do resíduo C que a `UIX-ORACLE-MEDICAO` mediu, não o
  //     comportamento correto. Verificado lendo direto o `DecoratorGradient.h`/`.cpp` (ver o
  //     próprio comentário de cabeçalho do `compute_two_stop_straight_gradient()` no
  //     value_compute.cpp pra derivação completa): `horizontal-gradient`/`vertical-gradient` usam
  //     `Colourb` plano, nunca premultiplicado pelo upstream -- passthrough reto é o correto,
  //     `#22D3EE26` imprime como `#22d3ee26` (lowercase, inalterado), nunca `#21d0ea26`.
  check(compute_decorator_list("vertical-gradient(#22D3EE26 #ffffffff)", LengthResolveContext{.dp_ratio = 1.0f}, &out) ==
            ValueComputeStatus::Ok,
        "vertical-gradient computes Ok");
  check_eq(out, "vertical-gradient(#22d3ee26;#ffffffff)",
           "vertical-gradient NEVER round-trips -- straight passthrough, lowercased only, never "
           "the #21d0ea26 this test used to (wrongly) assert");

  check(compute_decorator_list("image( runes-base.png )", LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
        "image() computes Ok");
  check_eq(out, "image(runes-base.png)", "image(): bare url, whitespace trimmed");

  check(compute_decorator_list(
            "polygon(6, radial-gradient(circle at 40% 35%, #F0D98C, #C9A24B 55%, #7A5A2E 100%))",
            LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Ok,
        "polygon() with nested radial-gradient fill computes Ok");
  check_eq(out,
           "polygon(6.0000;radial-gradient(40.0000%;35.0000%;#f0d98cff:0.0000%;#c9a24bff:55.0000%;"
           "#7a5a2eff:100.0000%);0.0000)",
           "polygon()'s own recursive nested-gradient <fill> argument, docs/effects.md's own "
           "documented example -- the nested gradient's OWN stop colors are also premultiplied "
           "(all alpha=ff here, so a no-op, same reasoning as the 15.3 reuse above)");

  check(compute_decorator_list("polygon(2, #fff)", LengthResolveContext{.dp_ratio = 1.0f}, &out) == ValueComputeStatus::Invalid,
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
  test_gradient_alpha_roundtrip_matches_upstream_storage_type();
  test_color_parsing_all_forms();
  test_color_parsing_esc5_named_color_parity();
  test_color_parsing_esc6_functional_forms();
  test_color_functional_forms_paren_aware_split_esc6();
  test_length_resolution();
  test_length_resolution_esc4_full_unit_parity();
  test_font_size_em_resolution();
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
      "it) | ESC-4 length-unit family: 11 of 11 (px, dp, em, rem, vw, vh, in, cm, mm, pt, pc -- "
      "full parity with the pin's own Unit::LENGTH, up from 2 pre-ESC-4) | x/resolution: 1 "
      "standalone function (parse_resolution), deliberately NOT part of LengthUnit (Unit::X is not "
      "Unit::LENGTH) | ESC-5 named-color family: 19 of 19 (full parity with the pin's own "
      "html_colours table, up from 3 pre-ESC-5, case-insensitive) | color functional forms: 8 of 8 "
      "(rgb/rgba/hsl/hsla/lab/lch/oklab/oklch -- ESC-6, transcribed function-for-function from "
      "PropertyParserColour.cpp, 2 briefing anchors corrected by independent Python oracle + "
      "actually-compiled verification: oklab(1 0 0)=#fefefeff not #ffffffff, "
      "lab(50 40 60/0.5)=Ok not Invalid -- see test_color_parsing_esc6_functional_forms's own "
      "header)\n");

  if (g_failures > 0) {
    std::fprintf(stderr, "value_compute_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("value_compute_sanity: PASS");
  return 0;
}
