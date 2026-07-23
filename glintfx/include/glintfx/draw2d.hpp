// SPDX-License-Identifier: MPL-2.0
// EN: D2D-1B -- the Draw2D module's public surface (docs/superpowers/plans/2026-07-23-onda3-
//     draw2d-d2d1.md section 4, LITERAL contract; ADR-0017 records the architectural decision
//     this implements -- own GL 3.3 pipeline, own atom `GLINTFX_MODULE_DRAW2D`, order-by-call-
//     order composition). This is the first way this library's public API draws the GAME WORLD
//     (as opposed to `.rcss`-declared UI): texture load + `draw_sprite` + internal per-texture
//     batching. ZERO third-party type crosses this header (no GL, no GLFW, no RmlUi) -- same
//     "golden boundary" discipline as every other public header (AGENTS.md).
//
//     COORDINATE CONTRACT (D5): screen-space pixels, top-left origin, y-down -- the SAME space as
//     `UiEvent` mouse coordinates and `UiLayer::get_element_box` (one coordinate story for the
//     whole library). `src_px` is a sub-rectangle of the texture in TEXEL coordinates (top-left
//     origin, y-down, matching image memory row order); `{0,0,0,0}` is the sentinel for "whole
//     texture". Camera/world-space transforms are explicitly OUT of this slice (D2D-2, next wave).
//
//     GL-STATE CONTRACT (D9, the anti-`GlStateGuard`-bug decision -- see gl_state.hpp for the
//     class this decision answers): Draw2D SETS every piece of GL state it depends on at each
//     internal flush and assumes NOTHING left behind by a cohabiting renderer (the RmlUi GL3
//     backend shares this same GL context). It does NOT restore state afterwards -- in
//     `App::set_frame_callback` the hook's state is auto-restored before the UI pass runs
//     (app.hpp's own doc-comment); in embed mode `UiLayer::render()` re-establishes its own full
//     state inside `GlStateGuard` regardless of what ran before it. What Draw2D leaves behind is
//     the host's to manage, exactly like the host's own raw GL calls today. Full touched-state
//     list: `docs/draw2d.md`.
//
//     LIFETIME (D11): move-only, pImpl (same facade discipline as `App`/`UiLayer`). `init()`
//     requires a CURRENT GL 3.3 core context; `shutdown()`/the destructor release GL objects ONLY
//     if `init()` ever succeeded, and require that same context (or an equivalent one on the same
//     share-group) to still be current -- same honesty `docs/embed-integration.md` already
//     demands of `UiLayer`.
//
//     WHAT THIS SLICE DOES NOT SHIP (declared, not hidden -- plan section 1): camera/rotation/
//     scale/flip/`screen_to_world` (D2D-2); layers/explicit draw order/untextured primitives/
//     scissor/measured perf budget (D2D-3); a post-UI `App` frame hook (sprites over the UI in
//     standalone -- INBOX seed `SEED-D2D-POSTUI`); `set_asset_base_url` honoured by
//     `load_texture` (INBOX seed `SEED-D2D-BASEURL`); a nearest-neighbour/filter option per
//     texture (INBOX seed `SEED-D2D-TEXFILTER`, default is `GL_LINEAR`).
// PT: D2D-1B -- a superfície pública do módulo Draw2D (docs/superpowers/plans/2026-07-23-onda3-
//     draw2d-d2d1.md seção 4, contrato LITERAL; a ADR-0017 grava a decisão arquitetural que isto
//     implementa -- pipeline GL 3.3 próprio, átomo próprio `GLINTFX_MODULE_DRAW2D`, composição
//     por ordem-de-chamada). É o primeiro jeito da API pública desta biblioteca desenhar o MUNDO
//     DO JOGO (diferente da UI declarada em `.rcss`): carregar textura + `draw_sprite` + batching
//     interno por-textura. ZERO tipo de terceiro cruza este header (sem GL, sem GLFW, sem RmlUi)
//     -- mesma disciplina de "fronteira dourada" de todo outro header público (AGENTS.md).
//
//     CONTRATO DE COORDENADAS (D5): pixels em espaço de tela, origem superior-esquerda, y pra
//     baixo -- o MESMO espaço das coordenadas de mouse do `UiEvent` e de
//     `UiLayer::get_element_box` (uma história de coordenadas só pra biblioteca inteira).
//     `src_px` é uma sub-retângulo da textura em coordenadas de TEXEL (origem superior-esquerda,
//     y pra baixo, batendo com a ordem de linha da memória de imagem); `{0,0,0,0}` é a sentinela
//     de "textura inteira". Transformações de câmera/espaço-de-mundo estão explicitamente FORA
//     desta fatia (D2D-2, próxima onda).
//
//     CONTRATO DE ESTADO GL (D9, a decisão anti-bug-`GlStateGuard` -- ver gl_state.hpp pra classe
//     de bug que esta decisão responde): o Draw2D SETA todo pedaço de estado GL de que depende a
//     cada flush interno e NÃO assume nada deixado por um renderer coabitante (o backend GL3 do
//     RmlUi compartilha este mesmo contexto GL). NÃO restaura estado depois -- no
//     `App::set_frame_callback` o estado do hook é auto-restaurado antes do passe de UI rodar
//     (doc-comment próprio do app.hpp); no modo embed o `UiLayer::render()` reestabelece o próprio
//     estado completo dentro do `GlStateGuard` independente do que rodou antes. O que o Draw2D
//     deixa para trás é responsabilidade do host gerenciar, exatamente como o próprio GL cru dele
//     hoje. Lista completa de estado tocado: `docs/draw2d.md`.
//
//     CICLO DE VIDA (D11): move-only, pImpl (mesma disciplina de fachada de `App`/`UiLayer`).
//     `init()` exige um contexto GL 3.3 core CORRENTE; `shutdown()`/o destrutor liberam objetos GL
//     SÓ se `init()` algum dia teve sucesso, e exigem que esse mesmo contexto (ou um equivalente
//     no mesmo share-group) ainda esteja corrente -- mesma honestidade que
//     `docs/embed-integration.md` já exige do `UiLayer`.
//
//     O QUE ESTA FATIA NÃO ENTREGA (declarado, não escondido -- plano seção 1):
//     câmera/rotação/escala/flip/`screen_to_world` (D2D-2); layers/ordem explícita de
//     desenho/primitivas não-texturizadas/scissor/perf budget medido (D2D-3); hook pós-UI no
//     frame do `App` (sprites sobre a UI no standalone -- seed `SEED-D2D-POSTUI`);
//     `set_asset_base_url` honrado pelo `load_texture` (seed `SEED-D2D-BASEURL`); opção de
//     filtro nearest-neighbour por textura (seed `SEED-D2D-TEXFILTER`, default é `GL_LINEAR`).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <cstdint>
#include <memory>

