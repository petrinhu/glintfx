// SPDX-License-Identifier: Apache-2.0
// EN: capture_framebuffer_packskip_regression (CAPTURE-PACKSKIP, W28, 2026-08-06) -- reproduces,
//     inside this repo's own test suite, the exact heap-corruption scenario a consumer reported
//     against glintfx::capture_framebuffer() (glintfx/include/glintfx/frame_capture.hpp): calling
//     it while GL_PACK_SKIP_PIXELS or GL_PACK_SKIP_ROWS is left non-zero in the SAME GL context
//     (the caller's own prior GL use, never touched or restored by glintfx before this fix) makes
//     the internal glReadPixels write PAST THE END of the exactly-sized `rgb` scratch buffer
//     (frame_capture.cpp), corrupting heap metadata -- observed by the consumer as a glibc malloc
//     abort ("free(): invalid next size (normal)") under Mesa/llvmpipe. See frame_capture.cpp's
//     own CAPTURE-PACKSKIP comment (top of the `try` block) for the full derivation; not
//     re-derived here.
//
//     PROCESS ISOLATION (mirrors the consumer's OWN measurement methodology verbatim, "um estado
//     sujo por processo, para não confundir causa"): this file is ONE source, but is registered
//     in tests/CMakeLists.txt as FIVE separate ctest cases, each launching its OWN process (its
//     own `xvfb-run` invocation via run_xvfb.cmake's ARGS parameter) with a DIFFERENT `argv[1]`
//     selecting exactly ONE dirty/control state -- so a corruption caused by one case can never
//     be misattributed to, or mask, another. Two DIRTY cases (the bug this ticket fixes) plus
//     three POSITIVE CONTROLS (states this function already guarded correctly BEFORE this fix --
//     if a future edit ever broke one of those five pre-existing guards, one of these three
//     control cases would go red too, distinguishing "this fix regressed" from "an unrelated
//     guard regressed"):
//       skip_pixels          -- DIRTY: GL_PACK_SKIP_PIXELS=7 (the consumer's own exact value).
//       skip_rows            -- DIRTY: GL_PACK_SKIP_ROWS=9 (the consumer's own exact value).
//       control_alignment    -- CONTROL: GL_PACK_ALIGNMENT=8 (already guarded pre-fix).
//       control_row_length   -- CONTROL: GL_PACK_ROW_LENGTH=1234 (already guarded pre-fix).
//       control_read_fbo     -- CONTROL: a second host FBO bound as GL_READ_FRAMEBUFFER (already
//                                guarded pre-fix).
//     Every case's own PRIMARY assertion is simply that the PROCESS SURVIVES to `main`'s own
//     `return` with `ok == true` and the correct width/height -- a corrupted heap that aborts
//     inside capture_framebuffer()'s own `try` block (at the latest, `rgb`'s destructor at the
//     end of that block, per the consumer's own measurement) never reaches ANY assertion this
//     file writes; the test binary itself exiting with a signal (SIGABRT) IS the failure ctest
//     reports, exactly like the consumer's own repro.
//     ⚠️ MESA/LLVMPIPE ONLY (same caveat frame_capture.cpp's own comment states): whether an
//     out-of-bounds WRITE aborts immediately, aborts later, or corrupts silently depends on heap
//     layout and allocator, not something any one test run can guarantee to observe on every
//     machine/build config -- ASan (this repo's nightly/heavy CI) is the sanitizer with a
//     GUARANTEED catch (heap-buffer-overflow, reported at the exact write, every run); a plain
//     debug build's catch rate is "very likely, not certain" (glibc's own malloc consistency
//     check, the consumer's own measured mechanism). This test's own value therefore compounds
//     across BOTH build configs this repo's CI already runs, not any single one of them.
// PT: capture_framebuffer_packskip_regression (CAPTURE-PACKSKIP, W28, 2026-08-06) -- reproduz,
//     dentro da própria suíte de teste deste repo, o exato cenário de corrupção de heap que um
//     consumidor reportou contra glintfx::capture_framebuffer()
//     (glintfx/include/glintfx/frame_capture.hpp): chamá-la com GL_PACK_SKIP_PIXELS ou
//     GL_PACK_SKIP_ROWS deixado não-zero no MESMO contexto GL (o próprio uso GL anterior do
//     chamador, nunca tocado ou restaurado pela glintfx antes deste conserto) faz o glReadPixels
//     interno escrever ALÉM DO FIM do buffer scratch `rgb` de tamanho exato (frame_capture.cpp),
//     corrompendo metadado de heap -- observado pelo consumidor como um abort do malloc do glibc
//     ("free(): invalid next size (normal)") sob Mesa/llvmpipe. Ver o próprio comentário
//     CAPTURE-PACKSKIP de frame_capture.cpp (topo do bloco `try`) pra derivação completa; não
//     re-derivado aqui.
//
//     ISOLAMENTO DE PROCESSO (espelha a PRÓPRIA metodologia de medição do consumidor ao pé da
//     letra, "um estado sujo por processo, para não confundir causa"): este arquivo é UMA fonte,
//     mas é registrado em tests/CMakeLists.txt como CINCO casos de ctest separados, cada um
//     lançando o PRÓPRIO processo (a própria invocação `xvfb-run` via o parâmetro ARGS de
//     run_xvfb.cmake) com um `argv[1]` DIFERENTE selecionando exatamente UM estado sujo/controle
//     -- então uma corrupção causada por um caso nunca pode ser mal-atribuída a, ou mascarar,
//     outro. Dois casos SUJOS (o bug que esta ticket conserta) mais três CONTROLES POSITIVOS
//     (estados que esta função já guardava corretamente ANTES deste conserto -- se uma edição
//     futura algum dia quebrasse uma dessas cinco guardas pré-existentes, um desses três casos de
//     controle também ficaria vermelho, distinguindo "este conserto regrediu" de "uma guarda sem
//     relação regrediu"):
//       skip_pixels          -- SUJO: GL_PACK_SKIP_PIXELS=7 (o próprio valor exato do consumidor).
//       skip_rows            -- SUJO: GL_PACK_SKIP_ROWS=9 (o próprio valor exato do consumidor).
//       control_alignment    -- CONTROLE: GL_PACK_ALIGNMENT=8 (já guardado antes do conserto).
//       control_row_length   -- CONTROLE: GL_PACK_ROW_LENGTH=1234 (já guardado antes do conserto).
//       control_read_fbo     -- CONTROLE: um segundo FBO do host vinculado como
//                                GL_READ_FRAMEBUFFER (já guardado antes do conserto).
//     A asserção PRIMÁRIA de cada caso é simplesmente que o PROCESSO SOBREVIVE até o próprio
//     `return` do `main` com `ok == true` e largura/altura corretas -- um heap corrompido que
//     aborta dentro do próprio bloco `try` de capture_framebuffer() (o mais tardar, o destrutor de
//     `rgb` no fim daquele bloco, pela própria medição do consumidor) nunca alcança asserção
//     nenhuma que este arquivo escreve; o próprio binário de teste saindo com um sinal (SIGABRT) É
//     a falha que o ctest reporta, exatamente como o próprio repro do consumidor.
//     ⚠️ SÓ MESA/LLVMPIPE (mesma ressalva que o próprio comentário de frame_capture.cpp declara):
//     se uma ESCRITA fora dos limites aborta na hora, aborta depois, ou corrompe em silêncio
//     depende do layout de heap e do alocador, não é algo que uma execução de teste qualquer
//     consiga garantir observar em toda máquina/config de build -- o ASan (o CI nightly/heavy
//     deste repo) é o sanitizador com captura GARANTIDA (heap-buffer-overflow, reportado na exata
//     escrita, toda execução); a taxa de captura de um build debug simples é "bem provável, não
//     certa" (a própria checagem de consistência do malloc do glibc, o próprio mecanismo medido
//     pelo consumidor). O próprio valor deste teste portanto se soma pelas DUAS configs de build
//     que o CI deste repo já roda, não qualquer uma sozinha delas.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp" // EN: includes gl_loader.h, provides gtest_off::make_fbo. PT: inclui gl_loader.h, fornece gtest_off::make_fbo.
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
  if (argc != 2) {
    std::fprintf(stderr,
                 "usage: %s "
                 "{skip_pixels|skip_rows|control_alignment|control_row_length|control_read_fbo}\n",
                 argv[0]);
    return 64;
  }
  const char* mode = argv[1];

  glintfx::WindowGlfw host;
  if (!host.create("capture_framebuffer_packskip_regression_host", 64, 48)) {
    std::puts("FAIL: host window create failed");
    return 1;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT); // EN: warm-up paint. PT: pintura de aquecimento.

  // EN: gtest_off::Fbo must outlive the dirty-state block below for control_read_fbo (its own
  //     `fbo`/`tex` handles are read by capture_framebuffer() through the GL_READ_FRAMEBUFFER
  //     binding); declared unconditionally, harmlessly unused GL objects for the other four
  //     modes (freed at scope exit regardless).
  // PT: gtest_off::Fbo precisa sobreviver ao bloco de estado sujo abaixo pro control_read_fbo (os
  //     próprios handles `fbo`/`tex` dele são lidos por capture_framebuffer() através do binding
  //     de GL_READ_FRAMEBUFFER); declarado incondicionalmente, objetos GL inofensivamente não
  //     usados pros outros quatro modos (liberados na saída de escopo de qualquer forma).
  gtest_off::Fbo read_fbo = gtest_off::make_fbo(16, 16);
  glBindFramebuffer(GL_FRAMEBUFFER, 0); // EN: back to the window. PT: de volta pra janela.

  if (std::strcmp(mode, "skip_pixels") == 0) {
    glPixelStorei(GL_PACK_SKIP_PIXELS, 7); // EN: the consumer's own exact dirty value.
  } else if (std::strcmp(mode, "skip_rows") == 0) {
    glPixelStorei(GL_PACK_SKIP_ROWS, 9); // EN: the consumer's own exact dirty value.
  } else if (std::strcmp(mode, "control_alignment") == 0) {
    glPixelStorei(GL_PACK_ALIGNMENT, 8);
  } else if (std::strcmp(mode, "control_row_length") == 0) {
    glPixelStorei(GL_PACK_ROW_LENGTH, 1234);
  } else if (std::strcmp(mode, "control_read_fbo") == 0) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo.fbo);
  } else {
    std::fprintf(stderr, "FAIL: unknown mode '%s'\n", mode);
    return 65;
  }

  // EN: THE CALL UNDER TEST. If the corresponding GL_PACK_* guard is missing, this line (or the
  //     `rgb` scratch buffer's own destructor immediately after it returns, still inside
  //     capture_framebuffer()) is where a corrupted heap aborts the whole process.
  // PT: A CHAMADA SOB TESTE. Se a guarda GL_PACK_* correspondente estiver faltando, esta linha
  //     (ou o próprio destrutor do buffer scratch `rgb` logo depois dela retornar, ainda dentro
  //     de capture_framebuffer()) é onde um heap corrompido aborta o processo inteiro.
  const glintfx::CapturedFramebuffer frame = glintfx::capture_framebuffer(0, 0, 64, 48);

  bool pass = true;
  if (!frame.ok) {
    std::printf("FAIL[%s]: capture_framebuffer() returned ok == false despite a valid read\n", mode);
    pass = false;
  } else if (frame.width != 64 || frame.height != 48) {
    std::printf("FAIL[%s]: size %dx%d, expected 64x48\n", mode, frame.width, frame.height);
    pass = false;
  } else if (!frame.pixels) {
    std::printf("FAIL[%s]: pixels is null despite ok == true\n", mode);
    pass = false;
  }

  if (!pass) return 3;
  std::printf(
      "capture_framebuffer_packskip_regression[%s]: PASS (process survived, ok=true, "
      "64x48)\n",
      mode);
  return 0;
}
