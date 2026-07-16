// SPDX-License-Identifier: MPL-2.0
// EN: Implementation of RenderGl3 — wraps RmlUi's RenderInterface_GL3.
//     gl_loader.h must be included before any other GL header.
// PT: Implementação de RenderGl3 — encapsula o RenderInterface_GL3 do RmlUi.
//     gl_loader.h deve ser incluído antes de qualquer outro header GL.

// EN: gl_loader.h FIRST — defines all GL 3.x function pointers loaded at runtime.
// PT: gl_loader.h PRIMEIRO — define todos os ponteiros de função GL 3.x carregados em tempo de execução.
#include "gl_loader.h"

#include "render_gl3.hpp"
// EN: Backend header from rmlui_SOURCE_DIR/Backends (added via CMake include path).
// PT: Header do backend do RmlUi a partir de rmlui_SOURCE_DIR/Backends (via CMake).
#include "RmlUi_Renderer_GL3.h"

// EN: RmlUi FileInterface (GetFileInterface) for asset-path-aware file reading.
// PT: FileInterface do RmlUi (GetFileInterface) para leitura de arquivo com resolução de asset.
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Core.h>

// EN: stb_image — header-only declarations; implementation compiled in stb_image_impl.cpp.
// PT: stb_image — declarações do header; implementação compilada em stb_image_impl.cpp.
#include "stb_image.h"

// EN: GLINTFX-TINT-2 — Dictionary/Variant (Rml::Get, CompileShader parameters) and Log, for the
//     "glintfx-tint" CompileShader/RenderShader/ReleaseShader overrides below. See ADR-0010 and
//     decorator_image_tint.{hpp,cpp} for the full design.
// PT: GLINTFX-TINT-2 — Dictionary/Variant (Rml::Get, parâmetros de CompileShader) e Log, para os
//     overrides "glintfx-tint" de CompileShader/RenderShader/ReleaseShader abaixo. Ver ADR-0010 e
//     decorator_image_tint.{hpp,cpp} para o design completo.
#include <RmlUi/Core/Dictionary.h>
#include <RmlUi/Core/Log.h>

#include <climits>  // EN: INT_MAX (AUD-L1-PARSE — LoadTexture's static_cast<int>(len) overflow guard).
                    // PT: INT_MAX (AUD-L1-PARSE — guarda de overflow do static_cast<int>(len) do LoadTexture).
#include <cmath>   // EN: std::sqrt (ripple's auto max-radius = captured backdrop diagonal, RenderShader).
                   // PT: std::sqrt (raio-max auto do ripple = diagonal do backdrop capturado, RenderShader).
#include <memory>
#include <vector>

// ---------------------------------------------------------------------------
// EN: GLINTFX-TINT-2 — "glintfx-tint" shader: own GLSL vertex+fragment program, own tiny raw
//     VAO/VBO/EBO per decorated element (built in decorator_image_tint.cpp's GenerateElementData,
//     never touched here). Dispatched via Gl3RenderInterface::CompileShader/RenderShader/
//     ReleaseShader overrides below (class Gl3RenderInterface, further down this file) — see
//     ADR-0010 (docs/adr/0010-image-tint-luminance-key.md) Decision (a) for why the actual GL
//     draw call MUST happen from inside a Gl3RenderInterface method (this->GetTransform()/
//     this->ResetProgram() are only reachable there, not from decorator code).
//
//     Vertex shader: 2 attributes only (position @location=0, tex_coord @location=1) — no vertex
//     colour, unlike RmlUi's own inColor0-carrying layout, since tint colour/mode/threshold/
//     opacity are all uniforms here, re-read fresh every RenderElement call (ADR-0010 Decision
//     (c)). `_transform` is fed directly from this->GetTransform() (public, ALREADY
//     projection×transform pre-combined — RenderInterface_GL3::SetTransform,
//     Backends/RmlUi_Renderer_GL3.cpp — no independent ortho-projection reconstruction needed).
//
//     Fragment shader: un-premultiplies before the luminance/saturation math and re-premultiplies
//     on the way out (ADR-0010 Decision (d) — MANDATORY, independent of the (deliberately
//     omitted, see Decision (d)) gamma/sRGB question: Gl3RenderInterface::LoadTexture already
//     premultiplies every texture's RGB by its own alpha on the CPU, so operating on that scaled-
//     down rgb directly would systematically under-weight the tint on semi-transparent/
//     anti-aliased pixels). MODE_NONE's branch is UNREACHABLE in practice — RenderElement takes
//     the plain Geometry::Render(translation, texture) path (no shader at all, see
//     decorator_image_tint.cpp) when mode==None — kept only as a defensive fallback.
// PT: GLINTFX-TINT-2 — shader "glintfx-tint": programa GLSL vertex+fragment próprio, VAO/VBO/EBO
//     crua própria e pequena por elemento decorado (construída em
//     decorator_image_tint.cpp:GenerateElementData, nunca tocada aqui). Despachado pelos overrides
//     de Gl3RenderInterface::CompileShader/RenderShader/ReleaseShader abaixo (classe
//     Gl3RenderInterface, mais adiante neste arquivo) — ver a Decisão (a) do ADR-0010
//     (docs/adr/0010-image-tint-luminance-key.md) pra por que a chamada de draw GL de fato
//     PRECISA acontecer de dentro de um método de Gl3RenderInterface (this->GetTransform()/
//     this->ResetProgram() só são alcançáveis lá, não do código de decorator).
//
//     Vertex shader: só 2 atributos (posição @location=0, tex_coord @location=1) — sem cor de
//     vértice, diferente do próprio layout inColor0-carregando do RmlUi, já que cor/modo/
//     threshold/opacity de tingimento são todos uniforms aqui, relidos frescos a cada chamada de
//     RenderElement (Decisão (c) do ADR-0010). `_transform` é alimentado direto de
//     this->GetTransform() (público, JÁ projeção×transform pré-combinados —
//     RenderInterface_GL3::SetTransform, Backends/RmlUi_Renderer_GL3.cpp — nenhuma reconstrução
//     independente de projeção ortográfica necessária).
//
//     Fragment shader: despremultiplica antes da matemática de luminância/saturação e
//     repremultiplica na saída (Decisão (d) do ADR-0010 — OBRIGATÓRIO, independente da questão
//     de gama/sRGB (deliberadamente omitida, ver Decisão (d)): Gl3RenderInterface::LoadTexture já
//     premultiplica o RGB de toda textura pelo próprio alpha na CPU, então operar direto nesse
//     rgb escalado pra baixo subponderaria sistematicamente o tingimento em pixels
//     semi-transparentes/anti-aliased). O ramo MODE_NONE é INALCANÇÁVEL na prática —
//     RenderElement toma o caminho Geometry::Render(translation, texture) simples (nenhum shader
//     nenhum, ver decorator_image_tint.cpp) quando mode==None — mantido só como fallback
//     defensivo.
// ---------------------------------------------------------------------------
namespace {

const char* const kGlintfxTintVertexShaderSrc = R"(#version 330
uniform vec2 _translate;
uniform mat4 _transform;
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord0;
out vec2 fragTexCoord;
void main() {
  fragTexCoord = inTexCoord0;
  vec2 translatedPos = inPosition + _translate;
  gl_Position = _transform * vec4(translatedPos, 0.0, 1.0);
}
)";

const char* const kGlintfxTintFragmentShaderSrc = R"(#version 330
#define MODE_NONE 0
#define MODE_MULTIPLY 1
#define MODE_LUMINANCE_MULTIPLY 2
#define MODE_SCREEN 3

uniform sampler2D _tex;
uniform vec4 _tint_color;   // straight (non-premultiplied) RGBA, from image-tint-color.
uniform int _mode;
uniform float _threshold;
uniform float _opacity;     // element's CSS opacity, read fresh every RenderElement call.

in vec2 fragTexCoord;
out vec4 finalColor;

void main() {
  vec4 texel = texture(_tex, fragTexCoord);          // PREMULTIPLIED (rgb already *= a).
  vec3 straight = texel.rgb / max(texel.a, 0.0001);   // un-premultiply, div-by-zero guarded.

  vec3 tinted = straight;
  if (_mode == MODE_MULTIPLY) {
    tinted = straight * _tint_color.rgb;
  } else if (_mode == MODE_LUMINANCE_MULTIPLY) {
    float L   = dot(straight, vec3(0.299, 0.587, 0.114));
    float mx  = max(max(straight.r, straight.g), straight.b);
    float mn  = min(min(straight.r, straight.g), straight.b);
    float sat = mx - mn;
    // Belt-and-suspenders: ResolveTintState (decorator_image_tint.cpp) already clamps
    // image-tint-threshold to [0, 0.999] on the CPU before it ever reaches this uniform, but
    // smoothstep(edge0, edge1, x) is GLSL UB the instant edge0 >= edge1 -- re-clamping here costs
    // one scalar min() and makes that invariant hold even if a future caller ever sets `_threshold`
    // via a path that bypasses ResolveTintState.
    float w   = smoothstep(min(_threshold, 0.999), 1.0, L) * (1.0 - sat);
    tinted = mix(straight, straight * _tint_color.rgb, w);
  } else if (_mode == MODE_SCREEN) {
    tinted = vec3(1.0) - (vec3(1.0) - straight) * (vec3(1.0) - _tint_color.rgb);
  }
  // MODE_NONE: tinted == straight (identity) -- unreachable in practice, see the file comment
  // above this shader source.

  float outAlpha = texel.a * _tint_color.a * _opacity;
  finalColor = vec4(tinted * outAlpha, outAlpha);      // re-premultiply.
}
)";

