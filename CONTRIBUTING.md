# Contributing to glintfx

> **EN:** How to build, test, and submit changes. Bilingual: English first, then Português.
> **PT:** Como buildar, testar e enviar mudanças. Bilíngue: inglês primeiro, depois português.

---

## English

Thank you for your interest in `glintfx`. The project is currently single-author (Petrus Silva Costa). Note the licensing point below before opening a pull request.

### Prerequisites

| Tool | Notes |
| :--- | :--- |
| Linux x86-64 | The only supported platform. |
| clang | C++17 floor, C++23 target. |
| CMake | >= 3.16. |
| System packages (Fedora) | `glfw-devel`, `freetype-devel`, `mesa-libGL-devel`. |
| Xvfb | For headless tests (`ctest`). |

RmlUi 6.3 is fetched automatically at configure time; gl3w is vendored in the repo.

### Build and test

```sh
# install system deps (Fedora)
sudo dnf install glfw-devel freetype-devel mesa-libGL-devel

# configure + build
cmake -S glintfx -B glintfx/build
cmake --build glintfx/build

# run the test suite (headless, under Xvfb)
ctest --test-dir glintfx/build --output-on-failure
```

The default suite has 5 tests: `window_smoke`, `render_smoke`, `bootstrap_smoke`, `app_smoke`, and `render_sanity`. The pixel-exact `golden_test` is opt-in (`-DGLINTFX_GOLDEN_TEST=ON`) and is flaky under software GL; run it only on real GPU hardware. Effects are validated visually on a real GPU.

Before submitting, build the `consumer-example/` too, to confirm the drop-in path still works.

### Coding standards

- **SPDX header on every code file:** first line `SPDX-License-Identifier: MPL-2.0`, in the comment style for the language (`//` C/C++, `;` NASM, `#` CMake). Do not add SPDX to `.md` files.
- **Identifiers in English only.** No pt-br in symbol names.
- **Docs are bilingual (en then pt) in the same file.**
- **Public headers must not expose third-party types.** Nothing from GL, GLFW, or RmlUi may appear in `glintfx/include/glintfx/`; use pImpl.
- **Assembly (Layer 0) is Intel syntax.**
- See [`AGENTS.md`](AGENTS.md) for the full convention list and the RmlUi/GL3 gotchas.

### Commits and pull requests

- Use **Conventional Commits** with the message body in pt-br (e.g. `docs: adiciona guia de efeitos`, `fix(glintfx): corrige flip do snapshot`).
- When a commit closes or advances a `TODO.md` item, cite its ID in the body (e.g. `L1-API`) and update its `Status` in the same commit (delivered work goes to `🔍 Pendente verificação`, never straight to `✅`).
- Keep PRs focused. Describe what changed and how you tested it. Confirm the test suite passes and the demo still renders.
- New unplanned findings go to the **INBOX** at the bottom of [`TODO.md`](TODO.md), not into the ordered table.

### Licensing of contributions

The project is **MPL-2.0** and currently has a single author, which keeps relicensing clean. External contributions may require a contributor agreement (CLA) or relicensing consent before they can be merged. If you plan a substantial contribution, open an issue first so this can be sorted out. Do not copy code from third-party libraries; any reimplementation of their behaviour must be clean-room (from understanding, not from their source).

### Reporting issues

Open an issue on [Codeberg](https://codeberg.org/petrinhu/glintfx) or [GitHub](https://github.com/petrinhu/glintfx). For security-sensitive reports, follow [`SECURITY.md`](SECURITY.md) instead.

---

## Português

Obrigado pelo interesse no `glintfx`. O projeto é atualmente de autor único (Petrus Silva Costa). Observe o ponto sobre licença abaixo antes de abrir um pull request.

### Pré-requisitos

| Ferramenta | Notas |
| :--- | :--- |
| Linux x86-64 | A única plataforma suportada. |
| clang | Piso C++17, alvo C++23. |
| CMake | >= 3.16. |
| Pacotes de sistema (Fedora) | `glfw-devel`, `freetype-devel`, `mesa-libGL-devel`. |
| Xvfb | Para os testes headless (`ctest`). |

RmlUi 6.3 é baixado automaticamente em tempo de configure; gl3w é vendorizado no repo.

### Buildar e testar

```sh
# instalar deps de sistema (Fedora)
sudo dnf install glfw-devel freetype-devel mesa-libGL-devel

# configure + build
cmake -S glintfx -B glintfx/build
cmake --build glintfx/build

# rodar a suíte de testes (headless, sob Xvfb)
ctest --test-dir glintfx/build --output-on-failure
```

A suíte padrão tem 5 testes: `window_smoke`, `render_smoke`, `bootstrap_smoke`, `app_smoke` e `render_sanity`. O `golden_test` pixel-exato é opt-in (`-DGLINTFX_GOLDEN_TEST=ON`) e é flaky sob GL de software; rode só em GPU real. Os efeitos são validados visualmente em GPU real.

Antes de enviar, builde também o `consumer-example/` para confirmar que o caminho drop-in continua funcionando.

### Padrões de código

- **Header SPDX em todo arquivo de código:** primeira linha `SPDX-License-Identifier: MPL-2.0`, no estilo de comentário da linguagem (`//` C/C++, `;` NASM, `#` CMake). Não adicione SPDX em arquivos `.md`.
- **Identificadores apenas em inglês.** Nada de pt-br em nome de símbolo.
- **Docs são bilíngues (en depois pt) no mesmo arquivo.**
- **Headers públicos não podem expor tipos de terceiros.** Nada de GL, GLFW ou RmlUi pode aparecer em `glintfx/include/glintfx/`; use pImpl.
- **Assembly (Camada 0) é sintaxe Intel.**
- Ver [`AGENTS.md`](AGENTS.md) para a lista completa de convenções e os gotchas de RmlUi/GL3.

### Commits e pull requests

- Use **Conventional Commits** com o corpo da mensagem em pt-br (ex.: `docs: adiciona guia de efeitos`, `fix(glintfx): corrige flip do snapshot`).
- Quando um commit fecha ou avança um item do `TODO.md`, cite o ID dele no corpo (ex.: `L1-API`) e atualize o `Status` no mesmo commit (trabalho entregue vai para `🔍 Pendente verificação`, nunca direto para `✅`).
- Mantenha os PRs focados. Descreva o que mudou e como testou. Confirme que a suíte passa e que o demo ainda renderiza.
- Descobertas novas não planejadas vão para a **INBOX** no fim do [`TODO.md`](TODO.md), não para a tabela ordenada.

### Licença das contribuições

O projeto é **MPL-2.0** e tem atualmente um único autor, o que mantém o relicenciamento limpo. Contribuições externas podem exigir um acordo de contribuidor (CLA) ou consentimento de relicenciamento antes de serem mergeadas. Se planeja uma contribuição substancial, abra uma issue primeiro para resolver isso. Não copie código de bibliotecas de terceiros; qualquer reimplementação do comportamento delas deve ser clean-room (a partir do entendimento, não do código-fonte delas).

### Reportar problemas

Abra uma issue no [Codeberg](https://codeberg.org/petrinhu/glintfx) ou no [GitHub](https://github.com/petrinhu/glintfx). Para reportes sensíveis de segurança, siga o [`SECURITY.md`](SECURITY.md).
