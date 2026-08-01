// SPDX-License-Identifier: Apache-2.0
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
//
// EN: CAPTURE-NOTHROW (W22, 2026-07-30) -- this function allocates TWO buffers below (the
//     intermediate `rgb` scratch, `w*h*3` bytes, and the final `out.pixels`, `w*h*4` bytes,
//     coexisting simultaneously during the flip/expand loop) with NO guard against
//     `std::bad_alloc` before this fix, despite `frame_capture.hpp`'s own doc-comment already
//     promising a clean `CapturedFramebuffer{}` on every OTHER documented failure (`w<=0||h<=0`,
//     the loader-not-ready guard) -- the SAME shape of gap the `never a crash` family
//     (`DEC-NOTHROW`/`ENC-NOTHROW`/`TEX-NOTHROW`/`FONT-NOTHROW`) already closed elsewhere in this
//     codebase, and the SAME defect `Engine::capture_frame()` (`engine.cpp`) has, fixed
//     separately there -- found by the promise-vs-code MATRIX audit (W22), not by grepping for
//     the literal phrase. Confirmed (byte-for-byte comparison, both files) that the two
//     duplicates had NOT diverged beyond the one documented difference (the loader-not-ready
//     guard this function alone needs) before this fix -- both received the IDENTICAL `try`/
//     `catch` treatment for that reason.
//
//     THIS is the WORSE of the two duplicates, and why it alone gets an ADDITIONAL guard
//     `Engine::capture_frame()` does not: that method's `w`/`h` always come from a REAL,
//     already-bounded window/viewport (`App::capture_frame()` reads the actual GLFW framebuffer
//     size; `UiLayer::capture_frame()` reads `impl_->w`/`impl_->h`, themselves capped by
//     `UiLayer::set_viewport()`'s own `kMaxViewportDim` guard, `ui_layer.cpp`) -- there is no
//     glintfx instance here to impose any such bound on THIS function's own caller-supplied
//     `w`/`h`: a direct caller can request `capture_framebuffer(0, 0, 100000, 100000)` and reach
//     the allocation step with NOTHING having rejected it first. `kMaxCaptureBytes` below closes
//     that: checked BEFORE either allocation (same "guard 1/2, pre-allocation" idiom
//     `Draw2d::load_texture()`/`decode_image_file()`/`load_font()` already use), against the
//     BYTE COUNT of the larger (`w*h*4`) of the two buffers -- not `w`/`h` individually -- because
//     it is the allocation size that can exhaust memory, not either dimension alone (a very wide,
//     very short rectangle and a square of the same pixel count carry the identical risk).
//     256 MiB, not an invented round number: the SAME value this codebase's own
//     `kMaxImageDecodeBytes` (`image_decode.hpp`) already uses as the ceiling for a decoded
//     RGBA8 pixel buffer -- reusing an EXISTING, already-reviewed constant's VALUE (declared
//     locally here, not `#include`d, to avoid coupling this module to the unrelated image-decode
//     seam for a single integer) rather than inventing a new one. Headroom check against the
//     largest real capture target this library plausibly needs to support: 8K UHD (7680x4320)
//     RGBA8 is `7680*4320*4` = 132 710 400 bytes ≈ 126.6 MiB -- 256 MiB gives ~2x headroom over
//     that, the SAME ratio `kMaxImageEncodeBytes`'s own 64 MiB cap (`image_encode.cpp`) already
//     uses over a 4K UHD frame's ~33 MiB (both derived the same way: real-world ceiling, doubled
//     for headroom, not picked freehand).
//     NO INTEGER OVERFLOW in the byte-count arithmetic itself, checked (not assumed): `w`/`h` are
//     `int`, individually bounded by `INT_MAX` (~2.1x10^9); `static_cast<size_t>(w) *
//     static_cast<size_t>(h) * 4` casts to `size_t` BEFORE multiplying (never multiplies in `int`
//     first), and on this library's `size_t`=64-bit target (Linux x86-64, CLAUDE.md) the largest
//     possible product, `INT_MAX * INT_MAX * 4` ≈ 1.8446744x10^19, stays JUST under `SIZE_MAX`
//     (2^64-1 ≈ 1.8446744x10^19) -- computed by hand and cross-checked, not eyeballed. This cap
//     rejects any `w`/`h` pair whose product could ever approach that range long before the
//     arithmetic itself would be at risk, so the overflow question is moot in practice, but is
//     recorded here as a checked fact, not a silent assumption.
// PT: CAPTURE-NOTHROW (W22, 2026-07-30) -- esta função aloca DOIS buffers abaixo (o scratch
//     intermediário `rgb`, `w*h*3` bytes, e o `out.pixels` final, `w*h*4` bytes, coexistindo
//     simultaneamente durante o laço de flip/expansão) SEM guarda nenhuma contra `std::bad_alloc`
//     antes deste conserto, apesar do próprio doc-comment de `frame_capture.hpp` já prometer um
//     `CapturedFramebuffer{}` limpo em toda OUTRA falha documentada (`w<=0||h<=0`, a guarda de
//     loader-não-pronto) -- a MESMA forma de lacuna que a família `never a crash`
//     (`DEC-NOTHROW`/`ENC-NOTHROW`/`TEX-NOTHROW`/`FONT-NOTHROW`) já fechou em outro lugar deste
//     código-base, e o MESMO defeito que o próprio `Engine::capture_frame()` (`engine.cpp`) tem,
//     consertado separadamente lá -- achado pela auditoria de MATRIZ promessa-vs-código (W22),
//     não por grep da frase literal. Confirmado (comparação byte-a-byte, os dois arquivos) que as
//     duas duplicatas NÃO tinham divergido além da única diferença documentada (a guarda de
//     loader-não-pronto que só esta função precisa) antes deste conserto -- as duas receberam o
//     MESMO tratamento `try`/`catch` por este motivo.
//
//     ESTA é a PIOR das duas duplicatas, e o porquê de só ela ganhar uma guarda ADICIONAL que o
//     `Engine::capture_frame()` não precisa: o `w`/`h` daquele método sempre vêm de uma
//     janela/viewport REAL, já limitada (`App::capture_frame()` lê o tamanho real do framebuffer
//     GLFW; `UiLayer::capture_frame()` lê `impl_->w`/`impl_->h`, eles mesmos limitados pela
//     própria guarda `kMaxViewportDim` do `UiLayer::set_viewport()`, `ui_layer.cpp`) -- não há
//     instância glintfx nenhuma aqui pra impor limite nenhum ao próprio `w`/`h` fornecido pelo
//     chamador DESTA função: um chamador direto pode pedir
//     `capture_framebuffer(0, 0, 100000, 100000)` e alcançar o passo de alocação sem NADA ter
//     rejeitado antes. O `kMaxCaptureBytes` abaixo fecha isso: checado ANTES de qualquer uma das
//     duas alocações (mesmo idioma "guarda 1/2, pré-alocação" que `Draw2d::load_texture()`/
//     `decode_image_file()`/`load_font()` já usam), contra a CONTAGEM DE BYTES do maior (`w*h*4`)
//     dos dois buffers -- não `w`/`h` isoladamente -- porque é o tamanho de alocação que pode
//     esgotar memória, não uma dimensão sozinha (um retângulo muito largo e muito baixo e um
//     quadrado da mesma contagem de pixel carregam o risco idêntico).
//     256 MiB, não um número redondo inventado: o MESMO valor que o próprio
//     `kMaxImageDecodeBytes` (`image_decode.hpp`) deste código-base já usa como teto pra um
//     buffer de pixel RGBA8 decodificado -- reusando o VALOR de uma constante EXISTENTE, já
//     revisada (declarada localmente aqui, não `#include`da, pra não acoplar este módulo à
//     costura de decode de imagem sem relação por causa de um único inteiro), em vez de inventar
//     uma nova. Checagem de folga contra o maior alvo de captura real que esta biblioteca
//     plausivelmente precisa suportar: 8K UHD (7680x4320) RGBA8 é `7680*4320*4` =
//     132.710.400 bytes ≈ 126,6 MiB -- 256 MiB dá ~2x de folga sobre isso, a MESMA razão que o
//     próprio teto de 64 MiB do `kMaxImageEncodeBytes` (`image_encode.cpp`) já usa sobre os
//     ~33 MiB de um frame 4K UHD (as duas derivadas do mesmo jeito: teto do mundo real, dobrado
//     pra folga, não escolhidas de cabeça).
//     SEM OVERFLOW de inteiro na própria aritmética de contagem de bytes, checado (não
//     presumido): `w`/`h` são `int`, individualmente limitados por `INT_MAX` (~2,1x10^9);
//     `static_cast<size_t>(w) * static_cast<size_t>(h) * 4` converte pra `size_t` ANTES de
//     multiplicar (nunca multiplica em `int` primeiro), e no alvo `size_t`=64-bit desta
//     biblioteca (Linux x86-64, CLAUDE.md) o maior produto possível, `INT_MAX * INT_MAX * 4` ≈
//     1,8446744x10^19, fica JUSTO abaixo do `SIZE_MAX` (2^64-1 ≈ 1,8446744x10^19) -- computado à
//     mão e conferido cruzado, não estimado de olho. Este teto rejeita qualquer par `w`/`h` cujo
//     produto pudesse algum dia se aproximar dessa faixa bem antes da própria aritmética correr
//     risco, então a questão do overflow é discutível na prática, mas fica registrada aqui como
//     fato checado, não uma suposição silenciosa.
//
// EN: `noexcept` ADDED to this function's own declaration (`frame_capture.hpp`) -- same
//     statement-by-statement audit as `Engine::capture_frame()`'s own (`engine.cpp`, identical
//     algorithm): every call inside the `try` below is a plain C GL entry point (never throws by
//     ABI) or one of the two now-guarded allocations; the two guards above the `try`
//     (`w<=0||h<=0`, the byte-count cap, the loader-not-ready check) are int comparisons and a
//     pointer-null check, nothing that can throw. UNLIKE `Engine::capture_frame()`, this function
//     has no OTHER caller wrapping it that itself calls unaudited code -- it IS the public entry
//     point, so there is no separate "outer" method left un-noexcept the way `App::capture_frame()`
//     stays for `Engine::capture_frame()`.
// PT: `noexcept` SOMADO à própria declaração desta função (`frame_capture.hpp`) -- mesma
//     auditoria instrução-por-instrução do próprio `Engine::capture_frame()` (`engine.cpp`,
//     algoritmo idêntico): toda chamada dentro do `try` abaixo é um entry point C puro do GL
//     (nunca lança, pela ABI) ou uma das duas alocações agora guardadas; as duas guardas acima do
//     `try` (`w<=0||h<=0`, o teto de contagem de bytes, a checagem de loader-não-pronto) são
//     comparação de int e checagem de ponteiro nulo, nada que possa lançar. DIFERENTE do
//     `Engine::capture_frame()`, esta função não tem NENHUM outro chamador a envolvê-la que ele
//     mesmo chame código não-auditado -- ELA é o próprio ponto de entrada público, então não há
//     um método "externo" separado que fique de fora do noexcept do jeito que o
//     `App::capture_frame()` fica pro `Engine::capture_frame()`.
#include <glintfx/frame_capture.hpp>
#include "gl_loader.h"

