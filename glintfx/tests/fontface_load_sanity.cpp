// SPDX-License-Identifier: Apache-2.0
// EN: fontface_load_sanity -- TDD gate for UI-FONTFACE (v0.24.0):
//     `glintfx::UiLayer::load_font_face(const FontFaceDesc&)` (and, by shared implementation,
//     `glintfx::App::load_font_face` -- both forward to the SAME `Engine::load_font_face` ->
//     `Bootstrap::load_font_face` implementation; see bootstrap.cpp's header comment for the
//     exact `Rml::LoadFontFace` overload-selection contract this file proves end-to-end). This
//     file drives everything through `UiLayer` -- the `App`-SPECIFIC forwarding wire (proven
//     NOT redundant coverage by the adversarial review, `w18-rev`: it found `App::load_font_face`
//     completely unexercised, mutated it to a hardcoded `false`, and the suite still passed
//     99/100) is covered separately by `app_font_face_sanity.cpp`, registered inside
//     `if(GLINTFX_BACKEND_GLFW)`. Covers seven criteria, each in its own labelled block below
//     (matching the multi-criterion single-file convention this suite already follows, e.g.
//     fonteng_fallback_sanity.cpp's "seven criteria"):
//
//     [1] VALID REGISTRATION + RENDER -- a real font file registered with an explicit family
//         returns `true`, and text actually assigned that family renders (non-blank ink,
//         non-zero advance width) -- a statistical (not pixel-exact) oracle, same convention as
//         every other fonteng_* readback test in this suite (llvmpipe under Xvfb is not
//         bit-stable across driver versions).
//     [2] HOSTILE INPUT -- `nullptr` path, empty (`""`) path, a path that does not exist on
//         disk, and a path that DOES exist but is not a font at all (this very test's own
//         `.rml` scene file, fed in as bogus font bytes) all return `false`, with NO crash --
//         AUD-TEC-5 fail-high, same discipline as every other Bootstrap method.
//     [3] TWO-ENGINE A/B, EXPLICIT FAMILY -- the SAME FontFaceDesc (non-null `family`) is fed
//         to a `FontEngine::Own` session AND a `FontEngine::FreeType` session, sequentially in
//         this one process (RmlUi's own `Shutdown()` between UiLayer instances resets the
//         global font_interface -- the SAME confirmed re-init semantics
//         fonteng_engine_select_sanity.cpp already relies on). Both MUST register successfully
//         and both MUST render non-blank -- proof that an explicit `family` resolves the
//         engine-asymmetry FontFaceDesc's own header comment documents (glintfx/include/
//         glintfx/font_face.hpp): a caller that needs a portable, predictable family string
//         across either engine gets one by passing `family` explicitly.
//     [4] DERIVED FAMILY (`family = nullptr`) -- the asymmetric path. `FontEngine::Own`
//         DERIVES the family from the file's own STEM (documented, exact contract:
//         FontEngineOwn::LoadFontFace's family-less overload, font_engine_own.cpp) -- this is
//         asserted POSITIVELY: registering `assets/OpenSans-Regular.ttf` with no family, then
//         selecting `font-family: opensans-regular;` (the exact derived stem, lower-cased),
//         MUST render. `FontEngine::FreeType` reads the font file's own SFNT 'name' table
//         instead (a value this test does NOT hard-code an assumption about) -- only
//         REGISTRATION success is asserted for that engine here, never a specific derived
//         family string; see this test's own [4] block comment and font_face.hpp's header
//         comment for why forcing an equality assumption there would be testing an
//         implementation detail of a third-party font file's metadata, not glintfx's contract.
//     [5] PROGRAMMATIC FALLBACK (`fallback_face = true`) -- reuses the exact two REAL fixture
//         fonts fonteng_fallback_sanity.cpp's review brief established
//         (assets/PixelOperatorMono.ttf, Latin-only, confirmed by that file's own header cmap
//         probe to LACK U+00B2 SUPERSCRIPT TWO; assets/OpenSans-Regular.ttf, confirmed to HAVE
//         it), but drives the SAME fallback mechanism via `FontFaceDesc::fallback_face`
//         PROGRAMMATICALLY instead of RCSS's `-rmlui-fallback-face: true;` `@font-face`
//         property -- proving the API-level knob reaches the identical BakeGlyph() fallback
//         walk (src/font_engine_own.cpp) without ever authoring an `@font-face` block.
//     [6] BEFORE-INIT / NOT-YET-ATTACHED -- WHITE-BOX, at the exact layer the guard lives in:
//         a default-constructed `glintfx::Bootstrap` (never `init()`ed) and a default-
//         constructed `glintfx::Engine` (never `attach()`ed) both return `false` from
//         `load_font_face()`, with NO crash and NO GL/RmlUi call made -- confirmed by running
//         BOTH checks before the test's single GLFW host window is even created (`Engine`'s
//         `impl_` is a raw pointer allocated unconditionally in its constructor -- see
//         engine.hpp's own ctor -- so this is always a safe, non-null, "not attached" state,
//         never a null-pointer-dereference risk). A genuinely MOVED-FROM UiLayer/App is
//         DELIBERATELY NOT exercised here: per UiLayer's own class-level doc-comment
//         (ui_layer.hpp), calling ANY method but `ok()`/the destructor on a moved-from instance
//         is documented undefined behaviour (the same convention as `std::unique_ptr`) -- this
//         test does not probe documented UB, it probes the REACHABLE "not yet initialised"
//         state every other guarded method in this codebase is tested against.
//     [7] STYLE/WEIGHT MAPPING (review follow-up, w18-rev) -- two faces of the SAME family,
//         tagged `Normal`/`Normal` and `Italic`/`Bold` respectively, exploit
//         `FontEngineOwn::FindBestFace`'s own scoring as the oracle: an `italic`/`bold` RCSS
//         request must resolve to the DIFFERENT (second-registered, differently-metriced) face
//         than a `normal`/`normal` request does -- proving `FontFaceDesc::style`/`::weight`
//         reach `Rml::LoadFontFace` as requested, not silently flattened to `Normal` by
//         `to_rml_font_style`/`to_rml_font_weight` (`src/bootstrap.cpp`). See this check's own
//         block comment for the full oracle derivation and a documented caveat about
//         `FindBestFace`'s unrelated no-family-match fallback path.
//
//     Deterministic, no sleeps/retries -- pure layout-math width + stable llvmpipe ink counts
//     for plain, un-blurred, un-antialiased-beyond-normal text, same basis every fonteng_*
//     readback test in this suite already relies on (see fonteng_ab_compare.cpp's header for
//     the measured own-vs-FreeType stability this rests on).
// PT: fontface_load_sanity -- gate de TDD pro UI-FONTFACE (v0.24.0):
//     `glintfx::UiLayer::load_font_face(const FontFaceDesc&)` (e, por implementação
//     compartilhada, `glintfx::App::load_font_face` -- ambos encaminham à MESMA implementação
//     `Engine::load_font_face` -> `Bootstrap::load_font_face`; ver o comentário de cabeçalho de
//     bootstrap.cpp pro contrato exato de seleção-de-sobrecarga de `Rml::LoadFontFace` que este
//     arquivo prova ponta-a-ponta). Este arquivo dirige tudo pelo `UiLayer` -- o fio de forward
//     ESPECÍFICO do `App` (provado NÃO ser cobertura redundante pelo review adversarial,
//     `w18-rev`: ele achou `App::load_font_face` completamente sem exercício, mutou pra `false`
//     fixo, e a suíte ainda passava 99/100) é coberto separadamente por
//     `app_font_face_sanity.cpp`, registrado dentro de `if(GLINTFX_BACKEND_GLFW)`. Cobre sete
//     critérios, cada um em seu próprio bloco rotulado abaixo (mesma convenção de
//     múltiplos-critérios-num-arquivo-só que esta suíte já segue, ex.: os "sete critérios" de
//     fonteng_fallback_sanity.cpp):
//
//     [1] REGISTRO VÁLIDO + RENDER -- um arquivo de fonte real registrado com família explícita
//         retorna `true`, e texto de fato atribuído àquela família renderiza (tinta não-branca,
//         largura de avanço não-zero) -- um oráculo ESTATÍSTICO (não pixel-exato), mesma
//         convenção de todo outro teste de readback fonteng_* desta suíte (llvmpipe sob Xvfb
//         não é bit-estável entre versões de driver).
//     [2] ENTRADA HOSTIL -- caminho `nullptr`, caminho vazio (`""`), um caminho que não existe
//         em disco, e um caminho que EXISTE mas não é fonte nenhuma (o próprio arquivo de cena
//         `.rml` deste teste, alimentado como bytes de fonte falsos) todos retornam `false`,
//         SEM crash -- fail-high AUD-TEC-5, mesma disciplina de todo outro método do Bootstrap.
//     [3] A/B DE DOIS MOTORES, FAMÍLIA EXPLÍCITA -- o MESMO FontFaceDesc (`family` não-nulo) é
//         alimentado a uma sessão `FontEngine::Own` E a uma sessão `FontEngine::FreeType`,
//         sequencialmente neste ÚNICO processo (o próprio `Shutdown()` do RmlUi entre instâncias
//         UiLayer reseta o font_interface global -- a MESMA semântica de re-init confirmada que
//         fonteng_engine_select_sanity.cpp já usa). AS DUAS precisam registrar com sucesso E
//         renderizar não-em-branco -- prova de que uma `family` explícita resolve a assimetria
//         de motor que o próprio comentário de cabeçalho do FontFaceDesc documenta
//         (glintfx/include/glintfx/font_face.hpp): um chamador que precisa de uma string de
//         família portável e previsível através de qualquer um dos motores consegue uma
//         passando `family` explicitamente.
//     [4] FAMÍLIA DERIVADA (`family = nullptr`) -- o caminho assimétrico. `FontEngine::Own`
//         DERIVA a família do próprio STEM do arquivo (contrato exato documentado: sobrecarga
//         sem-família de FontEngineOwn::LoadFontFace, font_engine_own.cpp) -- isto é afirmado
//         POSITIVAMENTE: registrar `assets/OpenSans-Regular.ttf` sem família, depois selecionar
//         `font-family: opensans-regular;` (o stem derivado exato, minusculizado), PRECISA
//         renderizar. `FontEngine::FreeType` lê a própria tabela 'name' SFNT do arquivo de fonte
//         em vez disso (um valor que este teste NÃO fixa suposição nenhuma sobre) -- só o
//         sucesso de REGISTRO é afirmado para aquele motor aqui, nunca uma string de família
//         derivada específica; ver o próprio comentário do bloco [4] deste teste e o comentário
//         de cabeçalho de font_face.hpp pro motivo de forçar uma suposição de igualdade ali
//         estar testando um detalhe de implementação dos metadados de um arquivo de fonte de
//         terceiro, não um contrato da glintfx.
//     [5] FALLBACK PROGRAMÁTICO (`fallback_face = true`) -- reusa as EXATAS duas fontes REAIS de
//         fixture que a tarefa de review do fonteng_fallback_sanity.cpp estabeleceu
//         (assets/PixelOperatorMono.ttf, Latin-only, confirmado pela sonda de cmap do próprio
//         cabeçalho daquele arquivo que NÃO TEM U+00B2 SUPERSCRIPT TWO; assets/
//         OpenSans-Regular.ttf, confirmado que TEM), mas dirige o MESMO mecanismo de fallback
//         via `FontFaceDesc::fallback_face` PROGRAMATICAMENTE em vez da propriedade RCSS
//         `-rmlui-fallback-face: true;` de `@font-face` -- provando que o botão em nível de API
//         alcança a caminhada de fallback idêntica do BakeGlyph() (src/font_engine_own.cpp) sem
//         nunca autorar um bloco `@font-face`.
//     [6] ANTES-DE-INIT / AINDA-NÃO-ANEXADO -- WHITE-BOX, exatamente na camada onde o guard
//         mora: um `glintfx::Bootstrap` default-construído (nunca `init()`ado) e um
//         `glintfx::Engine` default-construído (nunca `attach()`ado) ambos retornam `false` de
//         `load_font_face()`, SEM crash e SEM chamada nenhuma a GL/RmlUi -- confirmado rodando
//         OS DOIS cheques antes até da única janela host GLFW deste teste ser criada (o `impl_`
//         do `Engine` é um ponteiro cru alocado incondicionalmente no construtor dele -- ver o
//         próprio ctor em engine.hpp -- então este é sempre um estado seguro, não-nulo, de
//         "não anexado", nunca um risco de desreferência de ponteiro nulo). Um UiLayer/App
//         genuinamente MOVIDO-DE é DELIBERADAMENTE NÃO exercitado aqui: pelo próprio
//         doc-comment de nível de classe do UiLayer (ui_layer.hpp), chamar QUALQUER método além
//         de `ok()`/o destrutor numa instância movida-de é comportamento indefinido documentado
//         (a mesma convenção do `std::unique_ptr`) -- este teste não sonda UB documentado, sonda
//         o estado ALCANÇÁVEL de "ainda não inicializado" contra o qual todo outro método
//         guardado deste código-base é testado.
//     [7] MAPEAMENTO DE STYLE/WEIGHT (follow-up de review, w18-rev) -- duas faces da MESMA
//         família, marcadas `Normal`/`Normal` e `Italic`/`Bold` respectivamente, exploram o
//         próprio escore de `FontEngineOwn::FindBestFace` como oráculo: um pedido RCSS
//         `italic`/`bold` precisa resolver pra face DIFERENTE (segundo-registrada, com métrica
//         diferente) da que um pedido `normal`/`normal` resolve -- provando que
//         `FontFaceDesc::style`/`::weight` chegam ao `Rml::LoadFontFace` como pedido, não
//         achatados silenciosamente pra `Normal` por `to_rml_font_style`/`to_rml_font_weight`
//         (`src/bootstrap.cpp`). Ver o próprio comentário de bloco deste cheque pra derivação
//         completa do oráculo e uma ressalva documentada sobre o caminho de fallback
//         sem-match-de-família (não relacionado) do `FindBestFace`.
//
//     Determinístico, sem sleeps/retries -- pura matemática de layout pra largura e contagens de
//     tinta estáveis sob llvmpipe pra texto simples, sem blur, sem antialiasing além do normal,
//     mesma base que todo outro teste de readback fonteng_* desta suíte já usa (ver o cabeçalho
//     de fonteng_ab_compare.cpp pra estabilidade medida próprio-vs-FreeType em que isto se apoia).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/rml/bootstrap.hpp"
#include "../src/engine.hpp"
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp" // EN: pulls gl_loader.h; GL already loaded by the UiLayer ctor.
                         // PT: puxa gl_loader.h; GL já carregado pelo ctor da UiLayer.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

