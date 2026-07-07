<!--
EN: Diátaxis type: how-to guide. Audience: intermediate C++/CMake developer packaging or
    consuming a pre-built glintfx tree (as opposed to FetchContent, which builds from
    source and is covered in the README Quick start). Owner: glintfx maintainers.
    Last reviewed: 2026-07-07 (glintfx v0.4.0).
PT: Tipo Diátaxis: how-to guide. Audience: dev C++/CMake intermediário empacotando ou
    consumindo uma árvore pré-buildada do glintfx (em oposição ao FetchContent, que builda
    do fonte e está no Quick start do README). Owner: mantenedores do glintfx.
    Última revisão: 2026-07-07 (glintfx v0.4.0).
-->

# Packaging glintfx (`cmake --install` + `find_package`) / Empacotando o glintfx (`cmake --install` + `find_package`)

Languages / Idiomas: **[English](#english)** · **[Português](#português)**

---

## English

### When to use this guide

Use this guide when you need a **pre-built glintfx tree** that other projects can consume
with `find_package(glintfx)` -- for example, packaging glintfx for a Linux distribution, a
CI artifact cache, or a shared build machine where rebuilding from source on every consumer
is wasteful. If you are building glintfx from source as part of your own project's build,
the `FetchContent` / `add_subdirectory` path in the [README Quick start](../README.md#quick-start)
is simpler and is the recommended default.

### Prerequisites

- CMake >= 3.16.
- A C++ compiler (see the [compatibility table](../README.md#compatibility) in the README).
- System packages: `glfw-devel`, `freetype-devel`, `mesa-libGL-devel` (Fedora names; see
  [`docs/troubleshooting.md`](troubleshooting.md#a-missing-system-dependencies-glfwfreetypeopengl)
  for other distributions).
- Network access at **configure** time (RmlUi is fetched then -- see the note below).

### Step 1: configure and build

From a clone of the repository, configure with `GLINTFX_BUILD_TESTS`/`GLINTFX_BUILD_DEMOS`
`OFF` if you only want the library (they default to `ON` only when glintfx is the top-level
CMake project, but turning them off explicitly keeps the packaging build minimal and fast):

```sh
cmake -S glintfx -B glintfx/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DGLINTFX_BUILD_TESTS=OFF \
  -DGLINTFX_BUILD_DEMOS=OFF
cmake --build glintfx/build -j
```

`-DGLINTFX_BACKEND_GLFW=ON` (the default) builds the standalone `glintfx::App` plus GLFW
integration; pass `-DGLINTFX_BACKEND_GLFW=OFF` for an embed-only install (no GLFW dependency
at all -- see [ADR-0008](adr/0008-embed-guest-mode.md)).

### Step 2: install to a prefix

```sh
cmake --install glintfx/build --prefix ~/opt/glintfx
```

This populates `~/opt/glintfx` with:

| Path | Contents |
| :--- | :--- |
| `include/glintfx/*.hpp` | Public headers (the full API surface -- see [README "API reference"](../README.md#documentation)). |
| `include/glintfx/config.hpp` | Generated at build time; records which build options (e.g. `GLINTFX_BACKEND_GLFW`) the installed archive was built with. |
| `lib/libglintfx.a` | The static archive for your application to link against. |
| `lib/glintfx/librmlui.a` | RmlUi's own static archive, **co-installed under a private subdirectory** so it does not collide with a consumer's own RmlUi install (see the note below). |
| `lib/cmake/glintfx/glintfxConfig.cmake` and `glintfxConfigVersion.cmake` | The CMake package config that makes `find_package(glintfx)` work. |

Nothing outside `include/`, `lib/libglintfx.a`, `lib/glintfx/`, and `lib/cmake/glintfx/` is
installed -- no demos, no test binaries, no RmlUi headers (only its archive; RmlUi is a
`PRIVATE` link dependency of glintfx, not part of the public interface).

### Step 3: consume via `find_package`

Point a consumer's CMake configure step at the install prefix, either via
`-DCMAKE_PREFIX_PATH=...` on the command line or `CMAKE_PREFIX_PATH` in a preset/toolchain
file:

```sh
cmake -S your-app -B your-app/build -DCMAKE_PREFIX_PATH=~/opt/glintfx
```

In `your-app/CMakeLists.txt`:

```cmake
find_package(glintfx REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE glintfx::glintfx)   # same alias as the FetchContent path
```

`find_package` and `FetchContent` expose the **same target name** (`glintfx::glintfx`), so
application code does not need to know or care which path produced the library.

### Note: RmlUi is fetched at configure time -- what that means for you

`glintfx/CMakeLists.txt` pins RmlUi to a **fixed upstream commit** via `FetchContent`
(`GIT_REPOSITORY https://github.com/mikke89/RmlUi.git`, a specific commit hash, not a
floating branch or tag). Two consequences:

1. **Building glintfx from source requires network access** at the `cmake -S ...`
   (configure) step, to clone that pinned commit. If you are packaging glintfx for an
   offline/air-gapped build environment, run the configure+build step once with network
   access, then distribute the **installed tree** (via `cmake --install`, as above) or a
   pre-populated `FetchContent` cache (CMake's `FETCHCONTENT_FULLY_DISCONNECTED` /
   a local mirror of the RmlUi repository) to offline consumers.
2. **RmlUi is not re-fetched by consumers of the installed tree.** Once you have run
   `cmake --install`, `librmlui.a` is baked into the install prefix (`lib/glintfx/`,
   see the table above); a consumer using `find_package(glintfx)` never talks to the
   network for RmlUi -- `glintfxConfig.cmake` references the co-installed archive
   directly. Network access is only needed once, at the packaging step.

RmlUi is a `PRIVATE` link dependency: its headers are not installed and its types never
appear in glintfx's public headers (see [ADR-0006](adr/0006-layered-hybrid-architecture.md)).
You do not need your own RmlUi installation to consume `glintfx::glintfx`, and a
consumer-side RmlUi (if you happen to also depend on it directly, for an unrelated reason)
does not conflict with glintfx's private, co-installed copy.

gl3w (the OpenGL loader) does **not** appear as a separate installed archive: it is a
single-file, `INTERFACE`-library source that is compiled directly into `libglintfx.a`.

### Related documentation

- [README "Requirements" / "Compatibility"](../README.md#requirements) -- supported
  RmlUi/OpenGL/compiler/CMake versions and platform.
- [`docs/troubleshooting.md`](troubleshooting.md) -- `find_package(glintfx)` not found,
  missing system packages, and other common integration errors.
- [ADR-0008](adr/0008-embed-guest-mode.md) -- the `GLINTFX_BACKEND_GLFW` build option.

---

## Português

### Quando usar este guia

Use este guia quando precisar de uma **árvore pré-buildada do glintfx** que outros projetos
consumirão via `find_package(glintfx)` -- por exemplo, empacotar o glintfx para uma
distribuição Linux, um cache de artefato de CI, ou uma máquina de build compartilhada onde
rebuildar do fonte a cada consumidor é desperdício. Se você está buildando o glintfx do
fonte como parte do build do seu próprio projeto, o caminho `FetchContent` /
`add_subdirectory` do [Quick start do README](../README.md#quick-start) é mais simples e é
o padrão recomendado.

### Pré-requisitos

- CMake >= 3.16.
- Um compilador C++ (ver a [tabela de compatibilidade](../README.md#compatibilidade) no
  README).
- Pacotes de sistema: `glfw-devel`, `freetype-devel`, `mesa-libGL-devel` (nomes do Fedora;
  ver [`docs/troubleshooting.md`](troubleshooting.md#a-dependências-de-sistema-ausentes-glfwfreetypeopengl)
  para outras distribuições).
- Acesso à rede em tempo de **configure** (o RmlUi é baixado nesse momento -- ver a nota
  abaixo).

### Passo 1: configurar e buildar

A partir de um clone do repositório, configure com `GLINTFX_BUILD_TESTS`/`GLINTFX_BUILD_DEMOS`
em `OFF` se você só quer a biblioteca (eles são `ON` por padrão só quando o glintfx é o
projeto CMake top-level, mas desligá-los explicitamente mantém o build de empacotamento
mínimo e rápido):

```sh
cmake -S glintfx -B glintfx/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DGLINTFX_BUILD_TESTS=OFF \
  -DGLINTFX_BUILD_DEMOS=OFF
cmake --build glintfx/build -j
```

`-DGLINTFX_BACKEND_GLFW=ON` (o padrão) builda o `glintfx::App` standalone mais a integração
GLFW; passe `-DGLINTFX_BACKEND_GLFW=OFF` para um install embed-only (sem nenhuma dependência
de GLFW -- ver [ADR-0008](adr/0008-embed-guest-mode.md)).

### Passo 2: instalar num prefixo

```sh
cmake --install glintfx/build --prefix ~/opt/glintfx
```

Isso popula `~/opt/glintfx` com:

| Caminho | Conteúdo |
| :--- | :--- |
| `include/glintfx/*.hpp` | Headers públicos (toda a superfície de API -- ver ["API reference" no README](../README.md#documentação)). |
| `include/glintfx/config.hpp` | Gerado em tempo de build; registra com quais opções de build (ex.: `GLINTFX_BACKEND_GLFW`) o arquivo instalado foi buildado. |
| `lib/libglintfx.a` | O arquivo estático para sua aplicação linkar. |
| `lib/glintfx/librmlui.a` | O arquivo estático do próprio RmlUi, **co-instalado num subdiretório privado** para não colidir com uma instalação de RmlUi do consumidor (ver a nota abaixo). |
| `lib/cmake/glintfx/glintfxConfig.cmake` e `glintfxConfigVersion.cmake` | O config CMake do pacote que faz `find_package(glintfx)` funcionar. |

Nada fora de `include/`, `lib/libglintfx.a`, `lib/glintfx/` e `lib/cmake/glintfx/` é
instalado -- sem demos, sem binários de teste, sem headers do RmlUi (só o arquivo dele; o
RmlUi é uma dependência de link `PRIVATE` do glintfx, não parte da interface pública).

### Passo 3: consumir via `find_package`

Aponte o passo de configure do CMake do consumidor para o prefixo de instalação, seja via
`-DCMAKE_PREFIX_PATH=...` na linha de comando ou `CMAKE_PREFIX_PATH` num arquivo de
preset/toolchain:

```sh
cmake -S seu-app -B seu-app/build -DCMAKE_PREFIX_PATH=~/opt/glintfx
```

No `CMakeLists.txt` do seu app:

```cmake
find_package(glintfx REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE glintfx::glintfx)   # mesmo alias do caminho FetchContent
```

`find_package` e `FetchContent` expõem o **mesmo nome de alvo** (`glintfx::glintfx`), então
o código da aplicação não precisa saber nem se importar com qual caminho produziu a
biblioteca.

### Nota: o RmlUi é baixado em tempo de configure -- o que isso significa para você

O `glintfx/CMakeLists.txt` pina o RmlUi a um **commit fixo do upstream** via `FetchContent`
(`GIT_REPOSITORY https://github.com/mikke89/RmlUi.git`, um hash de commit específico, não
um branch ou tag flutuante). Duas consequências:

1. **Buildar o glintfx do fonte exige acesso à rede** no passo `cmake -S ...` (configure),
   para clonar esse commit pinado. Se você está empacotando o glintfx para um ambiente de
   build offline/air-gapped, rode o passo configure+build uma vez com acesso à rede, e
   depois distribua a **árvore instalada** (via `cmake --install`, como acima) ou um cache
   de `FetchContent` pré-populado (`FETCHCONTENT_FULLY_DISCONNECTED` do CMake / um mirror
   local do repositório RmlUi) para consumidores offline.
2. **Consumidores da árvore instalada NÃO baixam o RmlUi de novo.** Depois de rodar
   `cmake --install`, o `librmlui.a` fica embutido no prefixo de instalação (`lib/glintfx/`,
   ver a tabela acima); um consumidor usando `find_package(glintfx)` nunca fala com a rede
   por causa do RmlUi -- o `glintfxConfig.cmake` referencia diretamente o arquivo
   co-instalado. Acesso à rede só é necessário uma vez, no passo de empacotamento.

O RmlUi é uma dependência de link `PRIVATE`: seus headers não são instalados e seus tipos
nunca aparecem nos headers públicos do glintfx (ver
[ADR-0006](adr/0006-layered-hybrid-architecture.md)). Você não precisa da sua própria
instalação do RmlUi para consumir `glintfx::glintfx`, e um RmlUi do lado do consumidor (se
você também depender dele diretamente, por outro motivo qualquer) não conflita com a cópia
privada e co-instalada do glintfx.

O gl3w (o loader de OpenGL) **não** aparece como um arquivo instalado separado: é um source
de biblioteca `INTERFACE` de arquivo único, compilado diretamente dentro de `libglintfx.a`.

### Documentação relacionada

- ["Requisitos" / "Compatibilidade" no README](../README.md#requisitos) -- versões
  suportadas de RmlUi/OpenGL/compilador/CMake e plataforma.
- [`docs/troubleshooting.md`](troubleshooting.md) -- `find_package(glintfx)` não encontrado,
  dependências de sistema ausentes, e outros erros comuns de integração.
- [ADR-0008](adr/0008-embed-guest-mode.md) -- a opção de build `GLINTFX_BACKEND_GLFW`.