namespace glintfx {

// EN: Destination/source rectangle, screen px or texel space depending on call site (see the
//     file header comment's coordinate contract, D5). Zero-initialized to a degenerate (0-size)
//     rect on purpose -- `RectF{}` as a `src_px` argument is the documented "whole texture"
//     sentinel (see `Draw2d::draw_sprite`), and `RectF{}` as a `dst` is a legal no-op draw (D10).
// PT: Retângulo de destino/origem, em px de tela ou em texel dependendo do sítio de chamada (ver
//     o contrato de coordenadas do comentário de cabeçalho deste arquivo, D5). Zero-inicializado
//     como retângulo degenerado (tamanho 0) de propósito -- `RectF{}` como argumento `src_px` é a
//     sentinela documentada de "textura inteira" (ver `Draw2d::draw_sprite`), e `RectF{}` como
//     `dst` é um desenho no-op legal (D10).
struct RectF {
  float x = 0.f;
  float y = 0.f;
  float w = 0.f;
  float h = 0.f;
};

// EN: Straight (non-premultiplied) RGBA tint in [0,1] each, clamped on intake (D8). Default is
//     opaque white -- identity: a sprite drawn with the default tint samples its texture
//     unmodified. See `Draw2d::draw_sprite`'s doc-comment for the exact premultiplied-space
//     modulation formula.
// PT: Tint RGBA straight (não-premultiplicado) em [0,1] cada, clampado na entrada (D8). Padrão é
//     branco opaco -- identidade: um sprite desenhado com o tint padrão amostra a própria textura
//     sem modificação. Ver o doc-comment de `Draw2d::draw_sprite` pra fórmula exata de modulação
//     em espaço premultiplicado.
struct ColorF {
  float r = 1.f;
  float g = 1.f;
  float b = 1.f;
  float a = 1.f;
};

// EN: Value handle to a texture owned by a `Draw2d` instance (D7). Copyable, trivially small --
//     NOT an RAII owner (`Draw2d::destroy_texture` is the explicit release, same idiom as
//     `Audio::SoundId`). The three private fields are an opaque id + generation + owner tag,
//     validated against the owning `Draw2d`'s internal registry on every use -- an
//     unknown/stale/tampered handle is fail-high (logged once, dedup'd, no-op), NEVER
//     dereferenced as a raw GL name. The owner tag (the issuing `Draw2d`'s own identity, opaque)
//     is what makes rejection of a handle from a DIFFERENT `Draw2d` instance a GUARANTEE rather
//     than a numeric coincidence: id+generation alone cannot distinguish "id 3, generation 1 in
//     THIS instance's registry" from "id 3, generation 1 in a DIFFERENT instance's registry" if
//     both happen to be at the same point in their own independent id/generation sequences.
//     Default-constructed as `ok() == false`.
// PT: Handle-valor para uma textura possuída por uma instância `Draw2d` (D7). Copiável,
//     trivialmente pequeno -- NÃO é dono RAII (`Draw2d::destroy_texture` é a liberação explícita,
//     mesmo idioma do `Audio::SoundId`). Os três campos privados são um id opaco + geração + tag
//     de dono, validados contra o registry interno do `Draw2d` dono a cada uso -- um handle
//     desconhecido/obsoleto/adulterado é fail-high (logado uma vez, dedup'd, no-op), NUNCA
//     desreferenciado como nome GL cru. A tag de dono (a identidade própria do `Draw2d` emissor,
//     opaca) é o que torna a rejeição de um handle de uma instância `Draw2d` DIFERENTE uma
//     GARANTIA em vez de uma coincidência numérica: id+geração sozinhos não conseguem distinguir
//     "id 3, geração 1 no registry DESTA instância" de "id 3, geração 1 no registry de uma
//     instância DIFERENTE" se as duas por acaso estiverem no mesmo ponto das próprias sequências
//     independentes de id/geração. Default-construído como `ok() == false`.
struct Texture2d {
  Texture2d() = default;

