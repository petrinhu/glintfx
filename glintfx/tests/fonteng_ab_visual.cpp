// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_ab_visual -- L1.20-FONTFLIP (FT-F4) VISUAL sharpness harness (DIAGNOSTIC, not a
//     gate). Its numeric sibling fonteng_ab_compare.cpp mechanically flags GROSS A/B divergence;
//     THIS tool instead produces IMAGES so a human (the líder) can make the PERCEPTUAL call the
//     numbers cannot: is glintfx's own grayscale-AA font engine sharp enough at small body sizes
//     and low DPI, or does it need subpixel-AA/hinting before "the flip" makes it the default?
//
//     ONE BUILD, BOTH ENGINES, TWO DPIs -- same proven A/B mechanism as fonteng_ab_compare.cpp:
//     in a single GLINTFX_OWN_FONT_ENGINE=ON build both engines are present; the per-session
//     src-internal hook glintfx::own_font_engine_ab_bypass() selects which one Bootstrap::init()
//     installs (false -> own SOV-SFNT/SOV-RAST, true -> RmlUi's built-in FreeType). Two full
//     UiLayer sessions run sequentially in one process, sharing ONE hidden GLFW host GL context
//     (only RmlUi is re-initialised between them) -- the fairest possible A/B, both engines
//     rendering into the identical GL environment. WITHIN each engine session the SAME loaded
//     document is rendered at TWO dp_ratios (1.0 low-DPI = the critical case, 2.0 HiDPI); font-size
//     is authored in `dp` in fonteng_ab_visual_scene.rml, so 13dp rasterises at 13 physical px at
//     dp=1 and 26 px at dp=2 -- the own engine bakes a distinct atlas at each pixel size, so this
//     genuinely compares the rasteriser at each body size on both a low-DPI and a HiDPI screen.
//
//     OUTPUT: for every (dp_ratio, font-size) pair, the tool crops each engine's rendered line to
//     the UNION ink rectangle of the two engines (same crop rect for both, so they align pixel-for-
//     pixel side by side) and writes it as a binary PPM (P6): OUTDIR/own_dp{R}_s{N}.ppm and
//     OUTDIR/ft_dp{R}_s{N}.ppm. A companion Python/PIL step (see the session's report) upscales
//     these nearest-neighbour, stacks own|FreeType side by side with textual labels, and emits the
//     final PNGs. PPM is chosen over PNG deliberately: it needs ZERO extra library (a PNG writer
//     would mean vendoring stb_image_write into the test tree), and the tiny cropped rasters are
//     trivially converted downstream. OUTDIR is argv[1] (default "ab_visual" under the CWD).
//
//     HONEST BY CONSTRUCTION: the phrase and the comparison are NOT rigged. Both engines get the
//     exact same document, size, DPI, GL context, and clear colour; the crop rectangle is the
//     union of both engines' element boxes (neither engine is cropped tighter than the other); the
//     magnification is plain nearest-neighbour (no smoothing that would flatter either). If the own
//     engine's small-body text is blurrier, thinner, or patchier than FreeType's, it will show.
//     The phrase's `î` and em-dash `—` fall OUTSIDE the own engine's v1 MVP BAKE_SET and are
//     therefore blank in the own engine -- a real, current coverage gap (distinct from sharpness),
//     left visible on purpose (see the .rml's own header).
// PT: fonteng_ab_visual -- harness VISUAL de nitidez do L1.20-FONTFLIP (FT-F4) (DIAGNÓSTICO, não um
//     gate). Seu irmão numérico fonteng_ab_compare.cpp flagra mecanicamente divergência A/B
//     GROSSEIRA; ESTA ferramenta em vez disso produz IMAGENS para que um humano (o líder) faça a
//     decisão PERCEPTUAL que os números não fazem: o motor de fonte grayscale-AA próprio do glintfx
//     é nítido o bastante em corpos pequenos e baixa DPI, ou precisa de subpixel-AA/hinting antes de
//     "o flip" torná-lo o default?
//
//     UM BUILD, DOIS MOTORES, DOIS DPIs -- mesmo mecanismo A/B provado do fonteng_ab_compare.cpp:
//     num único build GLINTFX_OWN_FONT_ENGINE=ON os dois motores estão presentes; o hook
//     src-interno por-sessão glintfx::own_font_engine_ab_bypass() seleciona qual o Bootstrap::init()
//     instala (false -> próprio SOV-SFNT/SOV-RAST, true -> FreeType embutido do RmlUi). Duas sessões
//     UiLayer completas rodam sequencialmente num processo, compartilhando UM contexto GL de host
//     GLFW oculto (só o RmlUi é re-inicializado entre elas) -- o A/B mais justo possível, os dois
//     motores renderizando no ambiente GL idêntico. DENTRO de cada sessão de motor o MESMO documento
//     carregado é renderizado em DOIS dp_ratios (1.0 baixa-DPI = o caso crítico, 2.0 HiDPI); o
//     font-size é autorado em `dp` em fonteng_ab_visual_scene.rml, então 13dp rasteriza em 13 px
//     físicos em dp=1 e 26 px em dp=2 -- o motor próprio empacota um atlas distinto em cada
//     tamanho-em-px, então isto de fato compara o rasterizador em cada tamanho de corpo tanto numa
//     tela de baixa DPI quanto HiDPI.
//
//     SAÍDA: para cada par (dp_ratio, font-size), a ferramenta recorta a linha renderizada de cada
//     motor ao retângulo de tinta UNIÃO dos dois motores (mesmo retângulo de recorte para ambos,
//     então alinham pixel-a-pixel lado a lado) e a grava como PPM binário (P6):
//     OUTDIR/own_dp{R}_s{N}.ppm e OUTDIR/ft_dp{R}_s{N}.ppm. Um passo Python/PIL companheiro (ver o
//     relatório da sessão) faz upscale nearest-neighbour, empilha próprio|FreeType lado a lado com
//     rótulos textuais, e emite os PNGs finais. PPM é escolhido em vez de PNG de propósito: não
//     precisa de NENHUMA biblioteca extra (um writer de PNG significaria vendorizar stb_image_write
//     na árvore de teste), e os pequenos rasters recortados são trivialmente convertidos depois.
//     OUTDIR é argv[1] (default "ab_visual" sob o CWD).
//
//     HONESTO POR CONSTRUÇÃO: a frase e a comparação NÃO são maquiadas. Os dois motores recebem o
//     exato mesmo documento, tamanho, DPI, contexto GL e cor de clear; o retângulo de recorte é a
//     união das element boxes dos dois motores (nenhum motor é recortado mais apertado que o outro);
//     a ampliação é nearest-neighbour puro (sem suavização que favoreceria algum). Se o texto de
//     corpo pequeno do motor próprio for mais borrado, fino ou irregular que o do FreeType, vai
//     aparecer. O `î` e o em-dash `—` da frase caem FORA do BAKE_SET MVP v1 do motor próprio e
//     portanto ficam em branco no motor próprio -- uma lacuna de cobertura real e atual (distinta de
//     nitidez), deixada visível de propósito (ver o cabeçalho próprio do .rml).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: pulls gl_loader.h; GL already loaded by the UiLayer ctor.
                           // PT: puxa gl_loader.h; GL já carregado pelo ctor da UiLayer.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// EN: A/B engine-selection hook, forward-declared locally (not #include-ing font_engine_own.hpp,
