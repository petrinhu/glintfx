// SPDX-License-Identifier: MPL-2.0
// EN: CAPTURE-FREE (W22, S8, 2026-07-30) -- implementation of glintfx::capture_framebuffer()
//     (glintfx/include/glintfx/frame_capture.hpp, see that header's own top comment for the
//     full design rationale: why a free function, why the struct is a deliberate third
//     duplicate, the coordinate contract, the loader-not-ready guard, the fail-high contract).
//
//     DELIBERATE CODE DUPLICATION of Engine::capture_frame()'s own body (engine.cpp), NOT a
//     call to it -- read this before "why not just call Engine::capture_frame()": that method
//     is gated on `impl_->ok`, which requires an attached Engine (a live RmlUi context via
//     Engine::attach()) to even exist -- exactly the instance dependency this free function's
//     entire purpose is to NOT require. Constructing a throwaway Engine (RmlUi::Initialise(),
//     shader compilation, a font backend selection) purely to borrow its readback would be far
//     heavier than the plain `glReadPixels` call itself, and would still fail for a host that
//     has no document/RmlUi context to attach one against in the first place. The alternative
//     -- extracting Engine::capture_frame()'s own body into a NEW shared private helper both
//     call -- was deliberately NOT done here: engine.hpp/engine.cpp are SHARED files this
//     session's other in-flight work also touches, and docs/embed-integration.md pins several
//     `engine.hpp:N` citations against that file's CURRENT line numbers (see
//     tools/check_doc_line_refs.sh) -- a mid-session refactor of shared, actively-edited,
//     citation-pinned files carries real collision/re-citation risk for a gain (a few dozen
//     duplicated lines) this project's own established precedent already accepts elsewhere
//     (App::CapturedFrame/UiLayer::CapturedFrame are the SAME kind of intentional, documented
//     struct duplication for the identical reason -- see frame_capture.hpp's own "DUPLICATED
//     STRUCT" note). The body below is therefore a byte-for-byte port of
//     Engine::capture_frame()'s own algorithm (same GL_PACK_ALIGNMENT=1 fix, same 5-slot
//     save/restore, same bottom-up -> top-down flip + RGB -> RGBA8 expansion with synthetic
//     alpha=255) with ONE addition Engine::capture_frame() does not need: the loader-not-ready
//     guard (see below), which Engine::capture_frame() gets for free from its own `!impl_->ok`
//     gate (Engine::attach() already implies glx_gl_load() ran).
// PT: CAPTURE-FREE (W22, S8, 2026-07-30) -- implementação de glintfx::capture_framebuffer()
//     (glintfx/include/glintfx/frame_capture.hpp, ver o próprio comentário de topo daquele
//     header pro racional completo de desenho: por que uma função livre, por que a struct é
//     uma terceira duplicata deliberada, o contrato de coordenadas, a guarda de
//     loader-não-pronto, o contrato fail-high).
//
//     DUPLICAÇÃO DE CÓDIGO DELIBERADA do próprio corpo de Engine::capture_frame() (engine.cpp),
//     NÃO uma chamada a ele -- leia isto antes de "por que não só chamar
//     Engine::capture_frame()": aquele método é gateado por `impl_->ok`, que exige um Engine
//     anexado (um contexto RmlUi vivo via Engine::attach()) sequer existir -- exatamente a
//     dependência de instância que o propósito inteiro desta função livre é NÃO exigir.
//     Construir um Engine descartável (RmlUi::Initialise(), compilação de shader, seleção de
//     backend de fonte) só pra pegar emprestado o readback dele seria muito mais pesado que a
//     própria chamada `glReadPixels` pura, e ainda falharia pra um host que não tem
//     documento/contexto RmlUi nenhum pra anexar um contra, pra começo de conversa. A
//     alternativa -- extrair o próprio corpo de Engine::capture_frame() pra um NOVO helper
//     privado compartilhado que os dois chamam -- foi deliberadamente NÃO feita aqui:
//     engine.hpp/engine.cpp são arquivos COMPARTILHADOS que outro trabalho em voo desta sessão
//     também toca, e docs/embed-integration.md fixa várias citações `engine.hpp:N` contra os
//     números de linha CORRENTES daquele arquivo (ver tools/check_doc_line_refs.sh) -- um
//     refactor no meio da sessão de arquivos compartilhados, ativamente editados, com citação
//     fixada carrega risco real de colisão/re-citação por um ganho (algumas dezenas de linhas
//     duplicadas) que o próprio precedente já estabelecido deste projeto já aceita em outro
//     lugar (App::CapturedFrame/UiLayer::CapturedFrame são o MESMO tipo de duplicação de
//     struct intencional, documentada, pelo motivo idêntico -- ver a própria nota "STRUCT
//     DUPLICADA" de frame_capture.hpp). O corpo abaixo é portanto um port byte-a-byte do
//     próprio algoritmo de Engine::capture_frame() (mesmo conserto GL_PACK_ALIGNMENT=1, mesmo
//     salvar/restaurar de 5 slots, mesma inversão bottom-up -> top-down + expansão RGB ->
//     RGBA8 com alpha sintético=255) com UMA adição que Engine::capture_frame() não precisa: a
//     guarda de loader-não-pronto (ver abaixo), que Engine::capture_frame() ganha de graça do
//     próprio gate `!impl_->ok` (Engine::attach() já implica que glx_gl_load() rodou).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/frame_capture.hpp>
#include "gl_loader.h"
#include <vector>

