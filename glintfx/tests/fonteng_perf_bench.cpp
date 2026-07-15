// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_perf_bench -- L1.20-PERFBENCH (FT-F4 perf gate): the PERFORMANCE sibling of
//     fonteng_ab_compare.cpp/fonteng_ab_visual.cpp. Those two answer "does the own engine
//     RENDER the same thing as FreeType, and does it LOOK as sharp?"; this one answers "does the
//     own engine COST the same, on the CPU path a weak machine actually pays for?" -- the
//     GusWorld's consumer-driven condition for flipping GLINTFX_OWN_FONT_ENGINE's default (see
//     feedback_gusworld_corpo_pequeno_fonte.md / feedback_cadencia_releases_consumidor.md): perf
//     on weak hardware is a gate, not a nice-to-have.
//
//     DIAGNOSTIC, NOT a ctest gate (same "built but no add_test" convention as
//     fonteng_ab_visual.cpp) -- perf numbers vary machine to machine; a fixed pass/fail threshold
//     in CI would be either too loose to mean anything or too tight to survive a different
//     runner. Run by hand (or by a driver script that repeats it for K launches / all
//     combinations) under xvfb-run, one axis/engine/font/dp combination per process.
//
//     SAME A/B MECHANISM AS EVERY OTHER FONTFLIP HARNESS: own_font_engine_ab_bypass() (false ==
//     own SOV-SFNT/SOV-RAST, true == RmlUi's FreeType), set BEFORE the UiLayer ctor triggers
//     Bootstrap::init()'s font-engine install. The three sibling own-engine A/B hooks (Y-hint,
//     pen-snap, vsnap) are pinned to their SHIPPING default (all ON, darkening OFF) here --
//     unlike fonteng_ab_visual.cpp's gate legs, this bench measures the engine that would
//     actually ship, not an isolated A/B variant.
//
//     ISOLATING CPU FROM GPU (llvmpipe distorts GPU numbers under Xvfb -- the líder's brief is
//     explicit that the CPU number is the one that transfers to weak hardware):
//       --axis=bake   : UiLayer ctor + create_data_model/bind_* + load() + FIRST update()+
//                       render()+glFinish() -- the ONE-TIME per-(face,size) event (own engine's
//                       eager warm-set bake vs FreeType's on-demand bake). One-shot per process;
//                       a driver script runs this K>=15 times and takes the MEDIAN across
//                       process launches (a single in-process sample would be one cold-cache
//                       data point, not a distribution).
//       --axis=layout : Context::Update() ONLY (glintfx::UiLayer::update(), which maps straight
//                       to Rml::Context::Update() -- see ui_layer.cpp -- no GL calls at all), in
//                       a steady regime (after 4 discarded warm-up frames), timed over N=2500
//                       iterations (first 2 discarded) with the HUD numbers MUTATED every single
//                       iteration (HudModel::tick(), below) so GetStringWidth()/IterateGlyphs()
//                       is genuinely re-walked, not RmlUi's cached geometry -- THE headline
//                       number: CPU-only, llvmpipe-neutral, the one that scales to weak hardware.
//       --axis=frame  : Context::Render() (CPU regen + GL submit), glFinish() before stopping the
//                       clock, same steady regime and mutation discipline as --axis=layout.
//                       Reported but explicitly labelled NOT_REPRESENTATIVE_LLVMPIPE in the
//                       output -- Xvfb/Mesa llvmpipe's threaded tile rendering makes the GL-bound
//                       part of this number noisy and non-transferable to real GPU hardware (see
//                       tests/CMakeLists.txt's golden_test comment for the same root cause).
//       --axis=mem    : CPU-side memory footprint. For the own engine: EXACT bytes via the
//                       src-internal own_font_engine_atlas_bytes() hook (font_engine_own.hpp/
//                       .cpp, L1.20-PERFBENCH) -- sums every baked FaceInstance's atlas_rgba +
//                       glyph-map payload + darken LUT. For FreeType (not introspectable from
//                       this side of RmlUi's FontEngineInterface): a /proc/self/statm RSS-delta
//                       proxy, sampled at three points (before UiLayer ctor, right after the bake
//                       event, and after 500 further mutated frames) so a lazy-bake/growth signal
//                       would show up as rss_delta_frames_kb > 0. The own engine ALSO reports its
//                       RSS delta alongside the exact atlas_bytes -- a cross-check that the byte
//                       count is in the right ballpark, not a fabricated number.
//
//     ANTI-CACHE-HIT VALIDATION (the brief's explicit "measuring the wrong thing" trap):
//     RmlUi only re-calls GenerateString() for a span whose FontFaceHandle's GetVersion()
//     changed OR whose own text changed -- an UNCHANGED HUD would make update() a near-instant
//     no-op every iteration, silently measuring "how fast RmlUi confirms nothing changed"
//     instead of the font engine's real per-frame cost. --axis=layout therefore ALSO times a
//     60-iteration baseline loop with the SAME text held constant (no tick()), and prints
//     layout_mut_over_noop_ratio: mutated/baseline. A ratio near 1.0 (default WARN threshold
//     1.3x) means the mutation likely did NOT force real re-layout -- the run's layout numbers
//     are flagged UNTRUSTWORTHY on stderr rather than silently trusted.
//
// PT: fonteng_perf_bench -- irmão de PERFORMANCE do L1.20-PERFBENCH (gate FT-F4) do
//     fonteng_ab_compare.cpp/fonteng_ab_visual.cpp. Aqueles dois respondem "o motor próprio
//     RENDERIZA a mesma coisa que o FreeType, e PARECE tão nítido?"; este responde "o motor
//     próprio CUSTA o mesmo, no caminho de CPU que uma máquina fraca de fato paga?" -- a condição
//     consumer-driven do GusWorld para virar o default de GLINTFX_OWN_FONT_ENGINE (ver
//     feedback_gusworld_corpo_pequeno_fonte.md / feedback_cadencia_releases_consumidor.md): perf
//     em hardware fraco é gate, não um nice-to-have.
//
//     DIAGNÓSTICO, NÃO um gate do ctest (mesma convenção "buildado mas sem add_test" do
//     fonteng_ab_visual.cpp) -- números de perf variam de máquina pra máquina; um limiar
//     passa/falha fixo no CI seria frouxo demais pra significar algo ou apertado demais pra
//     sobreviver a um runner diferente. Rode à mão (ou por um script driver que o repete por K
//     lançamentos / todas as combinações) sob xvfb-run, uma combinação eixo/motor/fonte/dp por
//     processo.
//
//     MESMO MECANISMO A/B DE TODO OUTRO HARNESS DO FONTFLIP: own_font_engine_ab_bypass() (false
//     == próprio SOV-SFNT/SOV-RAST, true == FreeType do RmlUi), setado ANTES do ctor da UiLayer
//     disparar a instalação do motor de fonte pelo Bootstrap::init(). Os três hooks A/B irmãos do
//     motor próprio (Y-hint, pen-snap, vsnap) ficam presos no default de RELEASE aqui (todos ON,
//     darkening OFF) -- diferente das pernas de gate do fonteng_ab_visual.cpp, este bench mede o
//     motor que de fato vai pro release, não uma variante A/B isolada.
//
//     ISOLANDO CPU DE GPU (o llvmpipe distorce números de GPU sob Xvfb -- o brief do líder é
//     explícito que o número de CPU é o que transfere pra hardware fraco):
//       --axis=bake   : ctor da UiLayer + create_data_model/bind_* + load() + PRIMEIRO
//                       update()+render()+glFinish() -- o evento ÚNICO por-(face,tamanho) (bake
//                       ávido do warm set do motor próprio vs bake sob-demanda do FreeType).
//                       Um-tiro por processo; um script driver roda isto K>=15 vezes e toma a
//                       MEDIANA entre lançamentos de processo (uma amostra só no processo seria
//                       um ponto de dado de cache-frio, não uma distribuição).
//       --axis=layout : SÓ Context::Update() (glintfx::UiLayer::update(), que mapeia direto pra
//                       Rml::Context::Update() -- ver ui_layer.cpp -- sem NENHUMA chamada GL), em
//                       regime estável (após 4 frames de aquecimento descartados), medido ao
//                       longo de N=2500 iterações (2 primeiras descartadas) com os números do HUD
//                       MUTADOS a cada iteração (HudModel::tick(), abaixo) para que
//                       GetStringWidth()/IterateGlyphs() seja genuinamente re-percorrido, não a
//                       geometria cacheada do RmlUi -- O número PRINCIPAL: só-CPU,
//                       llvmpipe-neutro, o que escala pra hardware fraco.
//       --axis=frame  : Context::Render() (regen CPU + submit GL), glFinish() antes de parar o
//                       cronômetro, mesmo regime estável e disciplina de mutação do --axis=layout.
//                       Reportado mas explicitamente rotulado NOT_REPRESENTATIVE_LLVMPIPE na
//                       saída -- o rendering de tiles em threads do Xvfb/Mesa llvmpipe torna a
//                       parte ligada-a-GL deste número ruidosa e não-transferível pra hardware
//                       GPU real (ver o comentário do golden_test em tests/CMakeLists.txt pra
//                       mesma causa raiz).
//       --axis=mem    : pegada de memória lado-CPU. Pro motor próprio: bytes EXATOS via o hook
//                       src-interno own_font_engine_atlas_bytes() (font_engine_own.hpp/.cpp,
//                       L1.20-PERFBENCH) -- soma o atlas_rgba + payload do mapa de glyphs + LUT de
//                       darken de toda FaceInstance empacotada. Pro FreeType (não introspectável
//                       deste lado da FontEngineInterface do RmlUi): um proxy de RSS delta via
//                       /proc/self/statm, amostrado em três pontos (antes do ctor da UiLayer, logo
//                       após o evento de bake, e após mais 500 frames mutados) para que um sinal
//                       de bake-preguiçoso/crescimento apareça como rss_delta_frames_kb > 0. O
//                       motor próprio TAMBÉM reporta seu RSS delta ao lado do atlas_bytes exato --
//                       um cross-check de que a contagem de bytes está na faixa certa, não um
//                       número fabricado.
//
//     VALIDAÇÃO ANTI-CACHE-HIT (a armadilha "medindo a coisa errada" explícita do brief): o RmlUi
//     só rechama GenerateString() pra um span cujo GetVersion() do FontFaceHandle mudou OU cujo
//     próprio texto mudou -- um HUD INALTERADO faria update() ser um no-op quase-instantâneo a
//     cada iteração, silenciosamente medindo "quão rápido o RmlUi confirma que nada mudou" em vez
//     do custo real por-frame do motor de fonte. --axis=layout portanto TAMBÉM mede um laço
//     baseline de 60 iterações com o MESMO texto constante (sem tick()), e imprime
//     layout_mut_over_noop_ratio: mutado/baseline. Uma razão perto de 1.0 (limiar WARN default
//     1.3x) significa que a mutação provavelmente NÃO forçou re-layout real -- os números de
//     layout desta execução são flagrados UNTRUSTWORTHY no stderr em vez de confiados
//     silenciosamente.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: pulls gl_loader.h; GL already loaded by the UiLayer ctor.
                           // PT: puxa gl_loader.h; GL já carregado pelo ctor da UiLayer.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>