  // EN: false when never loaded, when load_texture() failed (hostile/corrupt/missing file), or
  //     after destroy_texture() zeroed this handle.
  // PT: false quando nunca carregado, quando load_texture() falhou (arquivo hostil/corrompido/
  //     ausente), ou após destroy_texture() zerar este handle.
  bool ok() const { return ok_; }
  // EN: Pixel dimensions of the decoded image. 0 when !ok().
  // PT: Dimensões em pixel da imagem decodificada. 0 quando !ok().
  int width() const { return width_; }
  int height() const { return height_; }

private:
  friend class Draw2d;
  bool ok_ = false;
  int width_ = 0;
  int height_ = 0;
  // EN: Opaque, documented do-not-touch -- id_==0 is the permanent invalid sentinel (same
  //     convention as Audio::SoundId); id_>0 indexes Draw2d's internal texture registry,
  //     generation_ detects a slot that was destroyed and reused since this handle was issued.
  // PT: Opaco, documentado como não-tocar -- id_==0 é a sentinela permanente de inválido (mesma
  //     convenção do Audio::SoundId); id_>0 indexa o registry interno de textura do Draw2d,
  //     generation_ detecta um slot que foi destruído e reusado desde que este handle foi emitido.
  std::uint32_t id_ = 0;
  std::uint32_t generation_ = 0;
  // EN: The issuing Draw2d::Impl's own address, type-erased -- distinct per Draw2d instance for
  //     the lifetime of the process (barring the classic ABA of a freed Impl's heap slot being
  //     reused by a LATER Draw2d while an old handle from the freed one is still floating around,
  //     an edge case not solved here, same acceptable residual risk generation_ alone already
  //     carries for slot reuse WITHIN one instance). nullptr for a never-loaded handle.
  // PT: O próprio endereço do Draw2d::Impl emissor, type-erased -- distinto por instância Draw2d
  //     durante a vida do processo (exceto o ABA clássico de um slot de heap de um Impl liberado
  //     ser reusado por um Draw2d POSTERIOR enquanto um handle antigo do liberado ainda circula,
  //     um caso de borda não resolvido aqui, o mesmo risco residual aceitável que generation_
  //     sozinho já carrega para reuso de slot DENTRO de uma instância). nullptr para um handle
  //     nunca carregado.
  const void* owner_ = nullptr;
};

// EN: Sprite-batching facade over an own GL 3.3 core pipeline (ADR-0017 axis 2, decision (2a)).
//     See this file's header comment for the coordinate/GL-state/lifetime contracts this class
//     implements; see `docs/draw2d.md` for the full public writeup (recipes for App and embed).
//
//     MOVED-FROM STATE: every method below is a safe no-op on a moved-from `Draw2d` (behaves
//     exactly like a never-initialized instance -- `ok()` returns false) -- a deliberately
//     STRICTER safety contract than `App`'s own "UB on anything but ok()/the destructor after a
//     move" (app.hpp), chosen because it costs nothing here and matches this module's own
//     fail-high discipline.
// PT: Fachada de batching de sprites sobre um pipeline GL 3.3 core próprio (ADR-0017 eixo 2,
//     decisão (2a)). Ver o comentário de cabeçalho deste arquivo pros contratos de
//     coordenadas/estado-GL/ciclo-de-vida que esta classe implementa; ver `docs/draw2d.md` pro
//     relato público completo (receitas para App e embed).
//
//     ESTADO MOVIDO-DE: todo método abaixo é um no-op seguro num `Draw2d` movido-de (se comporta
//     exatamente como uma instância nunca-inicializada -- `ok()` retorna false) -- um contrato
//     de segurança deliberadamente MAIS ESTRITO que o próprio "UB em qualquer coisa além de
//     ok()/o destrutor após um move" do `App` (app.hpp), escolhido porque não custa nada aqui e
//     bate com a própria disciplina fail-high deste módulo.
class Draw2d {
public:
  // EN: Does NOT touch GL -- construction is always cheap and side-effect-free, same discipline
  //     as Audio()/App()/UiLayer(). The real GL resource acquisition happens in init().
  // PT: NÃO toca em GL -- a construção é sempre barata e sem efeito colateral, mesma disciplina
  //     de Audio()/App()/UiLayer(). A aquisição real de recurso GL acontece em init().
  Draw2d();
  // EN: Calls shutdown() (idempotent, safe even if init() was never called or already failed/
  //     shut down) -- D11.
  // PT: Chama shutdown() (idempotente, seguro mesmo se init() nunca foi chamado ou já
  //     falhou/desligou) -- D11.
  ~Draw2d();
  Draw2d(Draw2d&&) noexcept;
  Draw2d& operator=(Draw2d&&) noexcept;
  Draw2d(const Draw2d&) = delete;
  Draw2d& operator=(const Draw2d&) = delete;

