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
#include "byte_ceiling.hpp"
#include "gl_loader.h"

#include <cstddef>
#include <exception>
#include <vector>

namespace glintfx {

namespace {

// EN: TETO-DEDUP (W26) -- was a locally re-typed 256 MiB literal (see this file's own top
//     comment, CAPTURE-NOTHROW, for the full derivation of why 256 MiB is the right ceiling
//     HERE). Now initialised from the shared policy constant (byte_ceiling.hpp) instead of
//     re-typing the literal -- this keeps the domain-named local constant (readability at THIS
//     call site) and keeps the checkpoint itself independent of the other three (this guard fires
//     BEFORE either allocation below, on its own, regardless of what any other call site does; see
//     byte_ceiling.hpp's own "what dedup does and does not change" paragraph), but ties the VALUE
//     to the one place it is now declared, so a future policy change is one edit, not a four-way
//     grep. The `static_assert` proves the two never silently drift apart.
// PT: TETO-DEDUP (W26) -- era um literal de 256 MiB re-tipado localmente (ver o próprio
//     comentário de topo deste arquivo, CAPTURE-NOTHROW, pra derivação completa de por que
//     256 MiB é o teto certo AQUI). Agora inicializado a partir da constante de política
//     compartilhada (byte_ceiling.hpp) em vez de re-tipar o literal -- isto mantém a constante
//     local com nome de domínio (legibilidade neste ponto de chamada) e mantém o checkpoint em si
//     independente dos outros três (esta guarda dispara ANTES de qualquer uma das duas alocações
//     abaixo, por conta própria, independente do que qualquer outro ponto de chamada faça; ver o
//     próprio parágrafo "o que o dedup muda e o que não muda" de byte_ceiling.hpp), mas amarra o
//     VALOR ao único lugar onde ele agora é declarado, então uma mudança futura de política é uma
//     edição, não um grep em quatro lugares. O `static_assert` prova que os dois nunca divergem em
//     silêncio.
constexpr std::size_t kMaxCaptureBytes = glintfx::kMaxUntrustedFileBytes; // 256 MiB (byte_ceiling.hpp).
static_assert(kMaxCaptureBytes == glintfx::kMaxUntrustedFileBytes,
              "kMaxCaptureBytes must track glintfx::kMaxUntrustedFileBytes (TETO-DEDUP)");

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

