// SPDX-License-Identifier: MPL-2.0
// EN: CAPTURE-FREE (W22, S8, 2026-07-30) -- a THIRD facade for the SAME shared GL framebuffer
//     readback `App::capture_frame()` (`app.hpp`) and `UiLayer::capture_frame()`
//     (`ui_layer.hpp`) already wrap, this one requiring NO glintfx INSTANCE of any kind (no
//     `App`, no `UiLayer`, no `Engine`) -- just a GL context already current on the calling
//     thread.
//
//     WHY THIS EXISTS (consumer-driven, real migration blocked without it): a host freezing
//     its own background behind a pause menu wanted to call glintfx's capture instead of
//     rolling its own `glReadPixels`, and could not: BOTH existing entry points are instance
//     METHODS, and in the host's own code the capture call is the statement IMMEDIATELY
//     BEFORE the `UiLayer` it would have to call it on is even constructed (the `UiLayer` is
//     a private member of a screen class, emplaced inside the very loop that would need the
//     capture first) -- there is no live instance to call `capture_frame()` ON at the point
//     the host needs the pixels. The deeper reason this is a real gap, not just an ordering
//     inconvenience: in embed mode the HOST owns the GL context (created it, made it current,
//     owns FBO 0) -- requiring it to also own an RmlUi-backed UI-layer INSTANCE merely to read
//     its OWN framebuffer's OWN pixels couples two unrelated concerns. Reading pixels back
//     from a framebuffer you already own needs nothing RmlUi/glintfx-internal knows about;
//     `capture_framebuffer()` below is that read, with the RmlUi dependency subtracted out.
//
//     WHY A FREE FUNCTION, NOT YET ANOTHER METHOD (same design shape as `gl_proc_address()`,
//     `gl_proc.hpp`, DOC-GLCOHAB -- read that header's own "WHY A FREE FUNCTION" note, the
//     reasoning is identical here): a plain pixel readback needs only a GL context already
//     current on the calling thread, not any per-instance `App`/`UiLayer`/`Engine` state --
//     tying it to a class would invent an ownership relationship ("this capture belongs to
//     that UI layer") the operation does not actually have. A free function also mirrors this
//     library's existing precedent for capability that is conceptually process/thread-wide
//     rather than per-object (`glintfx::version()`, `glintfx::gl_proc_address()`).
//
//     DUPLICATED STRUCT, DELIBERATELY, A THIRD TIME -- read this before "just reuse
//     `App::CapturedFrame`/`UiLayer::CapturedFrame`" (`CapturedFrame` is ALREADY duplicated
//     once between those two, `ui_layer.hpp`'s own doc-comment on its `CapturedFrame` explains
//     why unifying them was rejected there: it would require editing `app.hpp`, whose own
//     `file:line` citations `docs/embed-integration.md` pins would shift). This header does
//     NOT reuse either sibling shape, for a REASON specific to THIS function, not mere
//     laziness or copy-paste: `App::CapturedFrame` lives inside `app.hpp`, which is compiled
//     out ENTIRELY when `GLINTFX_BACKEND_GLFW=OFF` (`glintfx.hpp`'s own `#if
//     GLINTFX_BACKEND_GLFW` guard around its `#include`) -- reusing it would make
//     `capture_framebuffer()` UNAVAILABLE in exactly the embed-only configuration this
//     function exists FOR. `UiLayer::CapturedFrame` (`ui_layer.hpp`) IS available in both
//     configs, and reusing IT would compile -- but would tie a function whose entire point is
//     "you do not need to know RmlUi exists to read your own framebuffer" to a type nested
//     inside the RmlUi-backed `UiLayer` class, contradicting the design note two paragraphs up
//     in spirit even where it would not in the compiler's eyes. `CapturedFramebuffer` below is
//     therefore a THIRD, INDEPENDENT struct of the identical shape (`ok`/`width`/`height`/
//     `pixels`/`byte_count`) -- not a fourth shape invented from scratch, a fact any of the
//     three siblings' own tests can rely on if a future caller ever needs to convert between
//     them field-by-field.
//
//     OWNERSHIP: `std::unique_ptr<unsigned char[]>`, matching `App::CapturedFrame`/
//     `UiLayer::CapturedFrame`/the private `Engine::CapturedFramePixels` (`engine.hpp`) --
//     NOT `std::vector<unsigned char>` (the shape `glintfx::DecodedImagePixels`/
//     `EncodedImageBytes`, `image.hpp`, use instead). This is a DELIBERATE split by DATA
//     CATEGORY, not an inconsistency: the `image.hpp` pair moves already-CODEC pixel buffers
//     (post-decode, pre-encode -- typically small-to-moderate, exchanged with `stb_image`'s
//     own APIs which are `std::vector`-friendly); this function, like its two capture
//     siblings, hands back a RAW FRAMEBUFFER region that can be several MiB for a single full-
//     window capture -- `unique_ptr<unsigned char[]>` keeps an accidental copy a compile-time-
//     invisible-cost-free impossibility (move-only) rather than a silent multi-MiB `memcpy`, a
//     real cost for data this size, not a convenience.
//
//     COORDINATE CONTRACT (read this before calling -- the ONE thing this free function does
//     NOT do that both instance methods do FOR their caller): `(gl_x, gl_y)` are OpenGL-NATIVE
//     (bottom-left origin) -- the EXACT SAME convention `Engine::render_compose()`'s own
//     `(offset_x, offset_y)` and `Engine::capture_frame()`'s own `(gl_x, gl_y)` document
//     (`engine.hpp`). `App::capture_frame()` always passes `(0, 0)` (owns the whole window,
//     GL-native origin already matches window origin there); `UiLayer::capture_frame()`
//     converts ITS OWN caller-facing window-space origin (top-left, y-down -- see
//     `UiLayer::set_viewport()`'s own doc-comment) to GL-native via `gl_offset_y =
//     target_h - y - h` BEFORE calling the shared readback -- THIS free function does NONE of
//     that translation, because it has no `target_h`/letterbox state of any instance to
//     translate FROM. A caller capturing the region `(x, y, w, h)` of a window/framebuffer
//     `H` pixels tall, in the SAME top-left/y-down convention `UiLayer`'s own public surface
//     uses, must convert it itself, the identical one-line formula `UiLayer::set_viewport()`
//     already applies internally: `gl_y = H - y - h` (`gl_x = x`, unchanged on that axis). A
//     caller that already thinks in GL-native terms (most raw-GL code, including `glScissor`/
//     `glViewport` themselves) passes its own coordinates straight through, no conversion
//     needed -- this is, in fact, the MORE NATURAL contract for a function whose whole
//     purpose is "read exactly this GL framebuffer rectangle", matching `glReadPixels`' own
//     native convention rather than translating away from it.
//
//     PRE-REQUISITE, MEASURED NOT ASSUMED (GLPROC-EMBED's own caveat on `gl_proc_address()`,
//     `gl_proc.hpp`, is the closest precedent, but the outcome here is DIFFERENT, stated
//     loudly so a reader does not assume the same danger applies unchanged): every GL entry
//     point this function calls (`glReadPixels`, `glBindFramebuffer`, `glReadBuffer`,
//     `glPixelStorei`, `glBindBuffer`, `glGetIntegerv`) resolves, inside this library, through
//     the `glx_`-prefixed function-pointer TABLE `glx_gl_load()` populates
//     (`src/gl_loader.h`/`.c`) -- a zero-initialised static until that call runs at least once
//     in this process, as a side effect of ANY of: `App`'s own construction, `UiLayer`'s own
//     construction with the default `Config::load_gl = true`, `Draw2d::init()`, or a host's
//     own equivalent GL-context-creation step if it happens to route through this library's
//     `WindowGlfw` test/demo helper. UNLIKE `gl_proc_address()` (which resolves its OWN
//     single symbol through a DIFFERENT, bootstrap-safe OS resolver --
//     `glXGetProcAddressARB`/`wglGetProcAddress` -- that works even BEFORE `glx_gl_load()` has
//     ever run), this function's own GL calls have NO such bootstrap path: calling any of them
//     through a still-null function pointer is undefined behaviour (a SEGFAULT in practice --
//     the SAME crash class `gl_proc_address()`'s own doc-comment measured for a `UiLayer{
//     .load_gl = false}` host, `Engine::attach()` -> `RenderGl3::init()`, via `gdb`). BECAUSE
//     this function, unlike its two instance-method siblings, has no `App`/`UiLayer`
//     construction step of its own to lean on for this guarantee (a caller reaching this
//     function has, by construction, no glintfx instance at all), it adds the ONE guard its
//     siblings get for free from their own constructor's `ok()` gate: if the specific pointer
//     this function is about to call through (`glx_glReadPixels`) is still null, it returns
//     `CapturedFramebuffer{}` (`ok == false`) WITHOUT calling ANY GL function -- turning what
//     would otherwise be this function's own version of that exact SEGFAULT into a clean,
//     documented failure. This is a genuine, deliberate IMPROVEMENT over the implicit
//     assumption its siblings can afford to make (their own `Engine::ok()` gate already
//     implies `attach()`, which already implies `glx_gl_load()`, ran) -- not a claim that the
//     underlying precondition itself has changed: `glx_gl_load()` still needs to have run,
//     from SOME entry point, before a REAL (non-empty) capture can ever succeed; this function
//     merely fails cleanly instead of crashing when it has not.
//
//     FAIL-HIGH (same shape as `Engine::capture_frame()`'s own contract, `engine.hpp`):
//     `w <= 0 || h <= 0`, OR the loader-not-ready guard above, returns `CapturedFramebuffer{}`
//     (`ok == false`, `width == height == 0`, `pixels == nullptr`, `byte_count == 0`) WITHOUT
//     touching GL at all. `(gl_x, gl_y)` are NOT range-checked against the framebuffer's own
//     extent -- exactly like `Engine::capture_frame()` above it, and for the same reason
//     `glReadPixels` itself is not: a region that extends past the framebuffer edge is a
//     driver-defined read (typically clamped/garbage in the out-of-range pixels, never a
//     crash on a conformant GL 3.3 implementation) that this thin wrapper does not second-
//     guess, matching the exact permissiveness its own instance-method siblings already have.
//
//     DOES NOT RENDER A FRAME (same as `Engine::capture_frame()`): the caller is responsible
//     for FBO 0 already holding the desired content BEFORE any buffer swap (back-buffer
//     content is undefined post-swap on Mesa/llvmpipe -- see `App::snapshot()`'s own
//     doc-comment, `app.hpp`, for the derivation this equally applies to here).
//
//     PACK-ALIGNMENT / GL-STATE HYGIENE: identical fix and identical save-restore discipline
//     as `Engine::capture_frame()` -- see that method's own doc-comment (`engine.hpp`) for the
//     full derivation of the `GL_PACK_ALIGNMENT`-forced-to-1 fix (the "width not a multiple of
//     4" class of bug, NOT "odd width" -- `gcd(3,4)=1`, so an EVEN width such as 2/6/198
//     corrupts the exact same way) and the exact 5 pieces of GL state
//     (`GL_READ_FRAMEBUFFER` binding, `GL_READ_BUFFER`, `GL_PACK_ALIGNMENT`,
//     `GL_PACK_ROW_LENGTH`, `GL_PIXEL_PACK_BUFFER` binding) this function also saves before,
//     and restores after, its own read -- not re-derived here to avoid drifting out of sync
//     with that method's own account of it.
// PT: CAPTURE-FREE (W22, S8, 2026-07-30) -- uma TERCEIRA fachada pro MESMO readback GL de
//     framebuffer compartilhado que `App::capture_frame()` (`app.hpp`) e
//     `UiLayer::capture_frame()` (`ui_layer.hpp`) já encapsulam, esta sem exigir INSTÂNCIA
//     nenhuma da glintfx (nenhum `App`, nenhum `UiLayer`, nenhum `Engine`) -- só um contexto
//     GL já corrente na thread chamadora.
//
//     POR QUE ISTO EXISTE (consumer-driven, migração real bloqueada sem isto): um host que
//     congela o próprio fundo atrás de um menu de pausa queria chamar a captura da glintfx em
//     vez de rolar o próprio `glReadPixels`, e não conseguiu: OS DOIS pontos de entrada
//     existentes são MÉTODOS de instância, e no próprio código do host a chamada de captura é
//     a linha IMEDIATAMENTE ANTES do `UiLayer` no qual precisaria chamá-la sequer ser
//     construído (o `UiLayer` é membro privado de uma classe de tela, emplaçado dentro do
//     próprio loop que precisaria da captura primeiro) -- não existe instância viva pra chamar
//     `capture_frame()` NO ponto em que o host precisa dos pixels. O motivo mais profundo pelo
//     qual isto é uma lacuna real, não só um incômodo de ordem: em modo embed o HOST é dono do
//     contexto GL (criou, tornou corrente, é dono do FBO 0) -- exigir que ele também seja dono
//     de uma INSTÂNCIA de camada de UI baseada em RmlUi só pra ler os PRÓPRIOS pixels do
//     PRÓPRIO framebuffer acopla duas preocupações sem relação. Ler pixels de volta de um
//     framebuffer que você já possui não precisa de nada que a RmlUi/glintfx-interno saiba;
//     `capture_framebuffer()` abaixo é essa leitura, com a dependência de RmlUi subtraída.
//
//     POR QUE UMA FUNÇÃO LIVRE, NÃO MAIS UM MÉTODO (mesma forma de desenho de
//     `gl_proc_address()`, `gl_proc.hpp`, DOC-GLCOHAB -- leia a própria nota "POR QUE UMA
//     FUNÇÃO LIVRE" daquele header, o raciocínio é idêntico aqui): um readback de pixel puro
//     só precisa de um contexto GL já corrente na thread chamadora, não de estado nenhum
//     por-instância de `App`/`UiLayer`/`Engine` -- amarrar isto a uma classe inventaria uma
//     relação de posse ("esta captura pertence àquela camada de UI") que a operação não tem de
//     fato. Uma função livre também espelha o precedente já existente desta biblioteca pra
//     capacidade conceitualmente de escopo processo/thread, não por-objeto
//     (`glintfx::version()`, `glintfx::gl_proc_address()`).
//
//     STRUCT DUPLICADA, DE PROPÓSITO, UMA TERCEIRA VEZ -- leia isto antes de "só reusa
//     `App::CapturedFrame`/`UiLayer::CapturedFrame`" (`CapturedFrame` JÁ é duplicada uma vez
//     entre os dois, o próprio doc-comment do `CapturedFrame` de `ui_layer.hpp` explica por
//     que unificá-los foi rejeitado lá: exigiria editar `app.hpp`, cujas próprias citações
//     `arquivo:linha` que `docs/embed-integration.md` fixa se deslocariam). Este header NÃO
//     reusa nenhuma das duas formas irmãs, por um MOTIVO específico a ESTA função, não
//     preguiça ou copiar-colar: `App::CapturedFrame` vive dentro de `app.hpp`, que é compilado
//     fora POR INTEIRO quando `GLINTFX_BACKEND_GLFW=OFF` (o próprio guard `#if
//     GLINTFX_BACKEND_GLFW` do `#include` dele em `glintfx.hpp`) -- reusá-la tornaria
//     `capture_framebuffer()` INDISPONÍVEL exatamente na configuração embed-only pra qual esta
//     função existe. `UiLayer::CapturedFrame` (`ui_layer.hpp`) ESTÁ disponível nas duas
//     configs, e reusá-la compilaria -- mas amarraria uma função cujo ponto inteiro é "você
//     não precisa saber que a RmlUi existe pra ler o próprio framebuffer" a um tipo aninhado
//     dentro da classe `UiLayer` baseada em RmlUi, contradizendo a nota de desenho dois
//     parágrafos acima em espírito mesmo onde não contradiria aos olhos do compilador.
//     `CapturedFramebuffer` abaixo é portanto uma TERCEIRA struct, INDEPENDENTE, de forma
//     idêntica (`ok`/`width`/`height`/`pixels`/`byte_count`) -- não uma quarta forma inventada
//     do zero, um fato do qual qualquer um dos três testes irmãos pode depender se um
//     chamador futuro algum dia precisar converter entre eles campo a campo.
//
//     POSSE: `std::unique_ptr<unsigned char[]>`, batendo com `App::CapturedFrame`/
//     `UiLayer::CapturedFrame`/o `Engine::CapturedFramePixels` privado (`engine.hpp`) -- NÃO
//     `std::vector<unsigned char>` (a forma que `glintfx::DecodedImagePixels`/
//     `EncodedImageBytes`, `image.hpp`, usam em vez disso). Esta é uma divisão DELIBERADA por
//     CATEGORIA DE DADO, não uma inconsistência: o par de `image.hpp` move buffers de pixel já
//     de CODEC (pós-decode, pré-encode -- tipicamente pequenos a moderados, trocados com as
//     próprias APIs do `stb_image`, amigáveis a `std::vector`); esta função, como as duas
//     irmãs de captura, devolve uma região de FRAMEBUFFER CRU que pode ter vários MiB numa
//     única captura de janela inteira -- `unique_ptr<unsigned char[]>` torna uma cópia
//     acidental uma impossibilidade sem-custo-em-tempo-de-compilação (move-only) em vez de um
//     `memcpy` silencioso de vários MiB, um custo real pra dado deste tamanho, não uma
//     conveniência.
//
//     CONTRATO DE COORDENADAS (leia isto antes de chamar -- a ÚNICA coisa que esta função
//     livre NÃO faz que os dois métodos de instância fazem PELO próprio chamador): `(gl_x,
//     gl_y)` são NATIVOS do OpenGL (origem inferior-esquerda) -- a MESMA convenção que o
//     próprio `(offset_x, offset_y)` de `Engine::render_compose()` e o próprio `(gl_x, gl_y)`
//     de `Engine::capture_frame()` documentam (`engine.hpp`). `App::capture_frame()` sempre
//     passa `(0, 0)` (é dono da janela inteira, origem nativa do GL já bate com a origem da
//     janela ali); `UiLayer::capture_frame()` converte a PRÓPRIA origem em espaço-janela
//     voltada ao chamador (superior-esquerda, y-para-baixo -- ver o próprio doc-comment de
//     `UiLayer::set_viewport()`) pra nativo-GL via `gl_offset_y = target_h - y - h` ANTES de
//     chamar o readback compartilhado -- esta função livre NÃO faz NENHUMA dessa tradução,
//     porque não tem `target_h`/estado de letterbox de instância nenhuma pra traduzir A
//     PARTIR. Um chamador capturando a região `(x, y, w, h)` de uma janela/framebuffer de
//     altura `H` pixels, na MESMA convenção superior-esquerda/y-para-baixo que a própria
//     superfície pública de `UiLayer` usa, precisa convertê-la ele mesmo, a mesma fórmula de
//     uma linha que o próprio `UiLayer::set_viewport()` já aplica internamente: `gl_y = H - y
//     - h` (`gl_x = x`, sem mudança nesse eixo). Um chamador que já pensa em termos
//     nativo-GL (a maior parte do código GL cru, inclusive o próprio `glScissor`/
//     `glViewport`) passa as próprias coordenadas direto, sem conversão nenhuma necessária --
//     este é, de fato, o contrato MAIS NATURAL pra uma função cujo propósito inteiro é "leia
//     exatamente este retângulo de framebuffer GL", batendo com a própria convenção nativa do
//     `glReadPixels` em vez de traduzir pra longe dela.
//
//     PRÉ-REQUISITO, MEDIDO NÃO PRESUMIDO (a ressalva do próprio `gl_proc_address()`,
//     `gl_proc.hpp`, GLPROC-EMBED, é o precedente mais próximo, mas o resultado aqui é
//     DIFERENTE, dito em voz alta pra um leitor não presumir que o mesmo perigo se aplica sem
//     mudança): todo entry point GL que esta função chama (`glReadPixels`,
//     `glBindFramebuffer`, `glReadBuffer`, `glPixelStorei`, `glBindBuffer`, `glGetIntegerv`)
//     resolve, dentro desta biblioteca, através da TABELA de ponteiros de função prefixada
//     `glx_` que o `glx_gl_load()` popula (`src/gl_loader.h`/`.c`) -- um estático
//     zero-inicializado até aquela chamada rodar ao menos uma vez neste processo, como efeito
//     colateral de QUALQUER um de: a própria construção do `App`, a própria construção do
//     `UiLayer` com o `Config::load_gl = true` padrão, o `Draw2d::init()`, ou o próprio passo
//     equivalente de criação de contexto GL de um host se por acaso passar pelo helper de
//     teste/demo `WindowGlfw` desta biblioteca. DIFERENTE do `gl_proc_address()` (que resolve
//     o PRÓPRIO símbolo único através de um resolvedor de SO DIFERENTE, seguro-pra-bootstrap
//     -- `glXGetProcAddressARB`/`wglGetProcAddress` -- que funciona mesmo ANTES do
//     `glx_gl_load()` ter rodado alguma vez), as próprias chamadas GL desta função NÃO têm tal
//     caminho de bootstrap: chamar qualquer uma delas através de um ponteiro de função ainda-
//     nulo é comportamento indefinido (um SEGFAULT na prática -- a MESMA classe de crash que o
//     próprio doc-comment de `gl_proc_address()` mediu pra um host `UiLayer{.load_gl =
//     false}`, `Engine::attach()` -> `RenderGl3::init()`, via `gdb`). PORQUE esta função,
//     diferente das duas irmãs de método de instância, não tem passo de construção próprio de
//     `App`/`UiLayer` pra se apoiar nessa garantia (um chamador alcançando esta função não tem,
//     por construção, instância glintfx nenhuma), ela soma a ÚNICA guarda que as irmãs ganham
//     de graça do próprio gate `ok()` do construtor delas: se o ponteiro específico que esta
//     função está prestes a chamar através (`glx_glReadPixels`) ainda é nulo, ela devolve
//     `CapturedFramebuffer{}` (`ok == false`) SEM chamar NENHUMA função GL -- transformando o
//     que seria a própria versão desta função daquele exato SEGFAULT numa falha limpa,
//     documentada. Esta é uma MELHORIA genuína, deliberada, sobre a presunção implícita que as
//     irmãs podem se dar ao luxo de fazer (o próprio gate `Engine::ok()` delas já implica
//     `attach()`, que já implica `glx_gl_load()`, ter rodado) -- não uma alegação de que a
//     própria precondição subjacente mudou: o `glx_gl_load()` ainda precisa ter rodado, de
//     ALGUM ponto de entrada, antes de uma captura REAL (não-vazia) algum dia ter sucesso;
//     esta função só falha limpo em vez de crashar quando não rodou.
//
//     FAIL-HIGH (mesma forma do próprio contrato de `Engine::capture_frame()`, `engine.hpp`):
//     `w <= 0 || h <= 0`, OU a guarda de loader-não-pronto acima, devolve
//     `CapturedFramebuffer{}` (`ok == false`, `width == height == 0`, `pixels == nullptr`,
//     `byte_count == 0`) SEM tocar GL nenhum. `(gl_x, gl_y)` NÃO são checados contra a própria
//     extensão do framebuffer -- exatamente como o próprio `Engine::capture_frame()` acima
//     dela, e pelo mesmo motivo o próprio `glReadPixels` também não é: uma região que se
//     estende além da borda do framebuffer é uma leitura definida-pelo-driver (tipicamente
//     grampeada/lixo nos pixels fora de faixa, nunca um crash numa implementação GL 3.3
//     conforme) que este wrapper fino não questiona, batendo com a mesma permissividade que
//     as próprias irmãs de método de instância dela já têm.
//
//     NÃO RENDERIZA FRAME NENHUM (igual ao `Engine::capture_frame()`): o chamador é
//     responsável por o FBO 0 já guardar o conteúdo desejado ANTES de qualquer swap de buffer
//     (o conteúdo do back-buffer é indefinido pós-swap no Mesa/llvmpipe -- ver o próprio
//     doc-comment de `App::snapshot()`, `app.hpp`, pra derivação que se aplica igualmente
//     aqui).
//
//     HIGIENE DE PACK-ALIGNMENT/ESTADO GL: mesmo conserto e mesma disciplina de
//     salvar-restaurar do `Engine::capture_frame()` -- ver o próprio doc-comment daquele
//     método (`engine.hpp`) pra derivação completa do conserto de `GL_PACK_ALIGNMENT` forçado
//     a 1 (a classe de bug "largura não múltipla de 4", NÃO "largura ímpar" -- `mdc(3,4)=1`,
//     então uma largura PAR como 2/6/198 corrompe exatamente do mesmo jeito) e as exatas 5
//     peças de estado GL (binding de `GL_READ_FRAMEBUFFER`, `GL_READ_BUFFER`,
//     `GL_PACK_ALIGNMENT`, `GL_PACK_ROW_LENGTH`, binding de `GL_PIXEL_PACK_BUFFER`) que esta
//     função também salva antes, e restaura depois, da própria leitura -- não re-derivado
//     aqui pra não desviar da própria explicação daquele método.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstddef>
#include <memory>

