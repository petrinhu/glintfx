// SPDX-License-Identifier: MPL-2.0
// EN: IMG-DECODE -- decode an encoded image (PNG/JPG/TGA, whatever `Draw2d::load_texture()`
//     itself accepts) into CPU-side pixels, WITHOUT creating any GPU resource. This is the
//     sibling `Draw2d::create_texture()` (D2D-TEXPIXELS, `draw2d.hpp`) was always missing one
//     half of: `create_texture()` takes pixels a caller already has and makes a GPU texture;
//     `decode_image_file()`/`decode_image_memory()` below take an encoded FILE/BUFFER and hand
//     back the pixels, with no GL context required and no GL object created. Together the two
//     cover the full "file on disk or in memory -> pixels -> GPU texture" pipeline without a
//     caller ever touching a decode library directly (the motivating case: a host measuring/
//     inspecting/compositing image content on the CPU before deciding whether, or how, to
//     upload it).
//
//     Consumer-driven (GusWorld, 2026-07-29): the host previously called `stbi_load()` directly
//     in 6 files of its own application layer -- a layer that, by ITS OWN layering contract, is
//     not supposed to touch a third-party decode library at all. This header lets that layer
//     stop doing that. ZERO third-party type crosses this header (no `stbi_*`, no GL, no GLFW,
//     no RmlUi) -- same "golden boundary" discipline as every other public header (AGENTS.md);
//     the internal decode engine (`stb_image`) is an implementation detail entirely confined to
//     `src/image.cpp` and the pre-existing private `src/image_decode.hpp` seam it delegates to.
//
//     THIS IS A FREE FUNCTION PAIR, NOT A `Draw2d` METHOD, on purpose: decoding bytes into
//     pixels needs no GL context, no `init()`/`shutdown()` lifecycle, and no per-instance
//     state -- it is exactly as pure and headless-testable as the private
//     `image_decode.hpp::decode_premultiplied_rgba()` it is built on top of (that header's own
//     top-of-file comment states the same "pure seam" discipline this header inherits). Calling
//     it never requires, and never touches, a `Draw2d`/`App`/`UiLayer` instance.
//
//     OWNERSHIP (the load-bearing question for any "hand back a decoded image" API): the
//     returned `DecodedImagePixels::pixels` is an owned `std::vector<unsigned char>` -- RAII,
//     freed automatically when the struct goes out of scope or is destroyed, movable (cheap,
//     no copy) but also copyable (an explicit, visible cost if a caller chooses it). There is
//     NO raw pointer, NO manual free function, and NO stb-owned buffer or deleter type handed
//     back to the caller (the exact same design call `image_decode.hpp`'s own
//     `DecodedImage::rgba` already made, and for the identical reason stated in that header's
//     own comment: a "bytes in, pixels out" pure interface cannot leak `stb_image.h`'s deleter
//     type into every caller without also leaking the third-party type itself across the public
//     boundary this header must not cross).
//
//     ALPHA CONVENTION, stated loudly (the SAME halo-vs-double-darken class of bug
//     `draw2d.hpp`'s own `create_texture()` doc-comment names): pixels here are STRAIGHT
//     (non-premultiplied) alpha, matching `stb_image`'s own raw decode AND, not by accident,
//     exactly the input `Draw2d::create_texture(pixels, w, h, PixelFormat::Rgba8)` expects --
//     feeding this call's own output straight into that one needs no conversion. This is a
//     DELIBERATE DIVERGENCE from the private `image_decode.hpp::decode_premultiplied_rgba()`
//     this header's implementation is a sibling of: that function premultiplies on decode
//     because it feeds `Draw2d::load_texture()`'s OWN internal GL upload, which assumes
//     premultiplied input for its `GL_ONE, GL_ONE_MINUS_SRC_ALPHA` blend mode (`render_gl3.cpp`'s
//     own top-of-class comment). A general "give me the pixels" API returning ALREADY-
//     premultiplied data would silently corrupt any caller that is not about to feed those exact
//     bytes into that exact blend path (measuring content, inspecting a specific channel,
//     compositing with a DIFFERENT blend convention) -- straight alpha is the honest, unsurprising
//     default; a caller that wants premultiplied bytes for glintfx's own texture pipeline still
//     gets that for free, automatically, INSIDE `create_texture()`'s own ingest step.
//
//     FAIL-HIGH (D7/D10's own discipline, same shape as `create_texture()`/`load_texture()`):
//     `ok == false` is the ONLY failure signal, `width`/`height`/`pixels` left at their
//     default-constructed values (never partially filled) -- a null path/buffer, a zero-length
//     buffer, a file that cannot be opened/sized/fully read, an over-the-cap file/buffer size
//     (the SAME 256 MiB `kMaxImageDecodeBytes` ceiling `load_texture()`/`create_texture()`
//     already enforce, `image_decode.hpp`), or an unknown/corrupt encoded format all reject
//     cleanly, never a crash, never a partial read past the end of a hostile/truncated buffer.
//     `decode_image_file()`'s own size-then-read guard order mirrors `Draw2d::load_texture()`'s
//     literal ifstream idiom (`draw2d.cpp`): seek-to-end, check the size BEFORE ever allocating a
//     buffer for it (so a hostile/huge file on disk is rejected without an oversized allocation
//     attempt), then read. Neither function can log (no `Draw2d`/`Impl` instance exists to log
//     through here -- same "cannot log, `ok` is the only signal" discipline
//     `decode_premultiplied_rgba()` already documents): a caller that wants a diagnostic message
//     decides what to log around the call, exactly like every other pure/headless helper in this
//     library.
//
//     BOTH FUNCTIONS ARE `noexcept` (DEC-NOTHROW, W21, 2026-07-30): "never a crash" above is not
//     just a guideline, it is a `noexcept` guarantee at the signature -- an out-of-memory
//     condition during decode (a real, measured failure mode: a caller-supplied buffer large
//     enough to decode successfully but too large to also copy, e.g. a paletted PNG whose
//     compressed-to-decoded size ratio makes the decode's own intermediate buffer SMALLER than
//     the final pixel buffer) degrades to `ok == false`, exactly like every other rejection above,
//     never a `std::bad_alloc` escaping across this API boundary. See `src/image.cpp`'s own top
//     comment for the `try`/`catch` this guarantee is implemented with, and
//     `image_decode_hardening_sanity.cpp` for the forced-allocation-failure test that proves it
//     under a REAL `RLIMIT_AS` squeeze, not a hypothetical one.
// PT: IMG-DECODE -- decodifica uma imagem codificada (PNG/JPG/TGA, o que o próprio
//     `Draw2d::load_texture()` já aceita) em pixels do lado da CPU, SEM criar nenhum recurso de
//     GPU. Este é a metade que sempre faltou ao irmão `Draw2d::create_texture()` (D2D-TEXPIXELS,
//     `draw2d.hpp`): `create_texture()` pega pixels que o chamador já tem e faz uma textura GPU;
//     `decode_image_file()`/`decode_image_memory()` abaixo pegam um ARQUIVO/BUFFER codificado e
//     devolvem os pixels, sem exigir contexto GL nenhum e sem criar objeto GL nenhum. Juntas as
//     duas cobrem o pipeline completo "arquivo em disco ou em memória -> pixels -> textura GPU"
//     sem o chamador nunca tocar uma biblioteca de decode diretamente (o caso motivador: um host
//     medindo/inspecionando/compondo conteúdo de imagem na CPU antes de decidir se, ou como,
//     fazer upload dela).
//
//     Consumer-driven (GusWorld, 2026-07-29): o host antes chamava `stbi_load()` direto em 6
//     arquivos da própria camada de aplicação -- camada que, pelo PRÓPRIO contrato de camadas
//     dele, nem deveria tocar biblioteca de decode de terceiro nenhuma. Este header deixa essa
//     camada parar de fazer isso. ZERO tipo de terceiro cruza este header (nenhum `stbi_*`,
//     nenhum GL, nenhum GLFW, nenhum RmlUi) -- a mesma disciplina de "fronteira dourada" de todo
//     outro header público (AGENTS.md); o motor de decode interno (`stb_image`) é um detalhe de
//     implementação inteiramente confinado a `src/image.cpp` e à costura privada preexistente
//     `src/image_decode.hpp` pra qual ele delega.
//
//     ISTO É UM PAR DE FREE FUNCTIONS, NÃO UM MÉTODO DE `Draw2d`, de propósito: decodificar
//     bytes em pixels não precisa de contexto GL, nem de ciclo de vida `init()`/`shutdown()`,
//     nem de estado por-instância -- é exatamente tão pura e testável headless quanto o próprio
//     `image_decode.hpp::decode_premultiplied_rgba()` privado sobre o qual é construída (o
//     próprio comentário de topo daquele header declara a mesma disciplina de "costura pura" que
//     este header herda). Chamá-la nunca exige, e nunca toca, uma instância
//     `Draw2d`/`App`/`UiLayer`.
//
//     POSSE (a pergunta que carrega peso pra qualquer API de "devolva a imagem decodificada"): o
//     `DecodedImagePixels::pixels` devolvido é um `std::vector<unsigned char>` de posse própria
//     -- RAII, liberado automaticamente quando a struct sai de escopo ou é destruída, movível
//     (barato, sem cópia) mas também copiável (um custo explícito e visível se o chamador
//     escolher). NÃO há ponteiro cru, NÃO há função de free manual, e NÃO há buffer de posse do
//     stb nem tipo de deleter devolvido ao chamador (a MESMA escolha de desenho que o próprio
//     `DecodedImage::rgba` de `image_decode.hpp` já faz, e pelo motivo idêntico declarado no
//     próprio comentário daquele header: uma interface pura "bytes entram, pixels saem" não pode
//     vazar o tipo de deleter do `stb_image.h` pra todo chamador sem também vazar o próprio tipo
//     de terceiro através desta fronteira pública que este header não pode cruzar).
//
//     CONVENÇÃO DE ALPHA, dita em voz alta (a MESMA classe de bug halo-vs-escurecimento-duplo que
//     o próprio doc-comment do `create_texture()` de `draw2d.hpp` nomeia): pixels aqui são alpha
//     STRAIGHT (não-premultiplicado), batendo com o próprio decode cru do `stb_image` E, não por
//     acaso, exatamente com o input que `Draw2d::create_texture(pixels, w, h,
//     PixelFormat::Rgba8)` espera -- alimentar a própria saída desta chamada direto naquela não
//     precisa de conversão nenhuma. Esta é uma DIVERGÊNCIA DELIBERADA do
//     `image_decode.hpp::decode_premultiplied_rgba()` privado do qual a implementação deste
//     header é irmã: aquela função premultiplica no decode porque alimenta o PRÓPRIO upload GL
//     interno do `Draw2d::load_texture()`, que assume input premultiplicado pro modo de blend
//     `GL_ONE, GL_ONE_MINUS_SRC_ALPHA` dele (o próprio comentário de topo-de-classe de
//     `render_gl3.cpp`). Uma API geral de "me dê os pixels" que devolvesse dado JÁ
//     premultiplicado corromperia silenciosamente qualquer chamador que não esteja prestes a
//     alimentar esses bytes exatos naquele caminho de blend exato (medir conteúdo, inspecionar
//     um canal específico, compor com uma convenção de blend DIFERENTE) -- alpha straight é o
//     default honesto e sem surpresa; um chamador que quer bytes premultiplicados pro próprio
//     pipeline de textura da glintfx ainda ganha isso de graça, automaticamente, DENTRO do
//     próprio passo de ingestão do `create_texture()`.
//
//     FAIL-HIGH (a própria disciplina do D7/D10, mesma forma do `create_texture()`/
//     `load_texture()`): `ok == false` é o ÚNICO sinal de falha, `width`/`height`/`pixels` ficam
//     nos próprios valores default-construídos (nunca parcialmente preenchidos) -- um caminho/
//     buffer nulo, um buffer de comprimento zero, um arquivo que não pode ser aberto/medido/lido
//     por completo, um tamanho de arquivo/buffer acima do teto (o MESMO teto de 256 MiB
//     `kMaxImageDecodeBytes` que `load_texture()`/`create_texture()` já aplicam,
//     `image_decode.hpp`), ou um formato codificado desconhecido/corrompido todos rejeitam de
//     forma limpa, nunca um crash, nunca uma leitura parcial além do fim de um buffer hostil/
//     truncado. A própria ordem de guarda tamanho-depois-leitura do `decode_image_file()` espelha
//     o idioma ifstream literal do `Draw2d::load_texture()` (`draw2d.cpp`): seek até o fim,
//     checa o tamanho ANTES de sequer alocar um buffer pra ele (pra que um arquivo hostil/enorme
//     em disco seja rejeitado sem tentativa de alocação de tamanho excessivo), depois lê. Nenhuma
//     das duas funções pode logar (não existe instância `Draw2d`/`Impl` pra logar através aqui --
//     a mesma disciplina "não pode logar, `ok` é o único sinal" que `decode_premultiplied_rgba()`
//     já documenta): um chamador que quer uma mensagem de diagnóstico decide o que logar em volta
//     da chamada, igual a todo outro helper puro/headless desta biblioteca.
//
//     AS DUAS FUNÇÕES SÃO `noexcept` (DEC-NOTHROW, W21, 2026-07-30): "nunca um crash" acima não é
//     só uma diretriz, é uma garantia `noexcept` na própria assinatura -- uma condição de
//     esgotamento de memória durante o decode (um modo de falha real, medido: um buffer fornecido
//     pelo chamador grande o bastante pra decodificar com sucesso mas grande demais pra TAMBÉM
//     copiar, ex.: um PNG paletizado cuja razão tamanho-comprimido/tamanho-decodificado deixa o
//     próprio buffer intermediário do decode MENOR que o buffer de pixel final) degrada pra
//     `ok == false`, exatamente como toda outra rejeição acima, nunca um `std::bad_alloc`
//     escapando através desta fronteira de API. Ver o próprio comentário de topo de
//     `src/image.cpp` pro `try`/`catch` com que esta garantia é implementada, e
//     `image_decode_hardening_sanity.cpp` pro teste de falha-de-alocação-forçada que prova isto
//     sob um aperto REAL de `RLIMIT_AS`, não hipotético.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstddef>
#include <vector>

