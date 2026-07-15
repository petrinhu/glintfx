// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_fallback_sanity -- L1.20-FONTFLIP (FT-F4) INDEPENDENT ADVERSARIAL GATE for the
//     per-glyph font fallback (FontEngineOwn::fallback_faces_/BakeGlyph()/RasterizeGlyph()'s `src`
//     parameter, src/font_engine_own.cpp/.hpp, commit 957505b). Written by the QA reviewer -- NOT
//     the implementer -- as its own gate, not a rubber stamp. Executes the real GL/RmlUi pipeline
//     under Xvfb (statistical ink oracle + numeric width oracle, the two techniques this suite
//     already established in fonteng_lazy_sanity.cpp/fonteng_own_kern_sanity.cpp), never merely
//     reads source and asserts by inspection. See tests/fonteng_fallback_scene.rml's own header for
//     the fixture layout (three @font-face blocks, exact registration order, why it yields
//     fallback_faces_ == [OrderFbFirst(PixelOperatorMono, incapable), Fallback(OpenSans,
//     capable)]) -- this file assumes that document.
//
//     THE SEVEN CRITERIA THIS FILE (+ its two siblings) GATE, per the review brief:
//
//     [1] NON-TOFU FALLBACK (Greek + superscript-2). `greek_omega` (Ω, U+03A9) and
//         `greek_alpha_lo` (α, U+03B1) -- both confirmed absent from PixelOperatorMono's cmap by a
//         throwaway probe run during this review, both confirmed present in Open Sans -- plus
//         `sup2` (², U+00B2, the EXACT codepoint the GusWorld dev's "a²+b²=c²" problem needs) must
//         all render real ink through "Primary" (PixelOperatorMono). `ctrl_ascii` ("Axg", plain
//         ASCII PixelOperatorMono already covers) is the pipeline-sanity control -- if IT renders
//         blank, the whole render pipeline is broken, not merely the fallback path.
//     [1-contra] CONTRA-PROVA. The SAME four ids, run in a SEPARATE process
//         (fonteng_fallback_contraprova_sanity, its own .cpp/binary -- fallback_faces_ is
//         process-lifetime shared state, so "no fallback ever registered" cannot be isolated by a
//         second document loaded into this SAME process) against a document with "Primary" ONLY,
//         no -rmlui-fallback-face block anywhere: ctrl_ascii must still render, the other three
//         MUST render zero ink. This is what proves [1]'s non-blank result is attributable to the
//         fallback mechanism specifically, not some unrelated change.
//     [2] UPEM-MISMATCH SCALE CORRECTNESS (the review brief's named CRITICAL bug class).
//         `greek_omega` (Ω rasterised on the "Primary" FaceInstance -- upem 1600 -- via the
//         fallback path, so RasterizeGlyph()'s `src` is the "Fallback"/Open Sans LoadedFace, upem
//         2048) is compared against `omega_direct` (the SAME Ω codepoint, SAME 48px pixel_size, but
//         rendered through the "Fallback" family used DIRECTLY as ITS OWN instance's primary --
//         RasterizeGlyph() there always used the Open Sans upem too, by construction, independent
//         of this ticket's fallback feature). If RasterizeGlyph()'s scale correctly divides by
//         `src->sfnt.units_per_em` (2048) rather than `inst.face->sfnt.units_per_em` (1600, the
//         PRIMARY's, WRONG for a fallback-sourced glyph), both widths must be near-identical (small
//         independent-rounding noise only). Using the wrong upem would inflate every metric by
//         2048/1600 == 1.28x -- a ~28% width blow-up, unmistakably larger than the tolerance below.
//     [3] ORDER-WALK (partial -- see this file's own header derivation vs. the FULL "first-of-
//         several-CAPABLE-candidates" tie-break, which needs a 3rd distinguishable fixture font
//         this suite does not have). `greek_omega`/`greek_alpha_lo`/`sup2` above all resolve
//         through a TWO-fallback list where the FIRST-registered candidate (`OrderFbFirst`, a
//         SEPARATE registration of PixelOperatorMono's own bytes) is INCAPABLE of every one of
//         these codepoints and the SECOND (`Fallback`/Open Sans) is the only capable one -- so [1]
//         passing already proves BakeGlyph()'s walk does not stop/fail merely because the FIRST
//         fallback candidate lacked the glyph; it correctly continues to the next. The residual gap
//         -- does the walk pick the FIRST of two candidates that BOTH have the glyph, specifically
//         -- is NOT independently black-box-provable with only PixelOperatorMono (Latin-only) and
//         Open Sans (the only Greek/²-capable file) as fixtures: PixelOperatorMono can never be a
//         SECOND capable candidate for a Greek/² codepoint, so no combination of these two files
//         produces two visually/metrically DISTINGUISHABLE capable candidates to tell "first" from
//         "last" apart by rendered output. Confirmed INSTEAD by reading BakeGlyph()
//         (font_engine_own.cpp:1179-1189): the loop walks `fallback_faces_` in registration order
//         and `break`s on the FIRST non-zero `gid`, matching the pinned RmlUi source's own
//         FontFaceHandleDefault.cpp fallback walk this ticket mirrors -- same class of allowance
//         fonteng_lazy_sanity.cpp's own criterion [4] already used for this suite's negative-cache
//         re-attempt guarantee. Documented here as a residual, honestly-scoped gap, not silently
//         assumed.
//     [4] CROSS-FACE KERN GATE == NO adjustment ("sem kern"). `mixed_seq` ("AΩB" -- 'A'/'B' from
//         Primary directly, 'Ω' resolved via fallback to Open Sans) must measure width == the SUM
//         of `mixed_a` + `greek_omega` + `mixed_b`'s own independently-measured widths, within a
//         tight rounding-carry tolerance -- NOT tightened by any kern adjustment, because BOTH of
//         `mixed_seq`'s internal glyph-pair boundaries (A|Ω and Ω|B) cross src_face (Primary vs.
//         Fallback), and IterateGlyphs() only calls glx_sfnt_kern() when
//         `prev->src_face == cur->src_face` (font_engine_own.cpp ~1366+) -- a kern table is indexed
//         by gid WITHIN one font file, so applying one across files would look up garbage. Mesh
//         (render-time quads) is also checked not to spill past `mixed_seq`'s own layout box (same
//         right/bottom-margin ink technique fonteng_lazy_sanity.cpp's criterion [3b] uses).
//     [5] SAME-FACE KERN STILL APPLIES THROUGH THE "Fallback" FAMILY ("com kern"). `av_fallback`
//         ("AV", rendered via the "Fallback" family used DIRECTLY, i.e. NOT through the per-glyph
//         fallback-routing mechanism itself, but exercising the SAME general width==mesh
//         invariant this file's [4] checks) must measure MEASURABLY less than
//         `a_fallback`+`v_fallback`'s summed widths -- Open Sans' own `kern` table is known to have
//         kern(A,V) == -82 FUnits @ upem 2048 (the exact fact fonteng_own_kern_sanity.cpp's own
//         probe already established, reused here at the SAME 48px font-size for a directly
//         comparable >= 2px floor). Together, [4] (kern forced to 0 across faces) and [5] (kern
//         non-zero, mesh still matches width) is this file's answer to the review brief's "largura
//         == mesh para string mista... com e sem kern".
//     [6] NEGATIVE-CACHE, NO CRASH. `cjk_missing` (U+4E00, 一) -- NEITHER Primary NOR any
//         registered fallback contains this glyph (confirmed absent from both fixture fonts' cmaps
//         by the same throwaway probe) -- must render ZERO ink, `found`, no crash (a crash fails
//         the whole binary, caught structurally by ctest).
//     [7] REGRESSION + SANITIZER. Not this file's job directly -- REGRESSION is gated by re-running
//         the pre-existing fonteng_* ctest targets unmodified alongside this new one (`ctest -R
//         fonteng`, see the review report for the captured run); SANITIZER is this SAME binary
//         (plus its two siblings) built under `-DGLINTFX_SANITIZE=ON` and run again (see the review
//         report). This file's OWN contribution to the sanitizer ask: after the checks above, it
//         reloads fonteng_fallback_scene.rml kReloads=10 further times in the SAME UiLayer session
//         (exercising RegisterFace()'s dedup path across all THREE @font-face blocks -- including
//         the two fallback ones -- and BakeGlyph()'s fallback-resolution/atlas-growth path
//         repeatedly under ASan/LSan/UBSan) and asserts
//         glintfx::own_font_engine_debug_registered_face_count() -- the SAME src-internal DEBUG
//         hook fonteng_dedup_sanity.cpp already uses -- is EXACTLY 3 (one LoadedFace per distinct
//         (family, blob) pair: "Primary", "OrderFbFirst", "Fallback" -- all three DIFFERENT family
//         strings, so none dedup against each other despite two sharing byte-identical content)
//         after the FIRST load and STAYS EXACTLY 3 after all 10 reloads, plus a final-reload ink
//         re-check on `greek_omega` proving the deduped fallback path still renders correctly.
//
//     METHOD (shared with fonteng_lazy_sanity.cpp/fonteng_own_kern_sanity.cpp): one hidden GLFW
//     host window provides the GL context; UiLayer attaches embed-mode (GusWorld's own consumption
//     shape); two warm-up update+render cycles before every measurement; glReadPixels +
//     flip_to_top_down to agree with get_element_box()'s top-left/y-down convention;
//     count_ink_rect() counts near-white pixels in a box. Deterministic under llvmpipe -- no
//     sleeps, no retries, no tolerance beyond the explicitly-derived budgets above.
// PT: fonteng_fallback_sanity -- GATE ADVERSARIAL INDEPENDENTE do L1.20-FONTFLIP (FT-F4) pro
//     fallback de fonte por-glyph (FontEngineOwn::fallback_faces_/BakeGlyph()/o parâmetro `src` de
//     RasterizeGlyph(), src/font_engine_own.cpp/.hpp, commit 957505b). Escrito pelo revisor de QA
//     -- NÃO o implementador -- como gate próprio, não carimbo. Executa o pipeline real GL/RmlUi
//     sob Xvfb (oráculo estatístico de tinta + oráculo numérico de largura, as duas técnicas que
//     esta suíte já estabeleceu em fonteng_lazy_sanity.cpp/fonteng_own_kern_sanity.cpp), nunca só
//     lê o fonte e afirma por inspeção. Ver o cabeçalho próprio de tests/fonteng_fallback_scene.rml
//     pro layout de fixture (três blocos @font-face, ordem de registro exata, motivo de produzir
//     fallback_faces_ == [OrderFbFirst(PixelOperatorMono, incapaz), Fallback(OpenSans, capaz)]) --
//     este arquivo assume aquele documento.
//
//     OS SETE CRITÉRIOS QUE ESTE ARQUIVO (+ seus dois irmãos) GATEIAM, conforme a tarefa de review:
//
//     [1] NÃO-TOFU DO FALLBACK (Grego + superscript-2). `greek_omega` (Ω, U+03A9) e
//         `greek_alpha_lo` (α, U+03B1) -- ambos confirmados ausentes do cmap da PixelOperatorMono
//         por uma sonda descartável rodada durante este review, ambos confirmados presentes na
//         Open Sans -- mais `sup2` (², U+00B2, o codepoint EXATO que o problema "a²+b²=c²" do dev
//         do GusWorld precisa) todos precisam renderizar tinta real pela "Primary"
//         (PixelOperatorMono). `ctrl_ascii` ("Axg", ASCII plano que a PixelOperatorMono já cobre) é
//         o controle de sanidade de pipeline -- se ELE renderizar em branco, o pipeline de render
//         inteiro está quebrado, não só o caminho de fallback.
//     [1-contra] CONTRA-PROVA. Os mesmos quatro ids, rodados num processo SEPARADO
//         (fonteng_fallback_contraprova_sanity, .cpp/binário próprio -- fallback_faces_ é estado
//         compartilhado de vida-de-processo, então "nenhum fallback jamais registrado" não pode
//         ser isolado por um 2º documento carregado NESTE MESMO processo) contra um documento só
//         com "Primary", sem bloco -rmlui-fallback-face em lugar nenhum: ctrl_ascii ainda precisa
//         renderizar, os outros três PRECISAM renderizar tinta zero. Isto é o que prova que o
//         resultado não-branco do [1] é atribuível ao mecanismo de fallback especificamente, não a
//         alguma mudança não-relacionada.
//     [2] CORRETUDE DE ESCALA NO MISMATCH-DE-UPEM (a classe de bug CRÍTICA nomeada pela tarefa de
//         review). `greek_omega` (Ω rasterizado na FaceInstance "Primary" -- upem 1600 -- via o
//         caminho de fallback, então o `src` do RasterizeGlyph() é a LoadedFace
//         "Fallback"/Open Sans, upem 2048) é comparado contra `omega_direct` (o MESMO codepoint Ω,
//         MESMO pixel_size 48px, mas renderizado pela família "Fallback" usada DIRETO como a
//         primária da PRÓPRIA instância -- o RasterizeGlyph() ali sempre usou o upem da Open Sans
//         também, por construção, independente desta feature de fallback). Se a escala do
//         RasterizeGlyph() de fato divide por `src->sfnt.units_per_em` (2048) em vez de
//         `inst.face->sfnt.units_per_em` (1600, o da PRIMÁRIA, ERRADO pra um glyph vindo de
//         fallback), as duas larguras precisam ser quase-idênticas (só ruído pequeno de
//         arredondamento independente). Usar o upem errado infla toda métrica por 2048/1600 ==
//         1.28x -- um estouro de largura de ~28%, inconfundivelmente maior que a tolerância abaixo.
//     [3] CAMINHADA DE ORDEM (parcial -- ver a derivação própria do cabeçalho deste arquivo vs. o
//         tie-break COMPLETO "primeiro-de-vários-candidatos-CAPAZES", que precisa de uma 3ª fonte
//         de fixture distinguível que esta suíte não tem). `greek_omega`/`greek_alpha_lo`/`sup2`
//         acima todos resolvem através de uma lista de DOIS fallbacks onde o candidato registrado
//         PRIMEIRO (`OrderFbFirst`, um registro SEPARADO dos próprios bytes da PixelOperatorMono) é
//         INCAPAZ de todo um destes codepoints e o SEGUNDO (`Fallback`/Open Sans) é o único capaz
//         -- então o [1] passar já prova que a caminhada do BakeGlyph() não para/falha só porque o
//         PRIMEIRO candidato de fallback não tinha o glyph; ela corretamente continua pro próximo.
//         A lacuna residual -- se a caminhada escolhe o PRIMEIRO de dois candidatos que AMBOS têm o
//         glyph, especificamente -- NÃO é independentemente provável em caixa-preta só com
//         PixelOperatorMono (Latin-only) e Open Sans (o único arquivo capaz de Grego/²) como
//         fixture: a PixelOperatorMono nunca pode ser um SEGUNDO candidato capaz pra um codepoint
//         Grego/², então nenhuma combinação destes dois arquivos produz dois candidatos capazes
//         visual/metricamente DISTINGUÍVEIS pra separar "primeiro" de "último" pelo resultado
//         renderizado. Confirmado EM VEZ DISSO lendo o BakeGlyph() (font_engine_own.cpp:1179-1189):
//         o laço caminha `fallback_faces_` em ordem de registro e dá `break` no PRIMEIRO `gid`
//         não-zero, batendo com a própria caminhada de fallback do FontFaceHandleDefault.cpp do
//         source pinado do RmlUi que esta tarefa espelha -- mesma classe de permissão que o próprio
//         critério [4] do fonteng_lazy_sanity.cpp já usou pra garantia de reatentativa do cache
//         negativo desta suíte. Documentado aqui como uma lacuna residual, honestamente escopada,
//         não assumida em silêncio.
//     [4] GATE DE KERN ENTRE FACES == SEM ajuste ("sem kern"). `mixed_seq` ("AΩB" -- 'A'/'B' da
//         Primary direto, 'Ω' resolvido via fallback pra Open Sans) precisa medir largura == a SOMA
//         das larguras medidas independentemente de `mixed_a` + `greek_omega` + `mixed_b`, dentro
//         de uma tolerância apertada de carregamento de arredondamento -- NÃO apertada por ajuste
//         de kern nenhum, porque AS DUAS fronteiras de par de glyph internas de `mixed_seq` (A|Ω e
//         Ω|B) cruzam src_face (Primary vs. Fallback), e o IterateGlyphs() só chama
//         glx_sfnt_kern() quando `prev->src_face == cur->src_face` (font_engine_own.cpp ~1366+) --
//         uma tabela kern é indexada por gid DENTRO de um arquivo de fonte, então aplicá-la entre
//         arquivos buscaria lixo. O mesh (quads em render-time) também é checado pra não vazar pra
//         fora da própria box de layout de `mixed_seq` (mesma técnica de tinta de margem
//         direita/inferior que o critério [3b] do fonteng_lazy_sanity.cpp usa).
//     [5] KERN DA MESMA FACE AINDA SE APLICA PELA FAMÍLIA "Fallback" ("com kern"). `av_fallback`
//         ("AV", renderizado pela família "Fallback" usada DIRETO, i.e. NÃO pelo próprio mecanismo
//         de roteamento-de-fallback, mas exercitando o MESMO invariante geral largura==mesh que o
//         [4] deste arquivo checa) precisa medir MENSURAVELMENTE menos que a soma das larguras de
//         `a_fallback`+`v_fallback` -- a própria tabela `kern` da Open Sans é conhecida por ter
//         kern(A,V) == -82 FUnits @ upem 2048 (o mesmo fato que a própria sonda do
//         fonteng_own_kern_sanity.cpp já estabeleceu, reusado aqui no MESMO font-size de 48px pra
//         um piso de >= 2px diretamente comparável). Junto, [4] (kern forçado a 0 entre faces) e
//         [5] (kern não-zero, mesh ainda bate com a largura) é a resposta deste arquivo pro
//         "largura == mesh para string mista... com e sem kern" da tarefa de review.
//     [6] CACHE NEGATIVO, SEM CRASH. `cjk_missing` (U+4E00, 一) -- NEM a Primary NEM fallback
//         registrado nenhum contém este glyph (confirmado ausente do cmap das duas fontes de
//         fixture pela mesma sonda descartável) -- precisa renderizar tinta ZERO, `found`, sem
//         crash (um crash reprova o binário inteiro, capturado estruturalmente pelo ctest).
//     [7] REGRESSÃO + SANITIZER. Não é trabalho direto deste arquivo -- REGRESSÃO é gateada
//         rerodando os alvos ctest fonteng_* pré-existentes sem modificação junto com este novo
//         (`ctest -R fonteng`, ver o relatório de review pra rodada capturada); SANITIZER é ESTE
//         MESMO binário (mais os dois irmãos) buildado sob `-DGLINTFX_SANITIZE=ON` e rodado de novo
//         (ver o relatório de review). A CONTRIBUIÇÃO PRÓPRIA deste arquivo pro pedido de
//         sanitizer: depois dos cheques acima, ele recarrega fonteng_fallback_scene.rml
//         kReloads=10 vezes mais na MESMA sessão UiLayer (exercitando o caminho de dedup do
//         RegisterFace() pelos TRÊS blocos @font-face -- incluindo os dois de fallback -- e o
//         caminho de resolução-de-fallback/crescimento-de-atlas do BakeGlyph() repetidamente sob
//         ASan/LSan/UBSan) e verifica que
//         glintfx::own_font_engine_debug_registered_face_count() -- o MESMO hook de DEBUG
//         src-interno que o fonteng_dedup_sanity.cpp já usa -- é EXATAMENTE 3 (uma LoadedFace por
//         par (family, blob) distinto: "Primary", "OrderFbFirst", "Fallback" -- as três strings de
//         família DIFERENTES, então nenhuma dedupica contra a outra apesar de duas compartilharem
//         conteúdo byte-idêntico) após a 1ª carga e PERMANECE EXATAMENTE 3 após os 10 reloads, mais
//         um recheque de tinta no reload final em `greek_omega` provando que o caminho de fallback
//         deduplicado ainda renderiza corretamente.
//
//     MÉTODO (compartilhado com fonteng_lazy_sanity.cpp/fonteng_own_kern_sanity.cpp): uma única
//     janela host GLFW oculta provê o contexto GL; UiLayer anexa em modo embed (forma de consumo
//     própria do GusWorld); dois ciclos update+render de aquecimento antes de toda medição;
//     glReadPixels + flip_to_top_down pra concordar com a convenção top-left/y-para-baixo do
//     get_element_box(); count_ink_rect() conta pixels quase-brancos numa caixa. Determinístico sob
//     llvmpipe -- sem sleeps, sem retries, sem tolerância além dos orçamentos explicitamente
//     derivados acima.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: pulls gl_loader.h; GL already loaded by the UiLayer ctor.
                           // PT: puxa gl_loader.h; GL já carregado pelo ctor da UiLayer.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