  // EN: Requires a CURRENT GL 3.3 core context (loads glintfx's own internal `glx_`-prefixed
  //     function table itself, D2 -- idempotent-safe to call even when the host engine already
  //     loaded it for a cohabiting UiLayer/App). Compiles the one textured-quad shader program
  //     this module ever uses. Returns false on any GL/shader failure, OR when this instance is
  //     already initialized (call shutdown() first to re-init, same contract as Audio::init()).
  // PT: Exige um contexto GL 3.3 core CORRENTE (carrega a própria tabela de função interna
  //     prefixada `glx_` da glintfx, D2 -- seguro chamar de forma idempotente mesmo quando o
  //     engine host já a carregou para um UiLayer/App coabitante). Compila o único programa de
  //     shader de quad texturizado que este módulo usa. Retorna false em qualquer falha de
  //     GL/shader, OU quando esta instância já está inicializada (chame shutdown() primeiro para
  //     re-inicializar, mesmo contrato de Audio::init()).
  bool init();
  // EN: true between a successful init() and the next shutdown()/destruction.
  // PT: true entre um init() bem-sucedido e o próximo shutdown()/destruição.
  bool ok() const;
  // EN: Idempotent GL teardown -- releases every live texture, the VBO/VAO, and the shader
  //     program, ONLY if init() ever succeeded (D11). Safe to call any number of times, including
  //     on a never-initialized instance (no-op). Requires the context that created these GL
  //     objects (or an equivalent one on the same share-group) to still be current.
  // PT: Teardown GL idempotente -- libera toda textura viva, o VBO/VAO, e o programa de shader,
  //     SÓ SE init() alguma vez teve sucesso (D11). Seguro chamar quantas vezes for, inclusive
  //     numa instância nunca inicializada (no-op). Exige que o contexto que criou estes objetos GL
  //     (ou um equivalente no mesmo share-group) ainda esteja corrente.
  void shutdown();