#include <cstddef>
#include <exception>
#include <vector>

namespace glintfx {

namespace {

// EN: See this file's own top comment (CAPTURE-NOTHROW) for the full derivation -- reused VALUE
//     of `kMaxImageDecodeBytes` (`image_decode.hpp`), declared locally (not shared) on purpose.
// PT: Ver o próprio comentário de topo deste arquivo (CAPTURE-NOTHROW) pra derivação completa --
//     VALOR reusado de `kMaxImageDecodeBytes` (`image_decode.hpp`), declarado localmente (não
//     compartilhado) de propósito.
constexpr std::size_t kMaxCaptureBytes = 256u * 1024u * 1024u; // 256 MiB.

} // namespace

CapturedFramebuffer capture_framebuffer(int gl_x, int gl_y, int w, int h) noexcept {
  if (w <= 0 || h <= 0) return CapturedFramebuffer{};

  // EN: CAPTURE-NOTHROW -- byte-count cap, checked BEFORE either allocation below (see this
  //     file's own top comment for the full derivation of both the value and why it targets the
  //     byte count, not w/h individually). `w`/`h` are already known positive here (guard
  //     above), so this cast never wraps.
  // PT: CAPTURE-NOTHROW -- teto de contagem de bytes, checado ANTES de qualquer uma das duas
  //     alocações abaixo (ver o próprio comentário de topo deste arquivo pra derivação completa
  //     tanto do valor quanto do porquê de mirar a contagem de bytes, não w/h isoladamente).
  //     `w`/`h` já são conhecidos positivos aqui (guarda acima), então este cast nunca dá a volta.
  const std::size_t byte_count = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u;
  if (byte_count > kMaxCaptureBytes) return CapturedFramebuffer{};

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

  try {
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
    // EN: Hoisted out of the loop instead of calling `.get()` per row -- besides the usual DRY
    //     motivation, cppcheck 2.13.0 (the version this repo's CI installs on ubuntu-latest,
    //     TST-L1-STATIC) misinfers `.get()` chained straight into pointer arithmetic as `void*`
    //     (CI-LINT-RED, `arithOperationsOnVoidPointer`) even though `out.pixels` is
    //     `unique_ptr<unsigned char[]>` and its `.get()` is unambiguously `unsigned char*` by
    //     the standard. No UB either way (`unsigned char*` arithmetic is always well-defined);
    //     naming the pointer first is the same code either way and resolves the tool's false
    //     positive without a suppression comment. Same fix applied to the byte-for-byte-identical
    //     loop in engine.cpp (auditoria-dominó).
    // PT: Extraído do laço em vez de chamar `.get()` por linha -- além da motivação DRY usual, o
    //     cppcheck 2.13.0 (a versão que o CI deste repo instala no ubuntu-latest, TST-L1-STATIC)
    //     infere errado `.get()` encadeado direto em aritmética de ponteiro como `void*`
    //     (CI-LINT-RED, `arithOperationsOnVoidPointer`), mesmo `out.pixels` sendo
    //     `unique_ptr<unsigned char[]>` cujo `.get()` é inequivocamente `unsigned char*` pela
    //     norma. Sem UB de nenhum jeito (aritmética de `unsigned char*` é sempre bem definida);
    //     nomear o ponteiro primeiro é o mesmo código de qualquer forma e resolve o falso
    //     positivo da ferramenta sem comentário de supressão. Mesma correção aplicada ao laço
    //     byte-a-byte idêntico em engine.cpp (auditoria-dominó).
    unsigned char* const pixels_base = out.pixels.get();
    for (int dst_row = 0; dst_row < h; ++dst_row) {
      const int src_row = h - 1 - dst_row;
      const unsigned char* src = rgb.data() + static_cast<size_t>(src_row) * row_bytes_src;
      unsigned char* dst = pixels_base + static_cast<size_t>(dst_row) * static_cast<size_t>(w) * 4;
      for (int x = 0; x < w; ++x) {
        dst[x * 4 + 0] = src[x * 3 + 0];
        dst[x * 4 + 1] = src[x * 3 + 1];
        dst[x * 4 + 2] = src[x * 3 + 2];
        dst[x * 4 + 3] = 255;
      }
    }
    out.ok = true;
    return out;
  } catch (const std::exception&) {
    // EN: std::bad_alloc (the expected case -- see this file's own top comment) or any other
    //     std::exception -- degrade to a clean, default-constructed CapturedFramebuffer{}
    //     (ok == false), never a partially-filled result. Unreachable in practice once
    //     kMaxCaptureBytes rejects any w/h pair whose allocation could plausibly fail on a real
    //     machine -- kept anyway, belt-and-suspenders, same discipline as every other noexcept
    //     boundary this codebase already has (a lowered cap in the future must not silently
    //     reopen this gap).
    // PT: std::bad_alloc (o caso esperado -- ver o próprio comentário de topo deste arquivo) ou
    //     qualquer outro std::exception -- degrada pra um CapturedFramebuffer{} limpo,
    //     default-construído (ok == false), nunca um resultado parcialmente preenchido.
    //     Inalcançável na prática uma vez que o kMaxCaptureBytes já rejeita qualquer par w/h cuja
    //     alocação pudesse plausivelmente falhar numa máquina real -- mantido assim mesmo,
    //     cinto-e-suspensório, mesma disciplina de toda outra fronteira noexcept que este
    //     código-base já tem (um teto baixado no futuro não pode reabrir esta lacuna em
    //     silêncio).
    return CapturedFramebuffer{};
  } catch (...) {
    // EN/PT: belt-and-suspenders, same "never a crash" discipline as this codebase's own
    // load_texture()/load_font()/decode_*()/encode_*() catch(...) blocks.
    return CapturedFramebuffer{};
  }
}

} // namespace glintfx
