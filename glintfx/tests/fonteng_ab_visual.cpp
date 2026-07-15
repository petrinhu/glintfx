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
// EN: (L1.20-FONTFLIP sub-phase 1.4) Y-hint bypass hook -- same src-internal, forward-declared-
//     locally contract as own_font_engine_ab_bypass() above (see font_engine_own.hpp). `true`
//     makes the OWN engine skip its vertical grid-fitting (the "own, no hint" A/B leg).
// PT: (L1.20-FONTFLIP sub-fase 1.4) Hook de bypass do Y-hint -- mesmo contrato src-interno,
//     forward-declarado localmente, do own_font_engine_ab_bypass() acima (ver font_engine_own.hpp).
//     `true` faz o motor PRÓPRIO pular seu grid-fitting vertical (a perna A/B "próprio, sem hint").
bool& own_font_engine_hint_bypass();
// EN: (L1.20-FONTFLIP sub-phase 1.5) Stem-darkening enable hook -- same src-internal, forward-
//     declared-locally contract as the two above (see font_engine_own.hpp). OPT-IN: `true` makes the
//     OWN engine apply its coverage-curve stem darkening (the "own_yhint_dark" leg); `false` (the
//     DEFAULT, shipping behaviour) leaves raw coverage (the "own_yhint" baseline). OFF by default
//     because the darkening was measured NOT to close the stem-sharpness gap to FreeType.
// PT: (L1.20-FONTFLIP sub-fase 1.5) Hook de enable do stem-darkening -- mesmo contrato src-interno,
//     forward-declarado localmente, dos dois acima (ver font_engine_own.hpp). OPT-IN: `true` faz o
//     motor PRÓPRIO aplicar seu stem-darkening por curva-de-cobertura (a perna "own_yhint_dark");
//     `false` (o DEFAULT, comportamento de release) deixa cobertura crua (a baseline "own_yhint").
//     OFF por padrão porque o darkening foi medido como NÃO fechando o gap de nitidez pro FreeType.
bool& own_font_engine_darken_enable();
// EN: (L1.20-FONTFLIP FT-F4 pen-snap gate) Pen-snap bypass hook -- same src-internal, forward-
//     declared-locally contract as the others (see font_engine_own.hpp). DEFAULT `false` == pen-snap
//     ON (each per-glyph pen component -- advance / kern / letter-spacing / left bearing -- rounded
//     to a whole pixel, FreeType's integer-pen convention). `true` bypasses pen-snap (raw fractional
//     pen, the pre-snap behaviour). This is the leg the gate isolates.
// PT: (gate de pen-snap do L1.20-FONTFLIP FT-F4) Hook de bypass do pen-snap -- mesmo contrato
//     src-interno, forward-declarado localmente, dos outros (ver font_engine_own.hpp). DEFAULT
//     `false` == pen-snap LIGADO (cada componente do pen por-glyph -- advance / kern /
//     letter-spacing / bearing esquerdo -- arredondado a um pixel inteiro, convenção de pen inteiro
//     do FreeType). `true` bypassa o pen-snap (pen fracionário cru, comportamento pré-snap). É a
//     perna que o gate isola.
bool& own_font_engine_pensnap_bypass();
// EN: (L1.20-FONTFLIP FT-F4 VSNAP gate) V-snap bypass hook -- same src-internal, forward-declared-
//     locally contract as the others (see font_engine_own.hpp). DEFAULT `false` == vsnap ON (the
//     glyph's fractional vertical origin `origin_y_26_6` and quad `offset_y` are snapped to a whole
//     pixel, so the Y-hinted x-height edge lands on an integer atlas/screen row instead of being
//     re-smeared by a fractional phase). `true` bypasses vsnap (raw fractional vertical origin, the
//     pre-vsnap behaviour). This is the leg THIS gate isolates. Pure Y-axis transform -- must NOT
//     move the X (stem) metrics.
// PT: (gate de VSNAP do L1.20-FONTFLIP FT-F4) Hook de bypass do v-snap -- mesmo contrato src-interno,
//     forward-declarado localmente, dos outros (ver font_engine_own.hpp). DEFAULT `false` == vsnap
//     LIGADO (a origem vertical fracionaria `origin_y_26_6` e o `offset_y` do quad snapados a um
//     pixel inteiro, pra a aresta da altura-x hintada em Y pousar numa row inteira do atlas/tela em
//     vez de ser re-borrada por uma fase fracionaria). `true` bypassa o vsnap (origem vertical
//     fracionaria crua, comportamento pre-vsnap). E a perna que ESTE gate isola. Transform de eixo Y
//     puro -- NAO pode mover as metricas de X (haste).
bool& own_font_engine_vsnap_bypass();
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
// EN: (pen-snap gate) the GusWorld cockpit HP/AP body-size sweep: 9..13 dp, the small-body regime
//     where the pen's sub-pixel phase (and thus pen-snap) matters most.
// PT: (gate de pen-snap) o varredura de corpo do HP/AP do cockpit do GusWorld: 9..13 dp, o regime
//     de corpo pequeno onde a fase sub-pixel do pen (e portanto o pen-snap) mais importa.
const SizeRow kSizes[] = { {"s09", 9}, {"s10", 10}, {"s11", 11}, {"s12", 12}, {"s13", 13} };
constexpr int kNSizes = static_cast<int>(sizeof(kSizes) / sizeof(kSizes[0]));