//     whose Layer-0 includes are only wired for the lib's own font_engine_own.cpp TU) -- exactly as
//     fonteng_ab_compare.cpp does; see its comment and font_engine_own.hpp for the full contract.
// PT: Hook de seleção de motor A/B, forward-declarado localmente (sem #include de
//     font_engine_own.hpp, cujos includes de Camada 0 só são conectados para a própria TU
//     font_engine_own.cpp da lib) -- exatamente como fonteng_ab_compare.cpp faz; ver seu comentário
//     e font_engine_own.hpp para o contrato completo.
namespace glintfx {
bool& own_font_engine_ab_bypass();
}

namespace {

// EN: Viewport. Wide enough that the widest rendered line (24dp at dp=2 -> 48 physical px) never
//     clips at the right edge, so the readback captures the full string; tall enough for the four
//     dp-authored lines at dp=2 (their physical heights double vs dp=1).
// PT: Viewport. Largo o bastante para a linha mais larga renderizada (24dp em dp=2 -> 48 px físicos)
//     nunca cortar na borda direita, então o readback captura a string inteira; alto o bastante para
//     as quatro linhas autoradas em dp em dp=2 (as alturas físicas dobram vs dp=1).
constexpr int W = 1600, H = 560;

// EN: The four line ids in fonteng_ab_visual_scene.rml and their authored dp size (for labels).
// PT: Os quatro ids de linha em fonteng_ab_visual_scene.rml e seu tamanho dp autorado (para
//     rótulos).
struct SizeRow { const char* id; int dp; };
const SizeRow kSizes[] = { {"s11", 11}, {"s13", 13}, {"s16", 16}, {"s24", 24} };
constexpr int kNSizes = static_cast<int>(sizeof(kSizes) / sizeof(kSizes[0]));

// EN: The two DPI scenarios: 1.0 = low-DPI (critical), 2.0 = HiDPI. Integer tag used in filenames.
// PT: Os dois cenários de DPI: 1.0 = baixa-DPI (crítico), 2.0 = HiDPI. Tag inteira usada nos nomes
//     de arquivo.
const float kDpRatios[] = { 1.0f, 2.0f };
constexpr int kNDp = static_cast<int>(sizeof(kDpRatios) / sizeof(kDpRatios[0]));

// EN: One captured frame + the element box of each measured line, for one (engine, dp_ratio).
// PT: Um frame capturado + a element box de cada linha medida, para um (motor, dp_ratio).
struct Capture {
  std::vector<unsigned char> px;                 // RGB, top-down (flip already applied), W*H*3.
  glintfx::ElementBox box[kNSizes] = {};
};

// EN: glReadPixels origin is bottom-left; element boxes are top-left, y-down. Flip the readback in
//     place so both agree -- identical to fonteng_ab_compare.cpp / fonteng_own_embed_sanity.cpp.
// PT: A origem do glReadPixels é bottom-left; element boxes são top-left, y-para-baixo. Inverte o
//     readback in-place para os dois concordarem -- idêntico a fonteng_ab_compare.cpp /
//     fonteng_own_embed_sanity.cpp.
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

// EN: Runs ONE full UiLayer session with the requested engine (bypass=false -> own, true ->
//     FreeType). Loads the scene once, then for EACH dp_ratio sets it, renders (two warm-up frames,
//     suite convention), reads back the full frame and every line's element box into out[dp_index].
//     Returns 0 on success. The UiLayer is destroyed at return, resetting RmlUi's font_interface so
//     the next session cleanly re-selects its engine (see fonteng_ab_compare.cpp's header).
// PT: Roda UMA sessão UiLayer completa com o motor pedido (bypass=false -> próprio, true ->
//     FreeType). Carrega a cena uma vez, então para CADA dp_ratio o define, renderiza (dois frames
//     de aquecimento, convenção da suíte), lê de volta o frame inteiro e a element box de cada linha
//     em out[dp_index]. Retorna 0 no sucesso. A UiLayer é destruída no retorno, resetando o
//     font_interface do RmlUi para a próxima sessão re-selecionar seu motor limpo (ver o cabeçalho
//     de fonteng_ab_compare.cpp).
int capture_engine(bool bypass, const char* label, Capture out[kNDp]) {
  glintfx::own_font_engine_ab_bypass() = bypass;

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::fprintf(stderr, "fonteng_ab_visual FAIL: UiLayer attach failed (%s)\n", label);
    return 10;
  }
  if (!ui.load("fonteng_ab_visual_scene.rml")) {
    std::fprintf(stderr, "fonteng_ab_visual FAIL: load() returned false (%s)\n", label);
    return 11;
  }

