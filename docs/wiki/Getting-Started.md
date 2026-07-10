# Getting-Started

## English

There are **two** starting points, pick based on what you already know.

### "I have never really programmed, or I'm new to this kind of project"

Read **[`docs/GUIA_INICIANTE.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/GUIA_INICIANTE.md)** in the repository. It explains every piece of jargon the first time it appears (library, compile, link, syscall, and so on), walks you through installing dependencies and building both layers of the project step by step, and ends with a full glossary. No prior computing knowledge assumed.

### "I already know C++ and CMake, I just want to use glintfx"

Read **[`docs/getting-started.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/getting-started.md)** in the repository. It is a focused, 15-minute tutorial: empty folder to a window with your first visual effect on screen.

### Quick reference, if you just want the commands

**glintfx (Layer 1), building the bundled demo:**

```sh
sudo dnf install glfw-devel freetype-devel mesa-libGL-devel   # Fedora; adjust for your distro
cmake -S glintfx -B glintfx/build
cmake --build glintfx/build
./glintfx/build/demos/showcase/glintfx_showcase
```

**loucura_c_asm (Layer 0), building and testing the zero-libc runtime:**

```sh
make build
make test
```

See [Layer-1-glintfx](Layer-1-glintfx) and [Layer-0-Core](Layer-0-Core) for what these actually are.

---

## Português

Existem **dois** pontos de partida, escolha conforme o que você já sabe.

### "Eu nunca programei de verdade, ou sou novo(a) nesse tipo de projeto"

Leia **[`docs/GUIA_INICIANTE.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/GUIA_INICIANTE.md)** no repositório. Ele explica todo jargão na primeira vez que aparece (biblioteca, compilar, linkar, syscall, etc.), te leva por instalar as dependências e buildar as duas camadas do projeto passo a passo, e termina com um glossário completo. Não assume conhecimento prévio de computação.

### "Eu já sei C++ e CMake, só quero usar o glintfx"

Leia **[`docs/getting-started.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/getting-started.md)** no repositório. É um tutorial focado, de 15 minutos: de uma pasta vazia a uma janela com seu primeiro efeito visual na tela.

### Referência rápida, se você só quer os comandos

**glintfx (Camada 1), buildando o demo embutido:**

```sh
sudo dnf install glfw-devel freetype-devel mesa-libGL-devel   # Fedora; ajuste pra sua distro
cmake -S glintfx -B glintfx/build
cmake --build glintfx/build
./glintfx/build/demos/showcase/glintfx_showcase
```

**loucura_c_asm (Camada 0), buildando e testando o runtime zero-libc:**

```sh
make build
make test
```

Ver [Layer-1-glintfx](Layer-1-glintfx) e [Layer-0-Core](Layer-0-Core) pra saber o que isso de fato é.
