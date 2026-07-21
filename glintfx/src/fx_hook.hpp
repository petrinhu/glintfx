// SPDX-License-Identifier: MPL-2.0
// EN: FX-CARVE-1 -- seam header between the ui module (render_gl3.cpp) and the optional fx
//     module (src/fx/effects_gl3.cpp). Declares the abstract FxHook interface -- the full
//     surface of glintfx-AUTHORED shader effects (tint + ripple, see ADR-0010/ADR-0012) that
//     Gl3RenderInterface's CompileShader/RenderShader/ReleaseShader overrides delegate to, plus
//     the backdrop-capture "arm" entry point (L1.22-CAPTURE). Owned by ui (this header lives in
//     src/, not src/fx/) since ui is the sole caller; the CONCRETE implementation (Gl3FxHook) is
//     owned by the fx module and defined only in src/fx/effects_gl3.cpp, returned via the
//     CreateGl3FxHook() factory declared at the bottom of this file.
//
//     GLINTFX_MODULE_FX=OFF gate: render_gl3.cpp only ever calls CreateGl3FxHook() under
//     `#if GLINTFX_MODULE_FX` (see Gl3RenderInterface's constructor in render_gl3.cpp) -- with
//     the module OFF, that call site does not exist, so the factory symbol is never referenced
//     and never needs a definition; no undefined-symbol at link time despite this header being
//     included unconditionally (a pure interface declaration costs nothing to leave visible).
//
//     Fx=ON invariant: every Rml::CompiledShaderHandle Gl3RenderInterface ever hands out is a
//     Gl3FxHook-owned wrapper (see effects_gl3.cpp's GlintfxShaderHandle). Fx=OFF invariant:
//     every handle is RenderInterface_GL3's own, untouched. NEVER mixed within a single build --
//     see render_gl3.cpp's CompileShader/RenderShader/ReleaseShader overrides, which route
//     unconditionally through fx_ (either always present or always null for the build's whole
//     lifetime, never toggled at runtime).
// PT: FX-CARVE-1 -- header de costura entre o módulo ui (render_gl3.cpp) e o módulo opcional fx
//     (src/fx/effects_gl3.cpp). Declara a interface abstrata FxHook -- a superfície inteira de
//     efeitos de shader AUTORAIS da glintfx (tint + ripple, ver ADR-0010/ADR-0012) para a qual os
//     overrides de CompileShader/RenderShader/ReleaseShader do Gl3RenderInterface delegam, mais o
//     ponto de entrada "arm" da captura de backdrop (L1.22-CAPTURE). Dono é a ui (este header
//     mora em src/, não em src/fx/) já que a ui é a única chamadora; a implementação CONCRETA
//     (Gl3FxHook) é dona do módulo fx e só é definida em src/fx/effects_gl3.cpp, devolvida pela
//     factory CreateGl3FxHook() declarada no fim deste arquivo.
//
//     Gate GLINTFX_MODULE_FX=OFF: render_gl3.cpp só chama CreateGl3FxHook() sob
//     `#if GLINTFX_MODULE_FX` (ver o construtor de Gl3RenderInterface em render_gl3.cpp) -- com
//     o módulo OFF, esse call site nem existe, então o símbolo da factory nunca é referenciado e
//     nunca precisa de definição; nenhum símbolo indefinido em tempo de link apesar deste header
//     ser incluído incondicionalmente (uma declaração de interface pura não custa nada deixar
//     visível).
//
//     Invariante fx=ON: todo Rml::CompiledShaderHandle que o Gl3RenderInterface entrega é um
//     wrapper dono do Gl3FxHook (ver GlintfxShaderHandle de effects_gl3.cpp). Invariante fx=OFF:
//     todo handle é cru do próprio RenderInterface_GL3, intocado. NUNCA misto dentro de um único
//     build -- ver os overrides de CompileShader/RenderShader/ReleaseShader de render_gl3.cpp,
//     que roteiam incondicionalmente por fx_ (sempre presente ou sempre nulo pela vida inteira do
//     build, nunca alternado em runtime).
#pragma once

#include <RmlUi/Core/Types.h>

#include <memory>

// EN: Forward declaration only -- the concrete type lives in Backends/RmlUi_Renderer_GL3.h
//     (RmlUi's FetchContent tree), included by render_gl3.cpp (which defines Gl3RenderInterface)
//     and by effects_gl3.cpp (which needs the full definition to call the protected/public base
//     members the methods below take a reference to -- GetTransform()/ResetProgram(), ADR-0010
//     Decision (a)). This header itself never dereferences RenderInterface_GL3, so a forward
//     declaration is enough here.
// PT: Só forward declaration -- o tipo concreto mora em Backends/RmlUi_Renderer_GL3.h (árvore
//     FetchContent do RmlUi), incluído por render_gl3.cpp (que define Gl3RenderInterface) e por
//     effects_gl3.cpp (que precisa da definição completa pra chamar os membros protected/public
//     da base aos quais os métodos abaixo recebem referência -- GetTransform()/ResetProgram(),
//     Decisão (a) do ADR-0010). Este header em si nunca desreferencia RenderInterface_GL3, então
//     uma forward declaration basta aqui.
class RenderInterface_GL3;