// EN: Minimal GLSL stage-compile helper -- mirrors the ERROR-LOGGING shape of RmlUi's OWN
//     (private, static, unreachable from here) CreateShader helper in
//     Backends/RmlUi_Renderer_GL3.cpp, reimplemented locally since that one lives in a different
//     translation unit with internal (anonymous-namespace/static) linkage.
// PT: Helper mínimo de compilação de estágio GLSL -- espelha a forma de LOG-DE-ERRO do próprio
//     helper CreateShader (privado, static, inalcançável daqui) do RmlUi em
//     Backends/RmlUi_Renderer_GL3.cpp, reimplementado localmente já que aquele vive numa
//     translation unit diferente com linkage interno (namespace anônimo/static).
// ---------------------------------------------------------------------------
// EN: L1.22-WAVE -- "glintfx-ripple" shader: own GLSL fragment program, REUSES
//     kGlintfxTintVertexShaderSrc above verbatim (see decorator_ripple.hpp's file header --
//     "Vertex: idêntico ao do tint" -- the vertex stage is a plain position+transform,
//     tex_coord passthrough with no vertex-colour attribute, textually identical to the tint
//     program's own vertex stage, so it is compiled a second time from the SAME source string
//     rather than duplicated). Dispatched via the SAME Gl3RenderInterface::CompileShader/
//     RenderShader/ReleaseShader overrides as "glintfx-tint" (further down this file), tagged by
//     GlintfxShaderHandle::is_ripple.
//
//     Fragment shader samples `backdrop_capture_tex_` (the FBO-0 backdrop captured by
//     EnsureBackdropCaptured, below) through a radial refraction ring -- see decorator_ripple.hpp for
//     the full effect description. `fragTexCoord` here is box-local [0,1] (from the ripple
//     element's own BuildQuadCorners quad, decorator_ripple.cpp) -- for the backdrop sample to
//     land on the CORRECT screen pixel, the ripple element's box must cover the SAME region
//     that was captured (offset_x/offset_y/w/h passed to begin_frame_compose); this is a
//     documented authoring contract (see decorator_ripple.hpp's file header, "overlay
//     fullscreen"), not something this shader can verify.
//     The shader body below also flips the V coordinate at the FINAL `_backdrop` lookup only
//     (document-space fragTexCoord/uv is y-down/top-left; the captured backdrop texture is
//     OpenGL-native y-up/bottom-left) -- see the in-shader comment right above that
//     `texture(_backdrop, ...)` call for the full derivation (found via manual GPU
//     verification, not obvious from the coordinate-space description alone).
// PT: L1.22-WAVE -- shader "glintfx-ripple": programa GLSL de fragmento próprio, REUSA
//     kGlintfxTintVertexShaderSrc acima literalmente (ver o cabeçalho de arquivo de
//     decorator_ripple.hpp -- "Vertex: idêntico ao do tint" -- o estágio de vértice é um
//     passthrough simples de posição+transform, tex_coord, sem atributo de cor de vértice
//     nenhum, textualmente idêntico ao próprio estágio de vértice do programa de tint, então é
//     compilado uma segunda vez a partir da MESMA string de origem em vez de duplicado).
//     Despachado pelos MESMOS overrides de Gl3RenderInterface::CompileShader/RenderShader/
//     ReleaseShader que "glintfx-tint" (mais adiante neste arquivo), marcado por
//     GlintfxShaderHandle::is_ripple.
//
//     O shader de fragmento amostra `backdrop_capture_tex_` (o backdrop do FBO-0 capturado por
//     EnsureBackdropCaptured, abaixo) através de um anel de refração radial -- ver decorator_ripple.hpp
//     pra descrição completa do efeito. `fragTexCoord` aqui é box-local [0,1] (do próprio quad
//     BuildQuadCorners do elemento ripple, decorator_ripple.cpp) -- para a amostra do backdrop
//     cair no pixel de tela CORRETO, a caixa do elemento ripple precisa cobrir a MESMA região
//     que foi capturada (offset_x/offset_y/w/h passados a begin_frame_compose); isso é um
//     contrato de autoria documentado (ver o cabeçalho de arquivo de decorator_ripple.hpp,
//     "overlay fullscreen"), não algo que este shader possa verificar.
// ---------------------------------------------------------------------------
const char* const kGlintfxRippleFragmentShaderSrc = R"(#version 330
uniform sampler2D _backdrop;
uniform vec2 _origin_px;
uniform float _phase;
uniform float _strength;
uniform float _width;
uniform vec2 _resolution;
uniform float _max_radius;

in vec2 fragTexCoord;
out vec4 finalColor;

void main() {
  vec2 frag_px = fragTexCoord * _resolution;
  float d = distance(frag_px, _origin_px);
  float front = _phase * _max_radius;

  // Auditoria dominó (mesma classe de bug do image-tint threshold -- ver
  // decorator_image_tint.cpp's ResolveTintState doc comment): smoothstep(edge0, edge1, x) e' GLSL
  // UB quando edge0 >= edge1. A forma "ingenua" smoothstep(_width, 0.0, abs(d - front)) teria
  // edge0=_width > edge1=0.0 -- INVERTIDA e portanto UB. Reescrita como
  // 1.0 - smoothstep(0.0, w, ...), que mantem edge0(0.0) < edge1(w) incondicionalmente (w e'
  // clampado > 0 tanto aqui quanto do lado da CPU, ResolveRippleState em decorator_ripple.cpp) e
  // produz a MESMA rampa visual (1.0 no centro do anel, decaindo suavemente pra 0.0 na borda).
  float w = max(_width, 0.0001);
  float ring = 1.0 - smoothstep(0.0, w, abs(d - front));

  // Guarda de divisao por zero: normalize(frag_px - _origin_px) e' UB/NaN quando d==0 (fragmento
  // exatamente na origem) -- divide por `d` manualmente em vez de chamar normalize(), com o
  // ternario cobrindo o caso d==0 (dir=vec2(0.0), sem deslocamento, sem NaN propagado).
  vec2 dir = (d > 0.0001) ? (frag_px - _origin_px) / d : vec2(0.0);

  const float k = 0.15; // wave spatial frequency constant.
  vec2 offset_px = dir * ring * _strength * sin((d - front) * k) * (1.0 - _phase);

  vec2 uv = fragTexCoord + offset_px / _resolution;

  // Y-flip on the way into _backdrop ONLY (found via manual GPU verification before handoff --
  // NOT present in the original task spec, which assumed fragTexCoord could sample _backdrop
  // directly): fragTexCoord/uv are in RmlUi's own document-space convention (origin top-left,
  // y-down -- BuildQuadCorners in decorator_ripple.cpp maps tex_coord (0,0) to the box's TOP-LEFT
  // corner). _backdrop, however, is populated by glCopyTexSubImage2D straight from the window's
  // OpenGL-native backbuffer (origin BOTTOM-left, y-up -- confirmed against how every OTHER
  // pixel-reading test in this repo (e.g. tests/image_tint_sanity.cpp's own pixel_at() helper,
  // "row = h - 1 - content_y") already has to compensate for this exact convention mismatch).
  // Sampling with the raw (un-flipped) v would read the backdrop vertically MIRRORED -- confirmed
  // by an ad-hoc two-colour-band render+read-back before this file was handed off. Only the FINAL
  // texture lookup needs the flip; _origin_px/_resolution/the ring/offset math above all stay in
  // document-space intentionally (ripple-origin-y is an author-facing property and must keep
  // ordinary top-down semantics, consistent with every other glintfx coordinate the author sees).
  vec3 rgb = texture(_backdrop, vec2(uv.x, 1.0 - uv.y)).rgb;
  // Premultiplied output: alpha = ring -- composes ONLY inside the travelling ring; everywhere
  // else (ring~=0) this element is fully transparent and never repaints the host's backdrop.
  finalColor = vec4(rgb * ring, ring);
}
)";

bool CompileGlintfxStage(GLuint& out_shader, GLenum stage, const char* src) {
  GLuint id = glCreateShader(stage);
  glShaderSource(id, 1, &src, nullptr);
  glCompileShader(id);
  GLint status = 0;
  glGetShaderiv(id, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE) {
    GLint log_len = 0;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_len);
    std::vector<char> log(static_cast<size_t>(log_len) + 1, '\0');
    glGetShaderInfoLog(id, log_len, nullptr, log.data());
    Rml::Log::Message(Rml::Log::LT_ERROR, "glintfx: image-tint() shader compile failure: %s", log.data());
    glDeleteShader(id);
    return false;
  }
  out_shader = id;
  return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// EN: Gl3RenderInterface — subclass of RenderInterface_GL3 that overrides LoadTexture
//     to support PNG, JPG, and TGA files with correct premultiplied alpha. The override:
//       1. Reads raw bytes via RmlUi's FileInterface (respects the asset base-URL).
//       2. Decodes via stbi_load_from_memory (forces 4-channel RGBA output).
//       3. Premultiplies alpha: R = R*A/255, G = G*A/255, B = B*A/255.
//          The GL3 backend composites in premultiplied-alpha space (GL_ONE,
//          GL_ONE_MINUS_SRC_ALPHA); uploading straight-alpha would cause a bright
//          halo around semi-transparent or hard-edged PNG/TGA regions.
//       4. Delegates the GL upload to the base-class GenerateTexture (reuses
//          GL_RGBA8 + mipmaps + filter — no GL reimplementation here).
//       5. Falls back to RenderInterface_GL3::LoadTexture only on file read error
//          or stb decode failure.
//
//     TGA bypass note: this override also intercepts TGA files (stb decodes TGA).
//     This is intentional: the upstream RenderInterface_GL3::LoadTexture does NOT
//     premultiply TGA pixels, but the GL3 backend composites in premultiplied space.
//     By routing all decodable formats through stb + premultiply we get consistent
//     alpha compositing for every image format.  The upstream TGA path is only
//     reachable if stb fails to open or decode the file (error/unsupported case).
//
// PT: Gl3RenderInterface — subclasse de RenderInterface_GL3 que sobrescreve LoadTexture
//     para suportar PNG, JPG e TGA com alpha premultiplicado correto. O override:
//       1. Lê bytes brutos via FileInterface do RmlUi (respeita a base-URL de assets).
//       2. Decodifica via stbi_load_from_memory (força saída RGBA de 4 canais).
//       3. Premultiplica o alpha: R = R*A/255, G = G*A/255, B = B*A/255.
//          O backend GL3 compõe em espaço premultiplied-alpha (GL_ONE,
//          GL_ONE_MINUS_SRC_ALPHA); fazer upload de alpha straight causaria halo
//          claro ao redor de regiões PNG/TGA semi-transparentes ou de bordas duras.
//       4. Delega o upload GL ao GenerateTexture da base (reusa GL_RGBA8 + mipmaps
//          + filtro — sem reimplementação de GL aqui).
//       5. Faz fallback para RenderInterface_GL3::LoadTexture apenas em erro de
//          leitura de arquivo ou falha de decode do stb.
//
//     Nota de bypass do TGA: este override também intercepta TGA (stb decodifica TGA).
//     Isso é intencional: o RenderInterface_GL3::LoadTexture do upstream NÃO premultiplica
//     pixels TGA, mas o backend GL3 compõe em espaço premultiplied. Ao rotear todos os
//     formatos decodificáveis pelo stb + premultiply obtemos composição de alpha consistente
//     para todo formato de imagem. O caminho TGA do upstream só é alcançado se o stb
//     falhar ao abrir ou decodificar o arquivo (caso de erro / formato não suportado).
//
//     GLINTFX-TINT-2: this class ALSO overrides CompileShader/RenderShader/ReleaseShader to
//     recognise a "glintfx-tint" shader name (see the kGlintfxTint*ShaderSrc / CompileGlintfxStage
//     block above this class) — a tagged-wrapper dispatch (GlintfxShaderHandle, below) lets
//     RenderShader/ReleaseShader tell a glintfx-tint compile apart from every OTHER CompileShader
//     call (including polygon()'s own gradient shaders) that funnels through this same override
//     once virtual dispatch resolves to this concrete class. See ADR-0010 Decision (a) for the
//     full derivation and decorator_image_tint.cpp for the (only) caller of the "glintfx-tint"
//     name.
// PT: falhar ao abrir ou decodificar o arquivo (caso de erro / formato não suportado).
//
//     GLINTFX-TINT-2: esta classe TAMBÉM sobrescreve CompileShader/RenderShader/ReleaseShader
//     para reconhecer um nome de shader "glintfx-tint" (ver o bloco kGlintfxTint*ShaderSrc /
//     CompileGlintfxStage acima desta classe) — um despacho por wrapper marcado
//     (GlintfxShaderHandle, abaixo) permite que RenderShader/ReleaseShader distingam uma
//     compilação glintfx-tint de toda OUTRA chamada de CompileShader (inclusive os próprios
//     shaders de gradiente do polygon()) que passa por este mesmo override uma vez que o despacho
//     virtual resolve pra esta classe concreta. Ver a Decisão (a) do ADR-0010 pra derivação
//     completa e decorator_image_tint.cpp pro (único) chamador do nome "glintfx-tint".
// ---------------------------------------------------------------------------

