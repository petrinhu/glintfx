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
//     (rejected BEFORE ever allocating a read buffer for it), a short read, or a decode failure
//     (unknown/corrupt format) all yield `DecodedImagePixels{}` (`ok == false`). See this file's
//     own top comment for the full guard order and the "cannot log" contract.
// PT: Decodifica o arquivo em `path` (PNG/JPG/TGA -- o que o `stb_image` reconhecer, os mesmos
//     formatos que `Draw2d::load_texture()` aceita) em pixels RGBA8 alpha straight. `path ==
//     nullptr`, um arquivo que não pode ser aberto/medido, um arquivo de 0 bytes ou acima de
//     `kMaxImageDecodeBytes` (rejeitado ANTES de sequer alocar um buffer de leitura pra ele), uma
//     leitura curta, ou uma falha de decode (formato desconhecido/corrompido) todos rendem
//     `DecodedImagePixels{}` (`ok == false`). Ver o próprio comentário do topo deste arquivo pra
//     ordem de guarda completa e o contrato "não pode logar".
DecodedImagePixels decode_image_file(const char* path);

// EN: Decodes `len` bytes of an already-in-memory encoded image (a buffer downloaded, embedded
//     as a resource, or read by the caller through its own I/O path) into straight-alpha RGBA8
//     pixels. `data == nullptr`, `len == 0`, `len` over `kMaxImageDecodeBytes` (rejected BEFORE
//     ever touching `data`), or a decode failure all yield `DecodedImagePixels{}` (`ok ==
//     false`). This is the memory-buffer sibling `decode_image_file()` above delegates to
//     internally after its own file-read step.
// PT: Decodifica `len` bytes de uma imagem codificada já em memória (um buffer baixado,
//     embutido como recurso, ou lido pelo próprio chamador via caminho de I/O próprio) em pixels
//     RGBA8 alpha straight. `data == nullptr`, `len == 0`, `len` acima de `kMaxImageDecodeBytes`
//     (rejeitado ANTES de sequer tocar `data`), ou uma falha de decode todos rendem
//     `DecodedImagePixels{}` (`ok == false`). É o irmão de buffer-em-memória pro qual o próprio
//     `decode_image_file()` acima delega internamente após o próprio passo de leitura de
//     arquivo.
DecodedImagePixels decode_image_memory(const unsigned char* data, std::size_t len);

} // namespace glintfx
