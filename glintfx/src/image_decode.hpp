// SPDX-License-Identifier: MPL-2.0
// EN: D2D-1A -- the image-decode carve (docs/superpowers/plans/2026-07-23-onda3-draw2d-d2d1.md,
//     decision D3). Extracts the "bytes in, pixels out" core that used to live fused inside
//     Gl3RenderInterface::LoadTexture (render_gl3.cpp): the 256 MiB AUD-L1-PARSE size cap on the
//     ENCODED byte buffer, the forced-RGBA stb_image decode, and the in-place premultiply. This
//     header has ZERO GL and ZERO RmlUi dependency -- it is a pure, headless-testable seam (same
//     discipline as src/log_dedup.hpp / src/input_state.hpp): callers that need RmlUi's
//     FileInterface (render_gl3.cpp today) or GL upload (Draw2D, next slice) do that on their own
//     side and hand this helper a plain byte buffer.
//
//     Split of responsibility with render_gl3.cpp's LoadTexture (IMPORTANT, this is the seam
//     that keeps "zero behaviour change" true): the pre-read file-SIZE guard (Seek/Tell BEFORE
//     ever allocating a buffer for the file, so a hostile/huge asset is rejected without a
//     bounded-but-still-large allocation attempt) stays in render_gl3.cpp -- it needs
//     Rml::FileInterface, which this header must not depend on. This helper's OWN 256 MiB cap
//     (`kMaxImageDecodeBytes`, checked on the `len` parameter it is handed) is a second,
//     independent line of defense: unreachable dead code on LoadTexture's own call site today
//     (the caller's guard already caps `len` before this helper ever runs), but the FIRST line of
//     defense for any OTHER caller that reads its own file and skips straight to this helper
//     (Draw2D's own `load_texture`, D2D-1B, D7) -- same "guard 1/2 + guard 2/2" belt-and-suspenders
//     idiom the original LoadTexture already used for exactly this reason (see that function's own
//     comment on why guard 2/2 stays explicit even though guard 1/2 already subsumes it).
//
//     A decode/cap failure here never logs (no Rml::Log -- this header cannot reach it): the
//     caller decides what to log and how to degrade, same as before this carve. `DecodedImage::ok
//     == false` is the ONLY signal; render_gl3.cpp's rewired LoadTexture (this slice) folds every
//     `ok == false` outcome into its existing "fallback to RenderInterface_GL3::LoadTexture" path,
//     identical to the pre-carve stb-decode-failure branch.
//
//     One deliberate, bounded-cost design choice this carve adds: the pre-carve code premultiplied
//     stb's own malloc'd buffer in place and handed the raw pointer straight to GenerateTexture
//     before it went out of scope. A "bytes in, pixels out" pure interface cannot hand back a raw
//     stb-owned pointer without leaking stb_image.h's own deleter type into every caller, so this
//     helper copies the premultiplied pixels into an owned std::vector once, on decode. Extra cost:
//     one memcpy-sized copy, bounded by the SAME 256 MiB-derived ceiling (decoded RGBA8 pixel data
//     is capped by stb_image's own internal `(1<<30)` overflow guard regardless -- see this
//     header's own huge-dimensions test case). LoadTexture is a one-time asset load, not a
//     per-frame path (that discipline belongs to Draw2D/D2D-1B, out of scope here) -- this is not a
//     hot path, and the plan makes no performance claim for this wave.
// PT: D2D-1A -- o carve do decode de imagem (docs/superpowers/plans/2026-07-23-onda3-draw2d-d2d1.md,
//     decisão D3). Extrai o núcleo "bytes entram, pixels saem" que antes vivia fundido dentro de
//     Gl3RenderInterface::LoadTexture (render_gl3.cpp): o teto de 256 MiB do AUD-L1-PARSE sobre o
//     buffer de bytes CODIFICADOS, o decode forçado RGBA via stb_image, e o premultiply in-place.
//     Este header tem ZERO dependência de GL e ZERO de RmlUi -- é uma costura pura, testável
//     headless (mesma disciplina de src/log_dedup.hpp / src/input_state.hpp): chamadores que
//     precisam do FileInterface do RmlUi (render_gl3.cpp hoje) ou de upload GL (Draw2D, próxima
//     fatia) fazem isso do próprio lado e entregam a este helper um buffer de bytes simples.
//
//     Divisão de responsabilidade com o LoadTexture de render_gl3.cpp (IMPORTANTE, é a costura
//     que mantém "zero mudança de comportamento" verdadeiro): a guarda de TAMANHO de arquivo
//     pré-leitura (Seek/Tell ANTES de sequer alocar um buffer para o arquivo, para que um asset
//     hostil/enorme seja rejeitado sem tentar uma alocação grande mas limitada) permanece em
//     render_gl3.cpp -- ela precisa de Rml::FileInterface, do qual este header não pode depender.
//     O teto PRÓPRIO deste helper de 256 MiB (`kMaxImageDecodeBytes`, checado sobre o parâmetro
//     `len` recebido) é uma segunda linha de defesa independente: código morto inalcançável no
//     próprio ponto de chamada do LoadTexture hoje (a guarda do chamador já limita `len` antes
//     deste helper sequer rodar), mas a PRIMEIRA linha de defesa para qualquer OUTRO chamador que
//     lê o próprio arquivo e vai direto a este helper (o `load_texture` próprio do Draw2D, D2D-1B,
//     D7) -- o mesmo idioma cinto-e-suspensório "guarda 1/2 + guarda 2/2" que o LoadTexture
//     original já usava exatamente por este motivo (ver o próprio comentário daquela função sobre
//     por que a guarda 2/2 fica explícita mesmo já sendo subsumida pela guarda 1/2).
//
//     Uma falha de decode/teto aqui nunca loga (sem Rml::Log -- este header não pode alcançá-lo):
//     quem decide o que logar e como degradar é o chamador, igual a antes deste carve.
//     `DecodedImage::ok == false` é o ÚNICO sinal; o LoadTexture reconectado de render_gl3.cpp
//     (esta fatia) dobra todo resultado `ok == false` no caminho já existente de "fallback para
//     RenderInterface_GL3::LoadTexture", idêntico ao ramo pré-carve de falha de decode do stb.
//
//     Uma escolha de desenho deliberada e de custo limitado que este carve adiciona: o código
//     pré-carve premultiplicava o próprio buffer malloc'd do stb in place e entregava o ponteiro
//     cru direto ao GenerateTexture antes dele sair de escopo. Uma interface pura "bytes entram,
//     pixels saem" não pode devolver um ponteiro cru dono do stb sem vazar o tipo de deleter do
//     próprio stb_image.h para todo chamador, então este helper copia os pixels premultiplicados
//     para um std::vector próprio uma vez, no decode. Custo extra: uma cópia do tamanho de um
//     memcpy, limitada pelo MESMO teto derivado de 256 MiB (dados de pixel RGBA8 decodificados já
//     são limitados pela própria guarda interna `(1<<30)` de overflow do stb_image de qualquer
//     forma -- ver o caso de teste de dimensões-enormes deste próprio header). LoadTexture é um
//     carregamento de asset de uma vez só, não um caminho por-frame (essa disciplina é do
//     Draw2D/D2D-1B, fora de escopo aqui) -- isto não é caminho quente, e o plano não faz claim de
//     performance para esta onda.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "stb_image.h"