// EN: THREE DPI scenarios: 1.0 (~1080p low-DPI), 1.5 (~150dpi Steam Deck -- the most demanding
//     fractional-scale case), 2.0 (~4K HiDPI). Filenames tag dp as int(ratio*10) -> 10/15/20 (so
//     1.5 does not collide with 1.0/2.0 under an integer cast).
// PT: TRÊS cenários de DPI: 1.0 (~1080p baixa-DPI), 1.5 (~150dpi Steam Deck -- o caso de escala
//     fracionária mais exigente), 2.0 (~4K HiDPI). Nomes de arquivo marcam o dp como int(ratio*10)
//     -> 10/15/20 (pra 1.5 não colidir com 1.0/2.0 sob um cast inteiro).
const float kDpRatios[] = { 1.0f, 1.5f, 2.0f };
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
int capture_engine(bool ab_bypass, bool hint_bypass, bool darken_enable, bool pensnap_bypass,
                   bool vsnap_bypass, const char* label, Capture out[kNDp]) {
  // EN: (sub-phase 1.4/1.5) ab_bypass selects the ENGINE (false -> own, true -> FreeType);
  //     hint_bypass selects, WITHIN the own engine, whether Y grid-fitting runs (false -> hinted,
  //     true -> raw); darken_enable selects whether the own engine's coverage-curve stem darkening
  //     runs (true -> darkened, false -> raw coverage; OPT-IN, OFF by default). All three are set
  //     before the UiLayer ctor triggers Bootstrap::init()/the first bake, which read them.
  // PT: (sub-fase 1.4/1.5) ab_bypass seleciona o MOTOR (false -> próprio, true -> FreeType);
  //     hint_bypass seleciona, DENTRO do motor próprio, se o grid-fitting Y roda (false -> hintado,
  //     true -> cru); darken_enable seleciona se o stem-darkening por curva-de-cobertura do motor
  //     próprio roda (true -> escurecido, false -> cobertura crua; OPT-IN, OFF por padrão). Os três
  //     setados antes do ctor da UiLayer disparar o Bootstrap::init()/o primeiro bake, que os lêem.
  glintfx::own_font_engine_ab_bypass() = ab_bypass;
  glintfx::own_font_engine_hint_bypass() = hint_bypass;
  glintfx::own_font_engine_darken_enable() = darken_enable;
  // EN: (pen-snap gate) fourth axis: bypass the per-glyph integer pen rounding. Read at bake time
  //     (GetStringWidth / GenerateString), so it must be set before the UiLayer ctor / first bake.
  // PT: (gate de pen-snap) quarto eixo: bypassa o arredondamento inteiro do pen por-glyph. Lido em
  //     tempo de bake (GetStringWidth / GenerateString), então setado antes do ctor da UiLayer.
  glintfx::own_font_engine_pensnap_bypass() = pensnap_bypass;
  // EN: (VSNAP gate) fifth axis: bypass the per-glyph integer vertical origin/offset rounding. Read
  //     at bake time (BakeFaceInstance), so it must be set before the UiLayer ctor / first bake.
  // PT: (gate de VSNAP) quinto eixo: bypassa o arredondamento inteiro da origem/offset vertical por-
  //     glyph. Lido em tempo de bake (BakeFaceInstance), setado antes do ctor da UiLayer.
  glintfx::own_font_engine_vsnap_bypass() = vsnap_bypass;

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

// EN: Union of THREE element boxes (own_nohint, own_yhint, ft) -- the SAME crop rectangle for all
//     three variants, so their side-by-side crops align pixel-for-pixel and none is cut tighter than
//     the others (the honesty guarantee, now three-way). Built by folding union_rect over the boxes.
// PT: União de TRÊS element boxes (own_nohint, own_yhint, ft) -- o MESMO retângulo de recorte pra as
//     três variantes, pra seus recortes lado-a-lado alinharem pixel-a-pixel e nenhum ser cortado
//     mais apertado que os outros (a garantia de honestidade, agora a três). Construída dobrando o
//     union_rect sobre as boxes.
Rect union_rect3(const glintfx::ElementBox& a, const glintfx::ElementBox& b,
                 const glintfx::ElementBox& c, int pad) {
  const float x0 = std::min({a.x, b.x, c.x});
  const float y0 = std::min({a.y, b.y, c.y});
  const float x1 = std::max({a.x + a.w, b.x + b.w, c.x + c.w});
  const float y1 = std::max({a.y + a.h, b.y + b.h, c.y + c.h});
  Rect r;
  r.x0 = std::max(0, static_cast<int>(std::floor(x0)) - pad);
  r.y0 = std::max(0, static_cast<int>(std::floor(y0)) - pad);
  r.x1 = std::min(W, static_cast<int>(std::ceil(x1)) + pad);
  r.y1 = std::min(H, static_cast<int>(std::ceil(y1)) + pad);
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

// EN: ORACLE (L1.20-FONTFLIP FT-F4 sub-phase 1.4) -- MEASURABLE proof, printed to the console, that
//     Y grid-fitting concentrates the x-height top edge onto one pixel row WITHOUT hurting vertical
//     stems. The text is white (#ffffff) grayscale-AA on black, so the RED channel value (0..255) IS
//     each pixel's ink coverage. Two 1-D profiles over the crop drive every number below:
//       row_cov[y]  = sum over columns of coverage on scanline y  (drives the Y-axis / x-height metric)
//       col_cov[x]  = sum over rows    of coverage in column x     (drives the X-axis / stem metric)
// PT: ORÁCULO (L1.20-FONTFLIP FT-F4 sub-fase 1.4) -- prova MENSURÁVEL, impressa no console, de que o
//     grid-fitting Y concentra a aresta superior da altura-x numa linha de pixel SEM piorar as
//     hastes verticais. O texto é branco (#ffffff) grayscale-AA sobre preto, então o valor do canal
//     VERMELHO (0..255) É a cobertura de tinta de cada pixel. Dois perfis 1-D sobre o recorte
//     dirigem cada número abaixo: row_cov[y] (soma por-coluna, dirige a métrica Y/altura-x) e
//     col_cov[x] (soma por-linha, dirige a métrica X/haste).
struct Oracle {
  double vstem_edge_energy = 0.0; // EN: X-axis crispness (THE pen-snap metric). PT: nitidez X (a métrica do pen-snap).
  double col_contrast = 0.0;      // EN: peak/valley modulation of column coverage (stem sharpness). PT: modulação pico/vale.
  int    x_edge_ramp_rows = 0;    // EN: transition rows at the x-height top edge -- FEWER == crisper.
  double x_edge_body_frac = 0.0;  // EN: fraction of the caps->x-height jump captured in the first body row.
  long   total_ink = 0;           // EN: coverage>100 pixel count (regression sentinel). PT: sentinela.
};

Oracle measure_oracle(const std::vector<unsigned char>& px, const Rect& r) {
  Oracle o;
  const int rw = r.x1 - r.x0, rh = r.y1 - r.y0;
  if (rw <= 2 || rh <= 3) return o;

  std::vector<double> row_cov(static_cast<size_t>(rh), 0.0);
  std::vector<double> col_cov(static_cast<size_t>(rw), 0.0);
  for (int y = 0; y < rh; ++y)
    for (int x = 0; x < rw; ++x) {
      const size_t i = (static_cast<size_t>(r.y0 + y) * W + (r.x0 + x)) * 3;
      const double cov = px[i]; // red channel == coverage for white-on-black.
      row_cov[y] += cov;
      col_cov[x] += cov;
      o.total_ink += (px[i] > 100) ? 1 : 0;
    }

  // EN: X-AXIS STEM CRISPNESS (regression sentinel). Total absolute column-to-column gradient,
  //     normalised by total ink: crisp vertical stems produce steep col_cov steps (ink on/off within
  //     one column), so higher == crisper vertical edges. Y grid-fitting NEVER touches X, so this
  //     must stay ~equal between own_nohint and own_yhint -- a drop would flag an unexpected X-axis
  //     regression. (No lone 'l'/'I' exists in the scene phrase, so this whole-line column-gradient
  //     is the faithful global proxy for the brief's "per-column ink of a vertical stem".)
  // PT: NITIDEZ DE HASTE NO EIXO X (sentinela de regressão). Gradiente absoluto coluna-a-coluna
  //     total, normalizado pela tinta total: hastes verticais nítidas produzem degraus col_cov
  //     íngremes (tinta liga/desliga dentro de uma coluna), então maior == arestas verticais mais
  //     nítidas. O grid-fitting Y NUNCA toca X, então isto deve ficar ~igual entre own_nohint e
  //     own_yhint -- uma queda flagraria uma regressão X inesperada. (Não há 'l'/'I' isolado na
  //     frase da cena, então este gradiente-de-coluna da linha inteira é o proxy global fiel do
  //     "cobertura por-coluna de uma haste vertical" do brief.)
  double grad = 0.0, tot = 0.0;
  for (int x = 0; x < rw; ++x) tot += col_cov[x];
  for (int x = 1; x < rw; ++x) grad += std::fabs(col_cov[x] - col_cov[x - 1]);
  o.vstem_edge_energy = (tot > 0.0) ? grad / tot : 0.0;

  // EN: COLUMN CONTRAST (pen-snap sharpness proxy, complementary to vstem_edge). A vertical stem
  //     that lands ON a pixel column is a tall col_cov peak flanked by near-empty columns (deep
  //     modulation); a stem straddling two columns spreads into a low, wide plateau (shallow
  //     modulation). Over the INKED columns only (col_cov > 0.10*max, excluding blank margins and
  //     inter-word gaps that would otherwise inflate the ratio), we take the ratio of a high
  //     percentile (85th ~ the stem peaks) to a low percentile (20th ~ the inter-stem valleys):
  //     HIGHER == crisper, better-modulated vertical stems. Pen-snap, being a pure horizontal
  //     transform, should RAISE this toward FreeType when ON.
  // PT: CONTRASTE DE COLUNA (proxy de nitidez do pen-snap, complementar ao vstem_edge). Uma haste
  //     vertical que pousa SOBRE uma coluna de pixel é um pico col_cov alto ladeado por colunas quase
  //     vazias (modulação profunda); uma haste montada em duas colunas se espalha num platô baixo e
  //     largo (modulação rasa). Sobre as colunas COM tinta só (col_cov > 0.10*max, excluindo margens
  //     em branco e vãos entre palavras que inflariam a razão), tomamos a razão de um percentil alto
  //     (85 ~ os picos de haste) para um baixo (20 ~ os vales entre hastes): MAIOR == hastes
  //     verticais mais nítidas e melhor moduladas. O pen-snap, sendo um transform horizontal puro,
  //     deve SUBIR isto na direção do FreeType quando LIGADO.
  double maxc = 0.0;
  for (int x = 0; x < rw; ++x) maxc = std::max(maxc, col_cov[x]);
  if (maxc > 0.0) {
    std::vector<double> inked;
    for (int x = 0; x < rw; ++x)
      if (col_cov[x] > 0.10 * maxc) inked.push_back(col_cov[x]);
    if (inked.size() >= 4) {
      std::sort(inked.begin(), inked.end());
      auto pct = [&](double p) {
        const size_t n = inked.size();
        size_t idx = static_cast<size_t>(p * static_cast<double>(n - 1) + 0.5);
        if (idx >= n) idx = n - 1;
        return inked[idx];
      };
      const double peak = pct(0.85), valley = pct(0.20);
      o.col_contrast = (valley > 1e-9) ? peak / valley : 0.0;
    }
  }

  // EN: Y-AXIS x-HEIGHT ZONE ALIGNMENT (the grid-fitting win). ALL x-height letters share one
  //     font-unit x-height, so at a given size they ALL land on the SAME sub-pixel Y whether hinted
  //     or not -- the hint's job is to make that shared x-height top edge sit ON a pixel row (crisp,
  //     coverage in ~1 row) instead of straddling two (blurry, coverage split ~50/50). So the
  //     measurable is the SHARPNESS of the single x-height top edge, not scatter between letters.
  //     Find the x-height BODY top `yb`: scanning top-down, the first row whose coverage first
  //     exceeds 0.55*plateau AND whose next row is still high (>0.5*plateau) -- i.e. the row where
  //     the x-height bodies (o,c,a,e,n,v,s,...) turn on, on top of the sparser caps/accents ink
  //     above. Two numbers gauge its crispness:
  //       x_edge_ramp_rows = how many of the rows in [yb-4, yb] sit in the transition band
  //                          (0.25*plateau .. 0.85*plateau) -- a grid-aligned edge ramps in FEWER
  //                          rows (ideally 1-2); a mid-pixel edge smears across more.
  //       x_edge_body_frac = (row_cov[yb] - row_cov[yb-1]) / (plateau - row_cov[yb-1]) -- the
  //                          fraction of the caps->x-height jump captured in the FIRST body row;
  //                          HIGHER == the edge lands on that row crisply.
  //     HONEST CAVEAT (a graphics-programmer note, not hidden): the win scales with how far the
  //     x-height would otherwise land from a pixel boundary at THIS size -- Open Sans' x-height is
  //     ~0.535*px, so it already nearly aligns at 11/13px (tiny gain) and lands worst near 16px
  //     (~0.44px off -> the clearest gain). The metric reflects that size dependence rather than
  //     hiding it.
  // PT: ALINHAMENTO DE ZONA da altura-x no EIXO Y (o ganho do grid-fitting). TODAS as letras de
  //     altura-x compartilham uma altura-x em unidade-de-fonte, então num dado tamanho TODAS caem no
  //     MESMO sub-pixel Y, com ou sem hint -- o trabalho do hint é fazer essa aresta superior
  //     compartilhada pousar SOBRE uma linha de pixel (nítida, cobertura em ~1 linha) em vez de
  //     montar em duas (borrada, cobertura dividida ~50/50). Então o mensurável é a NITIDEZ da única
  //     aresta superior da altura-x, não dispersão entre letras. Acha o topo do CORPO da altura-x
  //     `yb`: varrendo de cima, a primeira linha cuja cobertura excede 0.55*plateau E cuja próxima
  //     linha ainda é alta (>0.5*plateau) -- i.e. a linha onde os corpos de altura-x turnam on, em
  //     cima da tinta mais esparsa de maiúsculas/acentos acima. Dois números medem sua nitidez:
  //       x_edge_ramp_rows = quantas das linhas em [yb-4, yb] ficam na banda de transição
  //                          (0.25*plateau .. 0.85*plateau) -- uma aresta grid-alinhada rampa em
  //                          MENOS linhas (idealmente 1-2); uma aresta mid-pixel borra em mais.
  //       x_edge_body_frac = (row_cov[yb] - row_cov[yb-1]) / (plateau - row_cov[yb-1]) -- a fração
  //                          do salto maiúsculas->altura-x capturada na PRIMEIRA linha de corpo;
  //                          MAIOR == a aresta pousa naquela linha nitidamente.
  //     RESSALVA HONESTA (nota de graphics-programmer, não escondida): o ganho escala com o quão
  //     longe a altura-x pousaria de uma fronteira de pixel NESTE tamanho -- a altura-x da Open Sans
  //     é ~0.535*px, então já quase alinha em 11/13px (ganho minúsculo) e pousa pior perto de 16px
  //     (~0.44px fora -> o ganho mais claro). A métrica reflete essa dependência de tamanho em vez
  //     de escondê-la.
  double plateau = 0.0;
  for (int y = 0; y < rh; ++y) plateau = std::max(plateau, row_cov[y]);
  if (plateau <= 0.0) return o;

  int yb = -1;
  for (int y = 1; y < rh; ++y) {
    if (row_cov[y] > 0.55 * plateau && row_cov[std::min(y + 1, rh - 1)] > 0.5 * plateau) { yb = y; break; }
  }
  if (yb >= 1) {
    const double pre = row_cov[yb - 1];
    const double lo = 0.25 * plateau, hi = 0.85 * plateau;
    int ramp = 0;
    for (int y = std::max(0, yb - 4); y <= yb; ++y)
      if (row_cov[y] > lo && row_cov[y] < hi) ++ramp;
    o.x_edge_ramp_rows = ramp;
    o.x_edge_body_frac = (plateau > pre) ? (row_cov[yb] - pre) / (plateau - pre) : 0.0;
  }
  return o;
}

} // namespace

int main(int argc, char** argv) {
  const std::string outdir = (argc > 1) ? argv[1] : "ab_visual";

  glintfx::WindowGlfw host;
  if (!host.create("fonteng_ab_visual_host", W, H)) {
    std::puts("fonteng_ab_visual FAIL: host window create failed");
    return 1;
  }

  // EN: VSNAP GATE -- THREE sessions, run sequentially, sharing ONE GLFW context (only RmlUi
  //     re-initialises between them). Each tuple is (ab_bypass, hint_bypass, darken_enable,
  //     pensnap_bypass, vsnap_bypass):
  //       (P) own_full     -- own, Y-hint ON, pen-snap ON, vsnap ON  (false, false, false, false, false) [SHIPPING candidate]
  //       (N) own_no_vsnap -- own, Y-hint ON, pen-snap ON, vsnap OFF (false, false, false, false, true ) [isolates vsnap]
  //       (F) freetype     -- RmlUi's FreeType (integer pen + snap)  (true,  -,     -,     -,     -    ) [baseline to match]
  //     Pen-snap stays ON in BOTH own legs (X must not regress). (P) vs (N) isolates the vsnap effect
  //     alone (same Y-hinting, same pen-snap, same coverage; the only difference is whether the
  //     fractional vertical origin/offset is rounded to a whole pixel). (P) vs (F) is THE gate
  //     question: does own_full with vsnap ON match FreeType's x-height crispness (the plateau of
  //     xe_body ~0.84) at 9-13 dp across three DPIs, and does it KILL the per-size variability the
  //     fractional-phase "dance" caused? vsnap is a PURE VERTICAL transform, so its signal lives in
  //     the Y-axis x-height metrics (xe_body / xe_ramp), NOT the column-domain stem ones; the stem
  //     metric (vstem_edge) is the X SENTINEL -- it must stay ~equal between P and N.
  // PT: GATE DE VSNAP -- TRES sessoes, sequenciais, compartilhando UM contexto GLFW (so o RmlUi
  //     re-inicializa entre elas). Cada tupla e (ab_bypass, hint_bypass, darken_enable,
  //     pensnap_bypass, vsnap_bypass):
  //       (P) own_full     -- proprio, Y-hint ON, pen-snap ON, vsnap ON  (false,false,false,false,false) [candidato de release]
  //       (N) own_no_vsnap -- proprio, Y-hint ON, pen-snap ON, vsnap OFF (false,false,false,false,true ) [isola o vsnap]
  //       (F) freetype     -- FreeType do RmlUi (pen inteiro + snap)     (true, -,    -,    -,    -    ) [baseline a empatar]
  //     Pen-snap fica LIGADO nas DUAS pernas own (X nao pode regredir). (P) vs (N) isola o efeito so do
  //     vsnap (mesmo Y-hint, mesmo pen-snap, mesma cobertura; a unica diferenca e se a origem/offset
  //     vertical fracionaria e arredondada a um pixel inteiro). (P) vs (F) e A pergunta do gate: o
  //     own_full com vsnap LIGADO empata a nitidez de altura-x do FreeType (o plato de xe_body ~0.84)
  //     em 9-13 dp nos tres DPIs, e MATA a variabilidade por-tamanho que a "danca" da fase fracionaria
  //     causava? vsnap e um transform VERTICAL PURO, entao seu sinal vive nas metricas de altura-x do
  //     eixo Y (xe_body / xe_ramp), NAO nas de haste do dominio-de-coluna; a metrica de haste
  //     (vstem_edge) e a SENTINELA X -- deve ficar ~igual entre P e N.
  Capture full[kNDp], novsnap[kNDp], ft[kNDp];
  if (int rc = capture_engine(false, false, false, false, false, "own_full",     full))    return rc;
  if (int rc = capture_engine(false, false, false, false, true,  "own_no_vsnap", novsnap)) return rc;
  if (int rc = capture_engine(true,  false, false, false, false, "freetype",     ft))      return rc;
  glintfx::own_font_engine_ab_bypass()      = false; // EN: restore defaults. PT: restaura defaults.
  glintfx::own_font_engine_hint_bypass()    = false;
  glintfx::own_font_engine_darken_enable()  = false;
  glintfx::own_font_engine_pensnap_bypass() = false;
  glintfx::own_font_engine_vsnap_bypass()   = false;

  // EN: dp -> integer filename tag: int(ratio*10) so 1.0/1.5/2.0 -> 10/15/20 (no collision).
  // PT: dp -> tag inteira de arquivo: int(ratio*10) pra 1.0/1.5/2.0 -> 10/15/20 (sem colisão).
  auto dp_tag_of = [](float r) { return static_cast<int>(std::lround(r * 10.0f)); };

  std::printf("fonteng_ab_visual (VSNAP GATE): %dx%d  writing crops to '%s/'\n", W, H, outdir.c_str());
  std::printf("  3 legs: own_full (P, vsnap ON) | own_no_vsnap (N, vsnap OFF) | freetype (F) -- pen-snap ON on both own legs\n\n");
  std::printf("== INK SANITY (pixels with coverage>100 in the shared 3-way crop) ==\n");
  std::printf("  %-10s %5s %5s   %11s %11s %11s\n",
              "quad", "dp", "px", "ink_P", "ink_N", "ink_F");

  int written = 0, missing = 0;
  for (int d = 0; d < kNDp; ++d) {
    const int dp_tag = dp_tag_of(kDpRatios[d]);
    for (int s = 0; s < kNSizes; ++s) {
      const glintfx::ElementBox& bp = full[d].box[s];
      const glintfx::ElementBox& bn = novsnap[d].box[s];
      const glintfx::ElementBox& bf = ft[d].box[s];
      if (!bp.found || !bn.found || !bf.found) {
        std::fprintf(stderr,
                     "fonteng_ab_visual WARN: box not found dp=%d %s (P=%d N=%d F=%d)\n",
                     dp_tag, kSizes[s].id, bp.found, bn.found, bf.found);
        ++missing;
        continue;
      }
      const Rect r = union_rect3(bp, bn, bf, /*pad=*/2);
      const int phys_px = static_cast<int>(std::lround(kSizes[s].dp * kDpRatios[d]));

      char pp[512], pnn[512], pf[512];
      std::snprintf(pp,  sizeof pp,  "%s/own_full_dp%d_s%02d.ppm",     outdir.c_str(), dp_tag, kSizes[s].dp);
      std::snprintf(pnn, sizeof pnn, "%s/own_no_vsnap_dp%d_s%02d.ppm", outdir.c_str(), dp_tag, kSizes[s].dp);
      std::snprintf(pf,  sizeof pf,  "%s/ft_dp%d_s%02d.ppm",           outdir.c_str(), dp_tag, kSizes[s].dp);

      const bool ok1 = write_ppm(pp,  full[d].px,    r);
      const bool ok2 = write_ppm(pnn, novsnap[d].px, r);
      const bool ok3 = write_ppm(pf,  ft[d].px,      r);
      if (ok1 && ok2 && ok3) written += 3;

      std::printf("  dp%02d_s%02d   %5.1f %5d   %11ld %11ld %11ld\n",
                  dp_tag, kSizes[s].dp, kDpRatios[d], phys_px,
                  count_ink(full[d].px, r), count_ink(novsnap[d].px, r), count_ink(ft[d].px, r));
    }
  }

  // EN: NUMERIC ORACLE -- the gate table, printed for EVERY (dp, body size). Each leg is measured on
  //     its OWN padded element-box crop, so the row profiles are independent of the others' extent.
  //     The vsnap gate's PRIMARY metric is the Y-axis xe_body (fraction of the caps->x-height jump
  //     captured in the FIRST body row) and its companion xe_ramp (transition rows at the x-height
  //     top edge -- FEWER == crisper): vsnap should PIN xe_body onto the FreeType plateau (~0.84) at
  //     every body size and FLATTEN its per-size variability (the diagnosis' "0.30<->0.87 dance").
  //     vstem_edge is the X SENTINEL -- vsnap is a pure vertical transform, so vstem_edge must stay
  //     ~equal between own_full (P) and own_no_vsnap (N); any drift there would flag an unexpected
  //     X-axis regression from a Y-only change.
  // PT: ORACULO NUMERICO -- a tabela do gate, impressa pra CADA (dp, corpo). Cada perna e medida no
  //     recorte da PROPRIA element-box (com folga), entao os perfis de linha independem da extensao
  //     das outras. A metrica PRIMARIA do gate de vsnap e o xe_body do eixo Y (fracao do salto
  //     maiusculas->altura-x capturada na PRIMEIRA linha de corpo) e sua companheira xe_ramp (linhas
  //     de transicao na aresta superior da altura-x -- MENOS == mais nitida): o vsnap deve PRENDER o
  //     xe_body no plato do FreeType (~0.84) em todo corpo e ACHATAR sua variabilidade por-tamanho (a
  //     "danca 0.30<->0.87" do diagnostico). vstem_edge e a SENTINELA X -- vsnap e um transform
  //     vertical puro, entao vstem_edge deve ficar ~igual entre own_full (P) e own_no_vsnap (N);
  //     qualquer deriva ali flagraria uma regressao X inesperada de uma mudanca so-Y.
  std::printf("\n== ORACLE (per dp, per body; PRIMARY=xe_body/xe_ramp (Y); vstem_edge=X sentinel) ==\n");
  std::printf("  %4s %-8s %-14s %9s %11s %12s %9s %9s\n",
              "dp", "size", "leg", "total_ink", "vstem_edge", "col_contrast", "xe_ramp", "xe_body");

  // EN: keep the per-(dp,leg) series across sizes for the stability pass: xe_body is the PRIMARY
  //     (Y-axis) dancing proxy; vstem_edge is kept as the X sentinel's steadiness cross-check.
  // PT: guarda as series por-(dp,perna) ao longo dos corpos pro passo de estabilidade: xe_body e o
  //     proxy de danca PRIMARIO (eixo Y); vstem_edge fica como cross-check de firmeza da sentinela X.
  double series_xebody[kNDp][3][kNSizes] = {};
  double series_vstem[kNDp][3][kNSizes] = {};
  bool   series_ok[kNDp][3][kNSizes] = {};

  for (int d = 0; d < kNDp; ++d) {
    const int dp_tag = dp_tag_of(kDpRatios[d]);
    for (int s = 0; s < kNSizes; ++s) {
      const glintfx::ElementBox& bp = full[d].box[s];
      const glintfx::ElementBox& bn = novsnap[d].box[s];
      const glintfx::ElementBox& bf = ft[d].box[s];
      if (!bp.found || !bn.found || !bf.found) continue;
      const Rect rp = union_rect(bp, bp, /*pad=*/2);
      const Rect rn = union_rect(bn, bn, /*pad=*/2);
      const Rect rf = union_rect(bf, bf, /*pad=*/2);
      const Oracle op = measure_oracle(full[d].px,    rp);
      const Oracle on = measure_oracle(novsnap[d].px, rn);
      const Oracle of = measure_oracle(ft[d].px,      rf);

      series_xebody[d][0][s] = op.x_edge_body_frac; series_vstem[d][0][s] = op.vstem_edge_energy; series_ok[d][0][s] = true;
      series_xebody[d][1][s] = on.x_edge_body_frac; series_vstem[d][1][s] = on.vstem_edge_energy; series_ok[d][1][s] = true;
      series_xebody[d][2][s] = of.x_edge_body_frac; series_vstem[d][2][s] = of.vstem_edge_energy; series_ok[d][2][s] = true;

      const struct { const char* nm; const Oracle& o; } rows[] = {
        { "own_full",     op },
        { "own_no_vsnap", on },
        { "freetype",     of },
      };
      for (const auto& rr : rows)
        std::printf("  %3d%c %-8s %-14s %9ld %11.4f %12.3f %9d %9.4f\n",
                    dp_tag, ' ', kSizes[s].id, rr.nm,
                    rr.o.total_ink, rr.o.vstem_edge_energy, rr.o.col_contrast,
                    rr.o.x_edge_ramp_rows, rr.o.x_edge_body_frac);
    }
  }

  // EN: STABILITY / DANCING PROXY -- per (dp, leg), a metric's series across sizes 9->13 reduced to:
  //       mean       = average value over the sizes,
  //       spread     = max - min over the sizes (THE variability the brief asks for: the amplitude
  //                    of the "0.30<->0.87 dance"; vsnap should COLLAPSE this on own_full),
  //       roughness  = mean |v[i+1]-v[i]| (mean absolute step; magnitude-and-trend-sensitive),
  //       reversals  = number of direction changes (sign flips of consecutive steps) -- the
  //                    TREND-INDEPENDENT dancing proxy: a steadily-varying series has 0, a jittering
  //                    one flips up/down each step.
  //     Run TWICE: first on xe_body (the PRIMARY Y-axis metric -- own_full should match FreeType's
  //     flat ~0.84 plateau with a tiny spread, own_no_vsnap should show the large dancing spread);
  //     then on vstem_edge (the X SENTINEL -- own_full and own_no_vsnap should be ~identical, since
  //     vsnap is vertical and must not touch the stems).
  // PT: PROXY DE ESTABILIDADE / DANCA -- por (dp, perna), a serie de uma metrica ao longo dos corpos
  //     9->13 reduzida a: mean (media), spread (max-min, A variabilidade que o brief pede: a amplitude
  //     da "danca 0.30<->0.87"; vsnap deve COLAPSAR isso no own_full), roughness (media |v[i+1]-v[i]|)
  //     e reversals (mudancas de direcao, proxy de danca INDEPENDENTE do trend). Roda DUAS vezes:
  //     primeiro no xe_body (metrica PRIMARIA do eixo Y -- own_full deve empatar o plato plano ~0.84
  //     do FreeType com spread minusculo, own_no_vsnap deve mostrar o grande spread da danca); depois
  //     no vstem_edge (a SENTINELA X -- own_full e own_no_vsnap devem ser ~identicos, ja que vsnap e
  //     vertical e nao pode tocar as hastes).
  const char* legnm[3] = { "own_full", "own_no_vsnap", "freetype" };
  auto stability_pass = [&](const char* title, const double series[kNDp][3][kNSizes]) {
    std::printf("\n== STABILITY: %s (across sizes 9..13; spread=max-min LOWER==steadier) ==\n", title);
    std::printf("  %4s %-14s %8s %8s %10s %10s   %s\n",
                "dp", "leg", "mean", "spread", "roughness", "reversals", "series 9..13");
    for (int d = 0; d < kNDp; ++d) {
      const int dp_tag = dp_tag_of(kDpRatios[d]);
      for (int L = 0; L < 3; ++L) {
        double sum = 0.0, rough = 0.0; int n = 0, steps = 0; double prev = 0.0; bool have_prev = false;
        double vmin = 0.0, vmax = 0.0;
        int reversals = 0; double prev_step = 0.0; bool have_step = false;
        char sbuf[256]; int off = 0;
        for (int s = 0; s < kNSizes; ++s) {
          if (!series_ok[d][L][s]) continue;
          const double v = series[d][L][s];
          if (n == 0) { vmin = vmax = v; } else { vmin = std::min(vmin, v); vmax = std::max(vmax, v); }
          sum += v; ++n;
          if (have_prev) {
            const double step = v - prev;
            rough += std::fabs(step); ++steps;
            if (have_step && ((step > 0.0) != (prev_step > 0.0)) && step != 0.0 && prev_step != 0.0)
              ++reversals;
            prev_step = step; have_step = true;
          }
          prev = v; have_prev = true;
          off += std::snprintf(sbuf + off, sizeof(sbuf) - off, "%s%.4f",
                               (s == 0 ? "" : " "), v);
        }
        const double mean = (n > 0) ? sum / n : 0.0;
        const double spread = (n > 0) ? (vmax - vmin) : 0.0;
        const double roughness = (steps > 0) ? rough / steps : 0.0;
        std::printf("  %3d  %-14s %8.4f %8.4f %10.4f %10d   %s\n",
                    dp_tag, legnm[L], mean, spread, roughness, reversals, sbuf);
      }
    }
  };
  stability_pass("xe_body (PRIMARY, Y-axis x-height crispness)", series_xebody);
  stability_pass("vstem_edge (X SENTINEL, must not move P vs N)", series_vstem);

  std::printf("\nfonteng_ab_visual: wrote %d PPM(s), %d line(s) with a missing box\n",
              written, missing);
  // EN: Not a pass/fail gate -- succeed as long as SOME crops were written (a totally empty run
  //     means a context/fixture failure worth a non-zero exit).
  // PT: Não é um gate passa/falha -- tem sucesso enquanto ALGUNS recortes foram escritos (uma run
  //     totalmente vazia significa falha de contexto/fixture, digna de saída não-zero).
  return (written > 0) ? 0 : 2;
}