  // EN: LOADER-NOT-READY GUARD, now LAZY-INIT (CAPTURE-FREE-LOADER, W27, 2026-08-04 -- see
  //     this file's own top comment for the full design rationale): glx_glReadPixels is a
  //     zero-initialised static function pointer until glx_gl_load() populates it at least
  //     once in this process; calling through it while still null is undefined behaviour (a
  //     SEGFAULT in practice). Before this fix the guard only CHECKED that precondition and
  //     gave up -- exactly the "API that promises to unlock does not reach who it should
  //     unlock" pattern this codebase already names (GLPROC-EMBED, FRAMEGRAB-EMBED): a caller
  //     with NO glintfx instance, the ENTIRE reason this free function exists, could never
  //     populate the table through this function alone, no matter how many times it called
  //     it. The guard now ATTEMPTS the population itself, once, before giving up.
  //     WHY THIS IS SAFE, NOT A NEW HIDDEN PRECONDITION: this function's own contract
  //     (frame_capture.hpp's "PRE-REQUISITE" section) already REQUIRES a GL context current
  //     on the calling thread -- the exact same thing glx_gl_load() itself needs ("resolves
  //     against the host's current GL context", src/gl_loader.h). Calling it here only
  //     exercises a capability the existing precondition already made possible.
  //     WHY THE RE-CHECK IS BY POINTER, NOT BY glx_gl_load()'s OWN RETURN CODE: a non-zero
  //     return means "one or more CORE symbols failed to resolve", which could in principle
  //     be a symbol this function never touches while glReadPixels itself still resolved --
  //     gating on the exact dependency this call is about to use is the narrower, correct
  //     check.
  //     WHY THE UNGATED CALL IS CHEAP: glx_gl_load() is documented idempotent ("safe to call
  //     more than once per process... cheap, no allocation kept around", src/gl_loader.h) --
  //     the `if` above still means this only runs when the table is still empty, so the
  //     common case (an App/UiLayer/Draw2d/host resolver already ran) pays nothing extra.
  //     RESIDUAL LIMITATION, DECLARED NOT SOLVED (CTO decision, 2026-08-04): this call goes
  //     through glx_gl_load()'s OWN glX/EGL/dlsym chain only -- the SAME three paths
  //     GLLOADER-HOST's own diagnosis already found do not cover every host (SDL/ANGLE/a
  //     private EGL wrapper, whichever loader that host's own GL context came from). This
  //     free function takes no resolver parameter, so a host whose context needs a resolver
  //     glx_gl_load() itself cannot find still gets a clean CapturedFramebuffer{} here -- the
  //     identical documented failure this function already had before this fix, not a
  //     regression. That host must populate the table itself first, via a UiLayer's own
  //     Config::gl_proc_resolver (glx_gl_load_with(), GLLOADER-HOST) -- a free "init with
  //     resolver" entry point is deliberately OUT of this fix's scope, and becomes real scope
  //     only if a consumer actually needs it, not invented here as a speculative gesture.
  // PT: GUARDA DE LOADER-NÃO-PRONTO, agora LAZY-INIT (CAPTURE-FREE-LOADER, W27, 2026-08-04 --
  //     ver o próprio comentário de topo deste arquivo pro racional completo de desenho):
  //     glx_glReadPixels é um ponteiro de função estático zero-inicializado até o
  //     glx_gl_load() populá-lo ao menos uma vez neste processo; chamar através dele ainda
  //     nulo é comportamento indefinido (um SEGFAULT na prática). Antes deste conserto a
  //     guarda só CHECAVA essa precondição e desistia -- exatamente o padrão "a API que
  //     promete destravar não alcança quem ela deveria destravar" que este código-base já
  //     nomeia (GLPROC-EMBED, FRAMEGRAB-EMBED): um chamador SEM instância glintfx nenhuma, o
  //     motivo INTEIRO pelo qual esta função livre existe, nunca conseguia popular a tabela
  //     por esta função sozinha, não importa quantas vezes a chamasse. A guarda agora TENTA
  //     a própria população, uma vez, antes de desistir.
  //     POR QUE ISTO É SEGURO, NÃO UMA PRECONDIÇÃO NOVA ESCONDIDA: o próprio contrato desta
  //     função (a seção "PRÉ-REQUISITO" de frame_capture.hpp) já EXIGE um contexto GL
  //     corrente na thread chamadora -- exatamente o mesmo que o próprio glx_gl_load()
  //     precisa ("resolve contra o contexto GL corrente do host", src/gl_loader.h). Chamá-lo
  //     aqui só exercita uma capacidade que a precondição já existente já tornava possível.
  //     POR QUE A RECHECAGEM É POR PONTEIRO, NÃO PELO PRÓPRIO CÓDIGO DE RETORNO DO
  //     glx_gl_load(): um retorno não-zero significa "um ou mais símbolos CORE falharam ao
  //     resolver", o que em princípio poderia ser um símbolo que esta função nunca toca
  //     enquanto o próprio glReadPixels ainda resolveu -- travar na dependência exata que
  //     esta chamada está prestes a usar é a checagem mais estreita e correta.
  //     POR QUE A CHAMADA SEM GUARDA EXTRA É BARATA: o glx_gl_load() é documentado
  //     idempotente ("seguro chamar mais de uma vez por processo... barato, sem alocação
  //     retida", src/gl_loader.h) -- o `if` acima ainda garante que isto só roda quando a
  //     tabela ainda está vazia, então o caso comum (um App/UiLayer/Draw2d/resolvedor do host
  //     já rodou) não paga nada extra.
  //     LIMITAÇÃO RESIDUAL, DECLARADA NÃO RESOLVIDA (decisão do CTO, 2026-08-04): esta
  //     chamada passa só pela PRÓPRIA cadeia glX/EGL/dlsym do glx_gl_load() -- os MESMOS três
  //     caminhos que o próprio diagnóstico do GLLOADER-HOST já achou que não cobrem todo host
  //     (SDL/ANGLE/um wrapper EGL próprio, seja qual for o loader de onde o contexto GL
  //     daquele host veio). Esta função livre não aceita parâmetro de resolvedor, então um
  //     host cujo contexto precisa de um resolvedor que o próprio glx_gl_load() não acha
  //     ainda recebe um CapturedFramebuffer{} limpo aqui -- a MESMA falha já documentada que
  //     esta função já tinha antes deste conserto, não uma regressão. Aquele host precisa
  //     popular a tabela ele mesmo primeiro, via o próprio Config::gl_proc_resolver de um
  //     UiLayer (glx_gl_load_with(), GLLOADER-HOST) -- um ponto de entrada livre de "init com
  //     resolvedor" fica deliberadamente FORA do escopo deste conserto, e vira escopo real só
  //     se um consumidor de fato precisar, não inventado aqui como gesto especulativo.
  if (glx_glReadPixels == nullptr) {
    glx_gl_load();
    if (glx_glReadPixels == nullptr) return CapturedFramebuffer{};
  }

