// SPDX-License-Identifier: MPL-2.0
// EN: Implementation of RenderGl3 — wraps RmlUi's RenderInterface_GL3.
//     gl3w must be included before any other GL header.
// PT: Implementação de RenderGl3 — encapsula o RenderInterface_GL3 do RmlUi.
//     gl3w deve ser incluído antes de qualquer outro header GL.

// EN: gl3w FIRST — defines all GL 3.x function pointers loaded at runtime.
// PT: gl3w PRIMEIRO — define todos os ponteiros de função GL 3.x carregados em tempo de execução.
#include <GL/gl3w.h>

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

#include <memory>
#include <vector>

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
// ---------------------------------------------------------------------------
class Gl3RenderInterface : public RenderInterface_GL3 {
public:
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

void RenderGl3::begin_frame_compose(int w, int h) {
  if (!impl_) return;
  // EN: NO glClear here — the host owns the framebuffer contents (scene already drawn).
  // PT: SEM glClear aqui — o host é dono do conteúdo do framebuffer (cena já desenhada).
  //
  // EN: Viewport origin is hardcoded to (0, 0): the UI composites from the bottom-left corner
  //     of the window. Sub-region compositing (e.g. a UI panel at a pixel offset) is not
  //     supported in the F1 contract; the host must use a full-window render target.
  // PT: Origem do viewport é hardcoded em (0, 0): a UI compõe a partir do canto inferior esquerdo
  //     da janela. Composição em sub-região (ex.: painel UI em offset de pixel) não é suportada
  //     no contrato F1; o host deve usar um render target de janela inteira.
  glViewport(0, 0, w, h);
  impl_->renderer.SetViewport(w, h);
  impl_->renderer.BeginFrame();
}

void RenderGl3::end_frame_compose() {
  if (!impl_) return;
  impl_->renderer.EndFrame();
  // EN: NO FBO0 alpha-fix — that hack is for the App-owned window; in embed the host owns FBO0.
  // PT: SEM alpha-fix do FBO0 — esse hack é da janela do App; em embed o host é dono do FBO0.
}

} // namespace glintfx
