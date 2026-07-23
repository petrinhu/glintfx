// SPDX-License-Identifier: MPL-2.0
// EN: D2D-1B -- the Draw2D module's public surface (docs/superpowers/plans/2026-07-23-onda3-
//     draw2d-d2d1.md section 4, LITERAL contract; ADR-0017 records the architectural decision
//     this implements -- own GL 3.3 pipeline, own atom `GLINTFX_MODULE_DRAW2D`, order-by-call-
//     order composition). This is the first way this library's public API draws the GAME WORLD
//     (as opposed to `.rcss`-declared UI): texture load + `draw_sprite` + internal per-texture
//     batching. ZERO third-party type crosses this header (no GL, no GLFW, no RmlUi) -- same
//     "golden boundary" discipline as every other public header (AGENTS.md).
//
//     COORDINATE CONTRACT (D5, REFRAMED by D2D-2 / ADR-0017's Addendum, Q1 (c)): screen-space
//     pixels, top-left origin, y-down -- the SAME space as `UiEvent` mouse coordinates and
//     `UiLayer::get_element_box` (one coordinate story for the whole library) -- is now the
//     documented NO-CAMERA case: with no camera set, `draw_sprite` runs this literal v0.20.0
//     contract, unchanged (an absence of diff, not a re-derivation of it). `src_px` is a
//     sub-rectangle of the texture in TEXEL coordinates (top-left origin, y-down, matching image
//     memory row order); `{0,0,0,0}` is the sentinel for "whole texture". `set_camera(Camera2d)`
//     switches subsequent draws to WORLD space (see `Camera2d`/`SpriteTransform`/
//     `world_to_screen`/`screen_to_world`/`camera_from_world_rect` below); `reset_camera()`
//     returns to screen space. The camera and per-sprite transforms are CPU-side, pure,
//     pre-batcher (D13) -- the GL layer and its published touched-state list (below) are
//     untouched by either, and switching camera mid-bracket never forces a flush.
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
//     CONTRATO DE COORDENADAS (D5, REENQUADRADO pelo D2D-2 / Adendo da ADR-0017, Q1 (c)): pixels
//     em espaço de tela, origem superior-esquerda, y pra baixo -- o MESMO espaço das coordenadas
//     de mouse do `UiEvent` e de `UiLayer::get_element_box` (uma história de coordenadas só pra
//     biblioteca inteira) -- agora é o caso documentado SEM CÂMERA: sem câmera setada,
//     `draw_sprite` roda este contrato literal da v0.20.0, inalterado (uma ausência de diff, não
//     uma re-derivação dele). `src_px` é uma sub-retângulo da textura em coordenadas de TEXEL
//     (origem superior-esquerda, y pra baixo, batendo com a ordem de linha da memória de imagem);
//     `{0,0,0,0}` é a sentinela de "textura inteira". `set_camera(Camera2d)` passa os desenhos
//     seguintes pro espaço de MUNDO (ver `Camera2d`/`SpriteTransform`/`world_to_screen`/
//     `screen_to_world`/`camera_from_world_rect` abaixo); `reset_camera()` volta pro espaço de
//     tela. A câmera e os transforms por-sprite são CPU-side, puros, pré-batcher (D13) -- a
//     camada GL e sua lista publicada de estado tocado (abaixo) ficam intocadas por qualquer um
//     dos dois, e trocar de câmera no meio do bracket nunca força um flush.
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

// EN: 2D point value type -- world OR screen space depending on call site (see the file header
//     comment's coordinate contract, D5). Zero third-party type crosses this boundary (no GLM,
//     no other vector-math library) -- same "golden boundary" discipline as every other public
//     type here. Aggregate, trivially copyable.
// PT: Tipo-valor de ponto 2D -- espaço de mundo OU de tela dependendo do sítio de chamada (ver o
//     contrato de coordenadas do comentário de cabeçalho deste arquivo, D5). Zero tipo de
//     terceiro cruza esta fronteira (sem GLM, sem outra lib de matemática vetorial) -- mesma
//     disciplina de "fronteira dourada" de todo outro tipo público daqui. Agregado, trivialmente
//     copiável.
struct Vec2F {
  float x = 0.f;
  float y = 0.f;
};