// EN: A/B engine-selection hook + the dedup DEBUG counter hook, forward-declared locally (see
//     fonteng_ab_compare.cpp/fonteng_dedup_sanity.cpp for the full rationale of why
//     font_engine_own.hpp cannot be #include-d from a test target).
// PT: Hook de seleção de motor A/B + o hook de DEBUG do contador de dedup, forward-declarados
//     localmente (ver fonteng_ab_compare.cpp/fonteng_dedup_sanity.cpp pro racional completo de por
//     que font_engine_own.hpp não pode ser #include-ado de um alvo de teste).
namespace glintfx {
bool& own_font_engine_ab_bypass();
size_t& own_font_engine_debug_registered_face_count();
}

namespace {

constexpr int W = 480, H = 720;
constexpr int kReloads = 10;

void flip_to_top_down(std::vector<unsigned char>& px, int w, int h) {
  const size_t row_bytes = static_cast<size_t>(w) * 3;
  std::vector<unsigned char> tmp(row_bytes);
  for (int top = 0, bottom = h - 1; top < bottom; ++top, --bottom) {
    unsigned char* rt = px.data() + static_cast<size_t>(top) * row_bytes;
    unsigned char* rb = px.data() + static_cast<size_t>(bottom) * row_bytes;
    std::copy(rt, rt + row_bytes, tmp.data());
    std::copy(rb, rb + row_bytes, rt);
    std::copy(tmp.data(), tmp.data() + row_bytes, rb);
  }
}

long count_ink_rect(const std::vector<unsigned char>& px, int fw, int fh,
                    float rx, float ry, float rw, float rh, int threshold = 100) {
  int x0 = std::max(0, static_cast<int>(std::floor(rx)));
  int y0 = std::max(0, static_cast<int>(std::floor(ry)));
  int x1 = std::min(fw, static_cast<int>(std::ceil(rx + rw)));
  int y1 = std::min(fh, static_cast<int>(std::ceil(ry + rh)));
  long count = 0;
  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      const size_t i = (static_cast<size_t>(y) * fw + x) * 3;
      if (px[i] > threshold || px[i + 1] > threshold || px[i + 2] > threshold) ++count;
    }
  }
  return count;
}