  for (int d = 0; d < kNDp; ++d) {
    ui.set_dp_ratio(kDpRatios[d]);

    // EN: Deterministic black baseline, then two warm-up frames (frame 1 settles the dp re-layout,
    //     frame 2 is stable) -- same convention as every embed render test in this suite.
    // PT: Baseline preto determinístico, depois dois frames de aquecimento (frame 1 estabiliza o
    //     re-layout de dp, frame 2 estável) -- mesma convenção de todo teste de render embed da
    //     suíte.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ui.update(); ui.render();
    ui.update(); ui.render();

    out[d].px.assign(static_cast<size_t>(W) * H * 3, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, out[d].px.data());
    flip_to_top_down(out[d].px, W, H);

    for (int s = 0; s < kNSizes; ++s)
      out[d].box[s] = ui.get_element_box(kSizes[s].id);
  }
  return 0;
}

// EN: Integer crop rectangle, clamped to the frame.
// PT: Retângulo de recorte inteiro, recortado ao frame.
struct Rect { int x0, y0, x1, y1; };

// EN: Union of two element boxes, padded by `pad` px on every side, clamped to [0,W)x[0,H). Using
//     the union (never one engine's box alone) means the SAME rectangle crops both engines, so
//     their crops align exactly when placed side by side -- and neither is cut tighter than the
//     other (the honesty guarantee).
// PT: União de duas element boxes, com `pad` px de folga em cada lado, recortada a [0,W)x[0,H).
//     Usar a união (nunca a box de um motor só) significa que o MESMO retângulo recorta os dois
//     motores, então seus recortes alinham exatamente lado a lado -- e nenhum é cortado mais
//     apertado que o outro (a garantia de honestidade).
Rect union_rect(const glintfx::ElementBox& a, const glintfx::ElementBox& b, int pad) {
  const float ax1 = a.x + a.w, ay1 = a.y + a.h;
  const float bx1 = b.x + b.w, by1 = b.y + b.h;
  Rect r;
  r.x0 = std::max(0, static_cast<int>(std::floor(std::min(a.x, b.x))) - pad);
  r.y0 = std::max(0, static_cast<int>(std::floor(std::min(a.y, b.y))) - pad);
  r.x1 = std::min(W, static_cast<int>(std::ceil (std::max(ax1, bx1))) + pad);
  r.y1 = std::min(H, static_cast<int>(std::ceil (std::max(ay1, by1))) + pad);
  if (r.x1 <= r.x0) r.x1 = std::min(W, r.x0 + 1);
  if (r.y1 <= r.y0) r.y1 = std::min(H, r.y0 + 1);
  return r;
}

