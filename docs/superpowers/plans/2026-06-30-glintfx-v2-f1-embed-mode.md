# glintfx v2 · F1 — embed/guest mode (`UiLayer`) — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Entregar o **keystone da v2** ([ADR-0008](../adr/0008-embed-guest-mode.md)): extrair de `App` um **núcleo de motor reusável** (RmlUi `Bootstrap` + `RenderGl3`) desacoplado do `WindowGlfw`, e expor `glintfx::UiLayer` — uma fachada pública que **anexa a um contexto GL já corrente do host** (NÃO cria janela), compõe a UI **por cima** sem limpar nem dar swap, com **save/restore de estado GL** e **injeção de eventos** neutra (sem depender de GLFW/SDL). 1º consumidor: GusWorld (SDL3 + `Render2dGl3` próprios) — ver [ADR-010 do GusWorld](../../../gusworld/docs/tech/adr/ADR-010-adopt-glintfx-embed-mode.md).

**Architecture:** Hoje `App` possui `WindowGlfw` + `RenderGl3` + `Bootstrap` e dirige o frame (clear→UI→alpha-fix→swap). A F1 fatora `RenderGl3`+`Bootstrap` num `Engine` interno (sem janela) com dois caminhos de frame: **standalone** (clear + alpha-fix no FBO0, usado por `App` — comportamento idêntico) e **compose** (sem clear/swap, com `GlStateGuard`, usado por `UiLayer`). O acoplamento GLFW sai do `Bootstrap` (que passa a receber um `Rml::SystemInterface*` do chamador): `App` injeta `SystemInterface_GLFW`; `UiLayer` injeta um `SystemClock` mínimo (só relógio, sem GLFW). Os dois modos de consumo do spec (§3) compartilham um único `Engine`.

**Tech Stack:** C++ (CMake ≥3.16), RmlUi @`2cd28864ae25ed345b70598751703a5433b12356`, gl3w vendorizado, GLFW (só no caminho `App`/harness de teste), OpenGL 3.3 core, FreeType. Testes sob Xvfb (`tests/run_xvfb.cmake`).

**Referências:** spec v2 §3–4 (`../specs/2026-06-30-glintfx-v2-design.md`), [ADR-0008](../adr/0008-embed-guest-mode.md). Código atual lido: `glintfx/src/app.cpp`, `bootstrap.{hpp,cpp}`, `render_gl3.{hpp,cpp}`, `window_glfw.{hpp,cpp}`, `include/glintfx/app.hpp`, `tests/`.

## Global Constraints

(Cada task herda implicitamente esta seção.)

- **Plataforma:** Linux x86-64 apenas.
- **Padrão C++:** alvo `cxx_std_23`; **piso C++17** (a API pública compila em C++17→23).
- **Licença:** `SPDX-License-Identifier: MPL-2.0` no topo de **todo** arquivo novo (`.hpp`/`.cpp` com `//`, `CMakeLists.txt` com `#`). © 2026 Petrus Silva Costa.
- **Idioma:** identificadores **en-intl**; comentários/doc-comments **bilíngues** (en, depois pt).
- **Encapsulamento (gate por grep):** **nenhum** tipo de RmlUi/GLFW/GL/gl3w/SDL pode aparecer em `include/glintfx/*.hpp`. `UiLayer`/`UiEvent`/`Key` usam só tipos neutros próprios (pImpl). Verificável: `! grep -rE 'Rml|GLFW|GL_|gl3w|SDL' glintfx/include/glintfx/`.
- **Invariante de não-regressão:** a `App` standalone (v1) **continua idêntica** — os 5 testes atuais (`window_smoke`, `render_smoke`, `bootstrap_smoke`, `app_smoke`, `render_sanity`) seguem **verdes** sem mudar de expectativa.
- **Estado global do RmlUi (N3, ADR-0008):** `Rml::Initialise/Shutdown` é processo-global e sem refcount. **Só uma** instância viva de `App` **ou** `UiLayer` por processo. Nenhum teste constrói `App` e `UiLayer` simultaneamente.
- **Embed = host é dono:** `UiLayer` **nunca** chama `glClear` do framebuffer do host, **nunca** dá swap, **nunca** cria janela. O host limpa, desenha a cena, chama `UiLayer::render()` e faz o swap.
- **Anti-OE / escopo F1:** entregar SÓ o embed mode + harness de teste. **Sem** componentes, **sem** tokens, **sem** alvo `glintfx::ui` (isso é F2). Sem backend SDL dentro do glintfx (o host traz o seu).
- **Deps pinadas:** RmlUi commit `2cd28864ae25ed345b70598751703a5433b12356`; gl3w vendorizado em `third_party/gl3w/`.
- **Testes de GL:** rodam sob `xvfb-run` via `tests/run_xvfb.cmake`.

---

## File Structure

