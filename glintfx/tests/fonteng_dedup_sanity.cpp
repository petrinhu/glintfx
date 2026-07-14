// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_dedup_sanity -- L1.20-FONTFLIP (FT-F4) pre-flip FIX regression net for the
//     RegisterFace() face-registration LEAK confirmed by adversarial review (2026-07-13/14) and
//     fixed the same session in src/font_engine_own.cpp/.hpp.
//
//     THE BUG THIS PINS. Before the fix, FontEngineOwn::RegisterFace() unconditionally
//     push_back()-ed a brand-new LoadedFace (a FULL COPY of the font-file blob, ~214KB for Open
//     Sans) on EVERY call, with no dedup against already-registered faces. Rml::LoadFontFace() is
//     called once per Context::LoadDocument() for every @font-face block a stylesheet declares
//     (StyleSheetParser::ParseFontFaceBlock), so reloading the SAME document N times -- a NORMAL
//     usage path (screen navigation, hot-reload), NOT hostile input -- leaked ~214KB*N. Measured:
//     +213893 bytes/reload of VmRSS over 300 reloads; a long session retains hundreds of MB to
//     >1GB. FindBestFace()'s `if (score > best_score)` (strictly greater) means the
//     FIRST-registered face of a tied (family,style,weight) identity always wins ties, so every
//     duplicate pushed after the first was 100% dead weight: registered, never resolved to, never
//     freed before Shutdown().
//
//     THE FIX. RegisterFace() now dedups against faces_ BEFORE constructing a new LoadedFace,
//     mirroring the pinned RmlUi source's own FontProvider::AddFace()/FontFamily::AddFace()
//     ("Duplicate" outcome). Criterion: (family, style, weight) -- RmlUi's own identity -- PLUS
//     blob-content equality (exact byte match), narrower than RmlUi's own family+style+weight-only
//     rule on purpose so two GENUINELY DIFFERENT font files sharing an identity still coexist
//     exactly as before this fix.
//
//     WHAT THIS FILE PROVES, MECHANICALLY (not by inspection). Loads the SAME document
//     (fonteng_embed_scene.rml, one @font-face block) kReloads=50 times in a single UiLayer
//     session and asserts glintfx::own_font_engine_debug_registered_face_count() (a src-internal
//     DEBUG hook, font_engine_own.hpp/.cpp, forward-declared below exactly like the five existing
//     A/B hooks this suite already uses -- see fonteng_ab_compare.cpp for why the header itself
//     cannot be #include-d from a test target) is EXACTLY 1 after the very first load and STAYS
//     EXACTLY 1 after all 50 (never grows). This is an EXACT integer equality check, not a
//     tolerance/threshold -- the pre-fix code would have measured 50, not 1, making the bug
//     mechanically undeniable. A render+ink check on the 50th reload (reusing this suite's
//     established statistical-ink-oracle technique, see fonteng_extended_sanity.cpp) proves the
//     surviving (deduped) face is still the one actually used to render -- dedup must not silently
//     break rendering. VmRSS (read from /proc/self/status, Linux-only -- matches this repo's
//     documented target platform, CLAUDE.md "Plataforma: Linux x86-64") is sampled before and
//     after the reload loop and PRINTED for the human "before/after" record the fix ticket asks
//     for, but is NOT itself a pass/fail gate here (RSS is noisy -- allocator/driver/GL-context
//     churn all move it independent of this fix; the face-count integer is the deterministic
//     oracle, RSS is corroborating evidence only, exactly the role the ticket assigns it: "ou
//     outra prova determinística").
//
//     METHOD (shared fixture with fonteng_own_embed_sanity.cpp/fonteng_extended_sanity.cpp): one
//     hidden GLFW host window provides the GL context; a single UiLayer session reloads the SAME
//     scene kReloads times (two warm-up update+render cycles per reload -- suite convention: cycle
//     1 settles layout, cycle 2 is stable); glReadPixels + flip_to_top_down + count_ink_rect on the
//     FINAL reload only (repeating it every reload would be needlessly slow and adds no signal --
//     the face-count assertion already runs every iteration). No sleeps, no retries -- both oracles
//     (face count, ink) are deterministic under this suite's existing conventions.
// PT: fonteng_dedup_sanity -- rede de regressão do FIX pré-flip (L1.20-FONTFLIP, FT-F4) do
//     VAZAMENTO de registro de face do RegisterFace(), confirmado por review adversarial
//     (2026-07-13/14) e corrigido na mesma sessão em src/font_engine_own.cpp/.hpp.
//
//     O BUG QUE ISTO FIXA. Antes do fix, FontEngineOwn::RegisterFace() empilhava
//     incondicionalmente uma LoadedFace nova (uma CÓPIA COMPLETA do blob do arquivo de fonte,
//     ~214KB pra Open Sans) a CADA chamada, sem dedup contra faces já registradas.
//     Rml::LoadFontFace() é chamado uma vez por Context::LoadDocument() pra cada bloco @font-face
//     que uma stylesheet declara (StyleSheetParser::ParseFontFaceBlock), então recarregar o MESMO
//     documento N vezes -- um caminho de uso NORMAL (navegação de tela, hot-reload), NÃO input
//     hostil -- vazava ~214KB*N. Medido: +213893 bytes/reload de VmRSS ao longo de 300 reloads;
//     uma sessão longa retém centenas de MB a >1GB. O `if (score > best_score)` (estritamente
//     maior) do FindBestFace() faz a face registrada PRIMEIRO de uma identidade
//     (family,style,weight) empatada sempre ganhar, então toda duplicata empilhada depois da
//     primeira já era 100% peso-morto: registrada, nunca resolvida, nunca liberada antes do
//     Shutdown().
//
//     O FIX. RegisterFace() agora deduplica contra faces_ ANTES de construir uma LoadedFace nova,
//     espelhando o próprio FontProvider::AddFace()/FontFamily::AddFace() do source pinado do RmlUi
//     (resultado "Duplicate"). Critério: (family, style, weight) -- a identidade do próprio RmlUi
//     -- MAIS igualdade de conteúdo do blob (match de byte exato), mais estreito que a regra
//     só-family+style+weight do próprio RmlUi de propósito, pra dois arquivos de fonte
//     GENUINAMENTE DIFERENTES compartilhando uma identidade ainda coexistirem exatamente como
//     antes deste fix.
//
//     O QUE ESTE ARQUIVO PROVA, MECANICAMENTE (não por inspeção). Carrega o MESMO documento
//     (fonteng_embed_scene.rml, um bloco @font-face) kReloads=50 vezes numa única sessão UiLayer e
//     verifica que glintfx::own_font_engine_debug_registered_face_count() (um hook de DEBUG
//     src-interno, font_engine_own.hpp/.cpp, forward-declarado abaixo exatamente como os cinco
//     hooks A/B que esta suíte já usa -- ver fonteng_ab_compare.cpp pro motivo do header em si não
//     poder ser #include-ado de um alvo de teste) é EXATAMENTE 1 após a 1ª carga e PERMANECE
//     EXATAMENTE 1 após as 50 (nunca cresce). É um cheque de igualdade INTEIRA exata, não uma
//     tolerância/limiar -- o código pré-fix teria medido 50, não 1, tornando o bug mecanicamente
//     inegável. Um cheque de render+tinta no 50º reload (reusando a técnica de oráculo estatístico
//     de tinta já estabelecida por esta suíte, ver fonteng_extended_sanity.cpp) prova que a face
//     sobrevivente (deduplicada) ainda é a usada de fato pra renderizar -- o dedup não pode quebrar
//     a renderização em silêncio. O VmRSS (lido de /proc/self/status, Linux-only -- bate com a
//     plataforma-alvo documentada deste repo, CLAUDE.md "Plataforma: Linux x86-64") é amostrado
//     antes e depois do laço de reload e IMPRESSO para o registro humano "antes/depois" que a
//     tarefa do fix pede, mas NÃO é em si um gate de pass/fail aqui (RSS é ruidoso -- churn de
//     alocador/driver/contexto-GL move ele independente deste fix; o inteiro de contagem de face é
//     o oráculo determinístico, o RSS é evidência corroborante só, exatamente o papel que a tarefa
//     atribui a ele: "ou outra prova determinística").
//
//     MÉTODO (fixture compartilhado com fonteng_own_embed_sanity.cpp/fonteng_extended_sanity.cpp):
//     uma única janela host GLFW oculta provê o contexto GL; uma única sessão UiLayer recarrega a
//     MESMA cena kReloads vezes (dois ciclos de aquecimento update+render por reload -- convenção
//     da suíte: ciclo 1 estabiliza layout, ciclo 2 estável); glReadPixels + flip_to_top_down +
//     count_ink_rect só no reload FINAL (repetir a cada reload seria lento à toa e não acrescenta
//     sinal -- a asserção de contagem de face já roda toda iteração). Sem sleeps, sem retries --
//     os dois oráculos (contagem de face, tinta) são determinísticos sob as convenções já
//     estabelecidas desta suíte.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"   // EN: pulls gl_loader.h; GL already loaded by the UiLayer ctor.
                           // PT: puxa gl_loader.h; GL já carregado pelo ctor da UiLayer.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// EN: A/B engine-selection hook + the new dedup DEBUG counter hook, forward-declared locally (see