// EN: A/B engine-selection + sibling hooks, forward-declared locally -- same contract as every
//     other FONTFLIP harness (fonteng_ab_compare.cpp/fonteng_ab_visual.cpp): NOT #include-ing
//     font_engine_own.hpp (its Layer-0 includes are only wired for font_engine_own.cpp's own
//     TU). own_font_engine_atlas_bytes() is the L1.20-PERFBENCH addition -- see its own
//     doc-comment in font_engine_own.hpp for the "single active instance" contract it relies on.
// PT: Seleção de motor A/B + hooks irmãos, forward-declarados localmente -- mesmo contrato de
//     todo outro harness do FONTFLIP (fonteng_ab_compare.cpp/fonteng_ab_visual.cpp): SEM
//     #include de font_engine_own.hpp (seus includes de Camada 0 só são conectados para a
//     própria TU do font_engine_own.cpp). own_font_engine_atlas_bytes() é o acréscimo do
//     L1.20-PERFBENCH -- ver o doc-comment próprio em font_engine_own.hpp pro contrato de
//     "instância única ativa" do qual depende.
namespace glintfx {
bool& own_font_engine_ab_bypass();
bool& own_font_engine_hint_bypass();
bool& own_font_engine_darken_enable();
bool& own_font_engine_pensnap_bypass();
bool& own_font_engine_vsnap_bypass();
size_t own_font_engine_atlas_bytes();
}