// EN: World camera, CENTER anchor (ADR-0017 Addendum, Q2 (2a)): `(x, y)` is the world point
//     shown at the viewport's CENTER -- "point the camera at the player" is
//     `cam.x = player.x; cam.y = player.y`, the whole follow-cam line, and zoom/rotation pivot
//     about the view center (what every consumer visually expects). Default-constructed
//     `Camera2d{}` is NOT the identity mapping (it centers the world origin on screen) --
//     harmless because the DEFAULT STATE of a `Draw2d` is "no camera" (Q1 (c)), never "camera
//     set to identity"; see `Draw2d::set_camera` below. `zoom` is screen px per world unit
//     (`zoom 2` = magnified 2x; must be `> 0`, D15 -- `zoom == 0` would collapse the world and
//     make `screen_to_world` divide by zero, and negative zoom is NOT a mirror idiom, `flip_h`/
//     `flip_v` on `SpriteTransform` exist for that). `rotation` is the camera's OWN orientation
//     in RADIANS (D14) -- a positive value turns the camera clockwise, so the world appears to
//     rotate counter-clockwise on screen; any finite value is legal, wrapping naturally through
//     `sin`/`cos` past `2*pi`. See `world_to_screen`/`screen_to_world` below for the exact
//     projection formulas (D12).
// PT: Câmera de mundo, âncora CENTRO (Adendo da ADR-0017, Q2 (2a)): `(x, y)` é o ponto do mundo
//     mostrado no CENTRO do viewport -- "apontar a câmera pro jogador" é
//     `cam.x = player.x; cam.y = player.y`, a linha inteira do follow-cam, e zoom/rotação
//     pivotam sobre o centro da vista (o que todo consumidor espera visualmente). Um
//     `Camera2d{}` default-construído NÃO é a transformação identidade (centra a origem do mundo
//     na tela) -- inofensivo porque o ESTADO default de um `Draw2d` é "sem câmera" (Q1 (c)),
//     nunca "câmera setada pra identidade"; ver `Draw2d::set_camera` abaixo. `zoom` é px de tela
//     por unidade de mundo (`zoom 2` = ampliado 2x; precisa ser `> 0`, D15 -- `zoom == 0`
//     colapsaria o mundo e faria `screen_to_world` dividir por zero, e zoom negativo NÃO é um
//     idioma de espelho, `flip_h`/`flip_v` em `SpriteTransform` existem pra isso). `rotation` é
//     a orientação da PRÓPRIA câmera em RADIANOS (D14) -- um valor positivo gira a câmera no
//     sentido horário, então o mundo parece girar anti-horário na tela; qualquer valor finito é
//     legal, dando wrap naturalmente por `sin`/`cos` além de `2*pi`. Ver `world_to_screen`/
//     `screen_to_world` abaixo pras fórmulas exatas de projeção (D12).
struct Camera2d {
  float x = 0.f;
  float y = 0.f;
  float zoom = 1.f;
  float rotation = 0.f;
};

// EN: Per-sprite transform (pivot, rotation, scale, flip), orthogonal to the camera -- applies
//     in BOTH spaces (world units when a camera is set, screen px otherwise, D18).
//     `origin_x`/`origin_y` are the pivot, NORMALIZED within `dst` (default `0.5/0.5` = center);
//     `rotation` is radians about the pivot (D14, independent of any camera rotation);
//     `scale_x`/`scale_y` are about the pivot (default 1; negative is LEGAL -- it mirrors the
//     geometry, winding is irrelevant because D9 already disables backface cull); `flip_h`/
//     `flip_v` swap UV on the RESOLVED `src_px` sub-rect (so an atlas frame flips within itself,
//     never bleeding a neighbouring frame) -- implemented by `Draw2d::draw_sprite`'s transform
//     overload below (D2D-2B), not by this value type. Corner math (scale about the pivot, THEN
//     rotate about the pivot, literal order) lives in the internal sibling header of
//     `world_to_screen` (`src/transform2d.hpp`, single source); see `docs/draw2d.md` for the
//     full formula.
// PT: Transform por-sprite (pivô, rotação, escala, flip), ortogonal à câmera -- vale nos DOIS
//     espaços (unidades de mundo com câmera setada, px de tela caso contrário, D18).
//     `origin_x`/`origin_y` são o pivô, NORMALIZADO dentro de `dst` (padrão `0.5/0.5` = centro);
//     `rotation` é radianos sobre o pivô (D14, independente de qualquer rotação de câmera);
//     `scale_x`/`scale_y` são sobre o pivô (padrão 1; negativo é LEGAL -- espelha a geometria, o
//     giro é irrelevante porque o D9 já desliga o cull de backface); `flip_h`/`flip_v` trocam UV
//     no sub-retângulo `src_px` RESOLVIDO (então um frame de atlas espelha dentro de si mesmo,
//     nunca vaza pra um frame vizinho) -- implementado pelo overload de transform do
//     `Draw2d::draw_sprite` abaixo (D2D-2B), não por este tipo-valor. A matemática de canto
//     (escala sobre o pivô, DEPOIS rotação sobre o pivô, ordem literal) mora no header interno
//     irmão de `world_to_screen` (`src/transform2d.hpp`, fonte única); ver `docs/draw2d.md` pra
//     fórmula completa.
struct SpriteTransform {
  float origin_x = 0.5f;
  float origin_y = 0.5f;
  float rotation = 0.f;
  float scale_x = 1.f;
  float scale_y = 1.f;
  bool flip_h = false;
  bool flip_v = false;
};