#include <climits>
#include <cstddef>
#include <memory>
#include <vector>

namespace glintfx {

// EN: AUD-L1-PARSE cap, re-derived here (not shared via a common header with render_gl3.cpp's own
//     private `kMaxAssetFileBytes`) -- see this file's top comment for why the two are
//     independent guards on purpose, and image_decode_sanity.cpp for the drift-detection test
//     that re-derives the same literal for the same reason asset_decode_hostile_sanity.cpp does.
//     `static_assert` below documents (and enforces at compile time) the invariant the original
//     LoadTexture's own "guard 2/2" comment relied on: this cap must stay under INT_MAX, because
//     `static_cast<int>(len)` is handed to stbi_load_from_memory's `int` length parameter.
// PT: Teto do AUD-L1-PARSE, re-derivado aqui (não compartilhado via header comum com o
//     `kMaxAssetFileBytes` privado do próprio render_gl3.cpp) -- ver o comentário do topo deste
//     arquivo pro porquê das duas guardas serem independentes de propósito, e
//     image_decode_sanity.cpp pro teste de detecção de desalinhamento que re-deriva o mesmo
//     literal pelo mesmo motivo de asset_decode_hostile_sanity.cpp. O `static_assert` abaixo
//     documenta (e força em tempo de compilação) o invariante de que o próprio comentário "guarda
//     2/2" do LoadTexture original dependia: este teto precisa ficar abaixo de INT_MAX, porque
//     `static_cast<int>(len)` é entregue ao parâmetro `int` de comprimento do
//     stbi_load_from_memory.
inline constexpr std::size_t kMaxImageDecodeBytes = 256u * 1024u * 1024u; // 256 MiB.
static_assert(kMaxImageDecodeBytes <= static_cast<std::size_t>(INT_MAX),
              "kMaxImageDecodeBytes must fit in the int length stbi_load_from_memory takes");

// EN: Result of a decode attempt. `ok == false` on ANY failure (oversized input, unknown/corrupt
//     format, zero-byte input) -- `width`/`height`/`rgba` are left at their default-constructed
//     values in that case (never partially filled). On success, `rgba` holds exactly
//     `width * height * 4` bytes, RGBA8, alpha-PREMULTIPLIED (R = R*A/255, G = G*A/255,
//     B = B*A/255, A unchanged) -- the space the GL3 backend's `GL_ONE, GL_ONE_MINUS_SRC_ALPHA`
//     blend assumes (see render_gl3.cpp's own top-of-class comment for the halo this avoids).
// PT: Resultado de uma tentativa de decode. `ok == false` em QUALQUER falha (input grande demais,
//     formato desconhecido/corrompido, input de zero bytes) -- `width`/`height`/`rgba` ficam nos
//     valores default-construídos nesse caso (nunca parcialmente preenchidos). Em sucesso, `rgba`
//     guarda exatamente `width * height * 4` bytes, RGBA8, alpha PREMULTIPLICADO (R = R*A/255,
//     G = G*A/255, B = B*A/255, A inalterado) -- o espaço que o blend `GL_ONE,
//     GL_ONE_MINUS_SRC_ALPHA` do backend GL3 assume (ver o próprio comentário de topo de classe de
//     render_gl3.cpp pro halo que isto evita).
struct DecodedImage {
  bool ok = false;
  int width = 0;
  int height = 0;
  std::vector<unsigned char> rgba;
};

// EN: Decodes `len` bytes of an encoded image (PNG/JPG -- any format stb_image recognises) into
//     alpha-premultiplied RGBA8 pixels. Pure function: no I/O, no GL, no RmlUi, no logging --
//     failure is communicated ONLY via `DecodedImage::ok`, callable and testable headless.
//     Guard order (checked BEFORE touching `data`, mirrors the pre-carve LoadTexture's own guard
//     ordering): `len == 0` or `data == nullptr` -> fail; `len > kMaxImageDecodeBytes` -> fail
//     (see this file's top comment on why this is a second, independent cap, not the primary
//     one); then stb_image decode (forced 4-channel RGBA regardless of the source format) ->
//     fail on null/zero-dimension result; then in-place premultiply into the returned buffer.
// PT: Decodifica `len` bytes de uma imagem codificada (PNG/JPG -- qualquer formato que o
//     stb_image reconheça) em pixels RGBA8 alpha-premultiplicados. Função pura: sem I/O, sem GL,
//     sem RmlUi, sem log -- falha é comunicada SÓ via `DecodedImage::ok`, chamável e testável
//     headless. Ordem das guardas (checadas ANTES de tocar `data`, espelha a própria ordem de
//     guarda do LoadTexture pré-carve): `len == 0` ou `data == nullptr` -> falha;
//     `len > kMaxImageDecodeBytes` -> falha (ver o comentário do topo deste arquivo pro porquê
//     disto ser um segundo teto independente, não o principal); depois decode via stb_image
//     (RGBA de 4 canais forçado independente do formato de origem) -> falha em resultado
//     nulo/dimensão zero; depois premultiply in-place no buffer retornado.
inline DecodedImage decode_premultiplied_rgba(const unsigned char* data, std::size_t len) {
  DecodedImage out;

  if (data == nullptr || len == 0)
    return out; // EN: ok stays false. PT: ok permanece false.

  if (len > kMaxImageDecodeBytes)
    return out; // EN: rejected before ever calling stb. PT: rejeitado antes de chamar o stb.

  // EN: RAII via unique_ptr: stbi_image_free runs on every exit path (including the early
  //     returns below), no manual free needed -- same idiom render_gl3.cpp's pre-carve
  //     LoadTexture used for the identical reason.
  // PT: RAII via unique_ptr: stbi_image_free roda em todo caminho de saída (inclusive os
  //     returns antecipados abaixo), sem free manual necessário -- mesmo idioma que o
  //     LoadTexture pré-carve de render_gl3.cpp usava pelo mesmo motivo.
  int w = 0, h = 0, n = 0;
  std::unique_ptr<unsigned char, decltype(&stbi_image_free)> px(
      stbi_load_from_memory(data, static_cast<int>(len), &w, &h, &n, 4),
      &stbi_image_free);
  if (!px || w <= 0 || h <= 0)
    return out; // EN: unknown format / decode error. PT: formato desconhecido / erro de decode.

  // EN: Premultiply alpha -- verbatim port of render_gl3.cpp's pre-carve loop, including the
  //     size_t pixel-count computation done BEFORE the loop so `w * h` never overflows signed
  //     `int` arithmetic for a hostile/malformed asset's large-but-still-under-cap dimensions.
  // PT: Premultiplica o alpha -- porte literal do loop pré-carve de render_gl3.cpp, inclusive o
  //     cálculo de contagem de pixel em size_t feito ANTES do loop para que `w * h` nunca dê
  //     overflow em aritmética `int` assinada para dimensões grandes-mas-ainda-sob-o-teto de um
  //     asset hostil/malformado.
  const unsigned char* p = px.get(); // read-only source: the premultiply writes into `out.rgba`, never back through `p`.
  const std::size_t pixel_count = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
  out.rgba.resize(pixel_count * 4u);
  for (std::size_t i = 0; i < pixel_count; ++i) {
    const unsigned int a = p[i * 4 + 3];
    out.rgba[i * 4 + 0] = static_cast<unsigned char>(p[i * 4 + 0] * a / 255u);
    out.rgba[i * 4 + 1] = static_cast<unsigned char>(p[i * 4 + 1] * a / 255u);
    out.rgba[i * 4 + 2] = static_cast<unsigned char>(p[i * 4 + 2] * a / 255u);
    out.rgba[i * 4 + 3] = static_cast<unsigned char>(a);
  }

  out.ok = true;
  out.width = w;
  out.height = h;
  return out;
}

// EN: D2D-3, decision D29 -- the pure bbox-of-non-transparent-texels computation. Called ONCE
//     inside `load_texture()` (draw2d.cpp, D2D-3B) on the SAME `DecodedImage::rgba` buffer
//     `decode_premultiplied_rgba()` above just produced, before that buffer is discarded after
//     GL upload -- so the smallest rect containing every texel with `alpha > 0` is computed
//     exactly once, on pixel data that already exists on the CPU, never retained as a standalone
//     buffer nor re-fetched via a `glGetTexImage` readback (both rejected alternatives, recorded
//     in the plan: the first is up-to-256-MiB-class memory for a rarely-used query, the second
//     is a GL round-trip and a GLES/core-profile portability trap). Coordinates are TEXEL space,
//     top-left origin, y-down -- the SAME space `src_px` already uses (matches image memory row
//     order, this file's own D5-class convention) -- so a caller's hitbox-from-bbox pattern
//     composes with `src_px` directly. Premultiply does NOT disturb this definition: the alpha
//     channel is UNCHANGED by the premultiply loop above (it multiplies R/G/B only, alpha passes
//     through verbatim) -- stated here so nobody has to re-derive it from that loop. Pure, total,
//     cannot log (same discipline as `decode_premultiplied_rgba` above): `rgba == nullptr`,
//     `w <= 0`, `h <= 0`, or every texel having `alpha == 0` (a fully-transparent image, distinct
//     from a 1x1 opaque image which returns `{true, 0, 0, 1, 1}`) all leave `ContentBbox::found
//     == false` with every other field at its default-constructed 0. A semi-transparent texel
//     (`alpha == 1`, the smallest nonzero byte value) COUNTS -- the `alpha > 0` definition, not
//     `alpha == 255`; an alpha THRESHOLD parameter is out of scope here (INBOX seed
//     `SEED-D2D-BBOX-THRESH`). This function returns its OWN small struct (`ContentBbox`, not
//     the public `glintfx::TextureBbox` declared in `<glintfx/draw2d.hpp>`) on purpose -- this
//     header has ZERO dependency on Draw2D's own public header (see this file's own top comment
//     on why: `render_gl3.cpp`'s ui-side `LoadTexture` also depends on this file and has nothing
//     to do with Draw2D); `draw2d.cpp` (D2D-3B) converts this to the public `TextureBbox` at its
//     own call site.
// PT: D2D-3, decisão D29 -- a computação pura de bbox-de-texels-não-transparentes. Chamada UMA
//     vez dentro de `load_texture()` (draw2d.cpp, D2D-3B) sobre o MESMO buffer
//     `DecodedImage::rgba` que o próprio `decode_premultiplied_rgba()` acima acabou de produzir,
//     antes desse buffer ser descartado após o upload GL -- então o menor retângulo contendo
//     todo texel com `alpha > 0` é computado exatamente uma vez, sobre dado de pixel que já
//     existe na CPU, nunca retido como buffer avulso nem re-buscado via readback
//     `glGetTexImage` (as duas alternativas rejeitadas, gravadas no plano: a primeira é memória
//     de até 256 MiB pra uma consulta pouco usada, a segunda é um round-trip de GL e uma
//     armadilha de portabilidade GLES/core-profile). Coordenadas em espaço de TEXEL, origem
//     superior-esquerda, y pra baixo -- o MESMO espaço que `src_px` já usa (bate com a ordem de
//     linha da memória de imagem, a mesma convenção classe-D5 deste arquivo) -- então o padrão
//     hitbox-a-partir-de-bbox de um chamador compõe direto com `src_px`. O premultiply NÃO
//     perturba esta definição: o canal alpha fica INALTERADO pelo loop de premultiply acima (ele
//     multiplica só R/G/B, o alpha passa direto) -- dito aqui pra ninguém precisar re-derivar
//     isso daquele loop. Pura, total, não pode logar (mesma disciplina do
//     `decode_premultiplied_rgba` acima): `rgba == nullptr`, `w <= 0`, `h <= 0`, ou todo texel
//     com `alpha == 0` (uma imagem totalmente transparente, distinta de uma imagem opaca 1x1 que
//     devolve `{true, 0, 0, 1, 1}`) todos deixam `ContentBbox::found == false` com todo outro
//     campo no próprio 0 default-construído. Um texel semi-transparente (`alpha == 1`, o menor
//     valor de byte não-zero) CONTA -- a definição é `alpha > 0`, não `alpha == 255`; um
//     parâmetro de LIMIAR de alpha fica fora de escopo aqui (semente INBOX `SEED-D2D-BBOX-
//     THRESH`). Esta função devolve a PRÓPRIA struct pequena (`ContentBbox`, não o
//     `glintfx::TextureBbox` público declarado em `<glintfx/draw2d.hpp>`) de propósito -- este
//     header tem ZERO dependência do header público próprio do Draw2D (ver o próprio comentário
//     de topo deste arquivo pro porquê: o `LoadTexture` do lado ui de `render_gl3.cpp` também
//     depende deste arquivo e não tem nada a ver com Draw2D); o `draw2d.cpp` (D2D-3B) converte
//     isto pro `TextureBbox` público no próprio sítio de chamada dele.
struct ContentBbox {
  bool found = false;
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
};

inline ContentBbox compute_content_bbox(const unsigned char* rgba, int w, int h) {
  ContentBbox out;
  if (rgba == nullptr || w <= 0 || h <= 0)
    return out; // EN: found stays false. PT: found permanece false.

  int min_x = w, min_y = h, max_x = -1, max_y = -1;
  for (int row = 0; row < h; ++row) {
    for (int col = 0; col < w; ++col) {
      const std::size_t idx = (static_cast<std::size_t>(row) * static_cast<std::size_t>(w) +
                               static_cast<std::size_t>(col)) *
                                  4u +
                              3u;
      if (rgba[idx] == 0)
        continue; // EN: alpha == 0 -- not part of the content bbox (D29's alpha>0 rule).
                  // PT: alpha == 0 -- fora do bbox de conteúdo (regra alpha>0 do D29).
      if (col < min_x) min_x = col;
      if (col > max_x) max_x = col;
      if (row < min_y) min_y = row;
      if (row > max_y) max_y = row;
    }
  }
  if (max_x < 0)
    return out; // EN: fully transparent -- found stays false. PT: totalmente transparente.

  out.found = true;
  out.x = min_x;
  out.y = min_y;
  out.w = max_x - min_x + 1;
  out.h = max_y - min_y + 1;
  return out;
}

} // namespace glintfx