namespace {

using Clock = std::chrono::steady_clock;

inline double us_since(Clock::time_point t0) {
  return std::chrono::duration<double, std::micro>(Clock::now() - t0).count();
}

double median_of(std::vector<double> v) {
  if (v.empty()) return 0.0;
  std::sort(v.begin(), v.end());
  const size_t n = v.size();
  return (n % 2) ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

double p95_of(std::vector<double> v) {
  if (v.empty()) return 0.0;
  std::sort(v.begin(), v.end());
  size_t idx = static_cast<size_t>(0.95 * static_cast<double>(v.size() - 1) + 0.5);
  if (idx >= v.size()) idx = v.size() - 1;
  return v[idx];
}

// EN: RSS (resident set size) in KiB, read from /proc/self/statm field 2 (in pages) -- the same
//     Linux-native proxy the líder's brief asks for as FreeType's memory number (FreeType is not
//     introspectable from this side of RmlUi's FontEngineInterface). Returns -1 on any I/O
//     failure (never crashes the bench -- a memory PROBE failing is not a reason to abort a
//     CPU-timing run).
// PT: RSS (resident set size) em KiB, lido do campo 2 (em páginas) de /proc/self/statm -- o
//     mesmo proxy nativo-Linux que o brief do líder pede pro número de memória do FreeType (o
//     FreeType não é introspectável deste lado da FontEngineInterface do RmlUi). Retorna -1 em
//     qualquer falha de I/O (nunca crasha o bench -- uma SONDA de memória falhando não é motivo
//     pra abortar uma execução de cronometragem de CPU).
long read_rss_kb() {
  std::FILE* f = std::fopen("/proc/self/statm", "r");
  if (!f) return -1;
  long pages = 0, resident = 0;
  const int n = std::fscanf(f, "%ld %ld", &pages, &resident);
  std::fclose(f);
  if (n != 2) return -1;
  const long page_kb = sysconf(_SC_PAGESIZE) / 1024;
  return resident * page_kb;
}

struct Args {
  std::string engine; // "own" | "ft"
  std::string axis;   // "bake" | "layout" | "frame" | "mem"
  std::string font;   // "pixelop" | "opensans"
  float dp = 1.0f;
};

bool take_prefixed(const std::string& arg, const char* prefix, std::string& out) {
  const size_t plen = std::strlen(prefix);
  if (arg.compare(0, plen, prefix) == 0) { out = arg.substr(plen); return true; }
  return false;
}

bool parse_args(int argc, char** argv, Args& a) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    std::string v;
    if (take_prefixed(arg, "--engine=", v)) a.engine = v;
    else if (take_prefixed(arg, "--axis=", v)) a.axis = v;
    else if (take_prefixed(arg, "--font=", v)) a.font = v;
    else if (take_prefixed(arg, "--dp=", v)) a.dp = std::strtof(v.c_str(), nullptr);
  }
  return !a.engine.empty() && !a.axis.empty() && !a.font.empty() && a.dp > 0.f;
}