namespace glintfx {

// EN: Result of capture_framebuffer() below. `ok == false` on ANY failure (invalid `w`/`h`, or
//     the loader-not-ready guard) -- width/height/pixels/byte_count stay at their
//     default-constructed values in that case (never partially filled). See this file's own
//     top comment for the full pixel-format (RGBA8, row 0 = TOP, synthetic alpha = 255,
//     identical to App::CapturedFrame/UiLayer::CapturedFrame), ownership, coordinate, and
//     fail-high contract.
// PT: Resultado do capture_framebuffer() abaixo. `ok == false` em QUALQUER falha (`w`/`h`
//     inválidos, ou a guarda de loader-não-pronto) -- width/height/pixels/byte_count ficam nos
//     próprios valores default-construídos nesse caso (nunca parcialmente preenchidos). Ver o
//     próprio comentário de topo deste arquivo pro contrato completo de formato de pixel
//     (RGBA8, linha 0 = TOPO, alpha sintético = 255, idêntico a App::CapturedFrame/
//     UiLayer::CapturedFrame), posse, coordenadas e fail-high.
struct CapturedFramebuffer {
  bool ok = false;
  int width = 0;
  int height = 0;
  std::unique_ptr<unsigned char[]> pixels;
  std::size_t byte_count = 0;
};

// EN: Read back a `w`x`h` RGBA8 region of the CURRENTLY BOUND read framebuffer's colour
//     content (FBO 0 -- the window/host's own back-buffer -- unless a host has bound a
//     different GL_READ_FRAMEBUFFER of its own before calling; this function's internal
//     save/restore, see this file's own top comment, targets FBO 0/GL_BACK, matching its two
//     instance-method siblings), into an owned CPU buffer, row 0 = top. `(gl_x, gl_y)` are
//     OpenGL-native (bottom-left origin) -- see this file's own "COORDINATE CONTRACT" section
//     above before calling with anything other than `(0, 0)`. Requires NO glintfx instance of
//     any kind -- only a GL context already current on the calling thread, with
//     `glx_gl_load()` (this library's internal GL function-pointer loader) already having run
//     at least once in this process (see this file's own "PRE-REQUISITE" section above for
//     the full list of what already triggers that as a side effect, and what this function
//     does when it has not).
// PT: Lê de volta uma região `w`x`h` RGBA8 do conteúdo de cor do framebuffer de leitura
//     CORRENTEMENTE VINCULADO (FBO 0 -- o próprio back-buffer da janela/host -- a menos que um
//     host tenha vinculado um GL_READ_FRAMEBUFFER diferente antes de chamar; o próprio
//     salvar/restaurar interno desta função, ver o comentário de topo deste arquivo, mira o
//     FBO 0/GL_BACK, batendo com as duas irmãs de método de instância dela), num buffer de CPU
//     de posse própria, linha 0 = topo. `(gl_x, gl_y)` são nativos do OpenGL (origem
//     inferior-esquerda) -- ver a própria seção "CONTRATO DE COORDENADAS" deste arquivo acima
//     antes de chamar com qualquer coisa além de `(0, 0)`. Não exige instância glintfx
//     nenhuma -- só um contexto GL já corrente na thread chamadora, com o `glx_gl_load()` (o
//     loader interno de ponteiros de função GL desta biblioteca) já tendo rodado ao menos uma
//     vez neste processo (ver a própria seção "PRÉ-REQUISITO" deste arquivo acima pra lista
//     completa do que já dispara isso como efeito colateral, e o que esta função faz quando
//     não rodou).
CapturedFramebuffer capture_framebuffer(int gl_x, int gl_y, int w, int h);

} // namespace glintfx