  // EN: Loads PNG/JPG/TGA/BMP (any format `image_decode.hpp`'s stb_image-backed decode
  //     recognises, D2D-1A's shared seam -- unlike `render_gl3.cpp`'s ui-side LoadTexture, Draw2D
  //     has no secondary RmlUi-base-class TGA loader to fall back to: a decode failure here is
  //     always the final answer), premultiplied on upload (D7) to match this module's
  //     `GL_ONE, GL_ONE_MINUS_SRC_ALPHA` blend. `path` is read directly by this call (a plain
  //     `const char*` ifstream overload --
  //     D7 deliberately avoids `std::filesystem::path::c_str()`'s wchar_t trap on MSVC), capped
  //     at 256 MiB, independent of `set_asset_base_url` (SEED-D2D-BASEURL, out of this slice).
  //     Returns a `Texture2d` with `ok() == false` (never a crash, never partially valid) on:
  //     `path == nullptr`, file-open failure, the 256 MiB cap, or a decode failure (unknown/
  //     corrupt format) -- same fail-high discipline as every other loader in this library
  //     (`Audio::load_sound`, `decorator_polygon.cpp`'s hostile-input handling). Also returns an
  //     invalid handle when called before init() or after shutdown().
  // PT: Carrega PNG/JPG/TGA/BMP (qualquer formato que o decode apoiado em stb_image do
  //     image_decode.hpp reconheça, a costura compartilhada do D2D-1A -- diferente do
  //     LoadTexture do lado ui em render_gl3.cpp, o Draw2D não tem um segundo loader TGA da
  //     classe-base do RmlUi pra cair de volta: uma falha de decode aqui é sempre a resposta
  //     final), premultiplicado no upload (D7) para bater com o blend
  //     `GL_ONE, GL_ONE_MINUS_SRC_ALPHA` deste módulo. `path` é lido diretamente por esta chamada
  //     (um overload de ifstream de `const char*` puro -- D7 evita deliberadamente a armadilha
  //     wchar_t do `std::filesystem::path::c_str()` no MSVC), com teto de 256 MiB, independente
  //     de `set_asset_base_url` (seed SEED-D2D-BASEURL, fora desta fatia). Retorna um `Texture2d`
  //     com `ok() == false` (nunca um crash, nunca parcialmente válido) em: `path == nullptr`,
  //     falha ao abrir o arquivo, o teto de 256 MiB, ou falha de decode (formato
  //     desconhecido/corrompido) -- mesma disciplina fail-high de todo outro loader desta
  //     biblioteca (`Audio::load_sound`, o tratamento de input hostil de
  //     `decorator_polygon.cpp`). Também retorna um handle inválido se chamado antes de init() ou
  //     depois de shutdown().
  Texture2d load_texture(const char* path);
  // EN: Releases the GL texture `tex` refers to (if any) and ALWAYS zeroes `tex` on return (a
  //     handle you tried to destroy never looks valid afterwards, D7) -- except when `tex` was
  //     already the invalid sentinel (`id() == 0`, effectively already zero, a true no-op).
  //     Every failure mode is fail-high (logged once, dedup'd), never a double-free, never UB:
  //     already-destroyed, tampered (a stale generation), or FOREIGN (issued by a different
  //     `Draw2d` instance -- that instance's own registry entry is left completely untouched, see
  //     the owner-tag comment on `Texture2d` above). No-op (still zeroes `tex`) when called
  //     before init() or after shutdown().
  // PT: Libera a textura GL que `tex` referencia (se alguma) e SEMPRE zera `tex` ao retornar (um
  //     handle que você tentou destruir nunca parece válido depois, D7) -- exceto quando `tex` já
  //     era a sentinela inválida (`id() == 0`, efetivamente já zero, um no-op de verdade). Todo
  //     modo de falha é fail-high (logado uma vez, dedup'd), nunca um double-free, nunca UB:
  //     já-destruído, adulterado (uma geração obsoleta), ou ESTRANGEIRO (emitido por uma
  //     instância `Draw2d` DIFERENTE -- o próprio registry daquela instância fica completamente
  //     intocado, ver o comentário da tag de dono em `Texture2d` acima). No-op (ainda zera `tex`)
  //     se chamado antes de init() ou depois de shutdown().
  void destroy_texture(Texture2d& tex);