long count_ink_right_margin(const std::vector<unsigned char>& px, int fw, int fh,
                             float box_x, float box_y, float box_w, float box_h, float margin) {
  return count_ink_rect(px, fw, fh, box_x + box_w, box_y, margin, box_h);
}
long count_ink_bottom_margin(const std::vector<unsigned char>& px, int fw, int fh,
                              float box_x, float box_y, float box_w, float box_h, float margin) {
  return count_ink_rect(px, fw, fh, box_x, box_y + box_h, box_w, margin);
}

struct Box { glintfx::ElementBox box; long ink; };

} // namespace

int main() {
  glintfx::own_font_engine_ab_bypass() = false; // force the OWN engine.

  glintfx::WindowGlfw host;
  if (!host.create("fonteng_fallback_sanity_host", W, H)) {
    std::puts("fonteng_fallback_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("fonteng_fallback_sanity FAIL: UiLayer attach failed");
    return 2;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  if (!ui.load("fonteng_fallback_scene.rml")) {
    std::puts("fonteng_fallback_sanity FAIL: load() returned false");
    return 3;
  }

  int failures = 0;
  auto capture_frame = [&](std::vector<unsigned char>& px) {
    ui.update(); ui.render();
    ui.update(); ui.render();
    px.assign(static_cast<size_t>(W) * H * 3, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    flip_to_top_down(px, W, H);
  };

  std::vector<unsigned char> px;
  capture_frame(px);

  auto measure = [&](const char* id) -> Box {
    const glintfx::ElementBox b = ui.get_element_box(id);
    const long ink = b.found ? count_ink_rect(px, W, H, b.x, b.y, b.w, b.h) : 0;
    return { b, ink };
  };

  const Box ctrl        = measure("ctrl_ascii");
  const Box omega        = measure("greek_omega");
  const Box alpha_lo     = measure("greek_alpha_lo");
  const Box sup2          = measure("sup2");
  const Box cjk           = measure("cjk_missing");
  const Box omega_direct = measure("omega_direct");
  const Box mixed_seq    = measure("mixed_seq");
  const Box mixed_a      = measure("mixed_a");
  const Box mixed_b      = measure("mixed_b");
  const Box av_fallback  = measure("av_fallback");
  const Box a_fallback   = measure("a_fallback");
  const Box v_fallback   = measure("v_fallback");

  std::printf("fonteng_fallback_sanity: frame 1 -- %dx%d\n", W, H);
  auto print_box = [&](const char* id, const Box& b, const char* what) {
    std::printf("  %-14s %6s %8.2f %6ld   %s\n", id, b.box.found ? "yes" : "no", b.box.w, b.ink, what);
  };
  print_box("ctrl_ascii",   ctrl,        "ASCII control 'Axg' (Primary)");
  print_box("greek_omega",  omega,       "Omega U+03A9 (Primary, via fallback)");
  print_box("greek_alpha",  alpha_lo,    "alpha U+03B1 (Primary, via fallback)");
  print_box("sup2",         sup2,        "superscript-2 U+00B2 (Primary, via fallback)");
  print_box("cjk_missing",  cjk,         "CJK U+4E00 (neither face has it)");
  print_box("omega_direct", omega_direct,"Omega U+03A9 (Fallback family used directly)");
  print_box("mixed_seq",    mixed_seq,   "AOmegaB mixed Primary+fallback");
  print_box("mixed_a",      mixed_a,     "A (Primary)");
  print_box("mixed_b",      mixed_b,     "B (Primary)");
  print_box("av_fallback",  av_fallback, "AV (Fallback family directly, kerned pair)");
  print_box("a_fallback",   a_fallback,  "A (Fallback family directly)");
  print_box("v_fallback",   v_fallback,  "V (Fallback family directly)");

  constexpr long kMinInk = 15; // same floor fonteng_lazy_sanity.cpp/fonteng_extended_sanity.cpp derive.

  // --- [1] non-tofu fallback ---------------------------------------------------------------------
  auto check_ink_present = [&](const Box& b, const char* id, bool is_control) {
    if (!b.box.found) {
      std::fprintf(stderr, "fonteng_fallback_sanity FAIL [1] span '%s' not found in layout\n", id);
      ++failures;
      return;
    }
    if (b.ink < kMinInk) {
      std::fprintf(stderr,
          "fonteng_fallback_sanity FAIL [1] '%s' rendered no ink (ink=%ld < %ld)%s\n",
          id, b.ink, kMinInk,
          is_control ? " -- the WHOLE render pipeline is broken, not merely the fallback path"
                     : " -- fallback did not resolve this codepoint (BakeGlyph()/fallback_faces_ regression)");
      ++failures;
    }
  };
  check_ink_present(ctrl,    "ctrl_ascii",     true);
  check_ink_present(omega,   "greek_omega",    false);
  check_ink_present(alpha_lo,"greek_alpha_lo", false);
  check_ink_present(sup2,    "sup2",           false);

  // --- [2] upem-mismatch scale correctness --------------------------------------------------------
  if (!omega.box.found || !omega_direct.box.found) {
    std::fprintf(stderr, "fonteng_fallback_sanity FAIL [2] greek_omega or omega_direct not found\n");
    ++failures;
  } else {
    const float gap = std::fabs(omega.box.w - omega_direct.box.w);
    const float wrong_scale_w = omega_direct.box.w * (2048.f / 1600.f); // what a upem=1600-instead-of-2048 bug would produce.
    std::printf("fonteng_fallback_sanity: [2] width(omega_via_fallback)=%.3f  width(omega_direct)=%.3f  "
                "gap=%.3f (budget <= 2.0)  wrong-upem-would-be~%.3f\n",
                omega.box.w, omega_direct.box.w, gap, wrong_scale_w);
    if (gap > 2.0f) {
      std::fprintf(stderr,
          "fonteng_fallback_sanity FAIL [2] width(omega_via_fallback)=%.3f vs width(omega_direct)=%.3f -- "
          "gap=%.3f exceeds the 2px budget; a fallback-sourced glyph's metrics look mis-scaled "
          "(RasterizeGlyph() may be dividing by inst.face->sfnt.units_per_em (1600, the PRIMARY's) "
          "instead of src->sfnt.units_per_em (2048, the FALLBACK FACE's) -- the exact CRITICAL bug "
          "class this criterion targets; a naive wrong-upem bug would produce a width near %.3f)\n",
          omega.box.w, omega_direct.box.w, gap, wrong_scale_w);
      ++failures;
    }
  }

  // --- [4] cross-face kern gate == no adjustment ("sem kern") -------------------------------------
  if (!mixed_seq.box.found || !mixed_a.box.found || !mixed_b.box.found || !omega.box.found) {
    std::fprintf(stderr, "fonteng_fallback_sanity FAIL [4] mixed_seq or a component span not found\n");
    ++failures;
  } else {
    const float sum_w = mixed_a.box.w + omega.box.w + mixed_b.box.w;
    const float gap = std::fabs(sum_w - mixed_seq.box.w);
    std::printf("fonteng_fallback_sanity: [4] width(mixed_seq)=%.3f  sum(A+Omega+B)=%.3f  gap=%.3f (budget <= 2.0)\n",
                mixed_seq.box.w, sum_w, gap);
    if (gap > 2.0f) {
      std::fprintf(stderr,
          "fonteng_fallback_sanity FAIL [4a] width(mixed_seq)=%.3f vs sum(component widths)=%.3f -- "
          "gap=%.3f exceeds the +-2px (two glyph-boundary) rounding-carry budget; either the "
          "cross-face kern gate (IterateGlyphs()'s `prev->src_face == cur->src_face`) let a kern "
          "adjustment leak across faces, or a fallback-sourced glyph's own advance is mis-scaled\n",
          mixed_seq.box.w, sum_w, gap);
      ++failures;
    }
    constexpr float kMargin = 8.f;
    const long right_spill = count_ink_right_margin(px, W, H, mixed_seq.box.x, mixed_seq.box.y,
                                                      mixed_seq.box.w, mixed_seq.box.h, kMargin);
    const long bottom_spill = count_ink_bottom_margin(px, W, H, mixed_seq.box.x, mixed_seq.box.y,
                                                        mixed_seq.box.w, mixed_seq.box.h, kMargin);
    std::printf("fonteng_fallback_sanity: [4b] mixed_seq mesh-overflow ink: right=%ld bottom=%ld (budget ==0)\n",
                right_spill, bottom_spill);
    if (right_spill != 0 || bottom_spill != 0) {
      std::fprintf(stderr,
          "fonteng_fallback_sanity FAIL [4b] mixed_seq mesh spills past its own layout box "
          "(right=%ld, bottom=%ld ink px) -- GenerateString()'s render-time mesh disagrees with "
          "GetStringWidth()'s layout-time measurement for a mixed Primary+fallback string\n",
          right_spill, bottom_spill);
      ++failures;
    }
  }

  // --- [5] same-face kern still applies via the "Fallback" family ("com kern") --------------------
  if (!av_fallback.box.found || !a_fallback.box.found || !v_fallback.box.found) {
    std::fprintf(stderr, "fonteng_fallback_sanity FAIL [5] av_fallback/a_fallback/v_fallback not found\n");
    ++failures;
  } else {
    const float sum_av = a_fallback.box.w + v_fallback.box.w;
    const float kern_gap = sum_av - av_fallback.box.w;
    std::printf("fonteng_fallback_sanity: [5] width(AV)=%.3f width(A)=%.3f width(V)=%.3f sum=%.3f "
                "kern_gap=%.3f (floor >= 2.0)\n",
                av_fallback.box.w, a_fallback.box.w, v_fallback.box.w, sum_av, kern_gap);
    if (kern_gap < 2.0f) {
      std::fprintf(stderr,
          "fonteng_fallback_sanity FAIL [5] kern_gap=%.3f < 2.0 -- width(AV) via the \"Fallback\" "
          "family is not measurably less than width(A)+width(V); Open Sans' own kern(A,V) is known "
          "to be -82 FUnits @ upem 2048 (fonteng_own_kern_sanity.cpp's own probe) -- kerning looks "
          "broken for a family reached other than the document's primary\n",
          kern_gap);
      ++failures;
    }
  }

  // --- [6] negative-cache, no crash ----------------------------------------------------------------
  if (!cjk.box.found) {
    std::fprintf(stderr, "fonteng_fallback_sanity FAIL [6] span 'cjk_missing' not found in layout\n");
    ++failures;
  } else if (cjk.ink != 0) {
    std::fprintf(stderr,
        "fonteng_fallback_sanity FAIL [6] 'cjk_missing' (U+4E00) rendered ink=%ld -- neither Primary "
        "nor any registered fallback is expected to contain this glyph\n", cjk.ink);
    ++failures;
  }

  // ===========================================================================================
  // [7] SANITIZER contribution -- reload the SAME document kReloads more times in this SAME
  // UiLayer session, exercising RegisterFace()'s dedup path for all three @font-face blocks and
  // BakeGlyph()'s fallback-resolution path repeatedly (meaningful under an ASan/LSan/UBSan build,
  // see this file's own header). Face count must be EXACTLY 3 throughout (Primary, OrderFbFirst,
  // Fallback -- three distinct family strings, none dedup against each other despite Primary and
  // OrderFbFirst sharing byte-identical content).
  // ===========================================================================================
  const size_t face_count_after_first_load = glintfx::own_font_engine_debug_registered_face_count();
  std::printf("fonteng_fallback_sanity: [7] registered_face_count after 1st load = %zu (expect 3)\n",
              face_count_after_first_load);
  if (face_count_after_first_load != 3) {
    std::fprintf(stderr,
        "fonteng_fallback_sanity FAIL [7] registered_face_count=%zu != 3 after the first load -- "
        "expected exactly one LoadedFace per distinct (family, blob) pair declared by "
        "fonteng_fallback_scene.rml's three @font-face blocks\n", face_count_after_first_load);
    ++failures;
  }

  for (int i = 0; i < kReloads; ++i) {
    if (!ui.load("fonteng_fallback_scene.rml")) {
      std::fprintf(stderr, "fonteng_fallback_sanity FAIL [7] reload #%d: load() returned false\n", i + 1);
      ++failures;
      break;
    }
    const size_t fc = glintfx::own_font_engine_debug_registered_face_count();
    if (fc != 3) {
      std::fprintf(stderr,
          "fonteng_fallback_sanity FAIL [7] reload #%d: registered_face_count=%zu != 3 -- "
          "RegisterFace()'s dedup regressed across a reload involving fallback faces\n", i + 1, fc);
      ++failures;
    }
  }

  std::vector<unsigned char> px2;
  capture_frame(px2);
  const glintfx::ElementBox omega2_box = ui.get_element_box("greek_omega");
  const long omega2_ink = omega2_box.found ? count_ink_rect(px2, W, H, omega2_box.x, omega2_box.y,
                                                             omega2_box.w, omega2_box.h) : 0;
  std::printf("fonteng_fallback_sanity: [7] greek_omega after %d reloads: found=%s ink=%ld (floor %ld)\n",
              kReloads, omega2_box.found ? "yes" : "no", omega2_ink, kMinInk);
  if (!omega2_box.found || omega2_ink < kMinInk) {
    std::fprintf(stderr,
        "fonteng_fallback_sanity FAIL [7] greek_omega stopped rendering after %d reloads (found=%d "
        "ink=%ld) -- the deduped fallback path broke across repeated reloads\n",
        kReloads, omega2_box.found, omega2_ink);
    ++failures;
  }

  // --- embed-mode hygiene ---------------------------------------------------------------------
  if (!ui.ok()) {
    std::fputs("fonteng_fallback_sanity FAIL: ok() false after render sequence\n", stderr);
    ++failures;
  }

  glintfx::own_font_engine_ab_bypass() = false;

  if (failures == 0) {
    std::puts("fonteng_fallback_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "fonteng_fallback_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