// EN: Mutates every HUD number the scene's {{hp}}/{{ap}}/{{mana}}/{{pct}}/{{sector}} views
//     interpolate, EVERY call -- the anti-cache-hit discipline this whole harness depends on
//     (see file header). Simple modulo ramps, not realistic game state: this bench measures the
//     font engine's per-frame COST, not gameplay semantics, so any always-changing value works.
// PT: Muta todo número do HUD que as views {{hp}}/{{ap}}/{{mana}}/{{pct}}/{{sector}} da cena
//     interpolam, a CADA chamada -- a disciplina anti-cache-hit da qual este harness inteiro
//     depende (ver cabeçalho do arquivo). Rampas de módulo simples, não estado de jogo realista:
//     este bench mede o CUSTO por-frame do motor de fonte, não semântica de gameplay, então
//     qualquer valor sempre-mudando serve.
struct HudModel {
  glintfx::UiLayer* ui = nullptr;
  void tick(int i) {
    ui->set_number("hp", static_cast<double>(1 + (i % 200)));
    ui->set_number("ap", static_cast<double>(i % 60));
    ui->set_number("mana", static_cast<double>(i % 500));
    ui->set_number("pct", static_cast<double>(i % 100));
    ui->set_number("sector", static_cast<double>(i % 999));
  }
};