  // EN: Opens a batching bracket for one frame/pass (D4). `target_width`/`target_height` are the
  //     viewport size in screen px this bracket's sprites project against (D5) -- the SAME size
  //     the host's own GL viewport is set to for this pass. `target_width <= 0 || target_height
  //     <= 0` is fail-high: the bracket is accepted (still opened, still must be closed by end())
  //     but every draw_sprite() call inside it is silently ignored until then (D10, logged once
  //     at begin(), not spammed per draw_sprite()). Calling begin() while a previous bracket is
  //     still open implicitly ends the previous one first (flushing whatever it had accumulated,
  //     D4) and logs once, dedup'd -- never silently drops pending draws.
  // PT: Abre um bracket de batching para um frame/passe (D4). `target_width`/`target_height` são
  //     o tamanho de viewport em px de tela contra o qual os sprites deste bracket projetam (D5)
  //     -- o MESMO tamanho para o qual o próprio viewport GL do host está definido neste passe.
  //     `target_width <= 0 || target_height <= 0` é fail-high: o bracket é aceito (ainda aberto,
  //     ainda precisa ser fechado por end()) mas toda chamada draw_sprite() dentro dele é
  //     ignorada em silêncio até lá (D10, logado uma vez no begin(), não em spam por
  //     draw_sprite()). Chamar begin() enquanto um bracket anterior ainda está aberto encerra o
  //     anterior implicitamente primeiro (fazendo flush do que ele tinha acumulado, D4) e loga
  //     uma vez, dedup'd -- nunca descarta desenhos pendentes em silêncio.
  void begin(int target_width, int target_height);