```
glintfx/
  CMakeLists.txt                       # + engine.cpp, system_clock.cpp, ui_layer.cpp na lib
  include/glintfx/
    glintfx.hpp                        # umbrella — passa a incluir ui_layer.hpp
    app.hpp                            # [INTACTO] fachada standalone v1
    ui_event.hpp        [NOVO]         # UiEvent + enum Key/Mod neutros (sem tipos de terceiros)
    ui_layer.hpp        [NOVO]         # fachada pública embed (pImpl)
  src/
    app.cpp                            # refatorado: App = WindowGlfw + SystemInterface_GLFW + Engine
    bootstrap.hpp / .cpp               # init() passa a receber Rml::SystemInterface* (sem WindowGlfw)
    render_gl3.hpp / .cpp              # + begin_frame_compose/end_frame_compose (sem clear/sem alpha-fix)
    engine.hpp / .cpp   [NOVO]         # núcleo reusável: Bootstrap + RenderGl3, 2 caminhos de frame
    gl_state.hpp        [NOVO]         # GlStateGuard (RAII save/restore de estado GL)
    system_clock.hpp / .cpp [NOVO]     # SystemClock : Rml::SystemInterface (só relógio; sem GLFW)
    ui_layer.cpp        [NOVO]         # impl da fachada embed + tradução UiEvent→Rml
  tests/
    offscreen.hpp       [NOVO]         # harness: contexto GL oculto + FBO offscreen + readback
    engine_smoke.cpp    [NOVO]         # (substitui o uso direto de Bootstrap no bootstrap_smoke)
    ui_layer_attach.cpp [NOVO]         # T2: attach/load/viewport
    ui_layer_compose.cpp[NOVO]         # T3: compose-only preserva o fundo do host (FBO)
    gl_state_guard.cpp  [NOVO]         # T3: estado GL antes == depois do render()
    ui_layer_events.cpp [NOVO]         # T4: injeção de eventos sem crash
    ui_layer_sanity.cpp [NOVO]         # T5: embed renderiza UI no FBO (estatística agregada)
    CMakeLists.txt                     # registra os novos testes (run_xvfb.cmake)
```

Mapeamento com o spec v2: F1 = §4 (embed mode). `glintfx::ui` (§9) e componentes (§6) ficam para F2. Faseamento §12: este plano é a **F1**.

---

### Task 1: Extrair `Engine` de `App` (sem mudar comportamento) + desacoplar GLFW do `Bootstrap`

**Files:**
- Create: `glintfx/src/engine.hpp`, `glintfx/src/engine.cpp`, `glintfx/src/system_clock.hpp`, `glintfx/src/system_clock.cpp`
- Modify: `glintfx/src/bootstrap.hpp`, `glintfx/src/bootstrap.cpp` (assinatura de `init`), `glintfx/src/app.cpp` (passa a usar `Engine`), `glintfx/CMakeLists.txt`, `glintfx/tests/CMakeLists.txt`
- Create: `glintfx/tests/engine_smoke.cpp`; Modify/replace: `glintfx/tests/bootstrap_smoke.cpp` (atualiza para a nova assinatura)

**Interfaces:**
- `Bootstrap::init(Rml::SystemInterface* system, RenderGl3& render, int w, int h)` — não recebe mais `WindowGlfw&`; **não** possui `system` (o chamador é dono).
- `glintfx::Engine` (interno): possui `RenderGl3` + `Bootstrap`; `bool attach(Rml::SystemInterface*, int w, int h)`, `bool ok()`, `bool load(const char*)`, `void set_viewport(int,int)`, `void update()`, `void render_standalone(int,int)`, `void render_compose(int,int)` (T3), `Rml::Context* context()`.
- `glintfx::SystemClock : Rml::SystemInterface` — só `GetElapsedTime()` via `std::chrono`.

- [x] **Step 1: `system_clock.hpp/.cpp` — SystemInterface mínimo (sem GLFW)**

`glintfx/src/system_clock.hpp`:
```cpp
// SPDX-License-Identifier: MPL-2.0
// EN: Minimal RmlUi SystemInterface for embed mode — provides only the clock.
//     No GLFW/SDL dependency (the host owns window/input).
// PT: SystemInterface mínimo do RmlUi para embed — provê só o relógio.
//     Sem dependência de GLFW/SDL (o host é dono de janela/input).
#pragma once
#include <RmlUi/Core/SystemInterface.h>
#include <chrono>
namespace glintfx {
class SystemClock final : public Rml::SystemInterface {
public:
  SystemClock() : start_(std::chrono::steady_clock::now()) {}
  double GetElapsedTime() override {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
  }
private:
  std::chrono::steady_clock::time_point start_;
};
}
```
(`system_clock.cpp` pode ser vazio com só o SPDX, ou inline-only; manter `.cpp` na lib para um TU dono do tipo se preferir — opcional.)

- [x] **Step 2: Desacoplar `Bootstrap` do `WindowGlfw`**

Em `bootstrap.hpp`: trocar o forward-decl e a assinatura.
```cpp
namespace Rml { class Context; class SystemInterface; }
// ...
bool init(Rml::SystemInterface* system, RenderGl3& render, int w, int h);
```
Em `bootstrap.cpp`: remover `#include "window_glfw.hpp"` e o `#include "RmlUi_Platform_GLFW.h"`; `Impl` **não** possui mais `SystemInterface_GLFW*` (o chamador é dono). `init` faz:
```cpp
bool Bootstrap::init(Rml::SystemInterface* system, RenderGl3& render, int w, int h) {
  if (impl_) return false;
  impl_ = new Impl();
  Rml::SetSystemInterface(system);
  Rml::SetRenderInterface(render.iface());
  if (!Rml::Initialise()) { delete impl_; impl_ = nullptr; return false; }
  impl_->initialised = true;
  impl_->ctx = Rml::CreateContext("main", Rml::Vector2i(w, h));
  if (!impl_->ctx) { Rml::Shutdown(); delete impl_; impl_ = nullptr; return false; }
  return true;
}
```
`shutdown()` deixa de dar `delete impl_->system` (não é mais dono).

- [x] **Step 3: `engine.hpp/.cpp` — núcleo reusável**

