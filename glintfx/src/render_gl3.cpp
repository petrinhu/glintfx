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

// EN: Discriminated union returned (as an opaque Rml::CompiledShaderHandle, via reinterpret_cast)
//     by every Gl3RenderInterface::CompileShader call — is_tint=false for every pre-existing
//     shader kind (RmlUi's own filters/gradients/creation shaders, including polygon()'s reused
//     gradient shaders), is_tint=true only for "glintfx-tint". See CompileShader/RenderShader/
//     ReleaseShader below for the unwrap-and-delegate-or-draw split.
// PT: União discriminada retornada (como um Rml::CompiledShaderHandle opaco, via
//     reinterpret_cast) por toda chamada de Gl3RenderInterface::CompileShader — is_tint=false
//     pra todo tipo de shader pré-existente (os próprios filtros/gradientes/shaders de creation
//     do RmlUi, inclusive os shaders de gradiente reusados pelo polygon()), is_tint=true só pro
//     "glintfx-tint". Ver CompileShader/RenderShader/ReleaseShader abaixo pra divisão
//     desembrulha-e-delega-ou-desenha.
struct GlintfxShaderHandle {
  bool is_tint = false;
  Rml::CompiledShaderHandle base_handle = 0;  // valid only if !is_tint.
  GlintfxTintShaderData tint_data;            // valid only if is_tint.
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
  ~Gl3RenderInterface() override {
    if (tint_program_)
      glDeleteProgram(tint_program_);
  }

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

    // EN: Read entire file into a buffer.
    // PT: Lê o arquivo inteiro em um buffer.
    fi->Seek(fh, 0, SEEK_END);
    size_t len = fi->Tell(fh);
    fi->Seek(fh, 0, SEEK_SET);
    std::vector<unsigned char> buf(len);
    size_t nread = fi->Read(buf.data(), len, fh);
    fi->Close(fh);
    if (nread != len) {
      // EN: Short read — file truncated or I/O error; fallback to base.
      // PT: Leitura incompleta — arquivo truncado ou erro de I/O; fallback para base.
      return RenderInterface_GL3::LoadTexture(dims, source);
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

    // EN: Every OTHER shader name (RmlUi's own filters/gradients/creation shaders, INCLUDING
    //     polygon()'s reused "radial-gradient"/"linear-gradient" — see decorator_polygon.cpp) —
    //     delegate unchanged to the base class, then wrap the result too (is_tint=false) so
    //     RenderShader/ReleaseShader can uniformly unwrap every Rml::CompiledShaderHandle this
    //     class ever hands out.
    // PT: Todo OUTRO nome de shader (os próprios filtros/gradientes/shaders de creation do
    //     RmlUi, INCLUSIVE os "radial-gradient"/"linear-gradient" reusados pelo polygon() — ver
    //     decorator_polygon.cpp) — delega inalterado pra classe base, depois embrulha o
    //     resultado também (is_tint=false) pra que RenderShader/ReleaseShader possam desembrulhar
    //     uniformemente todo Rml::CompiledShaderHandle que esta classe já entregou.
    Rml::CompiledShaderHandle base = RenderInterface_GL3::CompileShader(name, parameters);
    if (!base)
      return 0;
    auto* wrapper = new GlintfxShaderHandle{};
    wrapper->is_tint = false;
    wrapper->base_handle = base;
    return reinterpret_cast<Rml::CompiledShaderHandle>(wrapper);
  }

  void RenderShader(Rml::CompiledShaderHandle shader_handle, Rml::CompiledGeometryHandle geometry_handle, Rml::Vector2f translation,
      Rml::TextureHandle texture) override {
    auto* wrapper = reinterpret_cast<GlintfxShaderHandle*>(shader_handle);
    if (!wrapper)
      return;

    if (!wrapper->is_tint) {
      RenderInterface_GL3::RenderShader(wrapper->base_handle, geometry_handle, translation, texture);
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
    //     (wrapper->tint_data.vao and friends) are NEVER deleted here — they are owned by
    //     decorator_image_tint.cpp's ImageTintElementData (per-element, released in
    //     ReleaseElementData), NOT by this per-CompileShader-call wrapper — see ADR-0010
    //     Decision (a)'s explicit warning about this ownership split being the one place a
    //     double-free/use-after-free could sneak in.
    // PT: Divisão delega-ou-libera-só-a-tag, espelha CompileShader. A VAO/VBO/EBO
    //     (wrapper->tint_data.vao e companhia) NUNCA são deletadas aqui — são donas de
    //     ImageTintElementData de decorator_image_tint.cpp (por-elemento, liberada em
    //     ReleaseElementData), NÃO deste wrapper por-chamada-de-CompileShader — ver o aviso
    //     explícito da Decisão (a) do ADR-0010 sobre essa divisão de posse ser o único lugar em
    //     que um double-free/use-after-free poderia se infiltrar.
    if (!wrapper->is_tint)
      RenderInterface_GL3::ReleaseShader(wrapper->base_handle);
    delete wrapper;
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