  // EN: CONTEXT-LIVENESS CHECK, added by the SAME fix above (CAPTURE-FREE-LOADER, W27,
  //     2026-08-04) after the lazy glx_gl_load() call REGRESSED capture_framebuffer_loader_
  //     guard.cpp: symbol resolution succeeding is NOT proof a GL context is current on this
  //     thread. Measured, not assumed: on Linux, glXGetProcAddress() resolves function
  //     pointers as a pure client-side dispatch-table lookup that needs NEITHER a display
  //     connection NOR a bound context (part of the GLX ABI, not this library's own choice) --
  //     so glx_gl_load() can, and on this codebase's own CI/dev Mesa build DOES, successfully
  //     populate glx_glReadPixels even in a process that never created any GL context at all.
  //     Calling through such a pointer with nothing bound does not crash on Mesa either: its
  //     dispatch installs a silent no-op table when no context is current, so every GL call
  //     below would quietly do nothing -- `rgb`/`out.pixels` would stay at their
  //     value-initialised zero, and this function would return `ok == true` with a plausible-
  //     looking but MEANINGLESS all-black capture. That is a WORSE failure mode than the
  //     clean `ok == false` this function's own FAIL-HIGH contract already promises elsewhere
  //     (silent wrong data, not a documented refusal) -- the same "never a crash" family this
  //     codebase already holds itself to draws the line at silent garbage too, not just
  //     crashes. `glGetString(GL_VERSION)` is the conventional, widely-used GL-only probe for
  //     "is a context genuinely current" (the same idiom GLEW's own `glewInit()` and similar
  //     loaders rely on): with no context bound it resolves through the SAME no-op dispatch
  //     table and returns `nullptr`; with a real context current it always returns a non-null
  //     version string. Placed AFTER the loader guard above (needs `glx_glGetString` itself
  //     resolved first) and BEFORE the `try` block's own GL calls, so it protects the SAME
  //     whole body the loader guard already covers -- including the pre-existing case where
  //     some OTHER caller populated the table earlier from a context that has since gone away
  //     on this thread, a gap this function's own PRE-REQUISITE contract already implied but
  //     never actually checked before this fix.
  // PT: CHECAGEM DE CONTEXTO-VIVO, somada pelo MESMO conserto acima (CAPTURE-FREE-LOADER,
  //     W27, 2026-08-04) depois que a chamada lazy ao glx_gl_load() REGREDIU o
  //     capture_framebuffer_loader_guard.cpp: resolução de símbolo com sucesso NÃO é prova de
  //     que um contexto GL está corrente nesta thread. Medido, não presumido: no Linux, o
  //     glXGetProcAddress() resolve ponteiros de função como uma busca pura de tabela de
  //     dispatch do lado cliente que não precisa NEM de conexão de display NEM de contexto
  //     vinculado (parte da ABI do GLX, não escolha desta biblioteca) -- então o
  //     glx_gl_load() pode, e no próprio build Mesa de CI/dev deste código-base DE FATO,
  //     popular com sucesso o glx_glReadPixels mesmo num processo que nunca criou contexto GL
  //     nenhum. Chamar através de um ponteiro desses sem nada vinculado também não crasha no
  //     Mesa: o dispatch dele instala uma tabela no-op silenciosa quando nenhum contexto está
  //     corrente, então toda chamada GL abaixo simplesmente não faria nada -- `rgb`/
  //     `out.pixels` ficariam no próprio zero de inicialização por valor, e esta função
  //     devolveria `ok == true` com uma captura toda-preta plausível-mas-SEM-SENTIDO. Isto é
  //     um modo de falha PIOR que o `ok == false` limpo que o próprio contrato FAIL-HIGH desta
  //     função já promete em outro lugar (dado errado silencioso, não uma recusa
  //     documentada) -- a mesma família "nunca um crash" que este código-base já se cobra
  //     também traça a linha em dado-lixo silencioso, não só crashes. O
  //     `glGetString(GL_VERSION)` é a sonda convencional, amplamente usada, só-GL pra "um
  //     contexto está genuinamente corrente" (o mesmo idioma que o próprio `glewInit()` do
  //     GLEW e loaders parecidos usam): sem contexto vinculado ele resolve pela MESMA tabela
  //     de dispatch no-op e devolve `nullptr`; com um contexto real corrente sempre devolve
  //     uma string de versão não-nula. Colocado DEPOIS da guarda do loader acima (precisa do
  //     próprio `glx_glGetString` já resolvido) e ANTES das próprias chamadas GL do bloco
  //     `try`, então protege o MESMO corpo inteiro que a guarda do loader já cobre --
  //     inclusive o caso pré-existente em que algum OUTRO chamador populou a tabela antes, a
  //     partir de um contexto que já deixou de existir nesta thread, uma lacuna que o próprio
  //     contrato PRÉ-REQUISITO desta função já implicava mas nunca de fato checava antes
  //     deste conserto.
  if (glGetString(GL_VERSION) == nullptr) return CapturedFramebuffer{};