`engine.hpp` (forward-decls só; sem tipos concretos de terceiros):
```cpp
// SPDX-License-Identifier: MPL-2.0
#pragma once
namespace Rml { class Context; class SystemInterface; }
namespace glintfx {
class Engine {
public:
  Engine(); ~Engine();
  Engine(const Engine&) = delete; Engine& operator=(const Engine&) = delete;
  bool attach(Rml::SystemInterface* system, int w, int h); // requer contexto GL corrente
  bool ok() const noexcept;
  bool load(const char* rml_path);
  void set_viewport(int w, int h);
  void update();
  void render_standalone(int w, int h); // clear + UI + alpha-fix FBO0 (App)
  void render_compose(int w, int h);    // T3: sem clear/sem swap, com GlStateGuard (UiLayer)
  Rml::Context* context();
private:
  struct Impl; Impl* impl_;
};
}
```
`engine.cpp`: `Impl { RenderGl3 render; Bootstrap boot; bool ok=false; }`. `attach` = `render.init()` então `boot.init(system, render, w, h)`; `ok = render.iface() && boot.context()`. `render_standalone(w,h)` = `render.begin_frame(w,h); if(auto*c=boot.context())c->Render(); render.end_frame();` (idêntico ao frame atual do `App`, menos o swap). `render_compose` fica como stub em T1 (corpo real em T3) — pode chamar `render_standalone` temporariamente? **Não**: para não enganar, deixar `render_compose` como `// implementado em T3` lançando/no-op documentado; nenhum teste o exercita até T3.

- [x] **Step 4: Refatorar `App` para usar `Engine`**

`app.cpp`: `Impl { WindowGlfw window; SystemInterface_GLFW* system=nullptr; Engine engine; int w,h; bool ok; }`. No ctor: `window.create(...)`; `engine`/render exigem contexto corrente (a janela GLFW já o torna corrente em `create`); criar `system = new SystemInterface_GLFW(window.handle())`; `ok = engine.attach(system, w, h)`. `render()` = `int w,h; window.size(w,h); engine.render_standalone(w,h); window.swap();`. `update()` = `engine.update()`. `load()` = `engine.load()`. `snapshot()` = `engine.render_standalone(w,h);` então o mesmo bloco de `glReadPixels` do FBO0 **antes** de `window.swap()` (inalterado). Dtor: ordem `engine` (RmlUi) → `delete system` → `window`. Manter os guards `ok()` existentes.
> O include de `RmlUi_Platform_GLFW.h` migra de `bootstrap.cpp` para `app.cpp` (é onde o `SystemInterface_GLFW` agora vive).

- [x] **Step 5: Atualizar testes que tocavam `Bootstrap` direto**

`bootstrap_smoke.cpp` chamava `Bootstrap::init(win, render, w, h)`. Reescrever como `engine_smoke.cpp` (mantendo a cobertura: init RmlUi + contexto + load):
```cpp
// SPDX-License-Identifier: MPL-2.0
#include "../src/window_glfw.hpp"
#include "../src/engine.hpp"
#include "../src/system_clock.hpp"
#include <cstdio>
int main() {
  glintfx::WindowGlfw w;
  if (!w.create("engine", 320, 240)) return 1;   // janela = contexto GL corrente
  glintfx::SystemClock clock;
  glintfx::Engine e;
  if (!e.attach(&clock, 320, 240)) { std::puts("engine attach failed"); return 2; }
  if (!e.load("tests/min.rml")) { std::puts("load failed"); return 3; }
  std::puts("engine smoke OK");
  return 0;
}
```
No `tests/CMakeLists.txt`: remover o alvo `bootstrap_smoke` e registrar `engine_smoke` (mesmo padrão `run_xvfb.cmake` + `WORKING_DIRECTORY ${CMAKE_BINARY_DIR}` + `configure_file` do `min.rml`). `render_smoke` (usa só `RenderGl3::begin_frame/end_frame`) **não muda**.

- [x] **Step 6: Buildar e rodar TODOS os testes — v1 verde + engine_smoke**

Run: `cd glintfx && cmake -S . -B build -DGLINTFX_BUILD_DEMOS=OFF && cmake --build build -j 2>&1 | tail -5`
Run: `ctest --test-dir build --output-on-failure`
Expected: `window_smoke`, `render_smoke`, `engine_smoke`, `app_smoke`, `render_sanity` **passam**. `render_sanity` segue imprimindo `render_sanity: PASS` (prova que o comportamento do `App` não regrediu).

- [x] **Step 7: Gate de encapsulamento + commit**

Run: `! grep -rqE 'Rml|GLFW|GL_|gl3w|SDL' glintfx/include/glintfx/ && echo "headers limpos"`
```bash
git add glintfx/ docs/ && git commit -m "refactor(glintfx): extrai Engine reusável de App; Bootstrap recebe SystemInterface (F1 T1)"
```

---

### Task 2: `UiLayer` — attach a contexto corrente + load + viewport

**Files:**
- Create: `glintfx/include/glintfx/ui_layer.hpp`, `glintfx/src/ui_layer.cpp`
- Modify: `glintfx/include/glintfx/glintfx.hpp` (umbrella inclui `ui_layer.hpp`), `glintfx/CMakeLists.txt` (add `ui_layer.cpp`), `glintfx/tests/CMakeLists.txt`
- Create: `glintfx/tests/ui_layer_attach.cpp`

**Interfaces:**
- `glintfx::UiLayer` público (pImpl): `Config{int logical_width=1280, logical_height=720; bool load_gl=true;}`, ctor `explicit UiLayer(Config={})`, move-only, `bool ok()`, `void load(const char*)`, `void set_viewport(int,int)`, `void update()`, `void render()` (T3), `void process_event(const UiEvent&)` (T4).