// EN: Per-CompileShader-call resolved tint state, threaded from decorator_image_tint.cpp's
//     Dictionary parameters (see ImageTintDecorator::RenderElement) into RenderShader's uniforms.
//     `vao`/`index_count` identify the glintfx-owned geometry to draw (decorator_image_tint.cpp's
//     ImageTintElementData) — RenderShader's own `geometry_handle` parameter (RmlUi's opaque
//     Rml::CompiledGeometryHandle) is NEVER dereferenced for a tint draw, see ADR-0010's
//     Investigation summary for why that handle is legally undereferenceable from glintfx code.
// PT: Estado de tint resolvido por-chamada-de-CompileShader, repassado dos parâmetros Dictionary
//     de decorator_image_tint.cpp (ver ImageTintDecorator::RenderElement) pros uniforms do
//     RenderShader. `vao`/`index_count` identificam a geometria dona-da-glintfx a desenhar
//     (ImageTintElementData de decorator_image_tint.cpp) — o próprio parâmetro `geometry_handle`
//     de RenderShader (o Rml::CompiledGeometryHandle opaco do RmlUi) NUNCA é desreferenciado num
//     draw de tint, ver o "Resumo da investigação" do ADR-0010 pra por que esse handle é
//     legalmente indesreferenciável a partir do código da glintfx.
struct GlintfxTintShaderData {
  GLuint vao = 0;
  int index_count = 0;
  Rml::Colourf tint_color{1.f, 1.f, 1.f, 1.f};  // straight, from image-tint-color.
  int mode = 0;                                  // ImageTintMode ordinal (decorator_image_tint.hpp).
  float threshold = 0.55f;
  float opacity = 1.f;
};

// EN: L1.22-WAVE — per-CompileShader-call resolved ripple state, threaded from
//     decorator_ripple.cpp's Dictionary parameters (see RippleDecorator::RenderElement) into
//     RenderShader's uniforms. `vao`/`index_count` identify the glintfx-owned geometry to draw
//     (decorator_ripple.cpp's RippleElementData) — mirrors GlintfxTintShaderData's own
//     vao/index_count field pair immediately above, same rationale (RenderShader's own
//     `geometry_handle` parameter is NEVER dereferenced for a ripple draw either).
//     `max_radius_arg` is the RESOLVED decorator-argument value (0.f means "auto" — see
//     decorator_ripple.hpp's RippleDecorator doc comment); RenderShader's ripple branch
//     substitutes `sqrt(cap_w_^2 + cap_h_^2)` (the captured backdrop's own diagonal) whenever
//     `max_radius_arg<=0.f`, since only Gl3RenderInterface — not decorator_ripple.cpp, which runs
//     at RCSS-parse/style-recalc time with no viewport knowledge — knows the CURRENT capture
//     resolution (cap_w_/cap_h_, set by EnsureBackdropCaptured, below).
// PT: L1.22-WAVE — estado de ripple resolvido por-chamada-de-CompileShader, repassado dos
//     parâmetros Dictionary de decorator_ripple.cpp (ver RippleDecorator::RenderElement) pros
//     uniforms do RenderShader. `vao`/`index_count` identificam a geometria dona-da-glintfx a
//     desenhar (RippleElementData de decorator_ripple.cpp) — espelha o próprio par de campos
//     vao/index_count de GlintfxTintShaderData logo acima, mesma racional (o próprio parâmetro
//     `geometry_handle` de RenderShader também NUNCA é desreferenciado num draw de ripple).
//     `max_radius_arg` é o valor de argumento-de-decorator RESOLVIDO (0.f significa "auto" — ver
//     o doc-comment de RippleDecorator em decorator_ripple.hpp); o ramo de ripple de
//     RenderShader substitui por `sqrt(cap_w_^2 + cap_h_^2)` (a própria diagonal do backdrop
//     capturado) sempre que `max_radius_arg<=0.f`, já que só Gl3RenderInterface — não
//     decorator_ripple.cpp, que roda em tempo de parse-RCSS/recálculo-de-estilo sem
//     conhecimento nenhum de viewport — conhece a resolução de captura ATUAL (cap_w_/cap_h_,
//     setada por EnsureBackdropCaptured, abaixo).
struct GlintfxRippleShaderData {
  GLuint vao = 0;
  int index_count = 0;
  float origin_x = 0.f;
  float origin_y = 0.f;
  float phase = 0.f;
  float strength = 0.f;
  float width = 48.f;
  float max_radius_arg = 0.f;  // 0.f == auto (see doc comment above).
};

// EN: Discriminated union returned (as an opaque Rml::CompiledShaderHandle, via reinterpret_cast)
//     by every Gl3RenderInterface::CompileShader call — is_tint=false and is_ripple=false for
//     every pre-existing shader kind (RmlUi's own filters/gradients/creation shaders, including
//     polygon()'s reused gradient shaders), is_tint=true only for "glintfx-tint", is_ripple=true
//     only for "glintfx-ripple" (L1.22-WAVE) — the three are mutually exclusive. See
//     CompileShader/RenderShader/ReleaseShader below for the unwrap-and-delegate-or-draw split.
// PT: União discriminada retornada (como um Rml::CompiledShaderHandle opaco, via
//     reinterpret_cast) por toda chamada de Gl3RenderInterface::CompileShader — is_tint=false e
//     is_ripple=false pra todo tipo de shader pré-existente (os próprios filtros/gradientes/
//     shaders de creation do RmlUi, inclusive os shaders de gradiente reusados pelo polygon()),
//     is_tint=true só pro "glintfx-tint", is_ripple=true só pro "glintfx-ripple" (L1.22-WAVE) —
//     os três são mutuamente exclusivos. Ver CompileShader/RenderShader/ReleaseShader abaixo pra
//     divisão desembrulha-e-delega-ou-desenha.
struct GlintfxShaderHandle {
  bool is_tint = false;
  bool is_ripple = false;
  Rml::CompiledShaderHandle base_handle = 0;  // valid only if !is_tint && !is_ripple.
  GlintfxTintShaderData tint_data;            // valid only if is_tint.
  GlintfxRippleShaderData ripple_data;        // valid only if is_ripple.
};

class Gl3RenderInterface : public RenderInterface_GL3 {
public:
  // EN: GLINTFX-TINT-2 — releases the lazily-compiled "glintfx-tint" GL program, if any was ever
  //     compiled. Never touches per-element VAO/VBO/EBO resources (those are owned/released by
  //     decorator_image_tint.cpp's ImageTintElementData, not by this class — see
  //     ReleaseShader's doc comment below).
  // PT: GLINTFX-TINT-2 — libera o programa GL "glintfx-tint" compilado de forma lazy, se algum
  //     dia foi compilado. Nunca toca recursos VAO/VBO/EBO por-elemento (esses são donos/
  //     liberados por ImageTintElementData de decorator_image_tint.cpp, não por esta classe --
  //     ver o doc-comment de ReleaseShader abaixo).
  // EN: L1.22-WAVE — also releases the lazily-compiled "glintfx-ripple" GL program and the
  //     backdrop-capture GL texture (backdrop_capture_tex_, populated by EnsureBackdropCaptured, below),
  //     if either was ever allocated. Same "never touches per-element VAO/VBO/EBO" contract as
  //     the "glintfx-tint" release above — those are owned/released by decorator_ripple.cpp's
  //     RippleElementData, not by this class.
  // PT: L1.22-WAVE — também libera o programa GL "glintfx-ripple" compilado de forma lazy e a
  //     textura GL de captura de backdrop (backdrop_capture_tex_, preenchida por EnsureBackdropCaptured,
  //     abaixo), se alguma das duas foi alocada. Mesmo contrato "nunca toca VAO/VBO/EBO
  //     por-elemento" da liberação de "glintfx-tint" acima — essas são donas/liberadas por
  //     RippleElementData de decorator_ripple.cpp, não por esta classe.
  ~Gl3RenderInterface() override {
    if (tint_program_)
      glDeleteProgram(tint_program_);
    if (ripple_program_)
      glDeleteProgram(ripple_program_);
    if (backdrop_capture_tex_)
      glDeleteTextures(1, &backdrop_capture_tex_);
  }

  // EN: AUD-L1-PARSE — hard ceiling on the raw asset file size LoadTexture will read into
  //     memory. Without it, a hostile/corrupt asset (or a symlink/FUSE mount reporting a bogus
  //     huge size via SEEK_END) drives `std::vector<unsigned char> buf(len)` below to an
  //     unbounded allocation attempt: `bad_alloc` propagates straight out of LoadTexture,
  //     across the RmlUi virtual-call boundary, into the host application (DoS by asset, not a
  //     memory-safety bug, but just as real for an embedding host — see docs/embed-integration.md
  //     which promises the host graceful degradation, never a thrown exception in its face).
  //     256 MiB is generous for any texture a UI asset realistically needs (a 8192x8192 RGBA8
  //     texture, an extreme case already past what most GPUs' `GL_MAX_TEXTURE_SIZE` allows, is
  //     256 MiB raw) while still being far below what would pressure a typical host's memory
  //     budget. This ceiling is checked BEFORE the `std::vector` allocation (not after), so the
  //     allocation itself is bounded — that is the actual guard; the file is never read into
  //     memory once it fails this check.
  // PT: AUD-L1-PARSE — teto rígido no tamanho bruto do arquivo de asset que o LoadTexture vai ler
  //     pra memória. Sem ele, um asset hostil/corrompido (ou um symlink/mount FUSE reportando um
  //     tamanho enorme falso via SEEK_END) leva o `std::vector<unsigned char> buf(len)` abaixo a
  //     uma tentativa de alocação sem teto: `bad_alloc` propaga direto do LoadTexture, cruzando a
  //     fronteira de virtual-call do RmlUi, até a aplicação host (DoS via asset, não um bug de
  //     memory-safety, mas igualmente real pra um host embarcador — ver docs/embed-integration.md,
  //     que promete ao host degradação graciosa, nunca uma exceção lançada na cara dele).
  //     256 MiB é generoso pra qualquer textura que um asset de UI realisticamente precisa (uma
  //     textura RGBA8 8192x8192, já um caso extremo além do que `GL_MAX_TEXTURE_SIZE` permite na
  //     maioria das GPUs, são 256 MiB brutos) e ainda assim bem abaixo do que pressionaria o
  //     orçamento de memória de um host típico. Este teto é checado ANTES da alocação do
  //     `std::vector` (não depois), então a própria alocação já sai limitada — essa é a guarda de
  //     fato; o arquivo nunca chega a ser lido pra memória se falhar esta checagem.
  static constexpr size_t kMaxAssetFileBytes = 256u * 1024u * 1024u;  // 256 MiB.

