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

// EN: FX-CARVE-1 — the seam header to the optional fx module (src/fx/effects_gl3.cpp). Declares
//     FxHook + CreateGl3FxHook(); see fx_hook.hpp's own file header for the full gate mechanism.
//     <glintfx/config.hpp> is the generated build-option header (GLINTFX_MODULE_FX).
// PT: FX-CARVE-1 — o header de costura para o módulo opcional fx (src/fx/effects_gl3.cpp).
//     Declara FxHook + CreateGl3FxHook(); ver o próprio cabeçalho de arquivo de fx_hook.hpp pro
//     mecanismo de gate completo. <glintfx/config.hpp> é o header gerado de opção de build
//     (GLINTFX_MODULE_FX).
#include "fx_hook.hpp"
#include <glintfx/config.hpp>

// EN: Log — for LoadTexture's AUD-L1-PARSE hostile-asset warnings below (oversized/truncated
//     file). Dictionary.h/Rml::Get moved to src/fx/effects_gl3.cpp with the "glintfx-tint"/
//     "glintfx-ripple" CompileShader parameter unpacking that used them (FX-CARVE-1) — nothing
//     left in this file needs them; Rml::Dictionary itself is still visible for the
//     CompileShader override's own parameter type via RmlUi_Renderer_GL3.h's own
//     <RmlUi/Core/Types.h> include.
// PT: Log — para os avisos de asset hostil AUD-L1-PARSE do LoadTexture abaixo (arquivo grande
//     demais/truncado). Dictionary.h/Rml::Get migraram pra src/fx/effects_gl3.cpp junto do
//     desempacotamento de parâmetro de CompileShader "glintfx-tint"/"glintfx-ripple" que os usava
//     (FX-CARVE-1) — nada resta neste arquivo que precise deles; o próprio Rml::Dictionary
//     continua visível pro tipo de parâmetro do override de CompileShader via o próprio
//     <RmlUi/Core/Types.h> que RmlUi_Renderer_GL3.h inclui.
#include <RmlUi/Core/Log.h>

#include <climits>  // EN: INT_MAX (AUD-L1-PARSE — LoadTexture's static_cast<int>(len) overflow guard).
                    // PT: INT_MAX (AUD-L1-PARSE — guarda de overflow do static_cast<int>(len) do LoadTexture).
#include <memory>
#include <vector>

// EN: FX-CARVE-1 — the "glintfx-tint"/"glintfx-ripple" GLSL programs, CompileGlintfxStage, and
//     the Gl3FxHook class that dispatches/draws them all moved VERBATIM to
//     src/fx/effects_gl3.cpp (comments preserved) -- see that file and fx_hook.hpp for the full
//     design. This TU only keeps the `fx_` seam member (below) and the thin
//     compile/render/release/arm delegation on Gl3RenderInterface.
// PT: FX-CARVE-1 — os programas GLSL "glintfx-tint"/"glintfx-ripple", o CompileGlintfxStage, e a
//     classe Gl3FxHook que os despacha/desenha, tudo movido LITERALMENTE pra
//     src/fx/effects_gl3.cpp (comentários preservados) -- ver aquele arquivo e fx_hook.hpp pro
//     design completo. Esta TU só mantém o membro de costura `fx_` (abaixo) e a delegação fina de
//     compile/render/release/arm no Gl3RenderInterface.

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
//     FX-CARVE-1: this class ALSO delegates CompileShader/RenderShader/ReleaseShader to `fx_`
//     (an optional `std::unique_ptr<FxHook>`, see ../fx_hook.hpp) when the fx module is compiled
//     in, so it recognises the "glintfx-tint"/"glintfx-ripple" shader names -- the actual
//     tagged-wrapper dispatch (GlintfxShaderHandle) and the two GLSL programs live in
//     src/fx/effects_gl3.cpp now, not in this file. See fx_hook.hpp's own file header for the
//     module-gate mechanism and ADR-0010 Decision (a) for the full shader-injection derivation.
// PT: falhar ao abrir ou decodificar o arquivo (caso de erro / formato não suportado).
//
//     FX-CARVE-1: esta classe TAMBÉM delega CompileShader/RenderShader/ReleaseShader pra `fx_`
//     (um `std::unique_ptr<FxHook>` opcional, ver ../fx_hook.hpp) quando o módulo fx é
//     compilado, pra que reconheça os nomes de shader "glintfx-tint"/"glintfx-ripple" -- o
//     despacho por wrapper marcado de fato (GlintfxShaderHandle) e os dois programas GLSL moram
//     agora em src/fx/effects_gl3.cpp, não neste arquivo. Ver o próprio comentário de cabeçalho
//     de fx_hook.hpp pro mecanismo de gate do módulo e a Decisão (a) do ADR-0010 pra derivação
//     completa da injeção de shader.
// ---------------------------------------------------------------------------