- [x] **Step 1: Header público `ui_layer.hpp` (sem tipos de terceiros)**
```cpp
// SPDX-License-Identifier: MPL-2.0
// EN: Embed/guest facade — attaches the UI+effects engine to a GL context the HOST owns.
//     Does not create a window; render() is compose-only (no clear, no swap).
// PT: Fachada embed/guest — anexa o motor de UI+efeitos a um contexto GL do HOST.
//     Não cria janela; render() é compose-only (sem clear, sem swap).
#pragma once
#include <memory>
#include <glintfx/ui_event.hpp>
namespace glintfx {
class UiLayer {
public:
  struct Config { int logical_width = 1280; int logical_height = 720; bool load_gl = true; };
  explicit UiLayer(Config cfg = {});   // EN: requires a CURRENT GL context. PT: exige contexto GL corrente.
  ~UiLayer();
  UiLayer(UiLayer&&) noexcept; UiLayer& operator=(UiLayer&&) noexcept;
  UiLayer(const UiLayer&) = delete; UiLayer& operator=(const UiLayer&) = delete;
  bool ok() const noexcept;               // EN: attach + RmlUi init succeeded.
  void load(const char* rml_path);
  void set_viewport(int w, int h);        // EN: real target pixels (resize).
  void process_event(const UiEvent& ev);  // EN: host-injected input (T4).
  void update();
  void render();                          // EN: compose-only over current FBO (T3).
private:
  struct Impl; std::unique_ptr<Impl> impl_;
};
}
```
(O `ui_event.hpp` completo entra na Task 4; nesta task criar um stub mínimo só com `struct UiEvent;` forward, ou já criar o header neutro vazio — preferir criar o header neutro agora com a struct definida em T4. Para T2 compilar, declarar `struct UiEvent;` no header e não usar.)
> Ajuste anti-dependência circular: em T2, `ui_layer.hpp` faz `struct UiEvent;` (forward) e `process_event` recebe `const UiEvent&`; o `ui_event.hpp` com a definição chega na T4 e o umbrella passa a incluí-lo.

- [x] **Step 2: Teste de attach (falha primeiro — `ui_layer.cpp` ausente)**

`tests/ui_layer_attach.cpp`:
```cpp
// SPDX-License-Identifier: MPL-2.0
// EN: The HOST owns the GL context (hidden GLFW window here stands in for SDL3 in GusWorld).
// PT: O HOST é dono do contexto GL (janela GLFW oculta aqui faz o papel do SDL3 no GusWorld).
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include <cstdio>
int main() {
  glintfx::WindowGlfw host;                 // host: cria janela + contexto GL corrente
  if (!host.create("host", 640, 480)) return 1;
  glintfx::UiLayer ui({ .logical_width = 640, .logical_height = 480, .load_gl = true });
  if (!ui.ok()) { std::puts("ui attach failed"); return 2; }
  ui.load("tests/min.rml");
  ui.set_viewport(800, 600);                // resize não deve crashar
  std::puts("ui_layer_attach OK");
  return 0;
}
```
Registrar no `tests/CMakeLists.txt` (link `glintfx`, `run_xvfb.cmake`, `WORKING_DIRECTORY ${CMAKE_BINARY_DIR}`, `min.rml` já é copiado pela Task 1).

- [x] **Step 3: Rodar e ver falhar**
Run: `cd glintfx && cmake -S . -B build -DGLINTFX_BUILD_DEMOS=OFF && cmake --build build -j 2>&1 | tail -5`
Expected: erro de link (`UiLayer::...` indefinido).

- [x] **Step 4: Implementar `ui_layer.cpp` (attach/load/viewport)**
```cpp
// SPDX-License-Identifier: MPL-2.0
#include <GL/gl3w.h>            // EN: gl3w first. PT: gl3w primeiro.
#include <glintfx/ui_layer.hpp>
#include "engine.hpp"
#include "system_clock.hpp"
namespace glintfx {
struct UiLayer::Impl { SystemClock clock; Engine engine; int w, h; bool ok=false; };
UiLayer::UiLayer(Config cfg) : impl_(std::make_unique<Impl>()) {
  impl_->w = cfg.logical_width; impl_->h = cfg.logical_height;
  // EN: load GL pointers against the host's CURRENT context (idempotent enough across one process).
  // PT: carrega ponteiros GL contra o contexto CORRENTE do host (idempotente o bastante no processo).
  if (cfg.load_gl) { if (gl3wInit() != 0) return; }
  impl_->ok = impl_->engine.attach(&impl_->clock, impl_->w, impl_->h);
}
UiLayer::~UiLayer() = default;
UiLayer::UiLayer(UiLayer&&) noexcept = default;
UiLayer& UiLayer::operator=(UiLayer&&) noexcept = default;
bool UiLayer::ok() const noexcept { return impl_ && impl_->ok; }
void UiLayer::load(const char* p) { if (impl_->ok) impl_->engine.load(p); }
void UiLayer::set_viewport(int w, int h) { if (!impl_->ok) return; impl_->w=w; impl_->h=h; impl_->engine.set_viewport(w,h); }
void UiLayer::update() { if (impl_->ok) impl_->engine.update(); }
// render() / process_event() — T3 / T4
}
```
`Engine::set_viewport(w,h)` chama `context()->SetDimensions({w,h})` e guarda dims para o próximo `render_compose`.

- [x] **Step 5: Buildar e rodar (passa)**
Run: `cd glintfx && cmake --build build -j && ctest --test-dir build -R ui_layer_attach --output-on-failure`
Expected: `ui_layer_attach OK`.