  Rml::TextureHandle LoadTexture(Rml::Vector2i& dims,
                                 const Rml::String& source) override
  {
    // EN: Read file bytes via RmlUi's FileInterface so the asset base-URL
    //     set by set_asset_base_url() is honoured for image paths in documents.
    // PT: Lê bytes do arquivo via FileInterface do RmlUi para que a base-URL de
    //     asset definida por set_asset_base_url() seja respeitada em imagens.
    Rml::FileInterface* fi = Rml::GetFileInterface();
    Rml::FileHandle fh = fi ? fi->Open(source) : static_cast<Rml::FileHandle>(0);
    if (!fh) {
      // EN: File not found or no FileInterface — fallback to base (TGA / upstream).
      // PT: Arquivo não encontrado ou sem FileInterface — fallback para base (TGA / upstream).
      return RenderInterface_GL3::LoadTexture(dims, source);
    }

    // EN: Determine the file size before reading a single byte, so the size cap below can
    //     reject an oversized/hostile asset without ever allocating a buffer for it.
    // PT: Determina o tamanho do arquivo antes de ler um único byte, pra que o teto abaixo
    //     possa rejeitar um asset hostil/grande demais sem nunca alocar um buffer pra ele.
    fi->Seek(fh, 0, SEEK_END);
    size_t len = fi->Tell(fh);
    fi->Seek(fh, 0, SEEK_SET);

    // EN: AUD-L1-PARSE guard 1/2 — reject before allocating. `kMaxAssetFileBytes` (256 MiB) is
    //     itself well under INT_MAX (~2 GiB), so this check also subsumes guard 2/2 below in
    //     every case that actually reaches stbi_load_from_memory; guard 2/2 stays as an explicit,
    //     independent defense-in-depth check in case this ceiling is ever raised without
    //     re-deriving that invariant.
    //     DELIBERATELY DOES NOT delegate to `RenderInterface_GL3::LoadTexture(dims, source)` like
    //     every other failure path in this function below — that base-class fallback (RmlUi's own
    //     TGA loader, Backends/RmlUi_Renderer_GL3.cpp) re-`Open()`s the SAME file and does
    //     `new byte[buffer_size]` UNCONDITIONALLY, with no size cap of its own. Falling through to
    //     it here would re-attempt the exact unbounded allocation this guard exists to prevent,
    //     silently defeating it for oversized assets specifically (every OTHER fallback in this
    //     function is safe to delegate because by the time it runs, `len` has already been proven
    //     <= kMaxAssetFileBytes). `return false` mirrors the base loader's own idiom for a hard
    //     failure (Backends/RmlUi_Renderer_GL3.cpp's LoadTexture: `if (!file_handle) return false;`).
    // PT: guarda 1/2 do AUD-L1-PARSE — rejeita antes de alocar. `kMaxAssetFileBytes` (256 MiB) já
    //     está bem abaixo de INT_MAX (~2 GiB), então esta checagem já cobre a guarda 2/2 abaixo em
    //     todo caso que de fato alcança stbi_load_from_memory; a guarda 2/2 permanece como checagem
    //     explícita e independente de defesa em profundidade, caso este teto seja elevado algum dia
    //     sem re-derivar esse invariante.
    //     DELIBERADAMENTE NÃO delega para `RenderInterface_GL3::LoadTexture(dims, source)` como
    //     todo outro caminho de falha nesta função abaixo -- esse fallback da base (loader TGA
    //     próprio do RmlUi, Backends/RmlUi_Renderer_GL3.cpp) reabre o MESMO arquivo e faz
    //     `new byte[buffer_size]` INCONDICIONALMENTE, sem teto próprio nenhum. Cair nele aqui
    //     re-tentaria exatamente a alocação sem teto que esta guarda existe pra prevenir,
    //     derrotando-a silenciosamente para assets grandes demais especificamente (todo OUTRO
    //     fallback nesta função é seguro de delegar porque, quando roda, `len` já foi provado
    //     <= kMaxAssetFileBytes). `return false` espelha o próprio idioma do loader da base pra
    //     falha dura (Backends/RmlUi_Renderer_GL3.cpp, LoadTexture: `if (!file_handle) return
    //     false;`).
    if (len > kMaxAssetFileBytes) {
      Rml::Log::Message(Rml::Log::LT_WARNING,
          "LoadTexture: asset '%s' is %zu bytes, exceeding the %zu byte cap -- refusing to load.",
          source.c_str(), len, kMaxAssetFileBytes);
      fi->Close(fh);
      return false;
    }

    // EN: Read entire file into a buffer.
    // PT: Lê o arquivo inteiro em um buffer.
    std::vector<unsigned char> buf(len);
    size_t nread = fi->Read(buf.data(), len, fh);
    fi->Close(fh);
    if (nread != len) {
      // EN: Short read — file truncated or I/O error; fallback to base.
      // PT: Leitura incompleta — arquivo truncado ou erro de I/O; fallback para base.
      return RenderInterface_GL3::LoadTexture(dims, source);
    }

    // EN: AUD-L1-PARSE guard 2/2 — defense-in-depth against static_cast<int>(len) below wrapping
    //     negative for len > INT_MAX. Unreachable today (guard 1/2 above already caps `len` at
    //     256 MiB, far under INT_MAX), kept explicit per the same fail-secure idiom as the other
    //     hostile-input guards in this file (see e.g. decorator_polygon.cpp's `sides` guard).
    //     `return false` (NOT a fallback to the base loader) for the same reason as guard 1/2
    //     above: this guard's whole point is that `len` is too large to touch further, and the
    //     base loader's own `new byte[buffer_size]` has no cap of its own -- if `kMaxAssetFileBytes`
    //     is ever raised past INT_MAX without this guard also being revisited, delegating here
    //     would silently resurrect the same unbounded-allocation DoS this whole guard pair exists
    //     to close.
    // PT: guarda 2/2 do AUD-L1-PARSE — defesa em profundidade contra o static_cast<int>(len)
    //     abaixo dar wrap negativo para len > INT_MAX. Inalcançável hoje (a guarda 1/2 acima já
    //     limita `len` a 256 MiB, bem abaixo de INT_MAX), mantida explícita seguindo o mesmo
    //     idioma fail-secure das outras guardas de entrada hostil deste arquivo (ver p.ex. a
    //     guarda de `sides` de decorator_polygon.cpp).
    //     `return false` (NÃO um fallback pro loader da base) pela mesma razão da guarda 1/2
    //     acima: o ponto inteiro desta guarda é que `len` é grande demais pra sequer tocar, e o
    //     `new byte[buffer_size]` próprio do loader da base não tem teto nenhum -- se
    //     `kMaxAssetFileBytes` algum dia for elevado além de INT_MAX sem esta guarda também ser
    //     revisitada, delegar aqui ressuscitaria silenciosamente o mesmo DoS de alocação sem teto
    //     que este par de guardas existe pra fechar.
    if (len > static_cast<size_t>(INT_MAX)) {
      Rml::Log::Message(Rml::Log::LT_WARNING,
          "LoadTexture: asset '%s' is %zu bytes, exceeding INT_MAX -- refusing to load.",
          source.c_str(), len);
      return false;
    }

    // EN: Decode via stb_image (forces 4-channel RGBA output regardless of source format).
    //     RAII via unique_ptr: stbi_image_free is called on any exit path, including
    //     exceptions thrown by GenerateTexture, without a manual stbi_image_free call.
    // PT: Decodifica via stb_image (força saída RGBA de 4 canais independente do formato).
    //     RAII via unique_ptr: stbi_image_free é chamado em qualquer saída, incluindo
    //     exceções de GenerateTexture, sem chamada manual a stbi_image_free.
    int w = 0, h = 0, n = 0;
    std::unique_ptr<unsigned char, decltype(&stbi_image_free)> px(
        stbi_load_from_memory(buf.data(), static_cast<int>(len), &w, &h, &n, 4),
        &stbi_image_free);
    if (!px) {
      // EN: Unknown format or decode error — fallback to base.
      // PT: Formato desconhecido ou erro de decode — fallback para base.
      return RenderInterface_GL3::LoadTexture(dims, source);
    }

    // EN: Premultiply alpha — the GL3 backend blends with GL_ONE, GL_ONE_MINUS_SRC_ALPHA
    //     which assumes premultiplied RGB. Without this step, transparent pixels whose
    //     source image stores a non-zero RGB (common in anti-aliased edges and hard-edge
    //     transparent regions) produce a bright colour halo around the rendered shape.
    // PT: Premultiplica o alpha — o backend GL3 faz blend com GL_ONE, GL_ONE_MINUS_SRC_ALPHA
    //     que assume RGB premultiplicado. Sem esta etapa, pixels transparentes cuja imagem
    //     de origem armazena RGB não-zero (comum em bordas anti-aliased e regiões
    //     transparentes de borda dura) produzem um halo colorido claro ao redor da forma.
    unsigned char* p = px.get();
    // EN: Compute the pixel count in unsigned 64-bit arithmetic before the loop — `w`/`h`
    //     come straight from stb_image's decode of a possibly-hostile asset file, so `w * h`
    //     in signed `int` arithmetic is UB on overflow for large-enough malformed dimensions.
    // PT: Calcula a contagem de pixels em aritmética sem sinal de 64 bits antes do loop — `w`/`h`
    //     vêm direto do decode do stb_image de um arquivo de asset potencialmente hostil, então
    //     `w * h` em aritmética `int` assinada é UB por overflow para dimensões malformadas
    //     grandes o bastante.
    const size_t pixel_count = static_cast<size_t>(w) * static_cast<size_t>(h);
    for (size_t i = 0; i < pixel_count; ++i) {
      unsigned int a = p[i * 4 + 3];
      p[i * 4 + 0] = static_cast<unsigned char>(p[i * 4 + 0] * a / 255u);
      p[i * 4 + 1] = static_cast<unsigned char>(p[i * 4 + 1] * a / 255u);
      p[i * 4 + 2] = static_cast<unsigned char>(p[i * 4 + 2] * a / 255u);
    }

    dims = Rml::Vector2i(w, h);
    Rml::TextureHandle handle = RenderInterface_GL3::GenerateTexture(
        Rml::Span<const Rml::byte>(
            reinterpret_cast<const Rml::byte*>(p),
            static_cast<size_t>(w) * static_cast<size_t>(h) * 4u),
        dims);
    // EN: px freed automatically by unique_ptr destructor on scope exit.
    // PT: px liberado automaticamente pelo destrutor do unique_ptr ao sair do escopo.
    return handle;
  }

  // -------------------------------------------------------------------------
  // EN: GLINTFX-TINT-2 — CompileShader/RenderShader/ReleaseShader overrides. See the
  //     GlintfxShaderHandle/GlintfxTintShaderData doc comments above this class, and ADR-0010
  //     Decision (a), for the full design.
  // PT: GLINTFX-TINT-2 — overrides de CompileShader/RenderShader/ReleaseShader. Ver os
  //     doc-comments de GlintfxShaderHandle/GlintfxTintShaderData acima desta classe, e a
  //     Decisão (a) do ADR-0010, pro design completo.
  // -------------------------------------------------------------------------