constexpr int W = 640, H = 360;
constexpr float kMinWidthPx = 6.0f;
constexpr long kMinInk = 10;

int g_failures = 0;

void fail(const char* what) {
  std::fprintf(stderr, "fontface_load_sanity FAIL: %s\n", what);
  ++g_failures;
}

// EN: Same flip as fonteng_engine_select_sanity.cpp (glReadPixels is bottom-left origin;
//     get_element_box() is top-left, y-down).
// PT: Mesmo flip de fonteng_engine_select_sanity.cpp (origem do glReadPixels é bottom-left;
//     get_element_box() é top-left, y-para-baixo).
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

struct Probe {
  bool found = false;
  float w = 0.f;
  long ink = 0;
};

// EN: Renders TWO frames (kerning/glyph-bake settles by the 2nd, same convention as
//     fonteng_engine_select_sanity.cpp), reads the backbuffer back, and measures span "txt".
// PT: Renderiza DOIS frames (kerning/bake de glyph assenta no 2º, mesma convenção de
//     fonteng_engine_select_sanity.cpp), lê o backbuffer de volta, e mede o span "txt".
Probe measure_span(glintfx::UiLayer& ui) {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  ui.update();
  ui.render();
  ui.update();
  ui.render();

  std::vector<unsigned char> px(static_cast<size_t>(W) * H * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  flip_to_top_down(px, W, H);

  Probe p;
  const glintfx::ElementBox box = ui.get_element_box("txt");
  p.found = box.found;
  p.w = box.w;
  p.ink = box.found ? count_ink_rect(px, W, H, box.x, box.y, box.w, box.h) : 0;
  return p;
}

// EN: [6] BEFORE-INIT / NOT-YET-ATTACHED -- white-box, no GL/window needed at all. Run FIRST,
//     before the test's single host window exists, so a regression that somehow required a GL
//     context to hit these guards would be caught by an early crash rather than silently masked
//     by a context that happens to already be current later in main().
// PT: [6] ANTES-DE-INIT / AINDA-NÃO-ANEXADO -- white-box, sem GL/janela nenhuma necessária. Roda
//     PRIMEIRO, antes da única janela host deste teste existir, então uma regressão que de
//     algum jeito exigisse um contexto GL pra atingir estes guards seria pega por um crash cedo
//     em vez de mascarada em silêncio por um contexto que por acaso já está corrente depois em
//     main().
void check_before_init() {
  glintfx::FontFaceDesc desc{};
  desc.path = "assets/OpenSans-Regular.ttf";
  desc.family = "beforeinit";

  glintfx::Bootstrap boot; // EN: never init()ed. PT: nunca init()ado.
  if (boot.load_font_face(desc)) fail("[6-bootstrap] load_font_face on a never-init()ed Bootstrap returned true");

  glintfx::Engine eng; // EN: never attach()ed. PT: nunca attach()ado.
  if (eng.load_font_face(desc)) fail("[6-engine] load_font_face on a never-attach()ed Engine returned true");
}

// EN: [1] valid registration+render, and [2] hostile-input guards, both against ONE
//     FontEngine::Own session (path/family guards are engine-agnostic -- they run before
//     Rml::LoadFontFace is ever called, see Bootstrap::load_font_face's guard-1/guard-2).
// PT: [1] registro+render válido, e [2] guards de entrada hostil, ambos contra UMA sessão
//     FontEngine::Own (os guards de path/family são agnósticos de motor -- rodam antes do
//     Rml::LoadFontFace sequer ser chamado, ver guard-1/guard-2 de Bootstrap::load_font_face).
void check_valid_and_hostile() {
  glintfx::UiLayer ui({.logical_width = W, .logical_height = H, .font_engine = glintfx::FontEngine::Own});
  if (!ui.ok()) {
    fail("[1] UiLayer attach failed");
    return;
  }

  // ---- [2] hostile input --------------------------------------------------
  glintfx::FontFaceDesc bad{};
  bad.family = "hostile";

  bad.path = nullptr;
  if (ui.load_font_face(bad)) fail("[2-nullptr] load_font_face(path=nullptr) returned true");

  bad.path = "";
  if (ui.load_font_face(bad)) fail("[2-empty] load_font_face(path=\"\") returned true");

  bad.path = "assets/this_file_does_not_exist_at_all.ttf";
  if (ui.load_font_face(bad)) fail("[2-nonexistent] load_font_face(path=<nonexistent>) returned true");

  // EN: A REAL, EXISTING file that is not a font at all -- this test's own RML scene, fed in
  //     as bogus font bytes. Neither engine's SFNT parser should accept it.
  // PT: Um arquivo REAL, EXISTENTE, que não é fonte nenhuma -- a própria cena RML deste teste,
  //     alimentada como bytes de fonte falsos. Nenhum dos dois parsers SFNT deve aceitar.
  bad.path = "fontface_scene.rml";
  if (ui.load_font_face(bad)) fail("[2-corrupt] load_font_face(path=<non-font file>) returned true");

  // ---- [1] valid registration + render ------------------------------------
  glintfx::FontFaceDesc good{};
  good.path = "assets/OpenSans-Regular.ttf";
  good.family = "TestFace";
  if (!ui.load_font_face(good)) {
    fail("[1] load_font_face(valid, explicit family) returned false");
    return;
  }

  if (!ui.load("fontface_scene.rml")) {
    fail("[1] UiLayer::load(fontface_scene.rml) returned false");
    return;
  }
  ui.set_property("txt", "font-family", "TestFace");
  ui.set_text("txt", "Hello");

  const Probe p = measure_span(ui);
  if (!p.found)
    fail("[1] get_element_box(\"txt\") not found after load_font_face+set_property");
  else if (p.w < kMinWidthPx)
    fail("[1] rendered span width too small -- font likely did not resolve");
  else if (p.ink < kMinInk)
    fail("[1] rendered span ink too low -- font likely rendered blank");
}

// EN: Runs one UiLayer session under `engine`, registering `desc` (family already set by the
//     caller) and rendering "txt" under it. Returns whether load_font_face()+load()+render
//     succeeded and the measured span, via out-params.
// PT: Roda uma sessão UiLayer sob `engine`, registrando `desc` (family já definida pelo
//     chamador) e renderizando "txt" sob ela. Retorna se load_font_face()+load()+render deram
//     certo e o span medido, via out-params.
void run_session(glintfx::FontEngine engine, const glintfx::FontFaceDesc& desc,
                 const char* family_to_select, bool& load_ok, Probe& out) {
  load_ok = false;
  out = Probe{};

  glintfx::UiLayer ui({.logical_width = W, .logical_height = H, .font_engine = engine});
  if (!ui.ok()) return;

  if (!ui.load_font_face(desc)) return;
  load_ok = true;

  if (!ui.load("fontface_scene.rml")) return;
  // EN: A null `family_to_select` means the caller (check_derived_family's FreeType branch)
  //     only cares about the registration call above, not a rendered result -- skip
  //     set_property/set_text/render entirely so RmlUi does not log a spurious "no font-family"
  //     warning for a probe whose outcome is never read.
  // PT: Um `family_to_select` nulo significa que o chamador (ramo FreeType de
  //     check_derived_family) só se importa com a chamada de registro acima, não com um
  //     resultado renderizado -- pula set_property/set_text/render inteiramente para o RmlUi
  //     não logar um aviso espúrio de "sem font-family" pra uma sonda cujo resultado nunca é lido.
  if (!family_to_select) return;
  ui.set_property("txt", "font-family", family_to_select);
  ui.set_text("txt", "Hi");
  out = measure_span(ui);
}

// EN: [3] two-engine A/B with an EXPLICIT family -- proves the struct resolves the asymmetry.
// PT: [3] A/B de dois motores com família EXPLÍCITA -- prova que a struct resolve a assimetria.
void check_ab_explicit_family() {
  glintfx::FontFaceDesc desc{};
  desc.path = "assets/OpenSans-Regular.ttf";
  desc.family = "ABFace";

  bool own_load = false, ft_load = false;
  Probe own_p, ft_p;
  run_session(glintfx::FontEngine::Own, desc, "ABFace", own_load, own_p);
  run_session(glintfx::FontEngine::FreeType, desc, "ABFace", ft_load, ft_p);

  if (!own_load) fail("[3-own] load_font_face(explicit family) returned false on FontEngine::Own");
  if (!ft_load) fail("[3-ft] load_font_face(explicit family) returned false on FontEngine::FreeType");
  if (!own_load || !ft_load) return;

  if (!own_p.found || own_p.w < kMinWidthPx || own_p.ink < kMinInk)
    fail("[3-own] explicit-family session rendered blank/near-blank on FontEngine::Own");
  if (!ft_p.found || ft_p.w < kMinWidthPx || ft_p.ink < kMinInk)
    fail("[3-ft] explicit-family session rendered blank/near-blank on FontEngine::FreeType");

  std::printf(
      "fontface_load_sanity [3] explicit family 'ABFace': own(w=%.2f ink=%ld) "
      "freetype(w=%.2f ink=%ld) -- both engines resolved the SAME explicit family\n",
      own_p.w, own_p.ink, ft_p.w, ft_p.ink);
}

// EN: [4] derived family (family=nullptr) -- asymmetric by design; see this file's own header
//     for why FreeType's side asserts only registration, never a specific family string.
// PT: [4] família derivada (family=nullptr) -- assimétrico por design; ver o próprio cabeçalho
//     deste arquivo pro motivo do lado FreeType afirmar só o registro, nunca uma string de
//     família específica.
void check_derived_family() {
  glintfx::FontFaceDesc desc{};
  desc.path = "assets/OpenSans-Regular.ttf"; // family left nullptr -> derived.

  // EN: Own's documented, exact contract (font_engine_own.cpp's family-less LoadFontFace
  //     overload): family = the file's own stem, lower-cased -- "OpenSans-Regular" -> "opensans-regular".
  // PT: Contrato exato e documentado do Own (sobrecarga sem-família de LoadFontFace de
  //     font_engine_own.cpp): family = o próprio stem do arquivo, minusculizado --
  //     "OpenSans-Regular" -> "opensans-regular".
  bool own_load = false;
  Probe own_p;
  run_session(glintfx::FontEngine::Own, desc, "opensans-regular", own_load, own_p);
  if (!own_load) {
    fail("[4-own] load_font_face(family=nullptr) returned false on FontEngine::Own");
    return;
  }
  if (!own_p.found || own_p.w < kMinWidthPx || own_p.ink < kMinInk)
    fail(
        "[4-own] the documented stem-derivation ('opensans-regular') did not render -- "
        "FontEngineOwn's family-less derivation contract may have changed");
  else
    std::printf(
        "fontface_load_sanity [4-own] derived family renders as documented "
        "(stem 'opensans-regular', w=%.2f ink=%ld)\n",
        own_p.w, own_p.ink);

  // EN: FreeType derives the family from the font file's own SFNT 'name' table instead of the
  //     stem -- deliberately NOT probed for a specific string here (that would assert an
  //     assumption about OpenSans-Regular.ttf's embedded metadata, not a glintfx contract).
  //     Only the registration call itself (the part glintfx DOES own the contract for) is
  //     checked: the file must still load successfully through this engine too.
  // PT: O FreeType deriva a família da própria tabela 'name' SFNT do arquivo de fonte em vez do
  //     stem -- deliberadamente NÃO sondado por uma string específica aqui (isso afirmaria uma
  //     suposição sobre os metadados embutidos de OpenSans-Regular.ttf, não um contrato da
  //     glintfx). Só a própria chamada de registro (a parte cujo contrato a glintfx DE FATO
  //     possui) é checada: o arquivo precisa continuar carregando com sucesso também neste motor.
  bool ft_load = false;
  Probe ft_p;
  run_session(glintfx::FontEngine::FreeType, desc, nullptr, ft_load, ft_p);
  if (!ft_load)
    fail(
        "[4-ft] load_font_face(family=nullptr) returned false on FontEngine::FreeType "
        "-- registration itself (not a specific derived family) must still succeed");
  else
    std::printf(
        "fontface_load_sanity [4-ft] FreeType registered the family-less face "
        "successfully (family derivation is engine-internal, not asserted here)\n");
}

// EN: [5] fallback_face=true, driven purely programmatically -- reuses the exact two fixture
//     fonts fonteng_fallback_sanity.cpp's review brief established.
// PT: [5] fallback_face=true, dirigido puramente programaticamente -- reusa as exatas duas
//     fontes de fixture que a tarefa de review de fonteng_fallback_sanity.cpp estabeleceu.
void check_programmatic_fallback() {
  glintfx::UiLayer ui({.logical_width = W, .logical_height = H, .font_engine = glintfx::FontEngine::Own});
  if (!ui.ok()) {
    fail("[5] UiLayer attach failed");
    return;
  }

  glintfx::FontFaceDesc primary{};
  primary.path = "assets/PixelOperatorMono.ttf"; // EN: Latin-only, lacks U+00B2.
                                                 // PT: Latin-only, sem U+00B2.
  primary.family = "FbPrimary";
  primary.fallback_face = false;

  glintfx::FontFaceDesc fallback{};
  fallback.path = "assets/OpenSans-Regular.ttf"; // EN: has U+00B2. PT: tem U+00B2.
  fallback.family = "FbCandidate";
  fallback.fallback_face = true; // EN: THE knob under test. PT: O botão sob teste.

  if (!ui.load_font_face(primary)) {
    fail("[5] load_font_face(primary) returned false");
    return;
  }
  if (!ui.load_font_face(fallback)) {
    fail("[5] load_font_face(fallback_face=true) returned false");
    return;
  }

  if (!ui.load("fontface_scene.rml")) {
    fail("[5] UiLayer::load(fontface_scene.rml) returned false");
    return;
  }
  ui.set_property("txt", "font-family", "FbPrimary");
  // EN: U+00B2 SUPERSCRIPT TWO, UTF-8 "\xC2\xB2" -- absent from PixelOperatorMono, present in
  //     OpenSans-Regular (same fixture pair/codepoint fonteng_fallback_sanity.cpp's own header
  //     documents, cmap-probe-confirmed).
  // PT: U+00B2 SUPERSCRIPT TWO, UTF-8 "\xC2\xB2" -- ausente do PixelOperatorMono, presente no
  //     OpenSans-Regular (mesmo par de fixture/codepoint que o próprio cabeçalho de
  //     fonteng_fallback_sanity.cpp documenta, confirmado por sonda de cmap).
  ui.set_text("txt", "\xC2\xB2");

  const Probe p = measure_span(ui);
  if (!p.found) {
    fail("[5] get_element_box(\"txt\") not found");
    return;
  }
  if (p.ink < kMinInk)
    fail(
        "[5] fallback_face=true did not unlock U+00B2 via the fallback face -- span rendered "
        "blank/near-blank (primary font cannot render it on its own)");
  else
    std::printf(
        "fontface_load_sanity [5] programmatic fallback_face=true unlocked U+00B2 "
        "(ink=%ld)\n",
        p.ink);
}

// EN: [7] STYLE/WEIGHT MAPPING (review follow-up, w18-rev -- mutation (d)) -- proves
//     `FontFaceDesc::style`/`FontFaceDesc::weight` actually reach RmlUi as the requested
//     `Italic`/`Bold`, not silently flattened to `Normal` by `to_rml_font_style`/
//     `to_rml_font_weight` (`src/bootstrap.cpp`). Before this check, EVERY `FontFaceDesc` this
//     file constructed left `style`/`weight` at their defaults (`Normal`/`Auto`) -- the
//     adversarial review mutated `to_rml_font_weight` to map `Bold` to
//     `Rml::Style::FontWeight::Normal` and confirmed the WHOLE suite still passed, because
//     nothing exercised a non-default value.
//
//     NO real bold/italic FONT FILE fixture exists in this suite (`assets/OpenSans-Regular.ttf`
//     and `assets/PixelOperatorMono.ttf` are both a single, "Regular" cut each) -- rendering a
//     TRUE synthetic bold/italic is not something either font engine does on its own, so a
//     visual "does italic text lean" check is not constructible from what is on disk. Instead
//     this exploits `FontEngineOwn::FindBestFace`'s own SCORING (`src/font_engine_own.cpp`:
//     family match required, `+10` for an exact `style` match, `+5` for an exact `weight`
//     match, ties won by the FIRST-registered candidate) as the oracle.
//
//     TWO INDEPENDENT SUB-PROBES, EACH ISOLATING ONE AXIS (`TDD RED-PHASE FINDING`, see below
//     for why a SINGLE combined Italic+Bold probe is NOT sufficient to kill a weight-only
//     mutation): a request that differs from Face A in BOTH style AND weight lets a `+10`
//     style-match bonus alone decide the winner, so a broken `to_rml_font_weight` (the `+5`
//     term) can hide behind a correct `to_rml_font_style` and the check would still pass for
//     the wrong reason. Isolating each axis against a same-on-the-other-axis pair removes that
//     escape hatch:
//       - STYLE sub-probe: Face S1 `assets/PixelOperatorMono.ttf`, family `"StyleOnly"`,
//         `Normal`/`Normal` (registered FIRST); Face S2 `assets/OpenSans-Regular.ttf`, SAME
//         family, `Italic`/`Normal` (SAME weight as S1, registered SECOND). An `italic`/`normal`
//         RCSS request scores S1 `105` (`100 + 0 + 5`, weight matches, style does not) vs. S2
//         `115` (`100 + 10 + 5`) -- S2 must win. If `to_rml_font_style` collapsed S2's `Italic`
//         to `Normal`, BOTH would score `105` and the first-registered tie-break would make S1
//         (the WRONG face) win.
//       - WEIGHT sub-probe: Face W1 `assets/PixelOperatorMono.ttf`, family `"WeightOnly"`,
//         `Normal`/`Normal` (registered FIRST); Face W2 `assets/OpenSans-Regular.ttf`, SAME
//         family, `Normal`/`Bold` (SAME style as W1, registered SECOND). A `normal`/`bold` RCSS
//         request scores W1 `110` (`100 + 10 + 0`, style matches, weight does not) vs. W2 `115`
//         (`100 + 10 + 5`) -- W2 must win. If `to_rml_font_weight` collapsed W2's `Bold` to
//         `Normal` (the EXACT mutation the review applied), BOTH would score `110` and the
//         first-registered tie-break would make W1 (the WRONG face) win -- THIS is the probe
//         that specifically kills mutation (d); the combined single-probe design this file
//         shipped with initially did NOT (confirmed by re-running it against the applied
//         mutation during this check's own TDD red phase: it still passed, because the style
//         axis alone supplied enough margin to keep picking the intended face regardless of the
//         weight bug).
//     Both sub-probes reuse the SAME two font files (`PixelOperatorMono`/`OpenSans-Regular`,
//     under DIFFERENT family names so the two probes' `faces_` entries never collide) and the
//     SAME "which face actually got selected" width readout established by the file's earlier
//     A/B checks -- PixelOperatorMono and OpenSans-Regular are different fonts with different
//     advance-width metrics for the same string, so "which face won" is directly readable off
//     `ElementBox::w`.
//
//     KNOWN CAVEAT (the reviewer's "minor" note, documented here per their request): this
//     oracle depends on `FindBestFace` finding a FAMILY match in `faces_` at all -- if no face
//     of the requested family exists, `FindBestFace` falls back to "the first face loaded,
//     period" (`src/font_engine_own.cpp`, "better to show SOMETHING than nothing" comment),
//     which is a DIFFERENT, pre-existing fallback path this test does not exercise or depend on
//     (every face in both sub-probes always matches its own family here). A future test that
//     wants to probe an UNKNOWN family falling back to "first loaded" should not assume that
//     path also respects style/weight -- it explicitly does not (family-match candidates only).
// PT: [7] MAPEAMENTO DE STYLE/WEIGHT (follow-up de review, w18-rev -- mutação (d)) -- prova que
//     `FontFaceDesc::style`/`FontFaceDesc::weight` de fato chegam ao RmlUi como o `Italic`/
//     `Bold` pedido, não achatados silenciosamente pra `Normal` por `to_rml_font_style`/
//     `to_rml_font_weight` (`src/bootstrap.cpp`). Antes deste cheque, TODO `FontFaceDesc` que
//     este arquivo construía deixava `style`/`weight` nos defaults (`Normal`/`Auto`) -- o
//     review adversarial mutou `to_rml_font_weight` pra mapear `Bold` pra
//     `Rml::Style::FontWeight::Normal` e confirmou que a suíte INTEIRA ainda passava, porque
//     nada exercitava um valor não-default.
//
//     NENHUMA fixture de arquivo de fonte bold/italic DE VERDADE existe nesta suíte
//     (`assets/OpenSans-Regular.ttf` e `assets/PixelOperatorMono.ttf` são cada uma um único
//     corte "Regular") -- renderizar um bold/italic sintético DE VERDADE não é algo que nenhum
//     dos dois motores faz sozinho, então um cheque visual "o texto itálico inclina" não é
//     construível a partir do que existe em disco. Em vez disso, isto explora o próprio ESCORE
//     de `FontEngineOwn::FindBestFace` (`src/font_engine_own.cpp`: match de família obrigatório,
//     `+10` por match exato de `style`, `+5` por match exato de `weight`, empate vencido pelo
//     candidato PRIMEIRO-registrado) como oráculo.
//
//     DUAS SUB-SONDAS INDEPENDENTES, CADA UMA ISOLANDO UM EIXO (`ACHADO DA FASE VERMELHA DO
//     TDD`, ver abaixo pro motivo de UMA sonda combinada Italic+Bold NÃO ser suficiente pra
//     matar uma mutação só-de-weight): um pedido que difere da Face A EM STYLE E EM WEIGHT ao
//     mesmo tempo deixa um bônus `+10` de match-de-style sozinho decidir o vencedor, então um
//     `to_rml_font_weight` quebrado (o termo `+5`) pode se esconder atrás de um
//     `to_rml_font_style` correto e o cheque ainda passaria pelo motivo errado. Isolar cada eixo
//     contra um par igual-no-outro-eixo remove essa saída:
//       - Sub-sonda STYLE: Face S1 `assets/PixelOperatorMono.ttf`, família `"StyleOnly"`,
//         `Normal`/`Normal` (registrada PRIMEIRO); Face S2 `assets/OpenSans-Regular.ttf`, MESMA
//         família, `Italic`/`Normal` (MESMO weight de S1, registrada SEGUNDO). Um pedido RCSS
//         `italic`/`normal` escora S1 em `105` (`100 + 0 + 5`, weight casa, style não) vs. S2
//         em `115` (`100 + 10 + 5`) -- S2 precisa vencer. Se `to_rml_font_style` achatasse o
//         `Italic` de S2 pra `Normal`, AS DUAS escorariam `105` e o desempate por
//         primeiro-registrado faria S1 (a face ERRADA) vencer.
//       - Sub-sonda WEIGHT: Face W1 `assets/PixelOperatorMono.ttf`, família `"WeightOnly"`,
//         `Normal`/`Normal` (registrada PRIMEIRO); Face W2 `assets/OpenSans-Regular.ttf`, MESMA
//         família, `Normal`/`Bold` (MESMO style de W1, registrada SEGUNDO). Um pedido RCSS
//         `normal`/`bold` escora W1 em `110` (`100 + 10 + 0`, style casa, weight não) vs. W2 em
//         `115` (`100 + 10 + 5`) -- W2 precisa vencer. Se `to_rml_font_weight` achatasse o
//         `Bold` de W2 pra `Normal` (a mutação EXATA que o review aplicou), AS DUAS escorariam
//         `110` e o desempate por primeiro-registrado faria W1 (a face ERRADA) vencer -- ESTA é
//         a sonda que especificamente mata a mutação (d); o design de sonda-única-combinada com
//         o qual este arquivo saiu inicialmente NÃO matava (confirmado rerodando-o contra a
//         mutação aplicada durante a fase vermelha de TDD deste cheque: ainda passava, porque o
//         eixo style sozinho já dava margem suficiente pra continuar escolhendo a face
//         pretendida independente do bug de weight).
//     As duas sub-sondas reusam os MESMOS dois arquivos de fonte
//     (`PixelOperatorMono`/`OpenSans-Regular`, sob nomes de família DIFERENTES pra que as
//     entradas `faces_` das duas sondas nunca colidam) e a MESMA leitura de largura "qual face
//     de fato foi selecionada" já estabelecida pelos cheques A/B anteriores deste arquivo --
//     PixelOperatorMono e OpenSans-Regular são fontes diferentes com métricas de largura de
//     avanço diferentes pra mesma string, então "qual face venceu" é diretamente legível em
//     `ElementBox::w`.
//
//     RESSALVA CONHECIDA (a nota "menor" do revisor, documentada aqui a pedido dele): este
//     oráculo depende de `FindBestFace` achar um match de FAMÍLIA em `faces_` de qualquer
//     jeito -- se nenhuma face da família pedida existir, `FindBestFace` cai pra "a 1ª face
//     carregada, ponto" (`src/font_engine_own.cpp`, comentário "melhor mostrar ALGO que nada"),
//     que é um caminho de fallback DIFERENTE, pré-existente, que este teste não exercita nem
//     depende dele (toda face das duas sub-sondas sempre casa com a própria família aqui). Um
//     teste futuro que queira sondar uma família DESCONHECIDA caindo pra "1ª carregada" não deve
//     assumir que aquele caminho também respeita style/weight -- explicitamente não respeita
//     (só candidatos com match de família).

// EN: Shared oracle for both sub-probes below -- registers two same-family faces that differ on
//     exactly ONE axis (`varied_style`/`varied_weight` on the SECOND face only; the first is
//     always PixelOperatorMono at Normal/Normal), requests a specific RCSS style/weight pair,
//     and returns the winning face's measured width for the given string. `label` is only for
//     diagnostic printing.
// PT: Oráculo compartilhado pelas duas sub-sondas abaixo -- registra duas faces da mesma
//     família que diferem em exatamente UM eixo (`varied_style`/`varied_weight` só na SEGUNDA
//     face; a primeira é sempre PixelOperatorMono em Normal/Normal), pede um par de style/weight
//     RCSS específico, e retorna a largura medida da face vencedora pra string dada. `label` é
//     só pra impressão de diagnóstico.
struct AxisProbeResult {
  bool ok = false;
  float w_a = 0.f; // EN: width when requesting Face A's own tags (Normal/Normal). PT: largura pedindo as próprias tags de Face A (Normal/Normal).
  float w_b = 0.f; // EN: width when requesting Face B's own tags. PT: largura pedindo as próprias tags de Face B.
};

AxisProbeResult run_axis_probe(const char* label, const char* family, glintfx::FontStyle style_b,
                               glintfx::FontWeight weight_b, const char* rcss_style_b,
                               const char* rcss_weight_b) {
  AxisProbeResult result;

  glintfx::UiLayer ui({.logical_width = W, .logical_height = H, .font_engine = glintfx::FontEngine::Own});
  if (!ui.ok()) {
    fail("[7] UiLayer attach failed");
    return result;
  }

  glintfx::FontFaceDesc face_a{};
  face_a.path = "assets/PixelOperatorMono.ttf";
  face_a.family = family;
  face_a.style = glintfx::FontStyle::Normal;
  face_a.weight = glintfx::FontWeight::Normal;

  glintfx::FontFaceDesc face_b{};
  face_b.path = "assets/OpenSans-Regular.ttf";
  face_b.family = family;
  face_b.style = style_b;
  face_b.weight = weight_b;

  if (!ui.load_font_face(face_a)) {
    fail("[7] load_font_face(face_a, Normal/Normal) returned false");
    return result;
  }
  if (!ui.load_font_face(face_b)) {
    fail("[7] load_font_face(face_b) returned false");
    return result;
  }

  if (!ui.load("fontface_scene.rml")) {
    fail("[7] UiLayer::load(fontface_scene.rml) returned false");
    return result;
  }
  ui.set_property("txt", "font-family", family);
  // EN: A LONGER string than the other checks' "Hi"/"Hello" (empirically, "Hi" at 40px happened
  //     to render to the SAME ~40px width in BOTH PixelOperatorMono and OpenSans -- a genuine
  //     per-font-metrics coincidence for that specific 2-glyph string/size pair, confirmed by
  //     temporary instrumentation during this check's own TDD red phase, NOT a production bug).
  //     A longer, mixed-glyph string amplifies any real per-character advance difference between
  //     a monospace pixel font (PixelOperatorMono) and a proportional one (OpenSans) n-fold,
  //     making a coincidental tie astronomically unlikely.
  // PT: Uma string MAIS LONGA que o "Hi"/"Hello" dos outros cheques (empiricamente, "Hi" a 40px
  //     por acaso renderizava na MESMA largura ~40px TANTO no PixelOperatorMono QUANTO no
  //     OpenSans -- uma coincidência genuína de métrica-por-fonte pra aquele par específico de
  //     string de 2 glyphs/tamanho, confirmada por instrumentação temporária durante a fase
  //     vermelha de TDD deste cheque, NÃO um bug de produção). Uma string mais longa e de
  //     glyphs variados amplifica qualquer diferença real de avanço por-caractere entre uma
  //     fonte monospace de pixel (PixelOperatorMono) e uma proporcional (OpenSans) por N vezes,
  //     tornando um empate coincidente astronomicamente improvável.
  ui.set_text("txt", "HelloWorld123");

  ui.set_property("txt", "font-style", "normal");
  ui.set_property("txt", "font-weight", "normal");
  const Probe a_probe = measure_span(ui);

  ui.set_property("txt", "font-style", rcss_style_b);
  ui.set_property("txt", "font-weight", rcss_weight_b);
  const Probe b_probe = measure_span(ui);

  if (!a_probe.found || !b_probe.found) {
    fail("[7] get_element_box(\"txt\") not found for one of the two probes");
    return result;
  }
  if (a_probe.w < kMinWidthPx || b_probe.w < kMinWidthPx) {
    fail("[7] one of the two probes rendered a near-zero-width span -- neither face resolved");
    return result;
  }

  result.ok = true;
  result.w_a = a_probe.w;
  result.w_b = b_probe.w;
  std::printf("fontface_load_sanity [7-%s] normal/normal(Face A) w=%.2f  %s/%s(Face B) w=%.2f\n",
              label, a_probe.w, rcss_style_b, rcss_weight_b, b_probe.w);
  return result;
}

void check_style_weight_mapping() {
  // EN: PixelOperatorMono (a fixed-pitch pixel font) vs. OpenSans (proportional) at 40px
  //     produce widths for "HelloWorld123" that differ by tens of pixels in practice --
  //     kMinDeltaPx is a small fraction of that measured gap, deliberately loose (this check
  //     only needs to prove "a DIFFERENT face was selected", not measure the exact metrics of
  //     either font).
  // PT: PixelOperatorMono (fonte de pixel de passo fixo) vs. OpenSans (proporcional) a 40px
  //     produzem larguras para "HelloWorld123" que divergem por dezenas de pixels na prática --
  //     kMinDeltaPx é uma fração pequena desse gap medido, deliberadamente frouxo (este cheque
  //     só precisa provar "uma face DIFERENTE foi selecionada", não medir a métrica exata de
  //     nenhuma fonte).
  constexpr float kMinDeltaPx = 3.0f;

  // EN: STYLE sub-probe -- Face B differs ONLY in style (Italic vs Face A's Normal), SAME
  //     (Normal) weight as Face A. An italic/normal request must resolve Face B (OpenSans).
  // PT: Sub-sonda STYLE -- Face B difere SÓ em style (Italic vs. o Normal de Face A), MESMO
  //     weight (Normal) de Face A. Um pedido italic/normal precisa resolver Face B (OpenSans).
  const AxisProbeResult style_result = run_axis_probe(
      "style", "StyleOnly", glintfx::FontStyle::Italic, glintfx::FontWeight::Normal, "italic",
      "normal");
  if (style_result.ok) {
    const float delta = std::fabs(style_result.w_a - style_result.w_b);
    if (delta < kMinDeltaPx)
      fail(
          "[7-style] normal/normal and italic/normal requests resolved to the SAME width -- "
          "FontStyle::Italic likely collapsed to Normal by to_rml_font_style (bootstrap.cpp)");
  }

  // EN: WEIGHT sub-probe -- Face B differs ONLY in weight (Bold vs Face A's Normal), SAME
  //     (Normal) style as Face A. A normal/bold request must resolve Face B (OpenSans). THIS is
  //     the probe that specifically kills the review's mutation (d) -- see the block comment
  //     above for why the style sub-probe alone cannot.
  // PT: Sub-sonda WEIGHT -- Face B difere SÓ em weight (Bold vs. o Normal de Face A), MESMO
  //     style (Normal) de Face A. Um pedido normal/bold precisa resolver Face B (OpenSans). ESTA
  //     é a sonda que especificamente mata a mutação (d) do review -- ver o comentário de bloco
  //     acima pro motivo da sub-sonda de style sozinha não conseguir.
  const AxisProbeResult weight_result = run_axis_probe(
      "weight", "WeightOnly", glintfx::FontStyle::Normal, glintfx::FontWeight::Bold, "normal",
      "bold");
  if (weight_result.ok) {
    const float delta = std::fabs(weight_result.w_a - weight_result.w_b);
    if (delta < kMinDeltaPx)
      fail(
          "[7-weight] normal/normal and normal/bold requests resolved to the SAME width -- "
          "FontWeight::Bold likely collapsed to Normal by to_rml_font_weight (bootstrap.cpp), "
          "so both requests picked the SAME (first-registered, PixelOperatorMono) face instead "
          "of the weight-matched OpenSans one");
  }
}

} // namespace

int main() {
  // EN: [6] runs FIRST -- no GL/window dependency at all (see its own comment above).
  // PT: [6] roda PRIMEIRO -- nenhuma dependência de GL/janela (ver o próprio comentário acima).
  check_before_init();

  glintfx::WindowGlfw host;
  if (!host.create("fontface_load_sanity_host", W, H)) {
    std::puts("fontface_load_sanity FAIL: host window create failed");
    return 1;
  }

  check_valid_and_hostile();     // [1] + [2]
  check_ab_explicit_family();    // [3]
  check_derived_family();        // [4]
  check_programmatic_fallback(); // [5]
  check_style_weight_mapping();  // [7]

  if (g_failures == 0) {
    std::puts(
        "fontface_load_sanity: PASS "
        "(valid+render, hostile input, A/B explicit family, derived-family asymmetry "
        "documented, programmatic fallback, before-init guards, style/weight mapping)");
    return 0;
  }
  std::fprintf(stderr, "fontface_load_sanity: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