- [x] **Step 6: Gate de header + commit**
Run: `! grep -rqE 'Rml|GLFW|GL_|gl3w|SDL' glintfx/include/glintfx/ && echo OK`
```bash
git add glintfx/ && git commit -m "feat(glintfx): UiLayer attach/load/viewport sobre contexto do host (F1 T2)"
```

---

### Task 3: `render()` compose-only + `GlStateGuard` (save/restore de estado GL)

**Files:**
- Create: `glintfx/src/gl_state.hpp`, `glintfx/tests/offscreen.hpp`, `glintfx/tests/ui_layer_compose.cpp`, `glintfx/tests/gl_state_guard.cpp`
- Modify: `glintfx/src/render_gl3.hpp`, `glintfx/src/render_gl3.cpp` (caminho compose), `glintfx/src/engine.cpp` (`render_compose`), `glintfx/src/ui_layer.cpp` (`render`), `glintfx/tests/CMakeLists.txt`

**Interfaces:**
- `RenderGl3::begin_frame_compose(int w,int h)` (= `glViewport` + `SetViewport` + `BeginFrame`, **sem** `glClear`); `RenderGl3::end_frame_compose()` (= `renderer.EndFrame()`, **sem** o bloco de alpha-fix do FBO0).
- `glintfx::GlStateGuard` (RAII) — snapshot no ctor, restore no dtor.

- [x] **Step 1: Caminho compose no `RenderGl3` (não destrutivo)**

`render_gl3.hpp`: declarar `void begin_frame_compose(int w,int h);` e `void end_frame_compose();`.
`render_gl3.cpp`:
```cpp
void RenderGl3::begin_frame_compose(int w, int h) {
  if (!impl_) return;
  // EN: NO glClear here — the host owns the framebuffer contents (scene already drawn).
  // PT: SEM glClear aqui — o host é dono do conteúdo do framebuffer (cena já desenhada).
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
```

- [x] **Step 2: `GlStateGuard` (RAII)**

`gl_state.hpp`:
```cpp
// SPDX-License-Identifier: MPL-2.0
// EN: RAII snapshot/restore of the GL state the RmlUi GL3 backend touches, so the host's
//     renderer (e.g. Render2dGl3 in GusWorld) sees the context unchanged after UiLayer::render().
// PT: Snapshot/restore RAII do estado GL que o backend GL3 do RmlUi mexe, para o renderer do
//     host (ex.: Render2dGl3 no GusWorld) ver o contexto inalterado após UiLayer::render().
#pragma once
#include <GL/gl3w.h>
namespace glintfx {
class GlStateGuard {
public:
  GlStateGuard() {
    glGetIntegerv(GL_VIEWPORT, vp_);
    blend_   = glIsEnabled(GL_BLEND);
    scissor_ = glIsEnabled(GL_SCISSOR_TEST);
    cull_    = glIsEnabled(GL_CULL_FACE);
    depth_   = glIsEnabled(GL_DEPTH_TEST);
    stencil_ = glIsEnabled(GL_STENCIL_TEST);
    glGetIntegerv(GL_SCISSOR_BOX, scbox_);
    glGetIntegerv(GL_BLEND_SRC_RGB,   &bsrc_rgb_);
    glGetIntegerv(GL_BLEND_DST_RGB,   &bdst_rgb_);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &bsrc_a_);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &bdst_a_);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog_);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao_);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_tex_);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex2d_);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fbo_);
    glGetBooleanv(GL_COLOR_WRITEMASK, cmask_);
  }
  ~GlStateGuard() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)draw_fbo_);
    glViewport(vp_[0], vp_[1], vp_[2], vp_[3]);
    glScissor(scbox_[0], scbox_[1], scbox_[2], scbox_[3]);
    set(GL_BLEND, blend_); set(GL_SCISSOR_TEST, scissor_); set(GL_CULL_FACE, cull_);
    set(GL_DEPTH_TEST, depth_); set(GL_STENCIL_TEST, stencil_);
    glBlendFuncSeparate((GLenum)bsrc_rgb_, (GLenum)bdst_rgb_, (GLenum)bsrc_a_, (GLenum)bdst_a_);
    glUseProgram((GLuint)prog_);
    glBindVertexArray((GLuint)vao_);
    glActiveTexture((GLenum)active_tex_);
    glBindTexture(GL_TEXTURE_2D, (GLuint)tex2d_);
    glColorMask(cmask_[0], cmask_[1], cmask_[2], cmask_[3]);
  }
private:
  static void set(GLenum cap, GLboolean on){ on?glEnable(cap):glDisable(cap); }
  GLint vp_[4], scbox_[4];
  GLint bsrc_rgb_, bdst_rgb_, bsrc_a_, bdst_a_, prog_, vao_, active_tex_, tex2d_, draw_fbo_;
  GLboolean blend_, scissor_, cull_, depth_, stencil_, cmask_[4];
};
}
```

- [x] **Step 3: `Engine::render_compose` + `UiLayer::render`**

`engine.cpp`:
```cpp
void Engine::render_compose(int w, int h) {
  if (!impl_->ok) return;
  GlStateGuard guard;                          // EN: restores host GL state on scope exit.
  impl_->render.begin_frame_compose(w, h);
  if (auto* c = impl_->boot.context()) c->Render();
  impl_->render.end_frame_compose();
}                                              // ~guard restaura aqui
```
`ui_layer.cpp`: `void UiLayer::render(){ if(impl_->ok) impl_->engine.render_compose(impl_->w, impl_->h); }`

- [x] **Step 4: Harness offscreen (`tests/offscreen.hpp`)**