namespace glintfx {

CapturedFramebuffer capture_framebuffer(int gl_x, int gl_y, int w, int h) {
  if (w <= 0 || h <= 0) return CapturedFramebuffer{};

  // EN: LOADER-NOT-READY GUARD (see frame_capture.hpp's own "PRE-REQUISITE" section for the
  //     full derivation): glx_glReadPixels is a zero-initialised static function pointer until
  //     glx_gl_load() populates it at least once in this process. Calling through it while
  //     still null is undefined behaviour (a SEGFAULT in practice). This is the ONE guard
  //     Engine::capture_frame() does not need (its own `!impl_->ok` gate already implies
  //     attach() -> glx_gl_load() ran) but this instance-free entry point does, since it has
  //     no constructor of its own to lean on for that guarantee.
  // PT: GUARDA DE LOADER-NÃO-PRONTO (ver a própria seção "PRÉ-REQUISITO" de frame_capture.hpp
  //     pra derivação completa): glx_glReadPixels é um ponteiro de função estático
  //     zero-inicializado até o glx_gl_load() populá-lo ao menos uma vez neste processo.
  //     Chamar através dele ainda nulo é comportamento indefinido (um SEGFAULT na prática).
  //     Esta é a ÚNICA guarda que Engine::capture_frame() não precisa (o próprio gate
  //     `!impl_->ok` dele já implica que attach() -> glx_gl_load() rodou) mas este ponto de
  //     entrada sem instância precisa, já que não tem construtor próprio pra se apoiar nessa
  //     garantia.
  if (glx_glReadPixels == nullptr) return CapturedFramebuffer{};

  GLint prev_read_fbo = 0, prev_read_buffer = 0;
  GLint prev_pack_alignment = 4, prev_pack_row_length = 0, prev_pack_pbo = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo);
  glGetIntegerv(GL_READ_BUFFER, &prev_read_buffer);
  glGetIntegerv(GL_PACK_ALIGNMENT, &prev_pack_alignment);
  glGetIntegerv(GL_PACK_ROW_LENGTH, &prev_pack_row_length);
  glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &prev_pack_pbo);

  // EN: GL_PACK_ALIGNMENT forced to 1 (tightly-packed rows, no padding) for this read -- the
  //     fix for the "width not a multiple of 4 corrupts rows" class of bug the default
  //     alignment of 4 causes (NOT "odd width" -- gcd(3,4)=1, so an EVEN width that is not a
  //     multiple of 4, e.g. 2/6/198, corrupts the exact same way; see frame_capture.hpp's own
  //     top comment / Engine::capture_frame's own doc-comment, engine.hpp, for the full
  //     derivation).
  // PT: GL_PACK_ALIGNMENT forçado a 1 (linhas compactadas, sem padding) pra esta leitura -- o
  //     conserto pra classe de bug "largura não múltipla de 4 corrompe linhas" que o
  //     alinhamento default de 4 causa (NÃO "largura ímpar" -- mdc(3,4)=1, então uma largura
  //     PAR que não é múltipla de 4, ex. 2/6/198, corrompe exatamente do mesmo jeito; ver o
  //     próprio comentário de topo de frame_capture.hpp / o próprio doc-comment de
  //     Engine::capture_frame, engine.hpp, pra derivação completa).
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ROW_LENGTH, 0);

  std::vector<unsigned char> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3);
  glReadPixels(gl_x, gl_y, w, h, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());

  glPixelStorei(GL_PACK_ALIGNMENT, prev_pack_alignment);
  glPixelStorei(GL_PACK_ROW_LENGTH, prev_pack_row_length);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(prev_pack_pbo));
  glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prev_read_fbo));
  glReadBuffer(static_cast<GLenum>(prev_read_buffer));

  // EN: Row-flip (glReadPixels origin is bottom-left; CapturedFramebuffer row 0 is top -- same
  //     convention App::CapturedFrame/UiLayer::CapturedFrame document) + RGB -> RGBA8 expansion
  //     with synthetic alpha=255 -- byte-for-byte the same as Engine::capture_frame()'s own
  //     equivalent step (engine.cpp); see that method's own doc-comment (engine.hpp) and
  //     App::CapturedFrame's own doc-comment (app.hpp) for the full "why 255" rationale.
  // PT: Inversão de linha (a origem do glReadPixels é bottom-left; a linha 0 de
  //     CapturedFramebuffer é o topo -- mesma convenção que App::CapturedFrame/
  //     UiLayer::CapturedFrame documentam) + expansão RGB -> RGBA8 com alpha sintético=255 --
  //     byte-a-byte igual ao próprio passo equivalente de Engine::capture_frame() (engine.cpp);
  //     ver o próprio doc-comment daquele método (engine.hpp) e o próprio doc-comment de
  //     App::CapturedFrame (app.hpp) pro racional completo do "por que 255".
  CapturedFramebuffer out;
  out.width = w;
  out.height = h;
  out.byte_count = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
  out.pixels = std::make_unique<unsigned char[]>(out.byte_count);
  const size_t row_bytes_src = static_cast<size_t>(w) * 3;
  for (int dst_row = 0; dst_row < h; ++dst_row) {
    const int src_row = h - 1 - dst_row;
    const unsigned char* src = rgb.data() + static_cast<size_t>(src_row) * row_bytes_src;
    unsigned char* dst = out.pixels.get() + static_cast<size_t>(dst_row) * static_cast<size_t>(w) * 4;
    for (int x = 0; x < w; ++x) {
      dst[x * 4 + 0] = src[x * 3 + 0];
      dst[x * 4 + 1] = src[x * 3 + 1];
      dst[x * 4 + 2] = src[x * 3 + 2];
      dst[x * 4 + 3] = 255;
    }
  }
  out.ok = true;
  return out;
}

} // namespace glintfx