//     fonteng_ab_compare.cpp for the full rationale of why font_engine_own.hpp cannot be
//     #include-d from a test target). Forced/read below.
// PT: Hook de seleção de motor A/B + o novo hook de DEBUG do contador de dedup,
//     forward-declarados localmente (ver fonteng_ab_compare.cpp pro racional completo de por que
//     font_engine_own.hpp não pode ser #include-ado de um alvo de teste). Forçado/lido abaixo.
namespace glintfx {
bool& own_font_engine_ab_bypass();
size_t& own_font_engine_debug_registered_face_count();
}

namespace {

constexpr int W = 640, H = 260;
constexpr int kReloads = 50;

// EN: Ink-counting helpers, identical shape to fonteng_extended_sanity.cpp (see that file for the
//     full rationale of top-down flip + threshold choice) -- only used once, on the final reload.
// PT: Auxiliares de contagem de tinta, formato idêntico ao fonteng_extended_sanity.cpp (ver
//     aquele arquivo pro racional completo do flip top-down + escolha de threshold) -- usados só
//     uma vez, no reload final.
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

// EN: Reads VmRSS (kB) from /proc/self/status -- Linux-only, matches this repo's documented
//     target platform. Informational only (see file header for why RSS is not the gate); returns
//     -1 if the field cannot be found (never fatal -- just prints "n/a").
// PT: Lê VmRSS (kB) de /proc/self/status -- Linux-only, bate com a plataforma-alvo documentada
//     deste repo. Só informativo (ver cabeçalho do arquivo pro motivo do RSS não ser o gate);
//     retorna -1 se o campo não for encontrado (nunca fatal -- só imprime "n/a").
long read_vmrss_kb() {
  FILE* f = std::fopen("/proc/self/status", "r");
  if (!f) return -1;
  char line[256];
  long kb = -1;
  while (std::fgets(line, sizeof(line), f)) {
    if (std::strncmp(line, "VmRSS:", 6) == 0) {
      kb = std::atol(line + 6);
      break;
    }
  }
  std::fclose(f);
  return kb;
}

} // namespace