  // EN: CAPTURE-PACKSKIP (W28, 2026-08-06, CTO-directed) -- GL_PACK_SKIP_PIXELS/GL_PACK_SKIP_ROWS
  //     ADDED to the save-neutralize-restore block below, alongside the pre-existing 5 slots (now
  //     7). Found by a consumer under Mesa/llvmpipe: a caller with either state left non-zero in
  //     the SAME GL context (e.g. from its own prior glReadPixels/glTexSubImage use) made
  //     glReadPixels below start writing into `rgb` (below, exactly `w*h*3` bytes, no slack) at a
  //     row/column OFFSET past 0 -- the driver still writes `h` full rows of `w*3` bytes each, so
  //     that offset pushes the write past `rgb`'s own end, corrupting adjacent heap metadata
  //     (measured: glibc's malloc abort, "free(): invalid next size (normal)", on a LATER free in
  //     the SAME process -- not necessarily this function's own `rgb` destructor at the end of the
  //     `try` block, whichever heap chunk happens to sit next). GL_PACK_SKIP_PIXELS/
  //     GL_PACK_SKIP_ROWS were the only two of the eight pixel-store PACK parameters this function
  //     had never read, neutralized, or restored -- everything else this readback depends on
  //     (GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, the GL_PIXEL_PACK_BUFFER binding, plus the
  //     GL_READ_FRAMEBUFFER binding and GL_READ_BUFFER this read also targets) was already
  //     covered. Same idiom as those five: `glGetIntegerv` before, `glPixelStorei(..., 0)` to
  //     neutralize for the read (0 is each one's own GL default), `glPixelStorei` back to the
  //     caller's original value after -- not a new pattern invented for this fix.
  //     ⚠️ ONLY MEASURED under Mesa/llvmpipe (Xvfb, this codebase's own CI/dev GL implementation):
  //     the consumer's own report is explicit that a real-GPU driver was NOT tested. The failure
  //     mode on a conformant real-GPU driver is expected to be the SAME heap-buffer-overflow WRITE
  //     (the GL 3.3 spec defines GL_PACK_SKIP_PIXELS/GL_PACK_SKIP_ROWS identically for every
  //     conformant implementation -- this is not a Mesa quirk), but whether it happens to CRASH
  //     (glibc/ASan catching the corruption) or SILENTLY CORRUPT (a heap chunk that goes unused
  //     until much later, or never) is allocator- and heap-layout-dependent, not something this
  //     fix or its regression test can observe on any one machine -- the fix itself does not
  //     depend on which outcome a given run happens to hit, only on the write never going
  //     out-of-bounds in the first place.
  // PT: CAPTURE-PACKSKIP (W28, 2026-08-06, dirigido pelo CTO) -- GL_PACK_SKIP_PIXELS/
  //     GL_PACK_SKIP_ROWS SOMADOS ao bloco de salvar-neutralizar-restaurar abaixo, ao lado dos 5
  //     slots pré-existentes (agora 7). Achado por um consumidor sob Mesa/llvmpipe: um chamador
  //     com qualquer um dos dois estados deixado não-zero no MESMO contexto GL (ex.: do próprio
  //     uso anterior de glReadPixels/glTexSubImage) fazia o glReadPixels abaixo começar a escrever
  //     em `rgb` (abaixo, exatamente `w*h*3` bytes, sem folga) num OFFSET de linha/coluna além do
  //     0 -- o driver ainda escreve `h` linhas completas de `w*3` bytes cada, então esse offset
  //     empurra a escrita além do próprio fim de `rgb`, corrompendo metadado de heap adjacente
  //     (medido: abort do malloc do glibc, "free(): invalid next size (normal)", num free
  //     POSTERIOR no MESMO processo -- não necessariamente o próprio destrutor de `rgb` desta
  //     função no fim do bloco `try`, qualquer chunk de heap que por acaso esteja ao lado).
  //     GL_PACK_SKIP_PIXELS/GL_PACK_SKIP_ROWS eram os únicos dois dos oito parâmetros de
  //     pixel-store PACK que esta função nunca lia, neutralizava ou restaurava -- tudo mais de que
  //     este readback depende (GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, o binding de
  //     GL_PIXEL_PACK_BUFFER, mais o binding de GL_READ_FRAMEBUFFER e o GL_READ_BUFFER que esta
  //     leitura também mira) já estava coberto. Mesmo idioma dos cinco: `glGetIntegerv` antes,
  //     `glPixelStorei(..., 0)` pra neutralizar durante a leitura (0 é o próprio default do GL
  //     pros dois), `glPixelStorei` de volta ao valor original do chamador depois -- não é um
  //     padrão novo inventado pra este conserto.
  //     ⚠️ SÓ MEDIDO sob Mesa/llvmpipe (Xvfb, a própria implementação GL de CI/dev deste
  //     código-base): o próprio reporte do consumidor é explícito que um driver de GPU real NÃO
  //     foi testado. O modo de falha num driver de GPU real conforme é esperado ser a MESMA
  //     escrita de heap-buffer-overflow (a spec do GL 3.3 define GL_PACK_SKIP_PIXELS/
  //     GL_PACK_SKIP_ROWS identicamente pra toda implementação conforme -- isto não é uma
  //     peculiaridade do Mesa), mas se isso CRASHA (glibc/ASan pegando a corrupção) ou CORROMPE EM
  //     SILÊNCIO (um chunk de heap que fica sem uso até bem depois, ou nunca) depende do alocador e
  //     do layout de heap, não é algo que este conserto ou o próprio teste de regressão dele
  //     consigam observar em qualquer máquina -- o próprio conserto não depende de qual desfecho
  //     uma execução dada acabe batendo, só de a escrita nunca sair dos limites pra começo de
  //     conversa.
  try {
    GLint prev_read_fbo = 0, prev_read_buffer = 0;
    GLint prev_pack_alignment = 4, prev_pack_row_length = 0, prev_pack_pbo = 0;
    GLint prev_pack_skip_pixels = 0, prev_pack_skip_rows = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo);
    glGetIntegerv(GL_READ_BUFFER, &prev_read_buffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &prev_pack_alignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &prev_pack_row_length);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &prev_pack_pbo);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &prev_pack_skip_pixels);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &prev_pack_skip_rows);

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
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);

    std::vector<unsigned char> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3);
    glReadPixels(gl_x, gl_y, w, h, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());

    glPixelStorei(GL_PACK_ALIGNMENT, prev_pack_alignment);
    glPixelStorei(GL_PACK_ROW_LENGTH, prev_pack_row_length);
    glPixelStorei(GL_PACK_SKIP_PIXELS, prev_pack_skip_pixels);
    glPixelStorei(GL_PACK_SKIP_ROWS, prev_pack_skip_rows);
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