  Rml::CompiledShaderHandle CompileShader(const Rml::String& name, const Rml::Dictionary& parameters) override {
    if (name == "glintfx-tint") {
      EnsureTintProgramCompiled();
      if (!tint_program_) {
        // EN: Compile failure already logged inside EnsureTintProgramCompiled/CompileGlintfxStage
        //     — return a falsy handle (0) so RenderManager::CompileShader (RenderManager.cpp)
        //     itself returns a falsy Rml::CompiledShader, which decorator_image_tint.cpp's
        //     RenderElement is written to detect and degrade from (fall back to the vanilla
        //     textured-quad path) — see that file's RenderElement doc comment.
        // PT: Falha de compilação já logada dentro de EnsureTintProgramCompiled/
        //     CompileGlintfxStage — retorna um handle falso (0) pra que o próprio
        //     RenderManager::CompileShader (RenderManager.cpp) retorne um Rml::CompiledShader
        //     falso, que o RenderElement de decorator_image_tint.cpp é escrito pra detectar e
        //     degradar a partir daí (cai de volta pro caminho de quad texturizado vanilla) — ver
        //     o doc-comment de RenderElement daquele arquivo.
        return 0;
      }
      auto* wrapper = new GlintfxShaderHandle{};
      wrapper->is_tint = true;
      wrapper->tint_data.vao = static_cast<GLuint>(Rml::Get(parameters, "vao", 0));
      wrapper->tint_data.index_count = Rml::Get(parameters, "index_count", 0);
      wrapper->tint_data.tint_color = Rml::Get(parameters, "tint_color", Rml::Colourf(1.f, 1.f, 1.f, 1.f));
      wrapper->tint_data.mode = Rml::Get(parameters, "mode", 0);
      wrapper->tint_data.threshold = Rml::Get(parameters, "threshold", 0.55f);
      wrapper->tint_data.opacity = Rml::Get(parameters, "opacity", 1.f);
      return reinterpret_cast<Rml::CompiledShaderHandle>(wrapper);
    }

    // EN: L1.22-WAVE — "glintfx-ripple" branch, mirrors the "glintfx-tint" one immediately
    //     above (same lazily-compiled-program / falsy-handle-on-failure / Dictionary-unpack
    //     shape). Compile failure degrades to a falsy Rml::CompiledShader, which
    //     decorator_ripple.cpp's RenderElement is written to detect and draw NOTHING for (see
    //     that file's RenderElement doc comment — UNLIKE tint there is no vanilla fallback here).
    // PT: L1.22-WAVE — ramo "glintfx-ripple", espelha o "glintfx-tint" logo acima (mesma forma
    //     de programa-compilado-de-forma-lazy / handle-falso-em-falha / desempacotamento-de-
    //     Dictionary). Falha de compilação degrada pra um Rml::CompiledShader falso, que o
    //     RenderElement de decorator_ripple.cpp é escrito pra detectar e NÃO desenhar nada (ver
    //     o doc-comment de RenderElement daquele arquivo — DIFERENTE do tint não há fallback
    //     vanilla aqui).
    if (name == "glintfx-ripple") {
      EnsureRippleProgramCompiled();
      if (!ripple_program_)
        return 0;
      auto* wrapper = new GlintfxShaderHandle{};
      wrapper->is_ripple = true;
      wrapper->ripple_data.vao = static_cast<GLuint>(Rml::Get(parameters, "vao", 0));
      wrapper->ripple_data.index_count = Rml::Get(parameters, "index_count", 0);
      wrapper->ripple_data.origin_x = Rml::Get(parameters, "origin_x", 0.f);
      wrapper->ripple_data.origin_y = Rml::Get(parameters, "origin_y", 0.f);
      wrapper->ripple_data.phase = Rml::Get(parameters, "phase", 0.f);
      wrapper->ripple_data.strength = Rml::Get(parameters, "strength", 0.f);
      wrapper->ripple_data.width = Rml::Get(parameters, "width", 48.f);
      wrapper->ripple_data.max_radius_arg = Rml::Get(parameters, "max_radius", 0.f);
      return reinterpret_cast<Rml::CompiledShaderHandle>(wrapper);
    }

    // EN: Every OTHER shader name (RmlUi's own filters/gradients/creation shaders, INCLUDING
    //     polygon()'s reused "radial-gradient"/"linear-gradient" — see decorator_polygon.cpp) —
    //     delegate unchanged to the base class, then wrap the result too (is_tint=false,
    //     is_ripple=false) so RenderShader/ReleaseShader can uniformly unwrap every
    //     Rml::CompiledShaderHandle this class ever hands out.
    // PT: Todo OUTRO nome de shader (os próprios filtros/gradientes/shaders de creation do
    //     RmlUi, INCLUSIVE os "radial-gradient"/"linear-gradient" reusados pelo polygon() — ver
    //     decorator_polygon.cpp) — delega inalterado pra classe base, depois embrulha o
    //     resultado também (is_tint=false, is_ripple=false) pra que RenderShader/ReleaseShader
    //     possam desembrulhar uniformemente todo Rml::CompiledShaderHandle que esta classe já
    //     entregou.
    Rml::CompiledShaderHandle base = RenderInterface_GL3::CompileShader(name, parameters);
    if (!base)
      return 0;
    auto* wrapper = new GlintfxShaderHandle{};
    wrapper->is_tint = false;
    wrapper->is_ripple = false;
    wrapper->base_handle = base;
    return reinterpret_cast<Rml::CompiledShaderHandle>(wrapper);
  }

  void RenderShader(Rml::CompiledShaderHandle shader_handle, Rml::CompiledGeometryHandle geometry_handle, Rml::Vector2f translation,
      Rml::TextureHandle texture) override {
    auto* wrapper = reinterpret_cast<GlintfxShaderHandle*>(shader_handle);
    if (!wrapper)
      return;

    if (!wrapper->is_tint && !wrapper->is_ripple) {
      RenderInterface_GL3::RenderShader(wrapper->base_handle, geometry_handle, translation, texture);
      return;
    }

    if (wrapper->is_ripple) {
      // EN: L1.22-WAVE — ripple draw path. `geometry_handle` and `texture` are BOTH
      //     intentionally never touched here (see GlintfxRippleShaderData's doc comment above
      //     and decorator_ripple.hpp's class-level doc comment): we draw from
      //     `wrapper->ripple_data.vao` (glintfx-owned, built in decorator_ripple.cpp's
      //     GenerateElementData) and sample `backdrop_capture_tex_` (glintfx-owned, captured by
      //     EnsureBackdropCaptured, below) instead — NOT the decorator's own (nonexistent) texture
      //     handle. Same `this->ResetProgram()`-on-every-exit-path discipline as the tint branch
      //     below (see that branch's doc comment for the full rationale, which applies
      //     identically here).
      // PT: L1.22-WAVE — caminho de draw de ripple. `geometry_handle` e `texture` são AMBOS
      //     intencionalmente nunca tocados aqui (ver o doc-comment de GlintfxRippleShaderData
      //     acima e o doc-comment de nível de classe de decorator_ripple.hpp): desenhamos a
      //     partir de `wrapper->ripple_data.vao` (dono glintfx, construída em
      //     decorator_ripple.cpp:GenerateElementData) e amostramos `backdrop_capture_tex_` (dono
      //     glintfx, capturado por EnsureBackdropCaptured, abaixo) em vez disso — NÃO o texture handle
      //     (inexistente) do próprio decorator. Mesma disciplina de
      //     `this->ResetProgram()`-em-todo-caminho-de-saída do ramo de tint abaixo (ver o
      //     doc-comment daquele ramo pra racional completa, que se aplica identicamente aqui).
      if (ripple_program_ && wrapper->ripple_data.vao != 0 && wrapper->ripple_data.index_count > 0) {
        // EN: L1.22-CAPTURE COLD-START FIX -- capture ON DEMAND, exactly HERE, the first time
        //     (this frame) a ripple element actually needs to sample the backdrop -- NOT
        //     up-front in begin_frame_compose gated on a counter. See EnsureBackdropCaptured's
        //     own doc comment (below) for the full derivation of why this is the only place
        //     that is simultaneously correct AND still zero-cost-when-inactive: this branch is
        //     reached if and only if a "glintfx-ripple" shader is actually being drawn.
        // PT: FIX DE COLD-START DO L1.22-CAPTURE -- captura SOB DEMANDA, exatamente AQUI, na
        //     primeira vez (neste frame) que um elemento ripple de fato precisa amostrar o
        //     backdrop -- NÃO adiantado em begin_frame_compose protegido por um contador. Ver o
        //     próprio doc-comment de EnsureBackdropCaptured (abaixo) pra derivação completa de
        //     por que este é o único lugar simultaneamente correto E ainda custo-zero-quando-
        //     inativo: este ramo só é alcançado se e somente se um shader "glintfx-ripple"
        //     está de fato sendo desenhado.
        EnsureBackdropCaptured();
        if (backdrop_capture_tex_ != 0) {
          const GlintfxRippleShaderData& d = wrapper->ripple_data;
          const float max_radius = d.max_radius_arg > 0.f
              ? d.max_radius_arg
              : std::sqrt(static_cast<float>(cap_w_) * static_cast<float>(cap_w_) +
                          static_cast<float>(cap_h_) * static_cast<float>(cap_h_));
          glUseProgram(ripple_program_);
          glUniform2f(ripple_loc_translate_, translation.x, translation.y);
          glUniformMatrix4fv(ripple_loc_transform_, 1, GL_FALSE, this->GetTransform().data());
          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, backdrop_capture_tex_);
          glUniform1i(ripple_loc_backdrop_, 0);
          glUniform2f(ripple_loc_origin_, d.origin_x, d.origin_y);
          glUniform1f(ripple_loc_phase_, d.phase);
          glUniform1f(ripple_loc_strength_, d.strength);
          glUniform1f(ripple_loc_width_, d.width);
          glUniform2f(ripple_loc_resolution_, static_cast<float>(cap_w_), static_cast<float>(cap_h_));
          glUniform1f(ripple_loc_max_radius_, max_radius);
          glBindVertexArray(d.vao);
          glDrawElements(GL_TRIANGLES, d.index_count, GL_UNSIGNED_INT, nullptr);
          glBindVertexArray(0);
          glBindTexture(GL_TEXTURE_2D, 0);
        }
      }
      this->ResetProgram();
      return;
    }