Helper de teste: cria janela GLFW oculta (contexto) + FBO offscreen com textura RGBA, bind, clear a uma cor-âncora; readback do FBO para RGB. Esboço:
```cpp
// SPDX-License-Identifier: MPL-2.0
#pragma once
#include <GL/gl3w.h>
#include <vector>
namespace gtest_off {
struct Fbo { GLuint fbo=0, tex=0; int w=0,h=0; };
inline Fbo make_fbo(int w,int h){ Fbo f{0,0,w,h};
  glGenTextures(1,&f.tex); glBindTexture(GL_TEXTURE_2D,f.tex);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glGenFramebuffers(1,&f.fbo); glBindFramebuffer(GL_FRAMEBUFFER,f.fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,f.tex,0);
  return f; }
inline void read_rgb(const Fbo& f, std::vector<unsigned char>& px){
  px.resize((size_t)f.w*f.h*3); glBindFramebuffer(GL_FRAMEBUFFER,f.fbo);
  glPixelStorei(GL_PACK_ALIGNMENT,1);
  glReadPixels(0,0,f.w,f.h,GL_RGB,GL_UNSIGNED_BYTE,px.data()); }
}
```

- [x] **Step 5: Teste compose preserva o fundo do host (`ui_layer_compose.cpp`)**

Roteiro: host cria contexto (janela oculta), `UiLayer` attach, FBO offscreen, **bind FBO + clear a uma cor-âncora não-preta** (ex.: R=10,G=20,B=40), `ui.load("tests/min.rml")` (doc com fundo transparente/pequeno), `ui.update(); ui.render();` (compose **sobre** o FBO corrente), `read_rgb`. Asserções:
1. **fundo preservado:** existe uma quantidade significativa de pixels ainda na cor-âncora (prova que `render()` **não** limpou o FBO do host);
2. **UI compôs algo:** existe ao menos 1 pixel que mudou em relação à cor-âncora (a UI desenhou por cima).
> `min.rml` deve ser pequeno/parcial (não cobrir a tela toda) para os dois sinais coexistirem; se necessário criar `tests/min_partial.rml` com um elemento pequeno.

- [x] **Step 6: Teste de save/restore de estado GL (`gl_state_guard.cpp`)**

Host cria contexto + FBO; define um estado GL **conhecido e não-default** (ex.: `glEnable(GL_SCISSOR_TEST); glScissor(5,6,7,8); glViewport(1,2,300,200); glDisable(GL_BLEND);` e um programa/VAO dummy bound). Snapshot via `glGetIntegerv`. `ui.render()`. Re-leitura do estado. Asserção: **estado pós-render == estado pré-render** (viewport, scissor box, GL_BLEND/SCISSOR_TEST/DEPTH_TEST enables, draw FBO binding, color writemask). Falha se qualquer um divergir.

- [x] **Step 7: Build + rodar (passa) + commit**
Run: `cd glintfx && cmake --build build -j && ctest --test-dir build -R 'ui_layer_compose|gl_state_guard' --output-on-failure`
Expected: ambos `PASS`; e `ctest --test-dir build` inteiro segue verde (v1 intacto).
```bash
git add glintfx/ && git commit -m "feat(glintfx): render compose-only + GlStateGuard (sem clear/swap) (F1 T3)"
```

---

### Task 4: Injeção de eventos neutra (`UiEvent`) + `process_event`

**Files:**
- Create: `glintfx/include/glintfx/ui_event.hpp`
- Modify: `glintfx/include/glintfx/glintfx.hpp` (umbrella), `glintfx/src/ui_layer.cpp` (`process_event` + tradução), `glintfx/tests/CMakeLists.txt`
- Create: `glintfx/tests/ui_layer_events.cpp`

**Interfaces:**
- `glintfx::UiEvent` (POD neutro) + `enum class Key` / `enum Mod` — **sem** tipos de GLFW/SDL/RmlUi. O host (SDL3 no GusWorld) traduz seus eventos nativos para `UiEvent`; o gamepad mapeia d-pad/botões → `Key::Up/Down/Left/Right/Enter/Escape/Tab` (navegação de foco do RmlUi).

- [x] **Step 1: `ui_event.hpp` (tipos neutros)**
```cpp
// SPDX-License-Identifier: MPL-2.0
// EN: Neutral, host-agnostic input event. The host (GLFW, SDL3, …) fills this; glintfx
//     translates it to RmlUi. Gamepad nav: map buttons to Key::Up/Down/Left/Right/Enter/Escape.
// PT: Evento de entrada neutro, agnóstico de host. O host (GLFW, SDL3, …) preenche; o glintfx
//     traduz para o RmlUi. Nav por gamepad: mapeie botões para Key::Up/Down/Left/Right/Enter/Escape.
#pragma once
namespace glintfx {
enum class Key { None, Up, Down, Left, Right, Enter, Escape, Tab, Space, Backspace };
enum Mod { Mod_None=0, Mod_Shift=1, Mod_Ctrl=2, Mod_Alt=4 };
struct UiEvent {
  enum class Type { MouseMove, MouseButton, Key, Text, Resize };
  Type type{};
  float x=0, y=0;            // MouseMove (pixels)
  int  button=0;             // MouseButton: 0=left,1=right,2=middle
  bool pressed=false;        // MouseButton/Key: down?
  Key  key=Key::None;        // Key
  int  modifiers=Mod_None;   // bitmask de Mod
  const char* text=nullptr;  // Text (UTF-8, terminado em '\0')
  int  width=0, height=0;    // Resize
};
}
```
Substituir o forward `struct UiEvent;` do `ui_layer.hpp` pelo `#include <glintfx/ui_event.hpp>` (já previsto no header) e o umbrella `glintfx.hpp` passa a incluir ambos.