class Gl3RenderInterface : public RenderInterface_GL3 {
public:
  // EN: FX-CARVE-1 — constructs `fx_` when the fx module is compiled in; the base
  //     RenderInterface_GL3 default-constructs first (its own ctor compiles RmlUi's native
  //     shaders — unrelated to `fx_`). fx=OFF: `fx_` stays null (default member initializer),
  //     no CreateGl3FxHook() symbol reference at all — see fx_hook.hpp's own doc comment for why
  //     this leaves zero undefined symbol at link time.
  // PT: FX-CARVE-1 — constrói `fx_` quando o módulo fx é compilado; o RenderInterface_GL3 base
  //     se constrói por padrão primeiro (o próprio ctor dele compila os shaders nativos do
  //     RmlUi — sem relação com `fx_`). fx=OFF: `fx_` fica nulo (inicializador de membro
  //     default), nenhuma referência de símbolo CreateGl3FxHook() nenhuma — ver o próprio
  //     doc-comment de fx_hook.hpp pro motivo disto deixar zero símbolo indefinido em tempo de
  //     link.
  Gl3RenderInterface() {
#if GLINTFX_MODULE_FX
    fx_ = glintfx::CreateGl3FxHook();
#endif
  }

  // EN: FX-CARVE-1 — releases `fx_` (if constructed), which in turn releases the
  //     "glintfx-tint"/"glintfx-ripple" GL programs and the backdrop-capture texture, if any
  //     were ever allocated (see src/fx/effects_gl3.cpp's Gl3FxHook::~Gl3FxHook, which carries
  //     the body this destructor used to have before the carve). `std::unique_ptr<FxHook>`'s
  //     automatic member destruction already does the right thing with fx=OFF too (`fx_` is
  //     null, nothing to release) — no explicit body needed here anymore.
  // PT: FX-CARVE-1 — libera `fx_` (se construído), que por sua vez libera os programas GL
  //     "glintfx-tint"/"glintfx-ripple" e a textura de captura de backdrop, se alguma foi
  //     alocada (ver Gl3FxHook::~Gl3FxHook de src/fx/effects_gl3.cpp, que carrega o corpo que
  //     este destrutor tinha antes da carve). A destruição automática de membro do
  //     `std::unique_ptr<FxHook>` já faz a coisa certa também com fx=OFF (`fx_` é nulo, nada a
  //     liberar) — nenhum corpo explícito é mais necessário aqui.
  ~Gl3RenderInterface() override = default;

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

  // EN: FX-CARVE-1 — CompileShader/RenderShader/ReleaseShader delegate to `fx_` when the fx
  //     module is compiled in (every call, including polygon()'s own gradient shaders,
  //     funnels through here once virtual dispatch resolves to this concrete class); with
  //     fx=OFF (`fx_` null) every call falls straight through to the base class, unwrapped and
  //     untouched. See fx_hook.hpp's own file header for the fx=ON/OFF handle-uniformity
  //     invariant this split upholds, and src/fx/effects_gl3.cpp's Gl3FxHook for the actual
  //     "glintfx-tint"/"glintfx-ripple" dispatch these three overrides used to contain directly.
  // PT: FX-CARVE-1 — CompileShader/RenderShader/ReleaseShader delegam pra `fx_` quando o módulo
  //     fx é compilado (toda chamada, inclusive os próprios shaders de gradiente do polygon(),
  //     passa por aqui uma vez que o despacho virtual resolve pra esta classe concreta); com
  //     fx=OFF (`fx_` nulo) toda chamada cai direto na classe base, sem embrulho, intocada. Ver
  //     o próprio comentário de cabeçalho de fx_hook.hpp pro invariante de uniformidade-de-
  //     handle fx=ON/OFF que esta divisão mantém, e o Gl3FxHook de src/fx/effects_gl3.cpp pro
  //     despacho de fato "glintfx-tint"/"glintfx-ripple" que estes três overrides costumavam
  //     conter diretamente.
  // -------------------------------------------------------------------------