// EN: Pure, TOTAL helpers (D12/D16) -- defined out-of-line in draw2d.cpp (D21: kept out of this
//     header on purpose, so `nm` still proves their removal when GLINTFX_MODULE_DRAW2D=OFF),
//     delegating to the SAME internal formulas `Draw2d::draw_sprite`'s camera path uses
//     (`src/transform2d.hpp`, single source -- the draw path and these helpers cannot diverge).
//     D12, literal: `world_to_screen(cam, W, H, p) = rot((p - (cam.x,cam.y)) * cam.zoom,
//     -cam.rotation) + (W/2, H/2)`; `screen_to_world` is its exact inverse. D16 totality:
//     invalid input (any non-finite member of `cam` or the point, `cam.zoom <= 0`, or a
//     non-positive viewport) returns the INPUT POINT UNCHANGED -- deterministic, documented,
//     never NaN out; these functions cannot log (they are pure). `camera_from_world_rect`
//     (D17, the one cuttable convenience) returns the camera that fits `world_rect` by WIDTH
//     (`zoom = viewport_w / world_rect.w`, center = rect center); vertical coverage follows the
//     viewport's own aspect (compose with `UiLayer::set_viewport` for a letterbox-exact fit,
//     which already exists). Fail-high: non-finite `world_rect`, `world_rect.w <= 0 ||
//     world_rect.h <= 0`, or a non-positive viewport return `Camera2d{}`.
// PT: Helpers puros e TOTAIS (D12/D16) -- definidos fora de linha em draw2d.cpp (D21: mantidos
//     fora deste header de propósito, pra que o `nm` continue provando a remoção deles quando
//     GLINTFX_MODULE_DRAW2D=OFF), delegando pras MESMAS fórmulas internas que o caminho de
//     câmera do `Draw2d::draw_sprite` usa (`src/transform2d.hpp`, fonte única -- o caminho de
//     desenho e estes helpers não podem divergir). D12, literal: `world_to_screen(cam, W, H, p)
//     = rot((p - (cam.x,cam.y)) * cam.zoom, -cam.rotation) + (W/2, H/2)`; `screen_to_world` é o
//     inverso exato. Totalidade D16: input inválido (qualquer membro não-finito de `cam` ou do
//     ponto, `cam.zoom <= 0`, ou viewport não-positivo) devolve o PONTO DE ENTRADA INALTERADO --
//     determinístico, documentado, nunca NaN pra fora; estas funções não podem logar (são
//     puras). `camera_from_world_rect` (D17, a única conveniência cortável) devolve a câmera que
//     ajusta `world_rect` por LARGURA (`zoom = viewport_w / world_rect.w`, centro = centro do
//     retângulo); a cobertura vertical segue o próprio aspecto do viewport (compor com
//     `UiLayer::set_viewport` pra fit letterbox-exato, que já existe). Fail-high: `world_rect`
//     não-finito, `world_rect.w <= 0 || world_rect.h <= 0`, ou viewport não-positivo devolvem
//     `Camera2d{}`.
Vec2F world_to_screen(const Camera2d& cam, int viewport_w, int viewport_h, Vec2F world);
Vec2F screen_to_world(const Camera2d& cam, int viewport_w, int viewport_h, Vec2F screen);
Camera2d camera_from_world_rect(const RectF& world_rect, int viewport_w, int viewport_h);

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

  // EN: Q1 (c), ADR-0017 Addendum: switches subsequent draw_sprite() calls in ANY open (or
  //     future) bracket to WORLD space, projected through `camera` via `world_to_screen` (D12)
  //     -- CPU-side, pre-batcher (D13), so switching mid-bracket NEVER forces a flush
  //     (same-texture batching survives it, D4 untouched). Fail-high (D15, the set_dp_ratio
  //     idiom): any non-finite `camera` member, or `camera.zoom <= 0`, rejects the call -- the
  //     PREVIOUS state (including "no camera") is KEPT, one dedup'd log line. Safe no-op on a
  //     never-init/moved-from/post-shutdown `Draw2d` (every new public method shares this
  //     module's null-safe contract, D15).
  // PT: Q1 (c), Adendo da ADR-0017: passa as chamadas draw_sprite() seguintes em QUALQUER
  //     bracket aberto (ou futuro) pra espaço de MUNDO, projetado por `camera` via
  //     `world_to_screen` (D12) -- CPU-side, pré-batcher (D13), então trocar no meio do bracket
  //     NUNCA força um flush (o batching por-mesma-textura sobrevive, D4 intocado). Fail-high
  //     (D15, o idioma do set_dp_ratio): qualquer membro não-finito de `camera`, ou
  //     `camera.zoom <= 0`, rejeita a chamada -- o estado ANTERIOR (inclusive "sem câmera") é
  //     MANTIDO, uma linha de log dedup'd. No-op seguro num `Draw2d`
  //     nunca-inicializado/movido-de/pós-shutdown (todo método público novo compartilha o
  //     contrato null-safe deste módulo, D15).
  void set_camera(const Camera2d& camera);

  // EN: Q1 (c): returns subsequent draw_sprite() calls to screen space (the no-camera,
  //     v0.20.0-literal path, D5's reframed default). Idempotent -- safe to call with no camera
  //     set. `shutdown()` already does this implicitly (D22); safe no-op on a
  //     never-init/moved-from/post-shutdown `Draw2d`.
  // PT: Q1 (c): devolve as chamadas draw_sprite() seguintes pro espaço de tela (o caminho
  //     sem-câmera, literal-v0.20.0, o default reenquadrado do D5). Idempotente -- seguro
  //     chamar sem câmera setada. `shutdown()` já faz isso implicitamente (D22); no-op seguro
  //     num `Draw2d` nunca-inicializado/movido-de/pós-shutdown.
  void reset_camera();

  // EN: D18 overload -- `transform` applies pivot/rotation/scale/flip about `dst` in
  //     WHICHEVER space is currently active (world units with a camera set, screen px
  //     otherwise, D5/Q1 (c)); each resulting corner is then projected through
  //     `world_to_screen` when a camera is set (D12/D18, "one composition story, no special
  //     cases"). Fail-high (D20, D10 extended): every `transform` member finite BEFORE any
  //     arithmetic (NaN/Inf anywhere skips the draw, dedup'd log, same `NonFinite` class as
  //     `dst`/`src_px`/`tint`); `dst.w <= 0 || dst.h <= 0` stays a silent legal no-op
  //     (unchanged from the 4-arg overload); a post-math guard rejects a non-finite COMPUTED
  //     corner (overflow -- e.g. huge zoom times huge coordinate, dedup'd log, no v0.20.0 twin
  //     by design). The plain 4-arg `draw_sprite` above is UNCHANGED by this overload's
  //     existence -- it keeps calling the v0.20.0 batcher path verbatim, camera or not (that
  //     absence-of-diff IS the Q1 (c) compatibility guarantee, additionally pinned by an
  //     exact-equivalence unit test, D19).
  // PT: Overload D18 -- `transform` aplica pivô/rotação/escala/flip sobre `dst` no espaço que
  //     estiver ATIVO no momento (unidades de mundo com câmera setada, px de tela caso
  //     contrário, D5/Q1 (c)); cada canto resultante é então projetado por `world_to_screen`
  //     quando uma câmera está setada (D12/D18, "uma história de composição só, sem caso
  //     especial"). Fail-high (D20, D10 estendido): todo membro de `transform` finito ANTES de
  //     qualquer conta (NaN/Inf em qualquer lugar pula o desenho, log dedup'd, mesma classe
  //     `NonFinite` de `dst`/`src_px`/`tint`); `dst.w <= 0 || dst.h <= 0` continua um no-op
  //     legal silencioso (inalterado em relação ao overload de 4 args); uma guarda pós-conta
  //     rejeita um canto COMPUTADO não-finito (overflow -- ex.: zoom enorme vezes coordenada
  //     enorme, log dedup'd, sem gêmeo na v0.20.0 por design). O `draw_sprite` de 4 args puro
  //     acima fica INALTERADO pela existência deste overload -- continua chamando o caminho
  //     v0.20.0 do batcher verbatim, com ou sem câmera (essa ausência-de-diff É a garantia de
  //     compatibilidade da Q1 (c), fixada adicionalmente por um teste unitário de equivalência
  //     exata, D19).
  void draw_sprite(const Texture2d& tex, const RectF& dst, const RectF& src_px,
                   const ColorF& tint, const SpriteTransform& transform);

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