  // EN: Queues one textured quad. Internally batched by texture -- accumulates while `tex` stays
  //     the same as the previous call, flushes to GL on a texture change, on end(), or on
  //     reaching an internal 4096-quad capacity (a MEMORY bound against a hostile/degenerate call
  //     stream, transparent to the caller -- not a performance claim, D2D-3 measures that later).
  //     `src_px` defaults to `RectF{}`, the documented "whole texture" sentinel; any other value
  //     is texel coordinates (D5) CLAMPED to the texture's own bounds if it extends past them
  //     (a half-off-atlas frame under animation-math jitter is a common legitimate case, not an
  //     error). `tint` defaults to opaque white (identity); the exact premultiplied-space
  //     modulation is `out = texel_premult * vec4(tint.rgb * tint.a, tint.a)` (D8), each `tint`
  //     channel clamped to [0,1] on intake. Fail-high, every guard checked BEFORE any cast/
  //     arithmetic (D10): a NaN/Inf member anywhere in `dst`/`src_px`/`tint` skips the draw
  //     (logged once, dedup'd); `dst.w <= 0 || dst.h <= 0` is a SILENT no-op (a degenerate sprite,
  //     not an error -- not logged); a negative `dst.x`/`dst.y` is LEGAL (off-screen clipping is
  //     the GPU's job); an unknown/stale/tampered/foreign `tex` handle is rejected (logged once,
  //     dedup'd); calling this outside a begin()/end() bracket is rejected (logged once, dedup'd).
  // PT: Enfileira um quad texturizado. Batchado internamente por textura -- acumula enquanto
  //     `tex` permanece igual à chamada anterior, faz flush pra GL numa troca de textura, no
  //     end(), ou ao atingir uma capacidade interna de 4096 quads (um limite de MEMÓRIA contra um
  //     stream hostil/degenerado de chamadas, transparente ao chamador -- não é um claim de
  //     performance, o D2D-3 mede isso depois). `src_px` tem padrão `RectF{}`, a sentinela
  //     documentada de "textura inteira"; qualquer outro valor é coordenada de texel (D5)
  //     CLAMPADA aos próprios limites da textura se ultrapassar (um frame meio-fora-do-atlas sob
  //     jitter de matemática de animação é um caso legítimo comum, não um erro). `tint` tem
  //     padrão branco opaco (identidade); a modulação exata em espaço premultiplicado é
  //     `out = texel_premult * vec4(tint.rgb * tint.a, tint.a)` (D8), cada canal de `tint`
  //     clampado em [0,1] na entrada. Fail-high, toda guarda checada ANTES de qualquer
  //     cast/aritmética (D10): um membro NaN/Inf em qualquer lugar de `dst`/`src_px`/`tint` pula o
  //     desenho (logado uma vez, dedup'd); `dst.w <= 0 || dst.h <= 0` é um no-op SILENCIOSO (um
  //     sprite degenerado, não um erro -- não logado); um `dst.x`/`dst.y` negativo é LEGAL
  //     (recorte fora-da-tela é trabalho da GPU); um handle `tex` desconhecido/obsoleto/
  //     adulterado/estrangeiro é rejeitado (logado uma vez, dedup'd); chamar isto fora de um
  //     bracket begin()/end() é rejeitado (logado uma vez, dedup'd).
  void draw_sprite(const Texture2d& tex, const RectF& dst, const RectF& src_px = RectF{},
                   const ColorF& tint = ColorF{});

  // EN: Closes the batching bracket opened by begin(), flushing whatever this bracket still had
  //     pending (D4) -- unconditionally, even an empty/degenerate bracket (a no-op flush in that
  //     case). Safe to call without a matching begin() (no-op).
  // PT: Fecha o bracket de batching aberto por begin(), fazendo flush do que este bracket ainda
  //     tinha pendente (D4) -- incondicionalmente, mesmo um bracket vazio/degenerado (um flush
  //     no-op nesse caso). Seguro chamar sem um begin() correspondente (no-op).
  void end();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace glintfx