  Rml::CompiledShaderHandle CompileShader(const Rml::String& name, const Rml::Dictionary& parameters) override {
    if (fx_) {
      // EN: QW-RIPPLEAPP (v0.18.1) -- one-time (not per-frame) warning: "glintfx-ripple" refracts
      //     the HOST's backdrop (L1.22-CAPTURE, arm_backdrop() below), which only ever runs from
      //     RenderGl3::begin_frame_compose -- the UiLayer/embed entry point. A standalone App only
      //     ever calls RenderGl3::begin_frame (never begin_frame_compose, see engine.cpp's
      //     render_standalone/render_compose split), so backdrop_armed_ is still false the first
      //     time this compiles here: the decorator is by-design inert (ADR-0012 -- there is no
      //     host backdrop for a standalone App to refract), but silently so, which is confusing
      //     for an author who copy-pasted a ripple() rule from embed-mode docs. Checked here
      //     (not in begin_frame/begin_frame_compose) because this is the one place that already
      //     sees the plain shader NAME before it gets wrapped by fx_ -- see decorator_ripple.hpp's
      //     own doc-comment and app.hpp's set_frame_callback doc-comment for the public-facing
      //     note. Guarded by warned_ripple_standalone_ so this fires AT MOST once per
      //     Gl3RenderInterface instance (one App lifetime), never once per frame -- RmlUi may
      //     recompile the same named shader more than once across a session (style recalcs), and
      //     a game loop redrawing hundreds of frames a second must not flood stderr.
      // PT: QW-RIPPLEAPP (v0.18.1) -- aviso ÚNICO (não por-frame): "glintfx-ripple" refrata o
      //     backdrop do HOST (L1.22-CAPTURE, arm_backdrop() abaixo), que só roda a partir de
      //     RenderGl3::begin_frame_compose -- o ponto de entrada UiLayer/embed. Um App standalone
      //     só chama RenderGl3::begin_frame (nunca begin_frame_compose, ver a divisão
      //     render_standalone/render_compose de engine.cpp), então backdrop_armed_ ainda é falso
      //     na primeira vez que isto compila aqui: o decorator é inerte por design (ADR-0012 --
      //     não há backdrop de host pra um App standalone refratar), mas silenciosamente, o que
      //     confunde um autor que copiou uma regra ripple() da doc de embed mode. Checado aqui
      //     (não em begin_frame/begin_frame_compose) porque este é o único ponto que já vê o NOME
      //     cru do shader antes de ser embrulhado por fx_ -- ver o próprio doc-comment de
      //     decorator_ripple.hpp e o doc-comment de App::set_frame_callback em app.hpp pra nota
      //     pública. Guardado por warned_ripple_standalone_ pra disparar NO MÁXIMO uma vez por
      //     instância de Gl3RenderInterface (uma vida de App), nunca uma vez por frame -- o RmlUi
      //     pode recompilar o mesmo shader nomeado mais de uma vez na sessão (recálculos de
      //     estilo), e um game loop redesenhando centenas de frames por segundo não pode inundar o
      //     stderr.
      if (name == "glintfx-ripple" && !backdrop_armed_ && !warned_ripple_standalone_) {
        Rml::Log::Message(Rml::Log::LT_WARNING,
                          "decorator: ripple(...) compiled, but this glintfx::App appears to be running "
                          "standalone -- ripple() refracts the HOST's backdrop and only has visible effect in "
                          "embed/UiLayer mode (ADR-0012); in a standalone App there is no host backdrop, so the "
                          "effect is inert here (logged once).");
        warned_ripple_standalone_ = true;
      }
      return fx_->compile_shader(*this, name, parameters);
    }
    return RenderInterface_GL3::CompileShader(name, parameters);
  }

  void RenderShader(Rml::CompiledShaderHandle shader_handle, Rml::CompiledGeometryHandle geometry_handle, Rml::Vector2f translation,
      Rml::TextureHandle texture) override {
    if (fx_) {
      fx_->render_shader(*this, shader_handle, geometry_handle, translation, texture);
      return;
    }
    RenderInterface_GL3::RenderShader(shader_handle, geometry_handle, translation, texture);
  }

  void ReleaseShader(Rml::CompiledShaderHandle shader_handle) override {
    if (fx_) {
      fx_->release_shader(*this, shader_handle);
      return;
    }
    RenderInterface_GL3::ReleaseShader(shader_handle);
  }