- [x] **Step 2: Teste de injeção (falha primeiro — `process_event` indefinido)**

`tests/ui_layer_events.cpp`: host cria contexto, `UiLayer` attach + `load("tests/min.rml")`, injeta uma sequência:
```cpp
ui.process_event({ .type=glintfx::UiEvent::Type::MouseMove, .x=100, .y=80 });
ui.process_event({ .type=glintfx::UiEvent::Type::MouseButton, .button=0, .pressed=true });
ui.process_event({ .type=glintfx::UiEvent::Type::MouseButton, .button=0, .pressed=false });
ui.process_event({ .type=glintfx::UiEvent::Type::Key, .pressed=true, .key=glintfx::Key::Down });
ui.process_event({ .type=glintfx::UiEvent::Type::Resize, .width=800, .height=600 });
ui.update(); ui.render();
std::puts("ui_layer_events OK");
```
Asserção F1 (modesta, anti-flaky): **não crasha** e `ui.ok()` segue true após a sequência. (Asserção de movimento de foco fica para F2, quando houver componentes focáveis reais.)

- [x] **Step 3: Implementar `process_event` (tradução UiEvent→Rml)**

Em `ui_layer.cpp`, tabela de tradução `Key`→`Rml::Input::KeyIdentifier` (Up=KI_UP, Down=KI_DOWN, Left=KI_LEFT, Right=KI_RIGHT, Enter=KI_RETURN, Escape=KI_ESCAPE, Tab=KI_TAB, Space=KI_SPACE, Backspace=KI_BACK) e despacho ao contexto:
```cpp
void UiLayer::process_event(const UiEvent& e){
  if(!impl_->ok) return; auto* c = impl_->engine.context(); if(!c) return;
  using T = UiEvent::Type;
  switch(e.type){
    case T::MouseMove:   c->ProcessMouseMove((int)e.x,(int)e.y, to_rml_mods(e.modifiers)); break;
    case T::MouseButton: e.pressed ? c->ProcessMouseButtonDown(e.button, to_rml_mods(e.modifiers))
                                   : c->ProcessMouseButtonUp(e.button, to_rml_mods(e.modifiers)); break;
    case T::Key:         e.pressed ? c->ProcessKeyDown(to_rml_key(e.key), to_rml_mods(e.modifiers))
                                   : c->ProcessKeyUp(to_rml_key(e.key), to_rml_mods(e.modifiers)); break;
    case T::Text:        if(e.text) c->ProcessTextInput(Rml::String(e.text)); break;
    case T::Resize:      impl_->w=e.width; impl_->h=e.height; c->SetDimensions(Rml::Vector2i(e.width,e.height)); break;
  }
}
```
> `to_rml_key`/`to_rml_mods` são helpers internos no `ui_layer.cpp` (não vazam ao header). Nomes exatos dos métodos do `Rml::Context` (6.3 pinada) a confirmar no source fetchado; ajustar se divergirem.

- [x] **Step 4: Build + rodar (passa) + gate de header + commit**
Run: `cd glintfx && cmake --build build -j && ctest --test-dir build -R ui_layer_events --output-on-failure`
Expected: `ui_layer_events OK`.
Run: `! grep -rqE 'Rml|GLFW|GL_|gl3w|SDL' glintfx/include/glintfx/ && echo OK` (o `ui_event.hpp` só tem tipos próprios)
```bash
git add glintfx/ && git commit -m "feat(glintfx): injeção de eventos neutra UiEvent + process_event (F1 T4)"
```

---

### Task 5: Gate de CI do embed — sanidade visual no FBO offscreen

**Files:**
- Create: `glintfx/tests/ui_layer_sanity.cpp`
- Modify: `glintfx/tests/CMakeLists.txt` (copiar um `.rml`/`.rcss` de teste com efeito; registrar o teste)

**Interfaces:** nenhuma nova — consolida o harness `offscreen.hpp` (T3) num gate determinístico análogo ao `render_sanity` do `App`, mas pela via `UiLayer` + FBO (sem janela visível, sem swap).

- [x] **Step 1: Documento de teste com fundo + 1 efeito barato**

Reusar `showcase_test.rml`/`.rcss` (já sem o card `mask`, conforme nota do `tests/CMakeLists.txt` — `mask` crasha no llvmpipe) ou criar `tests/embed_scene.rml`/`.rcss` com um fundo escuro + um elemento com `box-shadow`/`linear-gradient` (efeitos 1ª classe do spec §7; **nunca** `.masked` no CI por software). Copiar via `file(COPY ...)` para `${CMAKE_CURRENT_BINARY_DIR}` como os demais assets.

- [x] **Step 2: Teste `ui_layer_sanity.cpp`**

Roteiro: host cria contexto (janela oculta) 900×600, FBO offscreen 900×600, **clear a preto**, `UiLayer` attach 900×600, `load` o documento, 2 frames de aquecimento (`update`+`render`), `read_rgb` do FBO. Estatística agregada (estilo `render_sanity`, tolerante ao llvmpipe): `mean_brightness > 5` (renderizou algo), `dark_count ≥ 10%` (fundo escuro presente), `bright_count ≥ 1%` (gradiente/glow presente). Imprime `ui_layer_sanity: PASS`.
> Importante: como é compose-only, o **host** faz o clear do FBO (a UI não limpa) — o teste prova a cadeia embed ponta-a-ponta no FBO offscreen, sem janela/swap.

