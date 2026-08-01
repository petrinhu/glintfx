// SPDX-License-Identifier: Apache-2.0
// EN: IMG-ENCODE -- single translation unit that compiles stb_image_write (PNG/JPG/BMP/TGA/HDR
//     encode), the encode-side sibling of `stb_image_impl.cpp`'s decode-side TU. The header is
//     included once here with `STB_IMAGE_WRITE_IMPLEMENTATION` defined; every other translation
//     unit that needs the write API (`image_encode.cpp`) includes the header without that
//     define (declarations only) -- same "one implementation TU, N declaration-only includes"
//     discipline as the decode side.
//
//     `STBI_WRITE_NO_STDIO` is DELIBERATELY NOT DEFINED here, unlike what a first read of this
//     module's own design (every disk write goes through `std::ofstream`, `image_encode.cpp`
//     never calls a filename-taking `stbi_write_*()` overload) would suggest -- see the WHY
//     below, an upstream quirk this module works around rather than fights.
//
//     UPSTREAM QUIRK, MEASURED (stb_image_write.h v1.16, identical on the `nothings/stb` `master`
//     branch as of this writing -- checked, not assumed, per this repository's own "if dev/
//     master already fixed it, do not open a ticket" discipline; the opposite holds here, master
//     has NOT fixed it, but opening an upstream report is a separate decision this task does not
//     make): the ENTIRE "Radiance RGBE HDR writer" section of the vendored header -- including
//     `stbi_write_hdr_to_func()`, the callback-based, non-file-I/O entry point this module's own
//     `image_encode.cpp` needs for `ImageFormat::Hdr` -- sits inside a SINGLE
//     `#ifndef STBI_WRITE_NO_STDIO` block that never closes until AFTER that entire section (see
//     `stb_image_write.h` itself, the block opening right before `stbiw__linear_to_rgbe()` and
//     closing only after `stbi_write_hdr()`'s own filename-taking overload). Every OTHER
//     format's `_to_func` variant (PNG/BMP/TGA/JPG) is correctly UNGATED -- only HDR's is
//     accidentally swept into the same guard as the stdio-only file writer it sits next to in
//     the source. Defining `STBI_WRITE_NO_STDIO` therefore silently deletes
//     `stbi_write_hdr_to_func()` from this translation unit entirely (confirmed with `nm` on the
//     compiled object: the symbol is simply absent, not merely unreachable), producing a LINK-
//     TIME "undefined reference" the moment `image_encode.cpp` calls it for `ImageFormat::Hdr` --
//     caught by this module's own `tests/image_encode_sanity.cpp` HDR group failing to even
//     LINK, not a runtime bug.
//
//     THE FIX: do not define `STBI_WRITE_NO_STDIO` at all -- the filename-taking `stbi_write_*()`
//     overloads it would have stripped simply compile in, unused (this module never calls them,
//     `image_encode.cpp`'s own doc-comment states why: a `const char*` overload avoids
//     `std::filesystem::path::c_str()`'s wchar_t trap on MSVC, and routing every disk write
//     through ONE C++ idiom -- `std::ofstream`, open/write/check/close via RAII -- is simpler to
//     audit than trusting a second, vendored file-I/O code path this module never exercises).
//     Their presence costs a small amount of unused object-file size and nothing else; the
//     alternative (keeping the define and hand-rolling a THIRD Radiance/HDR writer just to dodge
//     five accidentally-gated lines in a vendored file) would be strictly worse.
// PT: IMG-ENCODE -- única unidade de tradução que compila o stb_image_write (encode
//     PNG/JPG/BMP/TGA/HDR), a irmã do lado encode da TU `stb_image_impl.cpp` do lado decode. O
//     header é incluído uma vez aqui com `STB_IMAGE_WRITE_IMPLEMENTATION` definido; toda outra
//     unidade de tradução que precisa da API de escrita (`image_encode.cpp`) inclui o header sem
//     esse define (só declarações) -- mesma disciplina "uma TU de implementação, N includes só-
//     declaração" do lado decode.
//
//     `STBI_WRITE_NO_STDIO` é DELIBERADAMENTE NÃO DEFINIDO aqui, ao contrário do que uma primeira
//     leitura do próprio desenho deste módulo (toda escrita em disco passa por
//     `std::ofstream`, `image_encode.cpp` nunca chama um overload `stbi_write_*()` baseado em
//     nome de arquivo) sugeriria -- ver o PORQUÊ abaixo, uma peculiaridade upstream que este
//     módulo contorna em vez de combater.
//
//     PECULIARIDADE UPSTREAM, MEDIDA (stb_image_write.h v1.16, idêntico na própria branch
//     `master` de `nothings/stb` no momento desta escrita -- conferido, não assumido, seguindo a
//     própria disciplina deste repositório "se dev/master já corrigiu, não abre ticket"; aqui vale
//     o oposto, o master NÃO corrigiu, mas abrir um reporte upstream é uma decisão separada que
//     esta tarefa não toma): a seção INTEIRA "Radiance RGBE HDR writer" do header vendorizado --
//     incluindo `stbi_write_hdr_to_func()`, o ponto de entrada baseado em callback, sem I/O de
//     arquivo, que o próprio `image_encode.cpp` deste módulo precisa pro `ImageFormat::Hdr` --
//     fica dentro de UM ÚNICO bloco `#ifndef STBI_WRITE_NO_STDIO` que só fecha DEPOIS daquela
//     seção inteira (ver o próprio `stb_image_write.h`, o bloco abrindo logo antes de
//     `stbiw__linear_to_rgbe()` e fechando só depois do próprio overload baseado em nome de
//     arquivo de `stbi_write_hdr()`). A variante `_to_func` de TODO OUTRO formato (PNG/BMP/
//     TGA/JPG) fica corretamente DESGATEADA -- só a do HDR é varrida acidentalmente pra dentro
//     da mesma guarda do escritor de arquivo só-stdio ao lado do qual ela senta no source.
//     Definir `STBI_WRITE_NO_STDIO` portanto apaga silenciosamente `stbi_write_hdr_to_func()`
//     desta unidade de tradução por completo (confirmado com `nm` no objeto compilado: o símbolo
//     simplesmente está ausente, não meramente inalcançável), produzindo um "undefined reference"
//     EM TEMPO DE LINK no instante em que `image_encode.cpp` a chama pro `ImageFormat::Hdr` --
//     pego pelo próprio grupo HDR de `tests/image_encode_sanity.cpp` deste módulo falhando ao
//     sequer LINKAR, não um bug de runtime.
//
//     O CONSERTO: não definir `STBI_WRITE_NO_STDIO` nenhum -- os overloads `stbi_write_*()`
//     baseados em nome de arquivo que ele teria removido simplesmente compilam, sem uso (este
//     módulo nunca os chama, o próprio comentário de `image_encode.cpp` diz o porquê: um overload
//     `const char*` evita a armadilha wchar_t do `std::filesystem::path::c_str()` no MSVC, e
//     rotear toda escrita em disco por UM idioma C++ só -- `std::ofstream`, abre/escreve/checa/
//     fecha via RAII -- é mais simples de auditar do que confiar num segundo caminho de I/O de
//     arquivo vendorizado que este módulo nunca exercita). A presença deles custa uma quantidade
//     pequena de tamanho de objeto sem uso e nada mais; a alternativa (manter o define e escrever
//     à mão um TERCEIRO escritor Radiance/HDR só pra desviar de cinco linhas acidentalmente
//     gateadas num arquivo vendorizado) seria estritamente pior.
// Copyright (c) 2026 Petrus Silva Costa
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
