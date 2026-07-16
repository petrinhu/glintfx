# Home

[![License: MPL-2.0](https://img.shields.io/badge/License-MPL--2.0-orange.svg)](https://codeberg.org/petrinhu/glintfx/src/branch/main/LICENSE)
[![Version: 0.11.0](https://img.shields.io/badge/Version-0.11.0-blue.svg)](https://codeberg.org/petrinhu/glintfx/src/branch/main/CHANGELOG.md)
[![API: pre-1.0](https://img.shields.io/badge/API-pre--1.0%20(may%20change)-yellow.svg)](https://codeberg.org/petrinhu/glintfx/src/branch/main/CHANGELOG.md)
[![Standard: C++17 / C++23](https://img.shields.io/badge/Standard-C%2B%2B17%20to%20C%2B%2B23-00599C.svg)](#)
[![Platform: Linux x86-64](https://img.shields.io/badge/Platform-Linux%20x86--64-FCC624.svg)](#)
[![RmlUi 6.3](https://img.shields.io/badge/RmlUi-6.3-5fd0ff.svg)](https://github.com/mikke89/RmlUi)
[![OpenGL 3.3](https://img.shields.io/badge/OpenGL-3.3-5586A4.svg)](#)
[![CI (GitHub)](https://github.com/petrinhu/glintfx/actions/workflows/ci.yml/badge.svg)](https://github.com/petrinhu/glintfx/actions/workflows/ci.yml)
[![CI (Codeberg)](https://codeberg.org/petrinhu/glintfx/actions/workflows/ci.yml/badge.svg)](https://codeberg.org/petrinhu/glintfx/actions/workflows/ci.yml)
[![Coverage](https://img.shields.io/endpoint?url=https://codeberg.org/petrinhu/glintfx/raw/branch/main/glintfx/coverage-badge.json)](https://codeberg.org/petrinhu/glintfx/actions/workflows/ci.yml)

> Repository: [Codeberg](https://codeberg.org/petrinhu/glintfx) (primary) / [GitHub](https://github.com/petrinhu/glintfx) (mirror). This wiki is a lightweight index; the real documentation lives as versioned files inside the repository itself, under `docs/`.
> Repositório: [Codeberg](https://codeberg.org/petrinhu/glintfx) (principal) / [GitHub](https://github.com/petrinhu/glintfx) (espelho). Esta wiki é um índice leve; a documentação de verdade vive como arquivos versionados dentro do próprio repositório, em `docs/`.

## English

Welcome. This repository holds two very different pieces of software, called "layers":

- **[Layer 1, glintfx](Layer-1-glintfx)**: a released C++ library that fuses an HTML/CSS-style UI engine with GPU visual effects. This is the project's **active product**.
- **[Layer 0, loucura_c_asm](Layer-0-Core)**: an experimental, zero-libc C+Assembly runtime, talking to the Linux kernel only through raw syscalls. Implementation complete, awaiting a final audit; a long-term, no-deadline track.

**New here? Start at [Getting-Started](Getting-Started).** It routes you to the right tutorial depending on how much you already know.

Other useful pages:

- [FAQ](FAQ), quick answers to the questions this project gets asked most.
- [Layer-1-glintfx](Layer-1-glintfx), the product overview.
- [Layer-0-Core](Layer-0-Core), the experimental runtime overview.

Direct links into the repository's own docs (these are the source of truth, this wiki only points at them):

- [`README.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/README.md), the project's front page.
- [`docs/GUIA_INICIANTE.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/GUIA_INICIANTE.md), the extensive beginner guide (bilingual, assumes no prior computing knowledge).
- [`docs/getting-started.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/getting-started.md), the developer tutorial (assumes basic C++/CMake).
- [`docs/effects.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/effects.md), the visual-effects how-to and reference (RCSS syntax).
- [`docs/embed-integration.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/embed-integration.md), the reference for embedding glintfx into a host application.
- [`docs/adr/README.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/README.md), the index of Architecture Decision Records (the "why" behind every major design choice).
- [`CHANGELOG.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/CHANGELOG.md), the dated history of every released version.

---

## Português

Bem-vindo(a). Este repositório guarda dois softwares bem diferentes, chamados de "camadas":

- **[Camada 1, glintfx](Layer-1-glintfx)**: uma biblioteca C++ lançada que funde um motor de UI estilo HTML/CSS com efeitos visuais de GPU. Este é o **produto ativo** do projeto.
- **[Camada 0, loucura_c_asm](Layer-0-Core)**: um runtime experimental C+Assembly, zero libc, falando com o kernel Linux só por syscalls crus. Implementação completa, aguardando uma auditoria final; uma trilha de longo prazo, sem prazo.

**Novo(a) aqui? Comece em [Getting-Started](Getting-Started).** Ela te encaminha pro tutorial certo dependendo de quanto você já sabe.

Outras páginas úteis:

- [FAQ](FAQ), respostas rápidas às perguntas mais frequentes deste projeto.
- [Layer-1-glintfx](Layer-1-glintfx), a visão geral do produto.
- [Layer-0-Core](Layer-0-Core), a visão geral do runtime experimental.

Links diretos pros docs de verdade do repositório (eles são a fonte de verdade, esta wiki só aponta pra eles):

- [`README.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/README.md), a página de rosto do projeto.
- [`docs/GUIA_INICIANTE.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/GUIA_INICIANTE.md), o guia extenso para iniciantes (bilíngue, não assume conhecimento prévio de computação).
- [`docs/getting-started.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/getting-started.md), o tutorial de dev (assume C++/CMake básico).
- [`docs/effects.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/effects.md), o how-to e referência de efeitos visuais (sintaxe RCSS).
- [`docs/embed-integration.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/embed-integration.md), a referência pra embutir o glintfx numa aplicação host.
- [`docs/adr/README.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/README.md), o índice de Architecture Decision Records (o "porquê" de cada escolha de design importante).
- [`CHANGELOG.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/CHANGELOG.md), o histórico datado de toda versão lançada.