namespace glintfx {

// EN: Result of a decode attempt. `ok == false` on ANY failure -- `width`/`height`/`pixels` stay
//     at their default-constructed values in that case. On success, `pixels` holds exactly
//     `width * height * 4` bytes, RGBA8, STRAIGHT (non-premultiplied) alpha -- see this file's
//     own top comment for why (matches `Draw2d::create_texture()`'s `PixelFormat::Rgba8` input
//     contract, diverges deliberately from the private `image_decode.hpp::DecodedImage`'s own
//     premultiplied convention).
// PT: Resultado de uma tentativa de decode. `ok == false` em QUALQUER falha --
//     `width`/`height`/`pixels` ficam nos próprios valores default-construídos nesse caso. Em
//     sucesso, `pixels` guarda exatamente `width * height * 4` bytes, RGBA8, alpha STRAIGHT
//     (não-premultiplicado) -- ver o próprio comentário do topo deste arquivo pro porquê (bate
//     com o contrato de input `PixelFormat::Rgba8` do `Draw2d::create_texture()`, diverge de
//     propósito da própria convenção premultiplicada do `image_decode.hpp::DecodedImage`
//     privado).
struct DecodedImagePixels {
  bool ok = false;
  int width = 0;
  int height = 0;
  std::vector<unsigned char> pixels;
};

// EN: Decodes the file at `path` (PNG/JPG/TGA -- whatever `stb_image` recognises, the same
//     formats `Draw2d::load_texture()` accepts) into straight-alpha RGBA8 pixels. `path ==
//     nullptr`, a file that cannot be opened/sized, a 0-byte or over-`kMaxImageDecodeBytes` file
//     (rejected BEFORE ever allocating a read buffer for it), a short read, an out-of-memory
//     condition (DEC-NOTHROW -- see this file's own top comment), or a decode failure
//     (unknown/corrupt format) all yield `DecodedImagePixels{}` (`ok == false`). `noexcept`: see
//     this file's own top comment for the full guard order and the "cannot log" contract.
// PT: Decodifica o arquivo em `path` (PNG/JPG/TGA -- o que o `stb_image` reconhecer, os mesmos
//     formatos que `Draw2d::load_texture()` aceita) em pixels RGBA8 alpha straight. `path ==
//     nullptr`, um arquivo que não pode ser aberto/medido, um arquivo de 0 bytes ou acima de
//     `kMaxImageDecodeBytes` (rejeitado ANTES de sequer alocar um buffer de leitura pra ele), uma
//     leitura curta, uma condição de esgotamento de memória (DEC-NOTHROW -- ver o próprio
//     comentário de topo deste arquivo), ou uma falha de decode (formato desconhecido/corrompido)
//     todos rendem `DecodedImagePixels{}` (`ok == false`). `noexcept`: ver o próprio comentário do
//     topo deste arquivo pra ordem de guarda completa e o contrato "não pode logar".
DecodedImagePixels decode_image_file(const char* path) noexcept;

// EN: Decodes `len` bytes of an already-in-memory encoded image (a buffer downloaded, embedded
//     as a resource, or read by the caller through its own I/O path) into straight-alpha RGBA8
//     pixels. `data == nullptr`, `len == 0`, `len` over `kMaxImageDecodeBytes` (rejected BEFORE
//     ever touching `data`), an out-of-memory condition (DEC-NOTHROW -- see this file's own top
//     comment), or a decode failure all yield `DecodedImagePixels{}` (`ok == false`). `noexcept`.
//     This is the memory-buffer sibling `decode_image_file()` above delegates to internally after
//     its own file-read step.
// PT: Decodifica `len` bytes de uma imagem codificada já em memória (um buffer baixado,
//     embutido como recurso, ou lido pelo próprio chamador via caminho de I/O próprio) em pixels
//     RGBA8 alpha straight. `data == nullptr`, `len == 0`, `len` acima de `kMaxImageDecodeBytes`
//     (rejeitado ANTES de sequer tocar `data`), uma condição de esgotamento de memória
//     (DEC-NOTHROW -- ver o próprio comentário de topo deste arquivo), ou uma falha de decode
//     todos rendem `DecodedImagePixels{}` (`ok == false`). `noexcept`. É o irmão de
//     buffer-em-memória pro qual o próprio `decode_image_file()` acima delega internamente após
//     o próprio passo de leitura de arquivo.
DecodedImagePixels decode_image_memory(const unsigned char* data, std::size_t len) noexcept;

// EN: IMG-ENCODE (W21, 2026-07-30) -- the symmetric counterpart to the decode pair above:
//     encode already-in-memory pixels INTO an encoded file/buffer, the direction
//     `decode_image_file()`/`decode_image_memory()` never covered. Added at the END of this
//     file, after the decode pair, so `decode_image_file()`/`decode_image_memory()`'s own line
//     numbers (187/209/226 as of DEC-NOTHROW/DEC-MOVE, W21 2026-07-30 -- cited verbatim by
//     `docs/draw2d.md` in six places; the earlier 164/184/199 pins predate that same slice's own
//     `noexcept`-and-guard doc-comment growth above, re-derived and fixed up in the same commit)
//     stay put; same "append, do not insert" discipline `image_decode.hpp`'s own IMG-DECODE
//     addition already used for the identical reason (see that file's own top comment on
//     `decode_straight_rgba`).
//
//     SEVEN FORMATS, the team lead's own explicit scope decision (TODO.md `IMG-ENCODE`, W21):
//     PNG, JPG, PPM, BMP, TGA, HDR, QOI -- and no others. `ImageFormat` is an ENUM, not a
//     filename-extension guess, on purpose: the in-memory entry point (`encode_image_memory()`)
//     has no filename to guess FROM in the first place, so any extension-sniffing rule would
//     necessarily be a second, divergent code path that only the file entry point could use --
//     an explicit, caller-supplied enum keeps both entry points sharing ONE dispatch, and avoids
//     the entire class of "wrong format because the caller's own path had an unexpected/absent/
//     spoofed extension" bug an extension-guessing API invites.
//
//     PIXEL CONVENTION, matching the decode pair above on purpose (a decode -> re-encode round
//     trip needs zero conversion): `pixels` is straight (non-premultiplied) RGBA8, exactly
//     `width * height * 4` bytes, row-major, TOP-LEFT origin, y-DOWN -- the SAME layout
//     `DecodedImagePixels::pixels` itself is in. This is a "pixels already in CPU memory, top-
//     down" contract, NOT a raw GL-readback contract: a caller reading back a GL framebuffer
//     (bottom-up, `glReadPixels`' own convention -- see `App::snapshot()`, `app.cpp`, for that
//     exact flip) must flip rows itself before calling either function below; this module does
//     not do it for them, because it has no GL dependency of any kind to know a framebuffer was
//     ever involved (same "zero third-party type crosses this header" discipline as the decode
//     pair above, AGENTS.md's own encapsulation gate).
//
//     ALPHA-LESS FORMATS (JPG, PPM, HDR, and BMP/TGA's own OPTIONAL alpha channel is written when
//     present but never REQUIRED to be non-trivial): the alpha byte of every input pixel is
//     still read (JPG's own encoder ignores a 4th component internally rather than requiring the
//     caller to pre-strip it; PPM/HDR below explicitly drop it) -- no format below REJECTS an
//     input buffer merely for carrying a non-255 alpha channel it cannot represent; it silently
//     loses that channel on write, exactly as real-world lossy/legacy image formats do. A caller
//     that needs alpha preserved through a round trip picks PNG, TGA (with alpha), BMP (with
//     alpha), or QOI.
//
//     JPG has NO alpha channel and IS lossy (DCT + chroma subsampling, the same artifacts
//     `image_decode_sanity.cpp`'s own JPEG fixture already documents on the decode side) -- it is
//     the wrong format for anything that will be used as a sprite/UI asset needing exact colour
//     or transparency; `EncodeImageOptions::jpg_quality` (1..100, stb_image_write's own scale,
//     default 90) is the ONE format-specific knob this API exposes, and it is exposed ONLY for
//     JPG -- no other format in this set has a meaningful, cheap-to-explain quality axis (PNG's
//     own zlib compression LEVEL is a size/CPU trade-off with NO visual-quality effect, since PNG
//     is lossless either way, and is deliberately NOT exposed here, see `docs/capabilities.md`'s
//     declared ceiling for this module).
//
//     HDR is written by CONVERTING this module's own 8-bit RGBA8 input to float (each of R/G/B
//     divided by 255.0f; alpha is DROPPED, not encoded -- the Radiance/RGBE format `stbi_write_
//     hdr()` itself writes carries no alpha channel semantics at all) -- this is a DELIBERATE,
//     DECLARED DOWNGRADE, stated loudly here because it is easy to miss: NO actual extended
//     dynamic range is produced or ever COULD be from this input, because nothing in glintfx
//     today produces a float-valued pixel buffer to feed it (no HDR framebuffer, no linear-light
//     capture path) -- see `docs/capabilities.md`'s Image-encode row for the same ceiling stated
//     as a consumer-facing promise. HDR output from this API is useful only as a CONTAINER
//     (a caller that specifically needs the `.hdr`/Radiance file format for some downstream tool
//     that reads it) and MUST NOT be read as "this glintfx build can capture real HDR content" --
//     it cannot, by construction, until a genuine float-pixel producer exists somewhere upstream
//     of this call (tracked, if ever needed, as a future INBOX item; not filed here per this
//     module's own "measured need, not speculative surface" discipline, same as `FW-LOG`/
//     `FW-CLOCK`, `docs/capabilities.md`).
//
//     QOI is LOSSLESS and FAST to both encode and decode (see `src/qoi_encode.hpp`'s own top
//     comment for the format/algorithm detail) but is recognised by almost no external tool,
//     viewer, or other library -- it is the right choice for this library's OWN internal
//     round-trip use (a cache written and read back only by glintfx-based code, or a host's own
//     equivalent) and the WRONG choice for interchange with anything outside that (email it to
//     someone, open it in a generic image viewer, hand it to a web browser -- none of those read
//     QOI). This library offers QOI ENCODE only; QOI DECODE is out of scope until a real need for
//     reading QOI back in is measured (same declared-gap discipline as `docs/capabilities.md`'s
//     `i18n`/`log`/`clock` rows).
//
//     FAIL-HIGH (the SAME discipline the decode pair above documents): `encode_image_file()`
//     returns `false`, `encode_image_memory()` returns `EncodedImageBytes{false, {}}`, on ANY of:
//     `pixels == nullptr`; `width <= 0` or `height <= 0`; a `width`/`height` pair whose
//     `width * height * 4` byte count would overflow `size_t` arithmetic OR exceed
//     `kMaxImageEncodeBytes` (64 MiB -- `image_encode.cpp`'s own top comment derives this exact
//     number: `4096 * 4096 * 4` bytes exactly, chosen so a 4K UHD RGBA8 frame -- `3840 * 2160 * 4`
//     ≈ 33 MiB -- fits with comfortable headroom while the boundary itself stays cheap enough to
//     exercise with a REAL buffer in a test, not just an oversized `len` claim against a tiny
//     one); an `ImageFormat` value outside the seven declared enumerators (a hostile/malformed
//     `static_cast<ImageFormat>(n)`, rejected by an explicit `default:` branch, never a fall-
//     through into whichever encoder happens to sit at that bit pattern); or, for
//     `encode_image_file()` only, a `path` that is `nullptr`, empty, points at a directory, or
//     cannot be opened for writing (missing parent directory, permission denied) -- ALL of these
//     degrade to a clean `false`/`ok == false`, never a crash, never a partial file left on disk
//     (see `image_encode.cpp`'s own doc-comment for why a failed file write is truncated away,
//     not left as a corrupt partial artifact). `encode_image_memory()` returning by VALUE
//     (`EncodedImageBytes{ok, bytes}`, an owned `std::vector`, no raw pointer, no manual free --
//     the SAME "no raw pointer" design call `DecodedImagePixels` above already makes) structurally
//     ELIMINATES the "null output buffer" failure mode a caller-supplied-buffer API would need to
//     guard: there is no output-buffer PARAMETER for a caller to pass as null in the first place.
//
//     BOTH FUNCTIONS ARE `noexcept` (ENC-NOTHROW, W21, 2026-07-30, the SAME "never a crash"
//     guarantee DEC-NOTHROW gives the decode pair above): an out-of-memory condition anywhere in
//     the encode path -- the output buffer itself, a format-specific scratch buffer (HDR's own
//     float conversion, QOI's own chunk stream), or the destination file's own write buffer --
//     degrades to `false`/`ok == false`, exactly like every other rejection above, never a
//     `std::bad_alloc` escaping across this API boundary. See `image_encode.cpp`'s own top comment
//     for the enumerated allocation points this guarantee covers and the `try`/`catch` it is
//     implemented with.
// PT: IMG-ENCODE (W21, 2026-07-30) -- a contraparte simétrica do par de decode acima: codifica
//     pixels já em memória PARA um arquivo/buffer codificado, a direção que
//     `decode_image_file()`/`decode_image_memory()` nunca cobriu. Somado no FIM deste arquivo,
//     depois do par de decode, pra que os próprios números de linha de
//     `decode_image_file()`/`decode_image_memory()` (187/209/226 desde DEC-NOTHROW/DEC-MOVE, W21
//     2026-07-30 -- citados ao pé da letra pelo `docs/draw2d.md` em seis lugares; os antigos
//     164/184/199 são de antes do crescimento de doc-comment de `noexcept`-e-guarda daquela mesma
//     fatia acima, re-derivados e corrigidos no mesmo commit) fiquem parados; mesma disciplina
//     "acrescenta, não insere" que a própria adição IMG-DECODE de `image_decode.hpp` já usou pelo
//     motivo idêntico (ver o próprio comentário de topo daquele arquivo sobre
//     `decode_straight_rgba`).
//
//     SETE FORMATOS, a própria decisão de escopo explícita do líder de time (TODO.md
//     `IMG-ENCODE`, W21): PNG, JPG, PPM, BMP, TGA, HDR, QOI -- e nenhum outro. `ImageFormat` é um
//     ENUM, não um chute de extensão de nome de arquivo, de propósito: o ponto de entrada em
//     memória (`encode_image_memory()`) não tem nome de arquivo nenhum pra chutar A PARTIR, então
//     qualquer regra de farejar-extensão seria necessariamente um segundo caminho de código
//     divergente que só o ponto de entrada de arquivo poderia usar -- um enum explícito, fornecido
//     pelo chamador, mantém os dois pontos de entrada compartilhando UM dispatch só, e evita toda
//     a classe de bug "formato errado porque o próprio path do chamador tinha extensão
//     inesperada/ausente/falsificada" que uma API de chute-de-extensão convida.
//
//     CONVENÇÃO DE PIXEL, batendo com o par de decode acima de propósito (um round trip decode ->
//     re-encode não precisa de conversão nenhuma): `pixels` é RGBA8 straight (não-
//     premultiplicado), exatamente `width * height * 4` bytes, row-major, origem SUPERIOR-
//     ESQUERDA, y PRA BAIXO -- o MESMO layout em que o próprio `DecodedImagePixels::pixels` já
//     está. Este é um contrato "pixels já em memória de CPU, top-down", NÃO um contrato de
//     readback GL cru: um chamador lendo de volta um framebuffer GL (bottom-up, a própria
//     convenção do `glReadPixels` -- ver `App::snapshot()`, `app.cpp`, pro flip exato) precisa
//     inverter as linhas ele mesmo antes de chamar qualquer uma das funções abaixo; este módulo
//     não faz isso por ele, porque não tem dependência de GL nenhuma pra sequer saber que um
//     framebuffer chegou a estar envolvido (mesma disciplina "zero tipo de terceiro cruza este
//     header" do par de decode acima, o próprio gate de encapsulamento do AGENTS.md).
//
//     FORMATOS SEM ALPHA (JPG, PPM, HDR, e o canal alpha OPCIONAL do próprio BMP/TGA é escrito
//     quando presente mas nunca EXIGIDO ser não-trivial): o byte de alpha de todo pixel de input
//     ainda é lido (o próprio encoder do JPG ignora um 4º componente internamente em vez de
//     exigir que o chamador pré-remova ele; PPM/HDR abaixo o descartam explicitamente) -- nenhum
//     formato abaixo REJEITA um buffer de input só por carregar um canal alpha não-255 que não
//     consegue representar; ele simplesmente perde esse canal na escrita, exatamente como
//     formatos de imagem legados/com-perda do mundo real fazem. Um chamador que precisa de alpha
//     preservado por um round trip escolhe PNG, TGA (com alpha), BMP (com alpha), ou QOI.
//
//     JPG NÃO tem canal alpha e É com perda (DCT + subsampling de croma, os MESMOS artefatos que
//     a própria fixture JPEG de `image_decode_sanity.cpp` já documenta no lado decode) -- é o
//     formato errado pra qualquer coisa que vá ser usada como sprite/asset de UI precisando de
//     cor exata ou transparência; `EncodeImageOptions::jpg_quality` (1..100, a própria escala do
//     stb_image_write, default 90) é o ÚNICO botão específico de formato que esta API expõe, e é
//     exposto SÓ pro JPG -- nenhum outro formato deste conjunto tem um eixo de qualidade
//     significativo e barato-de-explicar (o próprio NÍVEL de compressão zlib do PNG é uma troca
//     tamanho/CPU SEM efeito de qualidade visual, já que PNG é sem perda de qualquer forma, e é
//     deliberadamente NÃO exposto aqui, ver o teto declarado deste módulo em
//     `docs/capabilities.md`).
//
//     HDR é escrito CONVERTENDO o próprio input RGBA8 de 8 bits deste módulo pra float (cada um
//     de R/G/B dividido por 255.0f; o alpha é DESCARTADO, não codificado -- o próprio formato
//     Radiance/RGBE que `stbi_write_hdr()` escreve não carrega semântica de canal alpha nenhuma)
//     -- isto é um DOWNGRADE DELIBERADO, DECLARADO, dito em voz alta aqui porque é fácil de
//     passar despercebido: NENHUMA faixa dinâmica estendida real é produzida ou jamais PODERIA
//     ser a partir deste input, porque nada na glintfx hoje produz um buffer de pixel com valor
//     float pra alimentá-la (nenhum framebuffer HDR, nenhum caminho de captura em luz linear) --
//     ver a linha de encode-de-imagem de `docs/capabilities.md` pro mesmo teto dito como promessa
//     voltada ao consumidor. A saída HDR desta API só é útil como CONTAINER (um chamador que
//     precisa especificamente do formato de arquivo `.hdr`/Radiance pra alguma ferramenta a
//     jusante que o lê) e NÃO PODE ser lida como "este build da glintfx consegue capturar
//     conteúdo HDR de verdade" -- não consegue, por construção, até que um produtor genuíno de
//     pixel-float exista em algum lugar a montante desta chamada (rastreado, se algum dia
//     necessário, como um item futuro de INBOX; não aberto aqui, seguindo a própria disciplina
//     "necessidade medida, não superfície especulativa" deste módulo, mesma do `FW-LOG`/
//     `FW-CLOCK`, `docs/capabilities.md`).
//
//     QOI é SEM PERDA e RÁPIDO tanto pra codificar quanto pra decodificar (ver o próprio
//     comentário de topo de `src/qoi_encode.hpp` pro detalhe de formato/algoritmo) mas é
//     reconhecido por quase nenhuma ferramenta, visualizador, ou outra biblioteca externa -- é a
//     escolha certa pro USO interno de round-trip da PRÓPRIA biblioteca (um cache escrito e lido
//     de volta só por código baseado em glintfx, ou o equivalente próprio de um host) e a escolha
//     ERRADA pra intercâmbio com qualquer coisa fora disso (mandar por email pra alguém, abrir
//     num visualizador de imagem genérico, entregar a um navegador web -- nenhum desses lê QOI).
//     Esta biblioteca oferece só ENCODE de QOI; DECODE de QOI fica fora de escopo até que uma
//     necessidade real de ler QOI de volta seja medida (mesma disciplina de lacuna declarada das
//     linhas `i18n`/`log`/`clock` de `docs/capabilities.md`).
//
//     FAIL-HIGH (a MESMA disciplina que o par de decode acima documenta): `encode_image_file()`
//     devolve `false`, `encode_image_memory()` devolve `EncodedImageBytes{false, {}}`, em
//     QUALQUER um de: `pixels == nullptr`; `width <= 0` ou `height <= 0`; um par `width`/`height`
//     cuja contagem de bytes `width * height * 4` daria overflow em aritmética `size_t` OU
//     excederia `kMaxImageEncodeBytes` (64 MiB -- o próprio comentário de topo de
//     `image_encode.cpp` deriva este número exato: `4096 * 4096 * 4` bytes exatamente, escolhido
//     pra que um frame 4K UHD RGBA8 -- `3840 * 2160 * 4` ≈ 33 MiB -- caiba com folga confortável
//     enquanto a própria fronteira fica barata o bastante pra exercitar com um buffer REAL num
//     teste, não só uma reivindicação de `len` acima do teto contra um buffer minúsculo); um
//     valor `ImageFormat` fora dos sete enumeradores declarados (um `static_cast<ImageFormat>(n)`
//     hostil/malformado, rejeitado por um ramo `default:` explícito, nunca um fall-through pro
//     encoder que por acaso estiver naquele padrão de bit); ou, só pra `encode_image_file()`, um
//     `path` que é `nullptr`, vazio, aponta pra um diretório, ou não pode ser aberto pra escrita
//     (diretório pai ausente, permissão negada) -- TODOS esses degradam pra um `false`/`ok ==
//     false` limpo, nunca um crash, nunca um arquivo parcial deixado no disco (ver o próprio
//     comentário de `image_encode.cpp` pro porquê de uma escrita de arquivo falha ser truncada
//     fora, não deixada como um artefato parcial corrompido). `encode_image_memory()` devolvendo
//     por VALOR (`EncodedImageBytes{ok, bytes}`, um `std::vector` de posse própria, sem ponteiro
//     cru, sem free manual -- a MESMA escolha de desenho "sem ponteiro cru" que o próprio
//     `DecodedImagePixels` acima já faz) ELIMINA por construção a classe de falha "buffer de
//     saída nulo" que uma API de buffer-fornecido-pelo-chamador precisaria guardar: não existe
//     PARÂMETRO de buffer de saída pro chamador sequer passar como nulo, pra começo de conversa.
//
//     AS DUAS FUNÇÕES SÃO `noexcept` (ENC-NOTHROW, W21, 2026-07-30, a MESMA garantia "nunca um
//     crash" que o DEC-NOTHROW dá ao par de decode acima): uma condição de esgotamento de memória
//     em qualquer ponto do caminho de encode -- o próprio buffer de saída, um buffer de scratch
//     específico de formato (a própria conversão float do HDR, o próprio fluxo de chunk do QOI),
//     ou o próprio buffer de escrita do arquivo de destino -- degrada pra `false`/`ok == false`,
//     exatamente como toda outra rejeição acima, nunca um `std::bad_alloc` escapando através
//     desta fronteira de API. Ver o próprio comentário de topo de `image_encode.cpp` pra lista
//     enumerada de pontos de alocação que esta garantia cobre e o `try`/`catch` com que ela é
//     implementada.
enum class ImageFormat {
  Png,
  Jpg,
  Ppm,
  Bmp,
  Tga,
  Hdr,
  Qoi,
};

// EN: `jpg_quality` is the ONLY knob this API exposes, and it is meaningful ONLY for
//     `ImageFormat::Jpg` -- ignored by every other format (see this file's top comment on why no
//     other format gets a quality axis here). Range 1..100 (stb_image_write's own scale, NOT
//     validated/clamped by this API -- an out-of-range value is passed straight through to
//     stb_image_write, whose own internal quantization-table clamp already makes any value
//     outside 1..100 degrade to its nearest in-range neighbour rather than crash or produce
//     malformed output; adding a second clamp here would just be defensive duplication of a
//     guarantee the vendored library already provides).
// PT: `jpg_quality` é o ÚNICO botão que esta API expõe, e só é significativo pro
//     `ImageFormat::Jpg` -- ignorado por todo outro formato (ver o comentário de topo deste
//     arquivo pro porquê de nenhum outro formato ganhar um eixo de qualidade aqui). Faixa 1..100
//     (a própria escala do stb_image_write, NÃO validada/limitada por esta API -- um valor fora
//     de faixa passa direto pro stb_image_write, cujo próprio clamp interno de tabela de
//     quantização já faz qualquer valor fora de 1..100 degradar pro vizinho válido mais próximo
//     em vez de crashar ou produzir saída malformada; adicionar um segundo clamp aqui seria só
//     duplicação defensiva de uma garantia que a biblioteca vendorizada já provê).
struct EncodeImageOptions {
  int jpg_quality = 90;
};

// EN: Result of an in-memory encode attempt. `ok == false` on ANY failure -- `bytes` stays empty
//     in that case (never partially filled with a truncated/corrupt stream). See this file's own
//     top comment for the full guard list and the "no output-buffer parameter" design note.
// PT: Resultado de uma tentativa de encode em memória. `ok == false` em QUALQUER falha -- `bytes`
//     fica vazio nesse caso (nunca parcialmente preenchido com um fluxo truncado/corrompido). Ver
//     o próprio comentário de topo deste arquivo pra lista de guarda completa e a nota de desenho
//     "sem parâmetro de buffer de saída".
struct EncodedImageBytes {
  bool ok = false;
  std::vector<unsigned char> bytes;
};

// EN: Encodes `width * height` straight-alpha RGBA8 pixels (see this file's top comment for the
//     exact layout contract) into an owned in-memory byte buffer, in `format`. This is the entry
//     point for the two consumer-driving use cases named in TODO.md's `IMG-ENCODE` item that
//     never want a file on disk: a screen-capture buffer handed straight to a caller's own
//     upload/save/network path, and a frozen background composited behind a menu. `noexcept` --
//     an out-of-memory condition (ENC-NOTHROW, see this file's own top comment) also yields
//     `EncodedImageBytes{}` (`ok == false`), same as every other guard. See this file's own top
//     comment for the full format-by-format contract and fail-high guard list.
// PT: Codifica `width * height` pixels RGBA8 alpha straight (ver o comentário de topo deste
//     arquivo pro contrato de layout exato) num buffer de bytes em memória de posse própria, no
//     `format`. Este é o ponto de entrada pros dois casos de uso motivadores do consumidor
//     citados no item `IMG-ENCODE` do TODO.md que nunca querem um arquivo em disco: um buffer de
//     captura de tela entregue direto pro próprio caminho de upload/save/rede do chamador, e um
//     fundo congelado composto atrás de um menu. `noexcept` -- uma condição de esgotamento de
//     memória (ENC-NOTHROW, ver o próprio comentário de topo deste arquivo) também rende
//     `EncodedImageBytes{}` (`ok == false`), igual a toda outra guarda. Ver o próprio comentário
//     de topo deste arquivo pro contrato completo formato-a-formato e a lista de guarda
//     fail-high.
EncodedImageBytes encode_image_memory(ImageFormat format, int width, int height,
                                      const unsigned char* pixels,
                                      const EncodeImageOptions& options = EncodeImageOptions{}) noexcept;

// EN: Encodes `width * height` straight-alpha RGBA8 pixels to the file at `path`, in `format`.
//     Delegates internally to `encode_image_memory()` above (encode once, write the resulting
//     buffer to disk) -- the SAME "file entry point is a thin wrapper over the memory entry
//     point" shape `decode_image_file()` uses in the opposite direction (file -> memory) above,
//     so there is exactly one encode implementation per format, not two. `noexcept` -- an
//     out-of-memory condition (ENC-NOTHROW, see this file's own top comment), including one
//     specific to the file-write step itself, also yields `false`, same as every other guard. See
//     this file's own top comment for the full fail-high guard list, including the
//     file-I/O-specific failure modes (`path == nullptr`, empty, a directory, or unopenable for
//     writing).
// PT: Codifica `width * height` pixels RGBA8 alpha straight pro arquivo em `path`, no `format`.
//     Delega internamente pro próprio `encode_image_memory()` acima (codifica uma vez, escreve o
//     buffer resultante em disco) -- a MESMA forma "ponto de entrada de arquivo é um wrapper fino
//     sobre o ponto de entrada de memória" que o `decode_image_file()` usa na direção oposta
//     (arquivo -> memória) acima, então existe exatamente uma implementação de encode por
//     formato, não duas. `noexcept` -- uma condição de esgotamento de memória (ENC-NOTHROW, ver o
//     próprio comentário de topo deste arquivo), inclusive uma específica do próprio passo de
//     escrita de arquivo, também rende `false`, igual a toda outra guarda. Ver o próprio
//     comentário de topo deste arquivo pra lista de guarda fail-high completa, incluindo os
//     modos de falha específicos de I/O de arquivo (`path == nullptr`, vazio, um diretório, ou
//     não-abrível pra escrita).
bool encode_image_file(const char* path, ImageFormat format, int width, int height,
                       const unsigned char* pixels,
                       const EncodeImageOptions& options = EncodeImageOptions{}) noexcept;

// EN: DEC-FORMAT-SURFACE (W22, 2026-07-30) -- two facts about `decode_image_file()`/
//     `decode_image_memory()` above that were NOT written down anywhere before this addendum
//     (confirmed: this header already said "PNG/JPG/TGA" in four places -- `:2`, `:70`,
//     `:171`, `:177` -- so that much was never the gap; these two specifics were), added at
//     the END of this file on purpose (same "append, do not insert" discipline this file's
//     own top comment already states for `decode_image_file()`/`decode_image_memory()`'s own
//     line numbers, cited verbatim by `docs/draw2d.md`) so nothing above shifts:
//
//     (1) FORMAT DETECTION IS BY CONTENT, NEVER BY FILENAME EXTENSION. Neither function
//     above ever inspects `path`'s extension (there is no substring/suffix parsing of `path`
//     anywhere in this module -- `decode_image_memory()` does not even RECEIVE a path to
//     parse in the first place). The ONLY signal either function ever looks at is the byte
//     content itself, handed to `stb_image`'s own format-sniffing dispatch. Concretely: a
//     file named `sprite.png` whose BYTES are actually a JPEG stream decodes successfully AS
//     A JPEG (there is no rejection, no "extension says PNG but content is JPEG" mismatch
//     error) -- and the reverse (a `.jpg`-named file containing real PNG bytes) decodes as a
//     PNG the same way. A caller that wants to REJECT a mismatch between a filename's own
//     extension and its actual content must check that itself, upstream of this call; this
//     module has no opinion on `path`'s spelling.
//
//     (2) THE TGA DECODER TOLERATES A SHORT/TRUNCATED BUFFER, SILENTLY ZERO-FILLING THE
//     MISSING PIXEL DATA, RATHER THAN FAILING. `decode_image_file()`/`decode_image_memory()`
//     compile in THREE formats (`STBI_ONLY_PNG` + `STBI_ONLY_JPEG` + `STBI_ONLY_TGA`,
//     `src/stb_image_impl.cpp`), tried in that order -- TGA is the LAST format `stb_image`
//     tries, and its own signature test is notoriously loose (a plausible `image_type` byte
//     plus a plausible `bits_per_pixel` is enough to pass; it does not require the pixel data
//     that follows the 18-byte header to actually be present). Consumer-measured: an 18-byte
//     plausible-looking TGA header (declaring 4096x4096, 32 bits per pixel) followed by only
//     64 bytes of unrelated noise -- 82 bytes total -- decodes "successfully" through
//     `decode_image_file()`/`decode_image_memory()` into a 4096x4096, all-zero-alpha RGBA8
//     image (67 108 864 bytes allocated from 82 bytes of input, a ~818 000x amplification).
//     The result is a VALID `ok == true` handle, not a rejection -- a caller whose own
//     fallback logic keys off "is the handle valid" (not "did anything get zero-filled") will
//     not notice anything went wrong: the asset silently becomes a large transparent square
//     instead of failing loudly. `decode_png_file()` below exists PRECISELY to give a caller
//     that only ever wants PNG a way to close this specific hole at its own call site,
//     without the general 3-format dispatch above needing to change for every OTHER caller
//     that still wants JPG/TGA.
// PT: DEC-FORMAT-SURFACE (W22, 2026-07-30) -- dois fatos sobre `decode_image_file()`/
//     `decode_image_memory()` acima que NÃO estavam escritos em lugar nenhum antes deste
//     adendo (confirmado: este header já dizia "PNG/JPG/TGA" em quatro lugares -- `:2`, `:70`,
//     `:171`, `:177` -- então aquilo nunca foi a lacuna; estes dois detalhes específicos
//     eram), somado no FIM deste arquivo de propósito (mesma disciplina "acrescenta, não
//     insere" que o próprio comentário de topo deste arquivo já declara pros próprios números
//     de linha de `decode_image_file()`/`decode_image_memory()`, citados ao pé da letra pelo
//     `docs/draw2d.md`) pra que nada acima se desloque:
//
//     (1) A DETECÇÃO DE FORMATO É POR CONTEÚDO, NUNCA POR EXTENSÃO DE NOME DE ARQUIVO.
//     Nenhuma das duas funções acima jamais inspeciona a extensão de `path` (não há parse de
//     substring/sufixo de `path` em lugar nenhum deste módulo -- o `decode_image_memory()`
//     nem RECEBE um path pra parsear, pra começo de conversa). O ÚNICO sinal que qualquer uma
//     das duas funções olha é o próprio conteúdo de bytes, entregue ao dispatch de sniff de
//     formato próprio do `stb_image`. Concretamente: um arquivo chamado `sprite.png` cujos
//     BYTES são na verdade um fluxo JPEG decodifica com sucesso COMO UM JPEG (não há
//     rejeição, não há erro de "extensão diz PNG mas conteúdo é JPEG") -- e o inverso (um
//     arquivo nomeado `.jpg` contendo bytes PNG de verdade) decodifica como PNG da mesma
//     forma. Um chamador que quer REJEITAR uma incompatibilidade entre a extensão do próprio
//     nome de arquivo e o conteúdo real precisa checar isso ele mesmo, a montante desta
//     chamada; este módulo não tem opinião sobre a grafia de `path`.
//
//     (2) O DECODER DE TGA TOLERA UM BUFFER CURTO/TRUNCADO, PREENCHENDO SILENCIOSAMENTE O
//     DADO DE PIXEL AUSENTE COM ZEROS, EM VEZ DE FALHAR. `decode_image_file()`/
//     `decode_image_memory()` compilam TRÊS formatos (`STBI_ONLY_PNG` + `STBI_ONLY_JPEG` +
//     `STBI_ONLY_TGA`, `src/stb_image_impl.cpp`), tentados nessa ordem -- TGA é o ÚLTIMO
//     formato que o `stb_image` tenta, e o próprio teste de assinatura dele é notoriamente
//     frouxo (um byte `image_type` plausível mais um `bits_per_pixel` plausível já basta pra
//     passar; ele não exige que o dado de pixel que segue o header de 18 bytes esteja de fato
//     presente). Medido pelo consumidor: um header TGA plausível de 18 bytes (declarando
//     4096x4096, 32 bits por pixel) seguido de só 64 bytes de ruído sem relação -- 82 bytes no
//     total -- decodifica "com sucesso" através de `decode_image_file()`/
//     `decode_image_memory()` numa imagem RGBA8 4096x4096, alpha-zero-em-tudo
//     (67.108.864 bytes alocados a partir de 82 bytes de input, uma amplificação de
//     ~818.000x). O resultado é um handle `ok == true` VÁLIDO, não uma rejeição -- um
//     chamador cuja própria lógica de fallback depende de "o handle é válido" (não "algo foi
//     preenchido com zero") não vai perceber que algo deu errado: o asset silenciosamente
//     vira um quadrado grande transparente em vez de falhar alto. O `decode_png_file()`
//     abaixo existe PRECISAMENTE pra dar a um chamador que só quer PNG uma forma de fechar
//     este buraco específico no próprio call site dele, sem que o dispatch geral de 3
//     formatos acima precise mudar pra todo OUTRO chamador que ainda quer JPG/TGA.

// EN: DEC-FORMAT-SURFACE (W22, 2026-07-30) -- a PNG-ONLY sibling of `decode_image_file()`
//     above (`:209`), for a caller whose OWN asset pipeline only ever produces/accepts PNG
//     and wants that intent declared, and ENFORCED, at the call site -- rather than trusting
//     `decode_image_file()`'s general PNG/JPG/TGA dispatch (see this file's own top-of-file
//     DEC-FORMAT-SURFACE addendum, just above, for the concrete failure mode this closes: a
//     short/malformed buffer that merely resembles a TGA header decoding "successfully" into
//     a large all-zero image via the TGA leg of that dispatch).
//
//     HOW: reads the file exactly like `decode_image_file()` does (same pre-allocation size
//     guard, same `kMaxImageDecodeBytes` cap), then checks the FIRST 8 BYTES against the
//     fixed PNG file signature (`0x89 'P' 'N' 'G' \r \n 0x1A \n` -- the magic number every
//     PNG encoder in existence writes, from the PNG specification itself, RFC 2083/ISO 15948)
//     BEFORE the buffer ever reaches the shared decode seam. A buffer whose first 8 bytes do
//     not match -- including one deliberately crafted to pass `stb_image`'s own loose
//     TGA/JPEG sniffing (the exact 82-byte case named above) -- is rejected right there,
//     never reaching `stb_image` at all. A buffer that DOES pass the signature check still
//     goes through the full PNG decode afterwards (`decode_straight_rgba()`, the SAME seam
//     `decode_image_file()` uses internally) -- the signature check is a FIRST gate, not a
//     replacement for real decoding: a file with a valid PNG signature but a corrupt or
//     truncated body still fails at decode, exactly like `decode_image_file()` already does
//     for corrupt PNG input.
//
//     NOT a general allowlist API (`decode_image_file(path, AllowedFormats)` or similar) --
//     the team's own consumer-driven scope decision (`TODO.md`'s `DEC-FORMAT-SURFACE`, W22):
//     the two call sites that need format restriction want to say "only PNG" and nothing more
//     expressive; a combinatorial allowlist would trade one function for a design surface
//     (precedence rules, multi-format sets, its own test matrix) nobody asked for. If a
//     JPG-only or TGA-only sibling is ever needed, it is the SAME small shape (swap the
//     magic-byte table) added on measured demand -- not built speculatively here.
//
//     Same FAIL-HIGH/ownership/alpha contract as `decode_image_file()` above in every other
//     respect (`ok == false` the only failure signal, straight non-premultiplied RGBA8 on
//     success, `noexcept`, cannot log -- see this file's own top comment for the shared
//     rationale). `path == nullptr`, an unopenable/unsizeable file, a 0-byte or
//     over-`kMaxImageDecodeBytes` file, a short read, a signature mismatch, or a genuine PNG
//     decode failure (corrupt/truncated body) all yield `DecodedImagePixels{}` (`ok ==
//     false`).
// PT: DEC-FORMAT-SURFACE (W22, 2026-07-30) -- uma irmã SÓ-PNG do `decode_image_file()` acima
//     (`:209`), pra um chamador cujo PRÓPRIO pipeline de asset só produz/aceita PNG e quer
//     essa intenção declarada, e IMPOSTA, no próprio call site -- em vez de confiar no
//     dispatch geral PNG/JPG/TGA do `decode_image_file()` (ver o próprio adendo DEC-FORMAT-
//     SURFACE do topo deste arquivo, logo acima, pro modo de falha concreto que isto fecha:
//     um buffer curto/malformado que só PARECE um header TGA decodificando "com sucesso" numa
//     imagem grande toda-alpha-zero via a perna TGA daquele dispatch).
//
//     COMO: lê o arquivo exatamente como o `decode_image_file()` faz (mesma guarda de tamanho
//     pré-alocação, mesmo teto `kMaxImageDecodeBytes`), depois checa os PRIMEIROS 8 BYTES
//     contra a assinatura fixa de arquivo PNG (`0x89 'P' 'N' 'G' \r \n 0x1A \n` -- o número
//     mágico que todo encoder de PNG já existente escreve, da própria especificação PNG, RFC
//     2083/ISO 15948) ANTES do buffer sequer alcançar a costura de decode compartilhada. Um
//     buffer cujos primeiros 8 bytes não batem -- inclusive um deliberadamente forjado pra
//     passar no próprio sniff frouxo de TGA/JPEG do `stb_image` (o caso exato de 82 bytes
//     nomeado acima) -- é rejeitado ali mesmo, nunca chega a alcançar o `stb_image`. Um buffer
//     que PASSA na checagem de assinatura ainda passa pelo decode PNG completo depois
//     (`decode_straight_rgba()`, a MESMA costura que o `decode_image_file()` usa
//     internamente) -- a checagem de assinatura é uma PRIMEIRA guarda, não um substituto do
//     decode de verdade: um arquivo com assinatura PNG válida mas corpo corrompido ou
//     truncado ainda falha no decode, exatamente como o `decode_image_file()` já faz pra
//     input PNG corrompido.
//
//     NÃO é uma API de allowlist geral (`decode_image_file(path, AllowedFormats)` ou
//     similar) -- a própria decisão de escopo consumer-driven do time (`DEC-FORMAT-SURFACE`
//     do `TODO.md`, W22): os dois call sites que precisam de restrição de formato querem
//     dizer "só PNG" e nada mais expressivo; uma allowlist combinatória trocaria uma função
//     por uma superfície de desenho (regras de precedência, conjuntos multi-formato, a
//     própria matriz de teste) que ninguém pediu. Se algum dia surgir necessidade de uma irmã
//     só-JPG ou só-TGA, é a MESMA forma pequena (troca a tabela de magic bytes) somada sob
//     demanda medida -- não construída especulativamente aqui.
//
//     Mesmo contrato FAIL-HIGH/posse/alpha do `decode_image_file()` acima em todo outro
//     aspecto (`ok == false` é o ÚNICO sinal de falha, RGBA8 straight não-premultiplicado em
//     sucesso, `noexcept`, não pode logar -- ver o próprio comentário de topo deste arquivo
//     pro racional compartilhado). `path == nullptr`, um arquivo não-abrível/não-mensurável,
//     um arquivo de 0 bytes ou acima de `kMaxImageDecodeBytes`, uma leitura curta, uma
//     assinatura que não bate, ou uma falha de decode PNG genuína (corpo corrompido/truncado)
//     todos rendem `DecodedImagePixels{}` (`ok == false`).
DecodedImagePixels decode_png_file(const char* path) noexcept;

} // namespace glintfx