namespace glintfx {

// EN: Abstract seam -- full takeover of the glintfx-authored shader pipeline (tint + ripple)
//     plus backdrop-capture arming. See render_gl3.cpp's Gl3RenderInterface::CompileShader/
//     RenderShader/ReleaseShader for the call sites (every call unconditionally delegates when
//     fx_ is non-null) and effects_gl3.cpp's Gl3FxHook for the only implementation.
// PT: Costura abstrata -- takeover completo do pipeline de shader autoral da glintfx (tint +
//     ripple) mais o armamento da captura de backdrop. Ver Gl3RenderInterface::CompileShader/
//     RenderShader/ReleaseShader de render_gl3.cpp pros call sites (toda chamada delega
//     incondicionalmente quando fx_ não é nulo) e o Gl3FxHook de effects_gl3.cpp pra única
//     implementação.
class FxHook {
public:
  virtual ~FxHook() = default;

  // EN: Recognises "glintfx-tint"/"glintfx-ripple" and handles them directly (see
  //     effects_gl3.cpp); every other name is delegated unchanged to
  //     `ri.RenderInterface_GL3::CompileShader(name, parameters)` and re-wrapped so
  //     render_shader/release_shader can tell the two cases apart uniformly (ADR-0010 Decision
  //     (a)).
  // PT: Reconhece "glintfx-tint"/"glintfx-ripple" e os trata diretamente (ver effects_gl3.cpp);
  //     todo outro nome é delegado inalterado a `ri.RenderInterface_GL3::CompileShader(name,
  //     parameters)` e reembrulhado pra que render_shader/release_shader distingam os dois casos
  //     de forma uniforme (Decisão (a) do ADR-0010).
  virtual Rml::CompiledShaderHandle compile_shader(RenderInterface_GL3& ri, const Rml::String& name,
      const Rml::Dictionary& parameters) = 0;

  // EN: Unwraps `shader_handle`; draws tint/ripple directly (glintfx-owned VAO, never
  //     `geometry_handle`) or delegates the base case via the qualified call
  //     `ri.RenderInterface_GL3::RenderShader(...)` (suppresses virtual dispatch back into this
  //     same override -- ADR-0010 Decision (a)).
  // PT: Desembrulha `shader_handle`; desenha tint/ripple diretamente (VAO dona da glintfx, nunca
  //     `geometry_handle`) ou delega o caso base via a chamada qualificada
  //     `ri.RenderInterface_GL3::RenderShader(...)` (suprime o despacho virtual de volta pra
  //     este mesmo override -- Decisão (a) do ADR-0010).
  virtual void render_shader(RenderInterface_GL3& ri, Rml::CompiledShaderHandle shader_handle,
      Rml::CompiledGeometryHandle geometry_handle, Rml::Vector2f translation, Rml::TextureHandle texture) = 0;

  // EN: Delegate-or-free-tag-only split mirroring compile_shader -- never deletes the
  //     glintfx-owned VAO/VBO/EBO (owned per-element by the decorators, not by the shader
  //     handle -- ADR-0010 Decision (a)'s explicit double-free/UAF warning).
  // PT: Divisão delega-ou-libera-só-a-tag espelhando compile_shader -- nunca deleta a VAO/VBO/
  //     EBO dona da glintfx (dona por-elemento dos decorators, não do shader handle -- aviso
  //     explícito de double-free/UAF da Decisão (a) do ADR-0010).
  virtual void release_shader(RenderInterface_GL3& ri, Rml::CompiledShaderHandle shader_handle) = 0;

  // EN: L1.22-CAPTURE cold-start fix's cheap "arm" step -- called unconditionally, once per
  //     frame, from RenderGl3::begin_frame_compose (render_gl3.cpp) via
  //     Gl3RenderInterface::arm_backdrop, BEFORE SetViewport()/BeginFrame() run (i.e. while
  //     FBO 0 still holds whatever the host already drew this frame). Just remembers the capture
  //     rectangle for the real, on-demand capture inside render_shader's ripple branch -- see
  //     effects_gl3.cpp's ArmBackdropCapture/EnsureBackdropCaptured doc comment for the full
  //     derivation (ADR-0012).
  // PT: Passo "arm" barato do fix de cold-start do L1.22-CAPTURE -- chamado incondicionalmente,
  //     uma vez por frame, a partir de RenderGl3::begin_frame_compose (render_gl3.cpp) via
  //     Gl3RenderInterface::arm_backdrop, ANTES de SetViewport()/BeginFrame() rodarem (isto é,
  //     enquanto o FBO 0 ainda tem o que o host já desenhou neste frame). Só lembra o retângulo
  //     de captura pra captura real, sob demanda, dentro do ramo de ripple de render_shader --
  //     ver o doc-comment de ArmBackdropCapture/EnsureBackdropCaptured de effects_gl3.cpp pra
  //     derivação completa (ADR-0012).
  virtual void arm_backdrop_capture(int offset_x, int offset_y, int w, int h) = 0;
};

// EN: Factory -- defined in src/fx/effects_gl3.cpp (fx module). Only ever called from
//     render_gl3.cpp under `#if GLINTFX_MODULE_FX`, so fx=OFF never leaves an undefined-symbol
//     reference at link time (see this file's own header comment).
// PT: Factory -- definida em src/fx/effects_gl3.cpp (módulo fx). Só chamada de render_gl3.cpp
//     sob `#if GLINTFX_MODULE_FX`, então fx=OFF nunca deixa uma referência de símbolo indefinido
//     em tempo de link (ver o próprio comentário de cabeçalho deste arquivo).
std::unique_ptr<FxHook> CreateGl3FxHook();

} // namespace glintfx
