<!--
EN: Diátaxis type: how-to guide (problem-solving / troubleshooting). Audience: developer
    integrating glintfx into their own project, novice-to-intermediate with CMake/OpenGL.
    Owner: glintfx maintainers. Last reviewed: 2026-07-07 (glintfx v0.4.0).
PT: Tipo Diátaxis: how-to guide (troubleshooting). Audience: dev integrando o glintfx no
    próprio projeto, novato-a-intermediário em CMake/OpenGL. Owner: mantenedores do
    glintfx. Última revisão: 2026-07-07 (glintfx v0.4.0).
-->

# Troubleshooting

Languages / Idiomas: **[English](#english)** · **[Português](#português)**

If your problem is not covered here, open an issue on
[Codeberg](https://codeberg.org/petrinhu/glintfx/issues) or
[GitHub](https://github.com/petrinhu/glintfx/issues) with your OS, compiler, and the exact
error output.

---

## English

### A. Missing system dependencies (GLFW/FreeType/OpenGL)

**Symptom.** `cmake -S glintfx -B glintfx/build` fails with `Could NOT find glfw3`,
`Could NOT find Freetype`, or `Could NOT find OpenGL` (a `find_package(... REQUIRED)`
error), or the build fails at the link step with `-lglfw`/`-lfreetype` "cannot find".

**Cause.** glintfx links GLFW (when `GLINTFX_BACKEND_GLFW=ON`, the default), FreeType, and
OpenGL from the **system**, not vendored. If the corresponding `-dev`/`-devel` packages are
not installed, CMake's `find_package` calls fail before any compilation happens.

**Fix.** Install the system packages for your distribution before configuring:

```sh
# Fedora
sudo dnf install glfw-devel freetype-devel mesa-libGL-devel

# Debian / Ubuntu (package names verified against this repo's own CI, .github/workflows/ci.yml)
sudo apt-get install -y libglfw3-dev libfreetype-dev libgl1-mesa-dev libgl1-mesa-dri
```

If you are on a different distribution, look for the `-dev`/`-devel` packages providing
`glfw3.pc`, `freetype2.pc`, and `gl.pc` (`pkg-config --list-all | grep -iE 'glfw|freetype|^gl '`
after install is a good sanity check).

If you only need the **embed-only** build (`-DGLINTFX_BACKEND_GLFW=OFF`), GLFW is not
required by the library itself -- but note the test suite still uses a hidden GLFW window
as an offscreen GL context fixture even in that configuration (see
[`AGENTS.md`](../AGENTS.md) "Gotchas críticos"), so `glfw-devel` is still needed to build
the tests.

### B. The `mask` effect crashes under Mesa/llvmpipe (software rendering)

**Symptom.** The application (or a test that renders the `mask` decorator/`mask-image`
property) crashes with something like `free(): invalid next size` or a segmentation fault,
specifically when running headless (Xvfb, a CI runner, a VM/container without GPU
passthrough, or `LIBGL_ALWAYS_SOFTWARE=1`).

**Cause.** Under Mesa's `llvmpipe` software GL renderer, the dual-sampler shader used by
the `mask` effect (`mask-image`) triggers heap corruption. This is a bug in Mesa's software
rasterizer, not in glintfx -- confirmed and documented in
[`AGENTS.md`](../AGENTS.md) "Gotchas críticos". It does not happen on a real GPU driver.

**Fix.**
- If you have a real GPU: make sure your process actually uses the hardware driver, not
  llvmpipe. Check with `glxinfo | grep "OpenGL renderer"` (from `mesa-demos` /
  `glx-utils`) -- it should name your GPU, not "llvmpipe".
- If you are running headless (CI, a container) and cannot avoid software rendering: avoid
  exercising the `mask` effect there. This repository's own CI runs its showcase-based
  render test (`showcase_test.rml`) **without** the mask card for exactly this reason.
- This is why `glintfx`'s bundled `golden_test` (pixel-exact reference comparison) is
  **opt-in** (`-DGLINTFX_GOLDEN_TEST=ON`) and requires a real GPU; the default CI gate uses
  the statistical `render_sanity` test instead.

### C. `find_package(glintfx)` does not find the package

**Symptom.** `find_package(glintfx REQUIRED)` fails with
`Could not find a package configuration file provided by "glintfx"`.

**Cause.** `find_package` (in "config mode", which is what glintfx provides) searches a
fixed set of default locations (system prefixes like `/usr`, `/usr/local`, plus a few
platform-specific paths) and does **not** know about a custom install prefix like
`~/opt/glintfx` unless told.

**Fix.** Point CMake at the install prefix used in `cmake --install glintfx/build --prefix
<dir>` (see [`docs/packaging.md`](packaging.md)), either on the command line:

```sh
cmake -S your-app -B your-app/build -DCMAKE_PREFIX_PATH=~/opt/glintfx
```

or as an environment variable before configuring:

```sh
export CMAKE_PREFIX_PATH=~/opt/glintfx:$CMAKE_PREFIX_PATH
cmake -S your-app -B your-app/build
```

`CMAKE_PREFIX_PATH` accepts a `:`-separated list, so you can combine it with other
installed dependencies. Verify the config file actually exists at
`<prefix>/lib/cmake/glintfx/glintfxConfig.cmake` before debugging further -- if it is
missing, the install step itself did not run or used a different `--prefix`.

### D. The window is translucent / see-through on some compositors

**Symptom.** Instead of an opaque window with your UI drawn on top, part or all of the
window shows the desktop/other windows behind it (most visible on compositing window
managers -- Wayland compositors and some X11 setups with a compositing manager active).

**Cause.** `glintfx`'s GL3 render layer works with **premultiplied alpha**
(`GL_ONE, GL_ONE_MINUS_SRC_ALPHA` blending) and composes onto **FBO 0** (the default
framebuffer / window backbuffer). If the window backbuffer's own alpha channel is not
forced opaque (`alpha = 1`), some compositors read that alpha and blend the whole window
against the desktop -- producing a translucent window even though glintfx itself rendered
correctly. This is a compositor/backbuffer-alpha interaction, not a bug in the effects
themselves. Full technical detail: [`AGENTS.md`](../AGENTS.md) "Gotchas críticos"
(premultiplied alpha / opaque backbuffer / FBO 0).

**Fix.**
- **Standalone `glintfx::App`:** this is already handled -- the GLFW window is created
  requesting an opaque backbuffer (see `window_glfw.cpp`/`render_gl3.cpp`). If you still
  see translucency with `App`, it is worth filing an issue with your compositor and driver
  details, since this path is expected to be opaque out of the box.
- **Embed mode (`glintfx::UiLayer`):** the host owns the window and its GL context, so the
  host is responsible for the backbuffer's alpha state (e.g. requesting an opaque
  framebuffer / EGL/GLX config, or clearing alpha to 1 before compositing glintfx's
  output). See [`docs/embed-integration.md`](embed-integration.md) sections 0 and 2 for the
  full frame-lifecycle contract `UiLayer` expects the host to honour.

---

## Português

### A. Dependências de sistema ausentes (GLFW/FreeType/OpenGL)

**Sintoma.** `cmake -S glintfx -B glintfx/build` falha com `Could NOT find glfw3`,
`Could NOT find Freetype` ou `Could NOT find OpenGL` (um erro de
`find_package(... REQUIRED)`), ou o build falha no passo de link com
`-lglfw`/`-lfreetype` "cannot find".

**Causa.** O glintfx linka GLFW (quando `GLINTFX_BACKEND_GLFW=ON`, o padrão), FreeType e
OpenGL do **sistema**, não vendorizados. Se os pacotes `-dev`/`-devel` correspondentes não
estiverem instalados, as chamadas `find_package` do CMake falham antes de qualquer
compilação acontecer.

**Correção.** Instale os pacotes de sistema da sua distribuição antes de configurar:

```sh
# Fedora
sudo dnf install glfw-devel freetype-devel mesa-libGL-devel

# Debian / Ubuntu (nomes de pacote verificados no próprio CI deste repo, .github/workflows/ci.yml)
sudo apt-get install -y libglfw3-dev libfreetype-dev libgl1-mesa-dev libgl1-mesa-dri
```

Se você está numa distribuição diferente, procure os pacotes `-dev`/`-devel` que fornecem
`glfw3.pc`, `freetype2.pc` e `gl.pc` (`pkg-config --list-all | grep -iE 'glfw|freetype|^gl '`
depois de instalar é uma boa checagem de sanidade).

Se você só precisa do build **embed-only** (`-DGLINTFX_BACKEND_GLFW=OFF`), o GLFW não é
exigido pela biblioteca em si -- mas note que a suíte de testes ainda usa uma janela GLFW
oculta como fixture de contexto GL offscreen mesmo nessa configuração (ver
[`AGENTS.md`](../AGENTS.md) "Gotchas críticos"), então `glfw-devel` ainda é necessário para
buildar os testes.

### B. O efeito `mask` crasha sob Mesa/llvmpipe (renderização por software)

**Sintoma.** A aplicação (ou um teste que renderiza o decorator `mask`/propriedade
`mask-image`) crasha com algo como `free(): invalid next size` ou um segmentation fault,
especificamente ao rodar headless (Xvfb, um runner de CI, uma VM/container sem passthrough
de GPU, ou `LIBGL_ALWAYS_SOFTWARE=1`).

**Causa.** Sob o renderer GL de software `llvmpipe` do Mesa, o shader dual-sampler usado
pelo efeito `mask` (`mask-image`) provoca corrupção de heap. É um bug do rasterizador de
software do Mesa, não do glintfx -- confirmado e documentado em
[`AGENTS.md`](../AGENTS.md) "Gotchas críticos". Não acontece num driver de GPU real.

**Correção.**
- Se você tem uma GPU real: garanta que seu processo de fato usa o driver de hardware, não
  o llvmpipe. Verifique com `glxinfo | grep "OpenGL renderer"` (do pacote `mesa-demos` /
  `glx-utils`) -- deve nomear sua GPU, não "llvmpipe".
- Se você está rodando headless (CI, um container) e não pode evitar renderização por
  software: evite exercitar o efeito `mask` lá. O próprio CI deste repositório roda seu
  teste de render baseado no showcase (`showcase_test.rml`) **sem** o card mask por
  exatamente este motivo.
- É por isso que o `golden_test` embutido do glintfx (comparação pixel-exata contra
  referência) é **opt-in** (`-DGLINTFX_GOLDEN_TEST=ON`) e exige GPU real; o gate padrão do
  CI usa o teste estatístico `render_sanity` em vez disso.

### C. `find_package(glintfx)` não encontra o pacote

**Sintoma.** `find_package(glintfx REQUIRED)` falha com
`Could not find a package configuration file provided by "glintfx"`.

**Causa.** `find_package` (em "modo config", que é o que o glintfx fornece) procura um
conjunto fixo de locais padrão (prefixos de sistema como `/usr`, `/usr/local`, mais alguns
caminhos específicos de plataforma) e **não** sabe de um prefixo de instalação customizado
como `~/opt/glintfx` a menos que seja informado.

**Correção.** Aponte o CMake para o prefixo de instalação usado em `cmake --install
glintfx/build --prefix <dir>` (ver [`docs/packaging.md`](packaging.md)), seja na linha de
comando:

```sh
cmake -S seu-app -B seu-app/build -DCMAKE_PREFIX_PATH=~/opt/glintfx
```

ou como variável de ambiente antes de configurar:

```sh
export CMAKE_PREFIX_PATH=~/opt/glintfx:$CMAKE_PREFIX_PATH
cmake -S seu-app -B seu-app/build
```

`CMAKE_PREFIX_PATH` aceita uma lista separada por `:`, então dá pra combinar com outras
dependências instaladas. Verifique se o arquivo de config realmente existe em
`<prefix>/lib/cmake/glintfx/glintfxConfig.cmake` antes de investigar mais -- se estiver
ausente, o passo de install em si não rodou ou usou um `--prefix` diferente.

### D. A janela fica translúcida / transparente em alguns compositors

**Sintoma.** Em vez de uma janela opaca com sua UI desenhada por cima, parte ou toda a
janela mostra a área de trabalho/outras janelas atrás dela (mais visível em gerenciadores
de janela com composição -- compositors Wayland e algumas configurações X11 com um gerenciador
de composição ativo).

**Causa.** O render layer GL3 do glintfx trabalha com **alpha premultiplicado**
(blend `GL_ONE, GL_ONE_MINUS_SRC_ALPHA`) e compõe sobre o **FBO 0** (o framebuffer padrão /
backbuffer da janela). Se o próprio canal alpha do backbuffer da janela não for forçado
opaco (`alpha = 1`), alguns compositors leem esse alpha e misturam a janela inteira contra
a área de trabalho -- produzindo uma janela translúcida mesmo que o glintfx em si tenha
renderizado corretamente. É uma interação compositor/alpha-do-backbuffer, não um bug nos
efeitos em si. Detalhe técnico completo: [`AGENTS.md`](../AGENTS.md) "Gotchas críticos"
(alpha premultiplicado / backbuffer opaco / FBO 0).

**Correção.**
- **`glintfx::App` standalone:** isso já é tratado -- a janela GLFW é criada pedindo um
  backbuffer opaco (ver `window_glfw.cpp`/`render_gl3.cpp`). Se você ainda ver translucidez
  com `App`, vale abrir uma issue com detalhes do seu compositor e driver, já que esse
  caminho é esperado ser opaco por padrão.
- **Modo embed (`glintfx::UiLayer`):** o host é dono da janela e do contexto GL dela,
  então o host é responsável pelo estado de alpha do backbuffer (ex.: pedir uma config
  EGL/GLX de framebuffer opaco, ou zerar/forçar alpha=1 antes de compor a saída do
  glintfx). Ver [`docs/embed-integration.md`](embed-integration.md) seções 0 e 2 para o
  contrato completo de ciclo de vida de frame que o `UiLayer` espera que o host honre.