void print_header() {
  std::printf("engine\taxis\tfont\tdp\tmetric\tvalue\n");
}

} // namespace

int main(int argc, char** argv) {
  Args args;
  if (!parse_args(argc, argv, args)) {
    std::fprintf(stderr,
      "usage: fonteng_perf_bench --engine=own|ft --axis=bake|layout|frame|mem "
      "--font=pixelop|opensans --dp=1.0|1.5|2.0\n");
    return 64;
  }
  if (args.engine != "own" && args.engine != "ft") {
    std::fprintf(stderr, "fonteng_perf_bench: bad --engine='%s'\n", args.engine.c_str());
    return 64;
  }
  if (args.axis != "bake" && args.axis != "layout" && args.axis != "frame" && args.axis != "mem") {
    std::fprintf(stderr, "fonteng_perf_bench: bad --axis='%s'\n", args.axis.c_str());
    return 64;
  }
  if (args.font != "pixelop" && args.font != "opensans") {
    std::fprintf(stderr, "fonteng_perf_bench: bad --font='%s'\n", args.font.c_str());
    return 64;
  }

  // EN: Engine selection + all sibling own-engine hooks pinned to the SHIPPING defaults (Y-hint
  //     ON, pen-snap ON, vsnap ON, darkening OFF) -- see file header for why this differs from
  //     fonteng_ab_visual.cpp's per-leg gate.
  // PT: Seleção de motor + todos os hooks irmãos do motor próprio presos nos defaults de RELEASE
  //     (Y-hint ON, pen-snap ON, vsnap ON, darkening OFF) -- ver o cabeçalho do arquivo pro
  //     motivo disto diferir do gate por-perna do fonteng_ab_visual.cpp.
  glintfx::own_font_engine_ab_bypass()      = (args.engine == "ft");
  glintfx::own_font_engine_hint_bypass()    = false;
  glintfx::own_font_engine_darken_enable()  = false;
  glintfx::own_font_engine_pensnap_bypass() = false;
  glintfx::own_font_engine_vsnap_bypass()   = false;

  // EN: Viewport big enough for 7 HUD lines at 11dp*2.0=22 physical px each, plus margin.
  // PT: Viewport grande o bastante para 7 linhas de HUD a 11dp*2.0=22 px físicos cada, mais margem.
  constexpr int W = 480, H = 360;

  glintfx::WindowGlfw host;
  if (!host.create("fonteng_perf_bench_host", W, H)) {
    std::fprintf(stderr, "fonteng_perf_bench FAIL: host window create failed\n");
    return 1;
  }

  const long rss_before_ctor_kb = read_rss_kb();

  const auto t_bake0 = Clock::now();

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true,
                         .dp_ratio = args.dp });
  if (!ui.ok()) {
    std::fprintf(stderr, "fonteng_perf_bench FAIL: UiLayer attach failed\n");
    return 10;
  }

  const bool show_pixelop = (args.font == "pixelop");
  if (!ui.create_data_model("hud") ||
      !ui.bind_number("hp", 1.0) ||
      !ui.bind_number("ap", 0.0) ||
      !ui.bind_number("mana", 0.0) ||
      !ui.bind_number("pct", 0.0) ||
      !ui.bind_number("sector", 0.0) ||
      !ui.bind_bool("show_pixelop", show_pixelop)) {
    std::fprintf(stderr, "fonteng_perf_bench FAIL: data-model setup failed\n");
    return 11;
  }

  if (!ui.load("fonteng_perf_scene.rml")) {
    std::fprintf(stderr, "fonteng_perf_bench FAIL: load() returned false\n");
    return 12;
  }

  HudModel hud{ &ui };

  // EN: First frame -- completes the BAKE event (own engine: eager warm-set atlas pack + 1st
  //     GPU upload; FreeType: its own on-demand first-use bake) for this (face, dp) combination.
  // PT: Primeiro frame -- completa o evento de BAKE (motor próprio: empacotamento ávido do warm
  //     set + 1º upload GPU; FreeType: seu próprio bake sob-demanda de 1º uso) para esta
  //     combinação (face, dp).
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  hud.tick(0);
  ui.update();
  ui.render();
  glFinish();

  const double bake_us = us_since(t_bake0);

  if (args.axis == "bake") {
    print_header();
    std::printf("%s\tbake\t%s\t%.1f\tbake_us\t%.1f\n",
                args.engine.c_str(), args.font.c_str(), args.dp, bake_us);
    return 0;
  }

  // EN: A few more warm-up frames so any timed loop below starts in a genuinely STEADY regime
  //     (texture already created, atlas_dirty cleared) -- same "discard first frames" discipline
  //     as every other capture in this suite, generalised because the loops below run thousands
  //     of iterations, not a single snapshot.
  // PT: Mais alguns frames de aquecimento para que qualquer laço cronometrado abaixo comece num
  //     regime genuinamente ESTÁVEL (textura já criada, atlas_dirty limpo) -- mesma disciplina
  //     "descarte os primeiros frames" de toda outra captura desta suíte, generalizada porque os
  //     laços abaixo rodam milhares de iterações, não um snapshot único.
  for (int i = 1; i <= 4; ++i) {
    hud.tick(i);
    ui.update();
    ui.render();
  }
  glFinish();

  if (args.axis == "mem") {
    const long rss_after_bake_kb = read_rss_kb();

    constexpr int kMemFrames = 500;
    for (int i = 5; i < 5 + kMemFrames; ++i) {
      hud.tick(i);
      ui.update();
      ui.render();
    }
    glFinish();
    const long rss_after_frames_kb = read_rss_kb();
    const size_t atlas_bytes = glintfx::own_font_engine_atlas_bytes();

    print_header();
    std::printf("%s\tmem\t%s\t%.1f\trss_before_load_kb\t%ld\n",
                args.engine.c_str(), args.font.c_str(), args.dp, rss_before_ctor_kb);
    std::printf("%s\tmem\t%s\t%.1f\trss_after_bake_kb\t%ld\n",
                args.engine.c_str(), args.font.c_str(), args.dp, rss_after_bake_kb);
    std::printf("%s\tmem\t%s\t%.1f\trss_after_frames_kb\t%ld\n",
                args.engine.c_str(), args.font.c_str(), args.dp, rss_after_frames_kb);
    std::printf("%s\tmem\t%s\t%.1f\trss_delta_bake_kb\t%ld\n",
                args.engine.c_str(), args.font.c_str(), args.dp,
                rss_after_bake_kb - rss_before_ctor_kb);
    std::printf("%s\tmem\t%s\t%.1f\trss_delta_frames_kb\t%ld\n",
                args.engine.c_str(), args.font.c_str(), args.dp,
                rss_after_frames_kb - rss_after_bake_kb);
    std::printf("%s\tmem\t%s\t%.1f\tatlas_bytes\t%zu\n",
                args.engine.c_str(), args.font.c_str(), args.dp, atlas_bytes);
    return 0;
  }

  if (args.axis == "layout") {
    // EN: Anti-cache-hit BASELINE: same text held constant (no tick()) -- see file header.
    // PT: BASELINE anti-cache-hit: mesmo texto mantido constante (sem tick()) -- ver cabeçalho.
    constexpr int kNoop = 62;
    std::vector<double> noop_us;
    noop_us.reserve(kNoop);
    for (int i = 0; i < kNoop; ++i) {
      const auto t0 = Clock::now();
      ui.update();
      const double dt = us_since(t0);
      if (i >= 2) noop_us.push_back(dt);
    }

    constexpr int kN = 2500;
    std::vector<double> mut_us;
    mut_us.reserve(kN);
    for (int i = 0; i < kN; ++i) {
      hud.tick(1000 + i);
      const auto t0 = Clock::now();
      ui.update();
      const double dt = us_since(t0);
      if (i >= 2) mut_us.push_back(dt);
    }

    const double noop_med = median_of(noop_us);
    const double mut_med  = median_of(mut_us);
    const double mut_p95  = p95_of(mut_us);
    const double ratio    = (noop_med > 1e-6) ? mut_med / noop_med : 0.0;

    print_header();
    std::printf("%s\tlayout\t%s\t%.1f\tlayout_median_us\t%.2f\n",
                args.engine.c_str(), args.font.c_str(), args.dp, mut_med);
    std::printf("%s\tlayout\t%s\t%.1f\tlayout_p95_us\t%.2f\n",
                args.engine.c_str(), args.font.c_str(), args.dp, mut_p95);
    std::printf("%s\tlayout\t%s\t%.1f\tlayout_noop_median_us\t%.2f\n",
                args.engine.c_str(), args.font.c_str(), args.dp, noop_med);
    std::printf("%s\tlayout\t%s\t%.1f\tlayout_mut_over_noop_ratio\t%.2f\n",
                args.engine.c_str(), args.font.c_str(), args.dp, ratio);
    std::printf("%s\tlayout\t%s\t%.1f\tlayout_n\t%d\n",
                args.engine.c_str(), args.font.c_str(), args.dp, kN - 2);

    if (ratio < 1.3) {
      std::fprintf(stderr,
        "fonteng_perf_bench WARN: layout mut/noop ratio %.2f < 1.3 -- mutation may not be "
        "forcing real re-layout (RmlUi cache hit); this run's layout numbers may be "
        "UNTRUSTWORTHY (measuring \"confirm nothing changed\", not the font engine).\n",
        ratio);
    }
    return 0;
  }

  // args.axis == "frame"
  {
    constexpr int kN = 1200;
    std::vector<double> frame_us;
    frame_us.reserve(kN);
    for (int i = 0; i < kN; ++i) {
      hud.tick(2000 + i);
      ui.update();
      const auto t0 = Clock::now();
      ui.render();
      glFinish();
      const double dt = us_since(t0);
      if (i >= 2) frame_us.push_back(dt);
    }

    const double med = median_of(frame_us);
    const double p95 = p95_of(frame_us);

    print_header();
    std::printf("%s\tframe\t%s\t%.1f\tframe_median_us\t%.2f\n",
                args.engine.c_str(), args.font.c_str(), args.dp, med);
    std::printf("%s\tframe\t%s\t%.1f\tframe_p95_us\t%.2f\n",
                args.engine.c_str(), args.font.c_str(), args.dp, p95);
    std::printf("%s\tframe\t%s\t%.1f\tframe_n\t%d\n",
                args.engine.c_str(), args.font.c_str(), args.dp, kN - 2);
    std::printf("%s\tframe\t%s\t%.1f\tframe_note\tNOT_REPRESENTATIVE_LLVMPIPE\n",
                args.engine.c_str(), args.font.c_str(), args.dp);
    return 0;
  }
}