  // EN: FX-CARVE-1 — thin forwarding wrapper so RenderGl3::begin_frame_compose (below) does not
  //     need to know whether `fx_` exists; fx=OFF makes this a true no-op (RenderGl3 stays
  //     oblivious to the module gate). Replaces the direct ArmBackdropCapture(...) call this
  //     class used to expose before the carve — see fx_hook.hpp's arm_backdrop_capture doc
  //     comment for the full L1.22-CAPTURE mechanism, now owned by src/fx/effects_gl3.cpp.
  // PT: FX-CARVE-1 — wrapper fino de encaminhamento pra que RenderGl3::begin_frame_compose
  //     (abaixo) não precise saber se `fx_` existe; fx=OFF faz disto um no-op de verdade
  //     (RenderGl3 fica alheio ao gate do módulo). Substitui a chamada direta a
  //     ArmBackdropCapture(...) que esta classe expunha antes da carve — ver o doc-comment de
  //     arm_backdrop_capture de fx_hook.hpp pro mecanismo completo do L1.22-CAPTURE, agora dono
  //     de src/fx/effects_gl3.cpp.
  void arm_backdrop(int offset_x, int offset_y, int w, int h) {
    if (fx_) {
      // EN: QW-RIPPLEAPP -- records that the compose-only (embed) backdrop-arming path has run at
      //     least once on THIS instance; see CompileShader's own doc-comment above for how this
      //     flag distinguishes a standalone App (never reaches here) from a UiLayer/embed host
      //     (always reaches here, and always BEFORE the first CompileShader("glintfx-ripple") of
      //     the same frame -- begin_frame_compose calls arm_backdrop() before SetViewport()+
      //     BeginFrame(), and Context::Render() -- which triggers CompileShader -- only runs
      //     after those return, see engine.cpp's render_compose).
      // PT: QW-RIPPLEAPP -- registra que o caminho de armamento de backdrop compose-only (embed)
      //     rodou ao menos uma vez NESTA instância; ver o próprio doc-comment de CompileShader
      //     acima pra como esta flag distingue um App standalone (nunca chega aqui) de um host
      //     UiLayer/embed (sempre chega aqui, e sempre ANTES do primeiro
      //     CompileShader("glintfx-ripple") do mesmo frame -- begin_frame_compose chama
      //     arm_backdrop() antes de SetViewport()+BeginFrame(), e Context::Render() -- que dispara
      //     CompileShader -- só roda depois que eles retornam, ver engine.cpp's render_compose).
      backdrop_armed_ = true;
      fx_->arm_backdrop_capture(offset_x, offset_y, w, h);
    }
  }

private:
  // EN: FX-CARVE-1 — the seam member: owns every glintfx-authored shader/backdrop-capture
  //     resource when the fx module is compiled in (see ../fx_hook.hpp), null otherwise. See
  //     this class's own constructor/destructor above for the construction/teardown timing.
  // PT: FX-CARVE-1 — o membro de costura: dono de todo recurso de shader/captura-de-backdrop
  //     autoral da glintfx quando o módulo fx é compilado (ver ../fx_hook.hpp), nulo caso
  //     contrário. Ver o próprio construtor/destrutor desta classe acima pro timing de
  //     construção/desmonte.
  std::unique_ptr<glintfx::FxHook> fx_;

  // EN: QW-RIPPLEAPP (v0.18.1) -- one-time standalone-App ripple-inert warning state. See
  //     CompileShader's/arm_backdrop's own doc comments above for the full derivation.
  // PT: QW-RIPPLEAPP (v0.18.1) -- estado do aviso único de ripple-inerte em App standalone. Ver
  //     os próprios doc-comments de CompileShader/arm_backdrop acima pra derivação completa.
  bool backdrop_armed_ = false;
  bool warned_ripple_standalone_ = false;
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
  //     Gl3FxHook's own arm_backdrop_capture/EnsureBackdropCaptured doc comment
  //     (src/fx/effects_gl3.cpp, FX-CARVE-1) for the full derivation of why arming here but
  //     capturing later is both CORRECT (FBO 0 stays untouched by RmlUi across that entire
  //     window, not just up to this point) and cost-zero-when-inactive (structurally, no
  //     counter needed) -- with fx=OFF, `arm_backdrop` below is a true no-op (see its own doc
  //     comment further up this file).
  // PT: L1.22-CAPTURE, FIX DE COLD-START (pós-647350f) — ARMA o retângulo de captura de backdrop
  //     incondicionalmente (barato: 4 stores de int + 1 reset de bool, zero chamadas GL) ANTES
  //     de SetViewport()/BeginFrame() abaixo rodarem, isto é, enquanto o FBO 0 ainda tem o que o
  //     host já desenhou neste frame. A captura GL REAL, sob demanda (glCopyTexSubImage2D e
  //     companhia) agora acontece depois, de DENTRO de Context::Render() (chamado pelo chamador
  //     logo após esta função retornar -- ver Engine::render_compose, engine.cpp), na primeira
  //     vez que um shader "glintfx-ripple" de fato desenha -- ver o próprio doc-comment de
  //     arm_backdrop_capture/EnsureBackdropCaptured do Gl3FxHook (src/fx/effects_gl3.cpp,
  //     FX-CARVE-1) pra derivação completa de por que armar aqui mas capturar depois é tanto
  //     CORRETO (o FBO 0 fica intocado pelo RmlUi por toda essa janela, não só até este ponto)
  //     quanto custo-zero-quando-inativo (estruturalmente, sem contador necessário) -- com
  //     fx=OFF, `arm_backdrop` abaixo é um no-op de verdade (ver o próprio doc-comment mais
  //     acima neste arquivo).
  impl_->renderer.arm_backdrop(offset_x, offset_y, w, h);

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