- [x] **Step 3: Registrar no `tests/CMakeLists.txt` + rodar (passa)**
Padrão `run_xvfb.cmake`, `WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}`.
Run: `cd glintfx && cmake -S . -B build -DGLINTFX_BUILD_DEMOS=OFF && cmake --build build -j && ctest --test-dir build -R ui_layer_sanity --output-on-failure`
Expected: `ui_layer_sanity: PASS`.

- [x] **Step 4: Commit**
```bash
git add glintfx/ && git commit -m "test(glintfx): gate de sanidade do embed via FBO offscreen (F1 T5)"
```

---

### Task 6: Integração final — build limpo, suíte completa, gates e fechamento do plano

**Files:**
- Modify: `glintfx/CMakeLists.txt` (confirmar que `engine.cpp`, `system_clock.cpp`, `ui_layer.cpp` estão na lib `glintfx`; **não** criar alvo `glintfx::ui` — é F2), `docs/superpowers/plans/2026-06-30-glintfx-v2-f1-embed-mode.md` (marcar checkboxes)

- [x] **Step 1: Confirmar fontes da lib e ausência de alvo `glintfx::ui`**

`glintfx/CMakeLists.txt` — o `add_library(glintfx STATIC ...)` deve listar `src/engine.cpp src/system_clock.cpp src/ui_layer.cpp` além dos existentes. **Anti-OE:** nenhum alvo/pasta de componentes nesta F1 (registrar como F2 no spec §9, não aqui).
> **VERIFICADO 2026-06-30:** `engine.cpp`, `system_clock.cpp`, `ui_layer.cpp` na lib; zero alvo `glintfx::ui`. PASS.

- [x] **Step 2: Build do zero + suíte inteira verde**
Run: `cd glintfx && rm -rf build && cmake -S . -B build -DGLINTFX_BUILD_DEMOS=ON && cmake --build build -j 2>&1 | tail -8`
Run: `ctest --test-dir build --output-on-failure`
Expected: passam — `window_smoke`, `render_smoke`, `engine_smoke`, `app_smoke`, `render_sanity` (v1 intacto) + `ui_layer_attach`, `ui_layer_compose`, `gl_state_guard`, `ui_layer_events`, `ui_layer_sanity` (embed F1). Total: 10 testes.
> **RESULTADO 2026-06-30:** 100% tests passed, 0 tests failed out of 10. Total Test time: 32.76 sec. Build sem warnings fora de `_deps`.

- [x] **Step 3: Gates finais (encapsulamento + SPDX)**
Run: `! grep -rqE 'Rml|GLFW|GL_|gl3w|SDL' glintfx/include/glintfx/ && echo "headers públicos limpos"`
Run: `for f in $(git ls-files 'glintfx/**/*.hpp' 'glintfx/**/*.cpp' | grep -vE '_deps|build/'); do head -1 "$f" | grep -q 'SPDX-License-Identifier: MPL-2.0' || echo "SEM SPDX: $f"; done` (esperado: sem saída)
> **RESULTADO 2026-06-30:** PASS (encapsulamento) + PASS (SPDX — 32 arquivos verificados, nenhum sem header).

- [x] **Step 4: Prova de drop-in do embed (opcional, recomendado)**

Mini-consumidor `consumer-example-embed/` que cria um contexto GL (GLFW oculto fazendo o papel do host), instancia `glintfx::UiLayer`, dá `load`+`render()` num FBO e lê 1 pixel — **sem** escrever código GL/RmlUi além do contexto do host. Valida o north-star do embed: o host só traz a janela/contexto; a UI+efeitos vêm da `UiLayer`. (Se o tempo apertar, cobrir isto pelo `ui_layer_sanity` da T5 e registrar o consumidor como follow-up.)
> **DECISÃO 2026-06-30:** coberto pelo `ui_layer_sanity` (T5), que valida a cadeia embed ponta-a-ponta no FBO offscreen. `consumer-example-embed/` registrado como follow-up na INBOX do TODO.md.

- [x] **Step 5: Fechar o plano**
Marcar todos os checkboxes; conferir que o spec §12 (F1) está satisfeito: embed attach + compose-only + eventos injetados + save/restore de estado GL, com `App` standalone intacto. Próximo: F2 (tokens + componentes onda 1).
> **FECHADO 2026-06-30:** F1 satisfeita. Spec §12 (F1): attach ✓ compose-only ✓ eventos ✓ GlStateGuard ✓ App standalone intacto ✓. Próximo: F2.

---

## Notas de execução

- **Ordem de risco:** T1 é o maior risco de regressão (mexe no `App`); o gate é `render_sanity` seguir `PASS`. T3 é o coração do embed (compose-only + estado GL); os testes `ui_layer_compose` (fundo preservado) e `gl_state_guard` (estado inalterado) são as provas-chave do contrato do ADR-0008(d).
- **RmlUi global (N3):** nunca instanciar `App` e `UiLayer` no mesmo processo/teste. Se um teste futuro precisar dos dois, isolar em processos separados.
- **`.masked`:** proibido no CI por software (llvmpipe crasha — já documentado no `tests/CMakeLists.txt`). Os testes de efeito do embed usam só `glow`/`gradient`/`box-shadow`.
- **Nomes de API do RmlUi 6.3:** `ProcessMouseMove/ProcessMouseButtonDown/Up/ProcessKeyDown/Up/ProcessTextInput/SetDimensions/SetViewport/BeginFrame/EndFrame` — confirmar contra o source pinado (`build/_deps/rmlui-src`) e ajustar se divergirem.
- **F2 (fora deste plano):** alvo opt-in `glintfx::ui`, `tokens.rcss`/`glintfx.rcss`/templates RML, componentes onda 1 (button/panel/dialog/menu/label), theming, contraste AA. Ver spec §6–9, §12.