    // EN: Tint draw path — `geometry_handle` is intentionally NEVER touched here (see
    //     GlintfxTintShaderData's doc comment above and ADR-0010's Investigation summary): we
    //     draw from `wrapper->tint_data.vao` (glintfx-owned, built in
    //     decorator_image_tint.cpp's GenerateElementData) instead.
    //     `this->GetTransform()`/`this->ResetProgram()` (both public, inherited from
    //     RenderInterface_GL3) are only reachable because this draw happens INSIDE a
    //     Gl3RenderInterface method — see ADR-0010 Decision (a). `this->ResetProgram()` runs on
    //     EVERY exit path below (there is only one, at the end — no early-return after
    //     glUseProgram) so the very next native RmlUi draw call in the same frame always
    //     re-issues its own glUseProgram rather than trusting a stale `active_program` cache
    //     (RenderInterface_GL3::UseProgram, Backends/RmlUi_Renderer_GL3.cpp — ResetProgram's
    //     UseProgram(ProgramId::None) does NOT itself call glUseProgram(0); it only invalidates
    //     the cache so the NEXT UseProgram(X) call is guaranteed to issue glUseProgram(X) for
    //     real, since None != X unconditionally).
    // PT: Caminho de draw de tint — `geometry_handle` intencionalmente NUNCA é tocado aqui (ver
    //     o doc-comment de GlintfxTintShaderData acima e o "Resumo da investigação" do
    //     ADR-0010): desenhamos a partir de `wrapper->tint_data.vao` (dono glintfx, construída
    //     em decorator_image_tint.cpp:GenerateElementData) em vez disso.
    //     `this->GetTransform()`/`this->ResetProgram()` (ambos públicos, herdados de
    //     RenderInterface_GL3) só são alcançáveis porque este draw acontece DENTRO de um método
    //     de Gl3RenderInterface — ver a Decisão (a) do ADR-0010. `this->ResetProgram()` roda em
    //     TODO caminho de saída abaixo (só há um, no fim — nenhum early-return após
    //     glUseProgram) pra que a próxíma chamada de draw nativa do RmlUi no mesmo frame sempre
    //     reemita seu próprio glUseProgram em vez de confiar num cache `active_program` obsoleto
    //     (RenderInterface_GL3::UseProgram, Backends/RmlUi_Renderer_GL3.cpp — o
    //     UseProgram(ProgramId::None) do ResetProgram NÃO chama glUseProgram(0) ele mesmo; só
    //     invalida o cache pra que a PRÓXIMA chamada UseProgram(X) seja garantida a emitir
    //     glUseProgram(X) de fato, já que None != X incondicionalmente).
    if (tint_program_ && wrapper->tint_data.vao != 0 && wrapper->tint_data.index_count > 0) {
      const GlintfxTintShaderData& d = wrapper->tint_data;
      glUseProgram(tint_program_);
      glUniform2f(tint_loc_translate_, translation.x, translation.y);
      glUniformMatrix4fv(tint_loc_transform_, 1, GL_FALSE, this->GetTransform().data());
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture));
      glUniform1i(tint_loc_tex_, 0);
      glUniform4f(tint_loc_tint_color_, d.tint_color.red, d.tint_color.green, d.tint_color.blue, d.tint_color.alpha);
      glUniform1i(tint_loc_mode_, d.mode);
      glUniform1f(tint_loc_threshold_, d.threshold);
      glUniform1f(tint_loc_opacity_, d.opacity);
      glBindVertexArray(d.vao);
      glDrawElements(GL_TRIANGLES, d.index_count, GL_UNSIGNED_INT, nullptr);
      glBindVertexArray(0);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    this->ResetProgram();
  }

  void ReleaseShader(Rml::CompiledShaderHandle shader_handle) override {
    auto* wrapper = reinterpret_cast<GlintfxShaderHandle*>(shader_handle);
    if (!wrapper)
      return;
    // EN: Delegate-or-free-tag-only split, mirrors CompileShader. The VAO/VBO/EBO
    //     (wrapper->tint_data.vao / wrapper->ripple_data.vao and friends) are NEVER deleted here
    //     — they are owned by decorator_image_tint.cpp's ImageTintElementData /
    //     decorator_ripple.cpp's RippleElementData respectively (per-element, released in each
    //     decorator's own ReleaseElementData), NOT by this per-CompileShader-call wrapper — see
    //     ADR-0010 Decision (a)'s explicit warning about this ownership split being the one
    //     place a double-free/use-after-free could sneak in (L1.22-WAVE's ripple branch inherits
    //     the identical warning).
    // PT: Divisão delega-ou-libera-só-a-tag, espelha CompileShader. A VAO/VBO/EBO
    //     (wrapper->tint_data.vao / wrapper->ripple_data.vao e companhia) NUNCA são deletadas
    //     aqui — são donas de ImageTintElementData de decorator_image_tint.cpp /
    //     RippleElementData de decorator_ripple.cpp respectivamente (por-elemento, liberada no
    //     próprio ReleaseElementData de cada decorator), NÃO deste wrapper
    //     por-chamada-de-CompileShader — ver o aviso explícito da Decisão (a) do ADR-0010 sobre
    //     essa divisão de posse ser o único lugar em que um double-free/use-after-free poderia
    //     se infiltrar (o ramo de ripple do L1.22-WAVE herda o aviso idêntico).
    if (!wrapper->is_tint && !wrapper->is_ripple)
      RenderInterface_GL3::ReleaseShader(wrapper->base_handle);
    delete wrapper;
  }

  // ---------------------------------------------------------------------------
  // EN: L1.22-CAPTURE, COLD-START FIX (post-647350f QA finding) — split into a cheap "arm" step
  //     (called unconditionally from RenderGl3::begin_frame_compose, below this file) and a
  //     real, on-demand "ensure" step (called from RenderShader's ripple branch, above, exactly
  //     once per frame, the very first time a ripple element actually needs to sample the
  //     backdrop). This REPLACES the original counter-gated design (`ripple_active_counter_`,
  //     `RenderGl3::set_ripple_active_counter`), which had a real cold-start bug: the counter
  //     (incremented in decorator_ripple.cpp's GenerateElementData) is only accurate DURING/AFTER
  //     Rml::Context::Render()'s tree walk (confirmed by reading the pinned
  //     Source/Core/ElementEffects.cpp: `InstanceEffects()`/`GenerateElementData` both run from
  //     `Element::Render()` -> `RenderEffects()`, i.e. INSIDE Context::Render(), never earlier),
  //     but the old gate read it BEFORE Context::Render() even started (at the top of
  //     begin_frame_compose) -- so on the very FIRST frame a ripple decorator became active, the
  //     counter still read 0 (last frame's value), the capture was skipped, and the ripple
  //     shader sampled a still-zero `backdrop_capture_tex_` -- the "opening frame" of a
  //     `ripple-phase: 0` keyframe animation (the most visually intense one) silently rendered as
  //     nothing. ripple_sanity.cpp (QA, independent of this implementer) proves this with a spy
  //     on the real glCopyTexSubImage2D entry point plus hand-derived expected pixels.
  //
  //     WHY ON-DEMAND-INSIDE-RENDERSHADER FIXES IT WITHOUT REINTRODUCING A COUNTER: the decision
  //     "does ANY ripple element need the backdrop this frame" no longer needs to be made in
  //     advance at all -- it is now IMPLICIT and free: EnsureBackdropCaptured() is only ever
  //     reachable from inside the "glintfx-ripple" RenderShader branch, which is only ever
  //     reached if RippleDecorator::RenderElement (decorator_ripple.cpp) actually compiled and
  //     is drawing that shader THIS frame. Zero ripple elements in the document => that branch
  //     never executes => EnsureBackdropCaptured() is never called => zero
  //     glCopyTexSubImage2D/glTexImage2D/allocation calls -- the SAME cost-zero-when-inactive
  //     guarantee as before, but derived structurally instead of from a counter whose accuracy
  //     window didn't line up with when the gate needed to read it. This also sidesteps a SECOND
  //     latent risk the counter design had (flagged, not yet hit): RmlUi's own decorator
  //     INSTANCES can be shared/cached across elements with an identical decorator declaration
  //     (StyleSheet::InstanceDecorators) -- a naive "count constructed instances" fix would have
  //     needed to reason about that sharing too; this design never counts instances at all.
  //
  //     CORRECTNESS (still capturing the HOST's undisturbed FBO 0, not glintfx's own
  //     in-progress render): unchanged from the original design's own reasoning, just re-derived
  //     for the new call SITE. FBO 0 is untouched by RmlUi from BeginFrame() (inside
  //     begin_frame_compose, called before Context::Render()) all the way through the ENTIRE
  //     Context::Render() tree walk -- RmlUi only ever draws into its OWN private render-layer
  //     FBOs during that walk (confirmed by reading the pinned RmlUi_Renderer_GL3.cpp) -- and
  //     only rebinds/blits onto FBO 0 inside EndFrame() (called from end_frame_compose, AFTER
  //     Context::Render() returns). Since EnsureBackdropCaptured() always runs strictly inside
  //     Context::Render() (from a RenderShader call), it is unconditionally BEFORE EndFrame(),
  //     so FBO 0 still holds exactly what the host drew before this glintfx frame began,
  //     regardless of how many OTHER elements (ripple or not) already painted into RmlUi's
  //     private layers earlier in the SAME tree walk.
  //
  //     ONE-CAPTURE-PER-FRAME (unchanged intent, new mechanism): `backdrop_captured_this_frame_`
  //     latches true on the first EnsureBackdropCaptured() call each frame (reset back to false
  //     by ArmBackdropCapture, called once per begin_frame_compose) -- if a document has MULTIPLE
  //     ripple elements, only the FIRST one drawn each frame triggers the real GL capture; every
  //     later ripple element that frame reuses the SAME `backdrop_capture_tex_` (still correct:
  //     none of them can see each other's own draws, since RmlUi's own render-layer output never
  //     reaches FBO 0 until EndFrame(), long after every ripple element has already drawn).
  //
  //     READ-FRAMEBUFFER SAFETY: unchanged from the original design -- EnsureBackdropCaptured()
  //     still rebinds GL_READ_FRAMEBUFFER to 0 + glReadBuffer(GL_BACK) and does NOT restore
  //     either itself; GlStateGuard (gl_state.hpp) still owns that restore (its ctor/dtor window
  //     -- Engine::render_compose, engine.cpp -- still fully encloses the entire
  //     begin_frame_compose -> Context::Render() -> end_frame_compose sequence, so moving the
  //     actual GL capture to occur partway THROUGH Context::Render() changes nothing about when
  //     the guard's own ctor/dtor run relative to it).
  // PT: L1.22-CAPTURE, FIX DE COLD-START (achado do QA pós-647350f) — dividido num passo "arm"
  //     barato (chamado incondicionalmente de RenderGl3::begin_frame_compose, mais abaixo neste
  //     arquivo) e num passo "ensure" real, sob demanda (chamado do ramo de ripple de
  //     RenderShader, acima, exatamente uma vez por frame, na primeira vez que um elemento
  //     ripple de fato precisa amostrar o backdrop). Isto SUBSTITUI o design original protegido
  //     por contador (`ripple_active_counter_`, `RenderGl3::set_ripple_active_counter`), que
  //     tinha um bug real de cold-start: o contador (incrementado em GenerateElementData de
  //     decorator_ripple.cpp) só é preciso DURANTE/DEPOIS da caminhada de árvore de
  //     Rml::Context::Render() (confirmado lendo o Source/Core/ElementEffects.cpp pinado:
  //     `InstanceEffects()`/`GenerateElementData` ambos rodam a partir de `Element::Render()` ->
  //     `RenderEffects()`, isto é, DENTRO de Context::Render(), nunca antes), mas o gate antigo o
  //     lia ANTES de Context::Render() sequer começar (no topo de begin_frame_compose) -- então
  //     no PRIMEIRO frame em que um decorator ripple ficava ativo, o contador ainda lia 0 (valor
  //     do frame anterior), a captura era pulada, e o shader de ripple amostrava um
  //     `backdrop_capture_tex_` ainda zero -- o frame de "abertura" de uma animação de
  //     `ripple-phase: 0` (o mais intenso visualmente) renderizava silenciosamente como nada.
  //     ripple_sanity.cpp (QA, independente deste implementador) prova isso com um espião no
  //     entry point real glCopyTexSubImage2D mais pixels esperados derivados à mão.
  //
  //     POR QUE SOB-DEMANDA-DENTRO-DO-RENDERSHADER CONSERTA SEM REINTRODUZIR UM CONTADOR: a
  //     decisão "algum elemento ripple precisa do backdrop neste frame" não precisa mais ser
  //     tomada de antemão -- agora é IMPLÍCITA e grátis: EnsureBackdropCaptured() só é alcançável
  //     de dentro do ramo "glintfx-ripple" de RenderShader, que só é alcançado se
  //     RippleDecorator::RenderElement (decorator_ripple.cpp) de fato compilou e está desenhando
  //     aquele shader NESTE frame. Zero elementos ripple no documento => aquele ramo nunca
  //     executa => EnsureBackdropCaptured() nunca é chamado => zero chamadas de
  //     glCopyTexSubImage2D/glTexImage2D/alocação -- a MESMA garantia de custo-zero-quando-
  //     inativo de antes, mas derivada estruturalmente em vez de um contador cuja janela de
  //     precisão não batia com quando o gate precisava lê-lo. Isto também contorna um SEGUNDO
  //     risco latente que o design de contador tinha (sinalizado, ainda não atingido): as
  //     PRÓPRIAS instâncias de decorator do RmlUi podem ser compartilhadas/cacheadas entre
  //     elementos com uma declaração de decorator idêntica (StyleSheet::InstanceDecorators) -- um
  //     fix ingênuo de "contar instâncias construídas" precisaria ter raciocinado sobre esse
  //     compartilhamento também; este design nunca conta instâncias.
  //
  //     CORREÇÃO (ainda capturando o FBO 0 intocado do HOST, não o render em progresso da
  //     própria glintfx): inalterada da racional do design original, só re-derivada pro novo
  //     PONTO de chamada. O FBO 0 fica intocado pelo RmlUi desde o BeginFrame() (dentro de
  //     begin_frame_compose, chamado antes de Context::Render()) até o FIM da caminhada de
  //     árvore inteira de Context::Render() -- o RmlUi só desenha nos PRÓPRIOS FBOs privados de
  //     render-layer durante essa caminhada -- e só revincula/blita no FBO 0 dentro de EndFrame()
  //     (chamado de end_frame_compose, DEPOIS de Context::Render() retornar). Como
  //     EnsureBackdropCaptured() sempre roda estritamente dentro de Context::Render() (a partir
  //     de uma chamada de RenderShader), está incondicionalmente ANTES de EndFrame(), então o
  //     FBO 0 ainda tem exatamente o que o host desenhou antes deste frame da glintfx começar,
  //     independente de quantos OUTROS elementos (ripple ou não) já pintaram nas camadas
  //     privadas do RmlUi mais cedo na MESMA caminhada de árvore.
  //
  //     UMA-CAPTURA-POR-FRAME (intenção inalterada, mecanismo novo): `backdrop_captured_this_frame_`
  //     trava true na primeira chamada de EnsureBackdropCaptured() de cada frame (resetada pra
  //     false por ArmBackdropCapture, chamado uma vez por begin_frame_compose) -- se um documento
  //     tem MÚLTIPLOS elementos ripple, só o PRIMEIRO desenhado a cada frame dispara a captura GL
  //     de fato; todo elemento ripple posterior naquele frame reusa o MESMO
  //     `backdrop_capture_tex_` (ainda correto: nenhum deles consegue ver os próprios draws uns
  //     dos outros, já que a saída de render-layer do próprio RmlUi nunca alcança o FBO 0 até o
  //     EndFrame(), muito depois de todo elemento ripple já ter desenhado).
  //
  //     SEGURANÇA DO READ-FRAMEBUFFER: inalterada do design original -- EnsureBackdropCaptured()
  //     ainda revincula GL_READ_FRAMEBUFFER a 0 + glReadBuffer(GL_BACK) e NÃO restaura nenhum dos
  //     dois por conta própria; GlStateGuard (gl_state.hpp) ainda é dono dessa restauração (a
  //     janela ctor/dtor dele -- Engine::render_compose, engine.cpp -- ainda envolve a sequência
  //     begin_frame_compose -> Context::Render() -> end_frame_compose inteira, então mover a
  //     captura GL de fato pra ocorrer no meio de Context::Render() não muda nada sobre quando o
  //     ctor/dtor do guard rodam em relação a ela).
  // ---------------------------------------------------------------------------

  // EN: Cheap, unconditional, zero-GL-call "arm" step -- called once per frame from
  //     RenderGl3::begin_frame_compose (below), BEFORE Context::Render() runs. Just remembers the
  //     capture rectangle for whenever (if ever) EnsureBackdropCaptured() needs it this frame,
  //     and resets the once-per-frame latch.
  // PT: Passo "arm" barato, incondicional, zero-chamada-GL -- chamado uma vez por frame a partir
  //     de RenderGl3::begin_frame_compose (abaixo), ANTES de Context::Render() rodar. Só lembra o
  //     retângulo de captura pra quando (se algum dia) EnsureBackdropCaptured() precisar dele
  //     neste frame, e reseta a trava de uma-vez-por-frame.
  void ArmBackdropCapture(int offset_x, int offset_y, int w, int h) {
    pending_offset_x_ = offset_x;
    pending_offset_y_ = offset_y;
    pending_w_ = w;
    pending_h_ = h;
    backdrop_captured_this_frame_ = false;
  }

  // EN: The real, on-demand capture -- called from RenderShader's "glintfx-ripple" branch
  //     (above), never from begin_frame_compose directly (see this section's own doc comment for
  //     the full derivation of why). Idempotent within a single frame via
  //     `backdrop_captured_this_frame_` (armed/reset by ArmBackdropCapture, above).
  // PT: A captura real, sob demanda -- chamada do ramo "glintfx-ripple" de RenderShader (acima),
  //     nunca diretamente de begin_frame_compose (ver o doc-comment desta seção pra derivação
  //     completa do porquê). Idempotente dentro de um único frame via
  //     `backdrop_captured_this_frame_` (armado/resetado por ArmBackdropCapture, acima).
  void EnsureBackdropCaptured() {
    if (backdrop_captured_this_frame_)
      return;
    backdrop_captured_this_frame_ = true;  // EN: latch BEFORE any early return below -- a failed
                                            //     attempt (e.g. w<=0) should not be retried every
                                            //     single ripple element this same frame.
                                            // PT: trava ANTES de qualquer retorno antecipado
                                            //     abaixo -- uma tentativa falha (ex.: w<=0) não
                                            //     deve ser retentada a cada elemento ripple deste
                                            //     mesmo frame.
    const int offset_x = pending_offset_x_, offset_y = pending_offset_y_;
    const int w = pending_w_, h = pending_h_;
    if (w <= 0 || h <= 0)
      return;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);

    if (backdrop_capture_tex_ == 0)
      glGenTextures(1, &backdrop_capture_tex_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backdrop_capture_tex_);

    if (cap_w_ != w || cap_h_ != h) {
      // EN: (Re)allocate storage — GL_RGBA8, no mipmaps, GL_LINEAR filtering, GL_CLAMP_TO_EDGE
      //     wrap (the ripple shader's UV can legitimately land slightly outside [0,1] at the
      //     edge of a strong ring displacement — clamp-to-edge degrades that to a stretched
      //     edge pixel rather than wrapping to the opposite side of the capture).
      // PT: (Re)aloca storage — GL_RGBA8, sem mipmaps, filtro GL_LINEAR, wrap GL_CLAMP_TO_EDGE
      //     (o UV do shader de ripple pode legitimamente cair um pouco fora de [0,1] na borda
      //     de um deslocamento de anel forte — clamp-to-edge degrada isso pra um pixel de borda
      //     esticado em vez de dar wrap pro lado oposto da captura).
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      cap_w_ = w;
      cap_h_ = h;
    }

    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, offset_x, offset_y, w, h);
  }