// EN: Writes the sub-rectangle `r` of the top-down RGB frame `src` (W wide) as a binary PPM (P6).
//     Returns true on success. No library, no dependency -- a P6 header line plus raw RGB rows.
// PT: Grava o sub-retângulo `r` do frame RGB top-down `src` (largura W) como PPM binário (P6).
//     Retorna true no sucesso. Sem biblioteca, sem dependência -- uma linha de cabeçalho P6 mais as
//     linhas RGB cruas.
bool write_ppm(const std::string& path, const std::vector<unsigned char>& src, const Rect& r) {
  const int cw = r.x1 - r.x0, ch = r.y1 - r.y0;
  std::FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) { std::fprintf(stderr, "fonteng_ab_visual: cannot open %s\n", path.c_str()); return false; }
  std::fprintf(f, "P6\n%d %d\n255\n", cw, ch);
  std::vector<unsigned char> row(static_cast<size_t>(cw) * 3);
  for (int y = r.y0; y < r.y1; ++y) {
    const unsigned char* srow = src.data() + (static_cast<size_t>(y) * W + r.x0) * 3;
    std::memcpy(row.data(), srow, row.size());
    std::fwrite(row.data(), 1, row.size(), f);
  }
  std::fclose(f);
  return true;
}

// EN: Count "ink" pixels (any channel > 100) in a rect -- a quick console sanity that both engines
//     actually drew the line (a blank line would mean a fixture/context failure worth flagging).
// PT: Conta pixels "tinta" (algum canal > 100) num retângulo -- uma sanidade rápida no console de
//     que os dois motores de fato desenharam a linha (uma linha em branco significaria falha de
//     fixture/contexto a sinalizar).
long count_ink(const std::vector<unsigned char>& px, const Rect& r) {
  long n = 0;
  for (int y = r.y0; y < r.y1; ++y)
    for (int x = r.x0; x < r.x1; ++x) {
      const size_t i = (static_cast<size_t>(y) * W + x) * 3;
      if (px[i] > 100 || px[i + 1] > 100 || px[i + 2] > 100) ++n;
    }
  return n;
}

} // namespace

