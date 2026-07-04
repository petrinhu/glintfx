# Getting started with glintfx

> **EN:** A beginner tutorial: from an empty folder to your first on-screen effect. Diátaxis type: **tutorial**. Audience: a developer new to glintfx.
> **PT:** Tutorial para iniciantes: de uma pasta vazia ao seu primeiro efeito na tela. Tipo Diátaxis: **tutorial**. Audiência: dev novo no glintfx.

- **Estimated time / Tempo estimado:** ~15 min
- **You will end with / Você terá ao fim:** a window showing a card with a cyan glow, built with glintfx.

---

## English

### Prerequisites

- Linux x86-64 with a working display (X11 or Wayland).
- `clang`, `cmake` (>= 3.16), and `git`.
- A GPU with OpenGL 3.3 (most machines have this).
- System packages (Fedora):

```sh
sudo dnf install glfw-devel freetype-devel mesa-libGL-devel
```

> On other distros, install the equivalents of `glfw`, `freetype`, and the OpenGL/Mesa development packages.

### 1. Create the project folder

```sh
mkdir hello-glintfx && cd hello-glintfx
```

You will create four files: `CMakeLists.txt`, `main.cpp`, `hello.rml`, and `hello.rcss`.

### 2. Tell CMake to pull in glintfx

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(hello_glintfx LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(glintfx
  GIT_REPOSITORY https://codeberg.org/petrinhu/glintfx.git
  GIT_TAG        v0.3.0)
FetchContent_MakeAvailable(glintfx)

add_executable(hello main.cpp)
target_link_libraries(hello PRIVATE glintfx::glintfx)   # one line: glintfx brings GL/GLFW/RmlUi
```

That single `target_link_libraries` line is the whole point of glintfx: you do not wire OpenGL, GLFW, or RmlUi yourself.

### 3. Write the program

Create `main.cpp`:

```cpp
#include <glintfx/glintfx.hpp>

int main() {
    glintfx::App app({ .title = "hello glintfx", .width = 900, .height = 600 });
    app.load("hello.rml");
    app.run();   // poll events + update + render, until you close the window
    return 0;
}
```

What each line does:

- `glintfx::App app({...})` opens a window and sets up the GL context and UI engine (RAII: it all gets cleaned up when `app` goes out of scope).
- `app.load("hello.rml")` loads your UI document.
- `app.run()` is the convenience loop. It keeps polling, updating, and rendering until the window is closed.

> **Only one `App` per process.** glintfx initialises process-global libraries, so do not create a second `App`.

### 4. Describe the UI

Create `hello.rml` (RML is HTML-like markup):

```html
<rml>
<head>
  <link type="text/rcss" href="hello.rcss"/>
  <title>hello glintfx</title>
</head>
<body>
  <div class="card glow">Glow</div>
</body>
</rml>
```

### 5. Style it with one effect

Create `hello.rcss` (RCSS is CSS-like styling):

```css
/* RmlUi defaults every element to display: inline, so set block explicitly. */
body { display: block; background-color: #0d1020; color: #fff; }

.card {
    display: block;
    width: 260px; height: 80px;
    margin: 16px; padding: 16px;
    border-radius: 12px;
    font-size: 20px;
}

/* The glow. In RmlUi the order is COLOR x y blur spread (color comes first),
   and colors are 8-digit hex #rrggbbaa. */
.glow {
    background-color: #1e2a50;
    box-shadow: #5fd0ff 0 0 32px 8px;
}
```

### 6. Build and run

```sh
cmake -S . -B build
cmake --build build
./build/hello
```

A window opens with a dark background and a card labelled "Glow" surrounded by a soft cyan halo. The first build takes longer because CMake fetches and compiles RmlUi.

### Verify

You succeeded if:

- A window titled "hello glintfx" appears.
- The card has a visible cyan glow around its rounded corners.
- Closing the window ends the program cleanly (exit code 0).

If the window is translucent or the glow looks wrong, check that you are on a real GPU; the software renderer (Mesa/llvmpipe) handles some effects differently.

### Summary

You built a glintfx app and learned:

- glintfx is consumed with **one CMake target** (`glintfx::glintfx`).
- The public API is the **`glintfx::App` facade**: construct, `load`, `run`.
- Effects are **declared in RCSS**, not called in code.
- RmlUi RCSS differs from CSS: **color first** in `box-shadow`, **8-digit hex**.

### Next steps

- How-to / reference for all effects: [`effects.md`](effects.md).
- The real demo stylesheet: [`../glintfx/demos/showcase/showcase.rcss`](../glintfx/demos/showcase/showcase.rcss).
- Architecture and rationale: the [ADRs](adr/README.md).

---

## Português

### Pré-requisitos

- Linux x86-64 com display funcional (X11 ou Wayland).
- `clang`, `cmake` (>= 3.16) e `git`.
- Uma GPU com OpenGL 3.3 (a maioria das máquinas tem).
- Pacotes de sistema (Fedora):

```sh
sudo dnf install glfw-devel freetype-devel mesa-libGL-devel
```

> Em outras distros, instale os equivalentes de `glfw`, `freetype` e os pacotes de desenvolvimento de OpenGL/Mesa.

### 1. Crie a pasta do projeto

```sh
mkdir hello-glintfx && cd hello-glintfx
```

Você vai criar quatro arquivos: `CMakeLists.txt`, `main.cpp`, `hello.rml` e `hello.rcss`.

### 2. Diga ao CMake para trazer o glintfx

Crie `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(hello_glintfx LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(glintfx
  GIT_REPOSITORY https://codeberg.org/petrinhu/glintfx.git
  GIT_TAG        v0.3.0)
FetchContent_MakeAvailable(glintfx)

add_executable(hello main.cpp)
target_link_libraries(hello PRIVATE glintfx::glintfx)   # uma linha: glintfx traz GL/GLFW/RmlUi
```

Essa única linha `target_link_libraries` é todo o propósito do glintfx: você não wira OpenGL, GLFW ou RmlUi você mesmo.

### 3. Escreva o programa

Crie `main.cpp`:

```cpp
#include <glintfx/glintfx.hpp>

int main() {
    glintfx::App app({ .title = "hello glintfx", .width = 900, .height = 600 });
    app.load("hello.rml");
    app.run();   // poll de eventos + update + render, até você fechar a janela
    return 0;
}
```

O que cada linha faz:

- `glintfx::App app({...})` abre uma janela e prepara o contexto GL e o motor de UI (RAII: tudo é limpo quando `app` sai de escopo).
- `app.load("hello.rml")` carrega seu documento de UI.
- `app.run()` é o laço de conveniência. Ele segue fazendo poll, update e render até a janela ser fechada.

> **Apenas um `App` por processo.** O glintfx inicializa bibliotecas globais de processo, então não crie um segundo `App`.

### 4. Descreva a UI

Crie `hello.rml` (RML é markup tipo HTML):

```html
<rml>
<head>
  <link type="text/rcss" href="hello.rcss"/>
  <title>hello glintfx</title>
</head>
<body>
  <div class="card glow">Glow</div>
</body>
</rml>
```

### 5. Estilize com um efeito

Crie `hello.rcss` (RCSS é estilo tipo CSS):

```css
/* O RmlUi usa display: inline em todo elemento por padrão, então defina block explícito. */
body { display: block; background-color: #0d1020; color: #fff; }

.card {
    display: block;
    width: 260px; height: 80px;
    margin: 16px; padding: 16px;
    border-radius: 12px;
    font-size: 20px;
}

/* O glow. No RmlUi a ordem é COR x y blur spread (a cor vem primeiro),
   e as cores são hex de 8 dígitos #rrggbbaa. */
.glow {
    background-color: #1e2a50;
    box-shadow: #5fd0ff 0 0 32px 8px;
}
```

### 6. Builde e rode

```sh
cmake -S . -B build
cmake --build build
./build/hello
```

Uma janela abre com fundo escuro e um card rotulado "Glow" cercado por um halo ciano suave. O primeiro build demora mais porque o CMake baixa e compila o RmlUi.

### Verificação

Você teve sucesso se:

- Uma janela com título "hello glintfx" aparece.
- O card tem um glow ciano visível ao redor dos cantos arredondados.
- Fechar a janela encerra o programa de forma limpa (exit code 0).

Se a janela ficar translúcida ou o glow parecer errado, confira se você está em GPU real; o renderer de software (Mesa/llvmpipe) trata alguns efeitos de forma diferente.

### Resumo

Você construiu um app glintfx e aprendeu:

- O glintfx é consumido com **um alvo CMake** (`glintfx::glintfx`).
- A API pública é a **fachada `glintfx::App`**: construir, `load`, `run`.
- Os efeitos são **declarados em RCSS**, não chamados no código.
- O RCSS do RmlUi difere do CSS: **cor primeiro** no `box-shadow`, **hex de 8 dígitos**.

### Próximos passos

- How-to / reference de todos os efeitos: [`effects.md`](effects.md).
- A folha de estilo real do demo: [`../glintfx/demos/showcase/showcase.rcss`](../glintfx/demos/showcase/showcase.rcss).
- Arquitetura e racional: os [ADRs](adr/README.md).