private:
  // EN: Lazily compiles the "glintfx-tint" GL program on first use (idempotent — a prior hard
  //     failure is NOT retried every frame, `tint_compile_attempted_` latches it). Uniform
  //     locations are looked up ONCE here (not per-RenderShader-call) since they never change
  //     for the lifetime of a linked program.
  // PT: Compila lazily o programa GL "glintfx-tint" no primeiro uso (idempotente — uma falha
  //     anterior NÃO é retentada a cada frame, `tint_compile_attempted_` trava isso). As
  //     locations de uniform são buscadas UMA VEZ aqui (não por-chamada-de-RenderShader) já que
  //     nunca mudam durante a vida de um programa linkado.
  void EnsureTintProgramCompiled() {
    if (tint_program_ || tint_compile_attempted_)
      return;
    tint_compile_attempted_ = true;

    GLuint vs = 0, fs = 0;
    if (!CompileGlintfxStage(vs, GL_VERTEX_SHADER, kGlintfxTintVertexShaderSrc))
      return;
    if (!CompileGlintfxStage(fs, GL_FRAGMENT_SHADER, kGlintfxTintFragmentShaderSrc)) {
      glDeleteShader(vs);
      return;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint link_status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE) {
      GLint log_len = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
      std::vector<char> log(static_cast<size_t>(log_len) + 1, '\0');
      glGetProgramInfoLog(program, log_len, nullptr, log.data());
      Rml::Log::Message(Rml::Log::LT_ERROR, "glintfx: image-tint() program link failure: %s", log.data());
      glDeleteProgram(program);
      return;
    }

    tint_program_ = program;
    tint_loc_translate_ = glGetUniformLocation(program, "_translate");
    tint_loc_transform_ = glGetUniformLocation(program, "_transform");
    tint_loc_tex_ = glGetUniformLocation(program, "_tex");
    tint_loc_tint_color_ = glGetUniformLocation(program, "_tint_color");
    tint_loc_mode_ = glGetUniformLocation(program, "_mode");
    tint_loc_threshold_ = glGetUniformLocation(program, "_threshold");
    tint_loc_opacity_ = glGetUniformLocation(program, "_opacity");
  }

  GLuint tint_program_ = 0;
  bool tint_compile_attempted_ = false;
  GLint tint_loc_translate_ = -1;
  GLint tint_loc_transform_ = -1;
  GLint tint_loc_tex_ = -1;
  GLint tint_loc_tint_color_ = -1;
  GLint tint_loc_mode_ = -1;
  GLint tint_loc_threshold_ = -1;
  GLint tint_loc_opacity_ = -1;

  // EN: L1.22-WAVE — lazily compiles the "glintfx-ripple" GL program on first use, same
  //     idempotent-latch discipline as EnsureTintProgramCompiled above. Vertex stage reuses
  //     kGlintfxTintVertexShaderSrc VERBATIM (see that constant's own doc comment further up
  //     this file) — only the fragment stage (kGlintfxRippleFragmentShaderSrc) is unique to this
  //     program.
  // PT: L1.22-WAVE — compila lazily o programa GL "glintfx-ripple" no primeiro uso, mesma
  //     disciplina de trava idempotente de EnsureTintProgramCompiled acima. O estágio de vértice
  //     reusa kGlintfxTintVertexShaderSrc LITERALMENTE (ver o próprio doc-comment daquela
  //     constante mais acima neste arquivo) — só o estágio de fragmento
  //     (kGlintfxRippleFragmentShaderSrc) é exclusivo deste programa.
  void EnsureRippleProgramCompiled() {
    if (ripple_program_ || ripple_compile_attempted_)
      return;
    ripple_compile_attempted_ = true;

    GLuint vs = 0, fs = 0;
    if (!CompileGlintfxStage(vs, GL_VERTEX_SHADER, kGlintfxTintVertexShaderSrc))
      return;
    if (!CompileGlintfxStage(fs, GL_FRAGMENT_SHADER, kGlintfxRippleFragmentShaderSrc)) {
      glDeleteShader(vs);
      return;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint link_status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE) {
      GLint log_len = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
      std::vector<char> log(static_cast<size_t>(log_len) + 1, '\0');
      glGetProgramInfoLog(program, log_len, nullptr, log.data());
      Rml::Log::Message(Rml::Log::LT_ERROR, "glintfx: ripple() program link failure: %s", log.data());
      glDeleteProgram(program);
      return;
    }

    ripple_program_ = program;
    ripple_loc_translate_ = glGetUniformLocation(program, "_translate");
    ripple_loc_transform_ = glGetUniformLocation(program, "_transform");
    ripple_loc_backdrop_ = glGetUniformLocation(program, "_backdrop");
    ripple_loc_origin_ = glGetUniformLocation(program, "_origin_px");
    ripple_loc_phase_ = glGetUniformLocation(program, "_phase");
    ripple_loc_strength_ = glGetUniformLocation(program, "_strength");
    ripple_loc_width_ = glGetUniformLocation(program, "_width");
    ripple_loc_resolution_ = glGetUniformLocation(program, "_resolution");
    ripple_loc_max_radius_ = glGetUniformLocation(program, "_max_radius");
  }

  GLuint ripple_program_ = 0;
  bool ripple_compile_attempted_ = false;
  GLint ripple_loc_translate_ = -1;
  GLint ripple_loc_transform_ = -1;
  GLint ripple_loc_backdrop_ = -1;
  GLint ripple_loc_origin_ = -1;
  GLint ripple_loc_phase_ = -1;
  GLint ripple_loc_strength_ = -1;
  GLint ripple_loc_width_ = -1;
  GLint ripple_loc_resolution_ = -1;
  GLint ripple_loc_max_radius_ = -1;

  // EN: L1.22-CAPTURE — backdrop-capture GL texture + its current dimensions (see
  //     EnsureBackdropCaptured's doc comment above). `backdrop_capture_tex_`/`cap_w_`/`cap_h_`
  //     stay at 0 for the entire lifetime of a Gl3RenderInterface that never renders a
  //     "ripple()" element — see EnsureBackdropCaptured's cost-zero-when-inactive derivation.
  // PT: L1.22-CAPTURE — textura GL de captura de backdrop + as dimensões atuais dela (ver o
  //     doc-comment de EnsureBackdropCaptured acima). `backdrop_capture_tex_`/`cap_w_`/`cap_h_`
  //     ficam em 0 pela vida inteira de um Gl3RenderInterface que nunca renderiza um elemento
  //     "ripple()" -- ver a derivação de custo-zero-quando-inativo de EnsureBackdropCaptured.
  GLuint backdrop_capture_tex_ = 0;
  int cap_w_ = 0;
  int cap_h_ = 0;

  // EN: L1.22-CAPTURE COLD-START FIX — the "armed" capture rectangle (set by ArmBackdropCapture,
  //     read by EnsureBackdropCaptured) and the once-per-frame latch. See the
  //     ArmBackdropCapture/EnsureBackdropCaptured doc comment above for the full mechanism.
  // PT: FIX DE COLD-START DO L1.22-CAPTURE — o retângulo de captura "armado" (setado por
  //     ArmBackdropCapture, lido por EnsureBackdropCaptured) e a trava de uma-vez-por-frame. Ver
  //     o doc-comment de ArmBackdropCapture/EnsureBackdropCaptured acima pro mecanismo completo.
  int pending_offset_x_ = 0;
  int pending_offset_y_ = 0;
  int pending_w_ = 0;
  int pending_h_ = 0;
  bool backdrop_captured_this_frame_ = false;
};