int main(int argc, char** argv) {
  const std::string outdir = (argc > 1) ? argv[1] : "ab_visual";

  glintfx::WindowGlfw host;
  if (!host.create("fonteng_ab_visual_host", W, H)) {
    std::puts("fonteng_ab_visual FAIL: host window create failed");
    return 1;
  }

  // EN: Session 1 -- own engine; Session 2 -- FreeType. Same clean sequential re-init as
  //     fonteng_ab_compare.cpp (shared GLFW context, only RmlUi re-initialised between sessions).
  // PT: Sessão 1 -- motor próprio; Sessão 2 -- FreeType. Mesma re-init sequencial limpa do
  //     fonteng_ab_compare.cpp (contexto GLFW compartilhado, só o RmlUi re-inicializado entre
  //     sessões).
  Capture own[kNDp], ft[kNDp];
  if (int rc = capture_engine(false, "own", own)) return rc;
  if (int rc = capture_engine(true,  "freetype", ft)) return rc;
  glintfx::own_font_engine_ab_bypass() = false;   // EN: restore default. PT: restaura default.

  std::printf("fonteng_ab_visual: %dx%d  writing crops to '%s/'\n", W, H, outdir.c_str());
  std::printf("  %-16s %5s %5s   %10s %10s\n", "file-pair", "dp", "px", "ink_own", "ink_ft");

  int written = 0, missing = 0;
  for (int d = 0; d < kNDp; ++d) {
    const int dp_tag = static_cast<int>(kDpRatios[d]);              // 1 or 2
    for (int s = 0; s < kNSizes; ++s) {
      const glintfx::ElementBox& bo = own[d].box[s];
      const glintfx::ElementBox& bf = ft[d].box[s];
      if (!bo.found || !bf.found) {
        std::fprintf(stderr, "fonteng_ab_visual WARN: box not found dp=%d %s (own=%d ft=%d)\n",
                     dp_tag, kSizes[s].id, bo.found, bf.found);
        ++missing;
        continue;
      }
      const Rect r = union_rect(bo, bf, /*pad=*/2);
      const int phys_px = kSizes[s].dp * dp_tag;   // authored dp -> physical px at this dp_ratio.

      char pown[512], pft[512];
      std::snprintf(pown, sizeof pown, "%s/own_dp%d_s%02d.ppm", outdir.c_str(), dp_tag, kSizes[s].dp);
      std::snprintf(pft,  sizeof pft,  "%s/ft_dp%d_s%02d.ppm",  outdir.c_str(), dp_tag, kSizes[s].dp);

      const bool ok1 = write_ppm(pown, own[d].px, r);
      const bool ok2 = write_ppm(pft,  ft[d].px,  r);
      if (ok1 && ok2) written += 2;

      std::printf("  dp%d_s%02d          %5.1f %5d   %10ld %10ld\n",
                  dp_tag, kSizes[s].dp, kDpRatios[d], phys_px,
                  count_ink(own[d].px, r), count_ink(ft[d].px, r));
      (void)phys_px;
    }
  }

  std::printf("fonteng_ab_visual: wrote %d PPM(s), %d line(s) with a missing box\n",
              written, missing);
  // EN: Not a pass/fail gate -- succeed as long as SOME crops were written (a totally empty run
  //     means a context/fixture failure worth a non-zero exit).
  // PT: Não é um gate passa/falha -- tem sucesso enquanto ALGUNS recortes foram escritos (uma run
  //     totalmente vazia significa falha de contexto/fixture, digna de saída não-zero).
  return (written > 0) ? 0 : 2;
}