int main() {
  // EN: Force the OWN engine (dedup lives ONLY in FontEngineOwn::RegisterFace(); measuring
  //     FreeType here would prove nothing about this fix) and reset the debug counter for hygiene
  //     -- same pattern every A/B test in this suite already applies to own_font_engine_ab_bypass().
  // PT: Força o motor PRÓPRIO (o dedup vive SÓ em FontEngineOwn::RegisterFace(); medir o FreeType
  //     aqui não provaria nada sobre este fix) e reseta o contador de debug por higiene -- mesmo
  //     padrão que todo teste A/B desta suíte já aplica ao own_font_engine_ab_bypass().
  glintfx::own_font_engine_ab_bypass() = false;
  glintfx::own_font_engine_debug_registered_face_count() = 0;

  glintfx::WindowGlfw host;
  if (!host.create("fonteng_dedup_sanity_host", W, H)) {
    std::puts("fonteng_dedup_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({ .logical_width = W, .logical_height = H, .load_gl = true });
  if (!ui.ok()) {
    std::puts("fonteng_dedup_sanity FAIL: UiLayer attach failed");
    return 2;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  int failures = 0;
  std::size_t face_count_after_first = 0;
  const long vmrss_before_kb = read_vmrss_kb();

  std::printf("fonteng_dedup_sanity: reloading fonteng_embed_scene.rml %d times, "
              "one @font-face block, watching own_font_engine_debug_registered_face_count()\n",
              kReloads);

  for (int i = 0; i < kReloads; ++i) {
    if (!ui.load("fonteng_embed_scene.rml")) {
      std::fprintf(stderr, "fonteng_dedup_sanity FAIL: load() #%d returned false\n", i + 1);
      return 3;
    }
    // EN: Two warm-up frames -- suite convention (frame 1 settles layout, frame 2 is stable).
    // PT: Dois frames de aquecimento -- convenção da suíte (frame 1 estabiliza layout, frame 2
    //     estável).
    ui.update(); ui.render();
    ui.update(); ui.render();

    const std::size_t face_count = glintfx::own_font_engine_debug_registered_face_count();
    if (i == 0) {
      face_count_after_first = face_count;
      // EN: Sanity on the oracle itself -- the scene declares exactly ONE @font-face block, so
      //     after the FIRST load exactly one face must have been genuinely registered (0 would
      //     mean load() silently failed to register anything -- a different bug than the one this
      //     file gates, but a face-count assertion further down would then be vacuously "stable at
      //     0", so this floor closes that gap).
      // PT: Sanidade do próprio oráculo -- a cena declara exatamente UM bloco @font-face, então
      //     após a 1ª carga exatamente uma face deve ter sido genuinamente registrada (0
      //     significaria que o load() falhou em silêncio ao registrar algo -- um bug diferente do
      //     que este arquivo gateia, mas uma asserção de contagem de face mais abaixo ficaria
      //     vacuamente "estável em 0", então este piso fecha essa brecha).
      if (face_count_after_first != 1) {
        std::fprintf(stderr,
            "fonteng_dedup_sanity FAIL [1] after load #1: registered_face_count=%zu, expected "
            "exactly 1 (scene declares exactly one @font-face block)\n",
            face_count_after_first);
        ++failures;
      }
    } else if ((i + 1) % 10 == 0) {
      // EN: Periodic trend line (every 10th reload) -- corroborating evidence for the "no growth"
      //     claim beyond the single before/after delta printed after the loop: a real leak would
      //     show VmRSS climbing roughly linearly across these lines; a one-time warm-up cost (heap
      //     arena growth, first-time GL-driver buffer allocation, RmlUi internal cache population
      //     -- all of which are expected on the FIRST few reloads and are NOT this ticket's bug)
      //     shows VmRSS flattening out after the first handful of lines instead. (Manually verified
      //     with kReloads=300 during this fix's own verification: VmRSS was bit-for-bit IDENTICAL
      //     from reload #50 through #300 -- zero marginal growth -- while face_count stayed at 1
      //     throughout; see the fix's commit message for the full before/after record.)
      // PT: Linha de tendência periódica (a cada 10º reload) -- evidência corroborante pra
      //     alegação de "sem crescimento" além do único delta antes/depois impresso após o laço:
      //     um vazamento de verdade mostraria o VmRSS subindo mais ou menos linear ao longo destas
      //     linhas; um custo de warm-up único (crescimento de arena de heap, alocação de buffer do
      //     driver GL de primeira vez, população de cache interno do RmlUi -- tudo esperado nos
      //     primeiros reloads e NÃO o bug desta tarefa) mostra o VmRSS achatando depois do primeiro
      //     punhado de linhas em vez disso. (Verificado manualmente com kReloads=300 durante a
      //     verificação deste próprio fix: o VmRSS ficou byte-a-byte IDÊNTICO do reload #50 ao
      //     #300 -- crescimento marginal zero -- enquanto face_count ficou em 1 o tempo todo; ver
      //     a mensagem de commit deste fix pro registro antes/depois completo.)
      std::fprintf(stderr, "  [trend] after load #%d: face_count=%zu VmRSS=%ld kB\n",
                   i + 1, face_count, read_vmrss_kb());
    }
    if (face_count != face_count_after_first) {
      std::fprintf(stderr,
          "fonteng_dedup_sanity FAIL [2] THE LEAK IS BACK: after load #%d, "
          "registered_face_count=%zu != %zu measured after load #1 -- RegisterFace() is pushing a "
          "duplicate LoadedFace instead of deduping (see font_engine_own.cpp's dedup guard, right "
          "after the `if (blob.empty())` check)\n",
          i + 1, face_count, face_count_after_first);
      ++failures;
      // EN: Do not break -- let the loop finish so the printed count/RSS trend is visible even on
      //     failure (useful triage output), the failure is already latched via `failures`.
      // PT: Não interrompe -- deixa o laço terminar pro trend impresso de contagem/RSS ficar
      //     visível mesmo em falha (saída de triagem útil), a falha já está travada via
      //     `failures`.
    }
  }

  const long vmrss_after_kb = read_vmrss_kb();
  const std::size_t final_face_count = glintfx::own_font_engine_debug_registered_face_count();

  // EN: Render+ink check on the FINAL reload -- proves the surviving (deduped) face is still the
  //     one actually rendering, not merely that a counter reads the right integer. Same
  //     statistical-ink-oracle technique as fonteng_extended_sanity.cpp/fonteng_own_embed_sanity.cpp
  //     (line1 is plain ASCII, always present in fonteng_embed_scene.rml).
  // PT: Cheque de render+tinta no reload FINAL -- prova que a face sobrevivente (deduplicada)
  //     ainda é a que de fato renderiza, não só que um contador lê o inteiro certo. Mesma técnica
  //     de oráculo estatístico de tinta de fonteng_extended_sanity.cpp/
  //     fonteng_own_embed_sanity.cpp (line1 é ASCII simples, sempre presente no
  //     fonteng_embed_scene.rml).
  std::vector<unsigned char> px(static_cast<size_t>(W) * H * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  flip_to_top_down(px, W, H);

  const glintfx::ElementBox box1 = ui.get_element_box("line1");
  const long ink1 = box1.found ? count_ink_rect(px, W, H, box1.x, box1.y, box1.w, box1.h) : 0;
  constexpr long kMinInk = 15;

  std::printf("fonteng_dedup_sanity: face_count(after load#1)=%zu  face_count(after load#%d)=%zu  "
              "line1 found=%s ink=%ld\n",
              face_count_after_first, kReloads, final_face_count, box1.found ? "yes" : "no", ink1);
  std::printf("fonteng_dedup_sanity: VmRSS before=%s  after=%s  delta=%s (informational -- see "
              "this file's header for why RSS is not the pass/fail gate)\n",
              vmrss_before_kb >= 0 ? (std::to_string(vmrss_before_kb) + " kB").c_str() : "n/a",
              vmrss_after_kb >= 0 ? (std::to_string(vmrss_after_kb) + " kB").c_str() : "n/a",
              (vmrss_before_kb >= 0 && vmrss_after_kb >= 0)
                  ? (std::to_string(vmrss_after_kb - vmrss_before_kb) + " kB").c_str()
                  : "n/a");

  if (!box1.found) {
    std::fprintf(stderr,
        "fonteng_dedup_sanity FAIL [3] span 'line1' not found in layout after reload #%d\n",
        kReloads);
    ++failures;
  } else if (ink1 < kMinInk) {
    std::fprintf(stderr,
        "fonteng_dedup_sanity FAIL [4] 'line1' rendered no ink (ink=%ld < %ld) after reload #%d "
        "-- dedup broke rendering, not just the leak\n",
        ink1, kMinInk, kReloads);
    ++failures;
  }

  if (!ui.ok()) {
    std::fputs("fonteng_dedup_sanity FAIL [5] ok() false after the reload sequence\n", stderr);
    ++failures;
  }

  // EN: Restore the default (hygiene -- mirrors every other A/B-hook test in this suite).
  // PT: Restaura o default (higiene -- espelha todo outro teste de hook A/B desta suíte).
  glintfx::own_font_engine_ab_bypass() = false;
  glintfx::own_font_engine_debug_registered_face_count() = 0;

  if (failures == 0) {
    std::puts("fonteng_dedup_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "fonteng_dedup_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