namespace glintfx {

// EN: Impl holds the concrete Gl3RenderInterface instance (subclass of RenderInterface_GL3).
//     It is constructed lazily inside init() to guarantee a live GL context.
// PT: Impl contém a instância concreta de Gl3RenderInterface (subclasse de RenderInterface_GL3).
//     É construído de forma lazy dentro de init() para garantir contexto GL ativo.
struct RenderGl3::Impl {
  Gl3RenderInterface renderer;
};

RenderGl3::RenderGl3() : impl_(nullptr) {}

RenderGl3::~RenderGl3() {
  delete impl_;   // EN: safe to call on nullptr. PT: seguro com nullptr.
}

bool RenderGl3::init() {
  if (impl_) {
    // EN: already initialized — report existing status.
    // PT: já inicializado — reporta status existente.
    return static_cast<bool>(impl_->renderer);
  }

  // EN: With RMLUI_GL3_CUSTOM_LOADER the namespace init is a no-op, but call
  //     it for forward compatibility and to signal intent.
  // PT: Com RMLUI_GL3_CUSTOM_LOADER a init do namespace é no-op, mas chamamos
  //     para compatibilidade futura e clareza de intenção.
  Rml::String msg;
  if (!RmlGL3::Initialize(&msg)) {
    return false;
  }

  // EN: Construct Impl; the RenderInterface_GL3 constructor compiles GL shaders.
  // PT: Constrói Impl; o construtor do RenderInterface_GL3 compila os shaders GL.
  impl_ = new Impl;
  bool renderer_ok = static_cast<bool>(impl_->renderer);
  if (!renderer_ok) {
    delete impl_;
    impl_ = nullptr;
    return false;
  }
  return true;
}

Rml::RenderInterface* RenderGl3::iface() {
  return impl_ ? &impl_->renderer : nullptr;
}

void RenderGl3::begin_frame(int w, int h) {
  // EN: Guard: no-op if init() was not called or failed (mirrors iface() behaviour).
  // PT: Guard: sem efeito se init() não foi chamado ou falhou (espelha iface()).
  if (!impl_) return;

  // EN: Set viewport and clear the colour buffer before handing control to RmlUi.
  // PT: Define viewport e limpa o buffer de cor antes de entregar controle ao RmlUi.
  glViewport(0, 0, w, h);
  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  impl_->renderer.SetViewport(w, h);
  impl_->renderer.BeginFrame();
}

void RenderGl3::end_frame() {
  // EN: Guard: no-op if init() was not called or failed (mirrors iface() behaviour).
  // PT: Guard: sem efeito se init() não foi chamado ou falhou (espelha iface()).
  if (!impl_) return;
  impl_->renderer.EndFrame();

  // EN: Force backbuffer alpha to 1 so compositors (Wayland/Mutter) do not treat translucent
  //     effect regions (glow/shadow/mask/backdrop) as window transparency. RmlUi composites in
  //     premultiplied alpha assuming an opaque background; the postprocess-to-FBO0 fullscreen
  //     quad can leave alpha < 1 in shadow/glow regions depending on the GPU blend path.
  //     Binding FBO0 explicitly is safe: EndFrame() restores the pre-BeginFrame binding (= 0).
  //     This masks RGB writes so only the alpha channel is touched.
  // PT: Força alpha do backbuffer = 1 para o compositor (Wayland/Mutter) não tratar regiões de
  //     efeito translúcidas (glow/sombra/mask/backdrop) como transparência de janela. RmlUi compõe
  //     em premultiplied alpha assumindo fundo opaco; o quad fullscreen postprocess→FBO0 pode deixar
  //     alpha < 1 nas regiões de sombra/brilho dependendo do caminho de blend da GPU.
  //     Bind FBO0 explícito é seguro: EndFrame() restaura o binding pré-BeginFrame (= 0).
  //     Mascara escrita RGB para tocar apenas o canal alpha.
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void RenderGl3::begin_frame_compose(int offset_x, int offset_y, int w, int h) {
  if (!impl_) return;

  // EN: L1.22-CAPTURE, COLD-START FIX (post-647350f) — ARM the backdrop capture rectangle
  //     unconditionally (cheap: 4 int stores + 1 bool reset, zero GL calls) BEFORE
  //     SetViewport()/BeginFrame() below run, i.e. while FBO 0 still holds whatever the host
  //     already drew this frame. The REAL, on-demand GL capture (glCopyTexSubImage2D and
  //     friends) now happens later, from INSIDE Context::Render() (called by the caller right
  //     after this function returns -- see Engine::render_compose, engine.cpp), the very first
  //     time a "glintfx-ripple" shader actually draws -- see
  //     Gl3RenderInterface::ArmBackdropCapture/EnsureBackdropCaptured's own doc comment
  //     (render_gl3.cpp) for the full derivation of why arming here but capturing later is both
  //     CORRECT (FBO 0 stays untouched by RmlUi across that entire window, not just up to this
  //     point) and cost-zero-when-inactive (structurally, no counter needed).
  // PT: L1.22-CAPTURE, FIX DE COLD-START (pós-647350f) — ARMA o retângulo de captura de backdrop
  //     incondicionalmente (barato: 4 stores de int + 1 reset de bool, zero chamadas GL) ANTES
  //     de SetViewport()/BeginFrame() abaixo rodarem, isto é, enquanto o FBO 0 ainda tem o que o
  //     host já desenhou neste frame. A captura GL REAL, sob demanda (glCopyTexSubImage2D e
  //     companhia) agora acontece depois, de DENTRO de Context::Render() (chamado pelo chamador
  //     logo após esta função retornar -- ver Engine::render_compose, engine.cpp), na primeira
  //     vez que um shader "glintfx-ripple" de fato desenha -- ver o próprio doc-comment de
  //     Gl3RenderInterface::ArmBackdropCapture/EnsureBackdropCaptured (render_gl3.cpp) pra
  //     derivação completa de por que armar aqui mas capturar depois é tanto CORRETO (o FBO 0
  //     fica intocado pelo RmlUi por toda essa janela, não só até este ponto) quanto
  //     custo-zero-quando-inativo (estruturalmente, sem contador necessário).
  impl_->renderer.ArmBackdropCapture(offset_x, offset_y, w, h);

  // EN: NO glClear here -- the host owns the framebuffer contents (scene already drawn).
  //     NO direct glViewport() call here either (removed, v0.2.5): SetViewport()+BeginFrame()
  //     below already issue glViewport(0,0,w,h) internally for the content-rasterisation
  //     passes (RmlUi_Renderer_GL3.cpp:857); a pre-call here was dead code, immediately
  //     overwritten. The offset-aware glViewport(offset_x,offset_y,w,h) happens INSIDE
  //     EndFrame() (see header doc comment).
  // PT: SEM glClear aqui -- o host é dono do conteúdo do framebuffer (cena já desenhada).
  //     SEM chamada direta a glViewport() aqui tampouco (removida, v0.2.5): SetViewport()+
  //     BeginFrame() abaixo já emitem glViewport(0,0,w,h) internamente pros passes de
  //     rasterização de conteúdo (RmlUi_Renderer_GL3.cpp:857); uma pré-chamada aqui era código
  //     morto, imediatamente sobrescrito. O glViewport(offset_x,offset_y,w,h) com offset
  //     acontece DENTRO de EndFrame() (ver doc-comment do header).
  impl_->renderer.SetViewport(w, h, offset_x, offset_y);
  impl_->renderer.BeginFrame();
}

void RenderGl3::end_frame_compose() {
  if (!impl_) return;
  impl_->renderer.EndFrame();
  // EN: NO FBO0 alpha-fix — that hack is for the App-owned window; in embed the host owns FBO0.
  // PT: SEM alpha-fix do FBO0 — esse hack é da janela do App; em embed o host é dono do FBO0.
}

} // namespace glintfx
