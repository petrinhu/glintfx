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

RmlUi 6.3 is fetched automatically at configure time; glintfx's own GL loader (`src/gl_loader.h`/`.c`, generated from the Khronos `gl.xml` registry) is vendored in the repo.

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

The default suite has **102 tests** with `-DGLINTFX_BACKEND_GLFW=ON` (the default) and **71 tests** in an embed-only build (`-DGLINTFX_BACKEND_GLFW=OFF`), including `window_smoke`, `render_smoke`, `engine_smoke`, `app_smoke`, and `render_sanity`. `bootstrap_smoke` from earlier releases was replaced by `engine_smoke` once `Bootstrap::init` stopped accepting `WindowGlfw` directly. `ctest -N` lists every test name currently registered. The pixel-exact `golden_test` is opt-in (`-DGLINTFX_GOLDEN_TEST=ON`) and is flaky under software GL; run it only on real GPU hardware. Effects are validated visually on a real GPU.

Before submitting, build the `consumer-example/` too, to confirm the drop-in path still works.

### Coding standards

- **SPDX header on every code file:** first line `SPDX-License-Identifier: Apache-2.0`, in the comment style for the language (`//` C/C++, `;` NASM, `#` CMake). Do not add SPDX to `.md` files.
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

### Versioning

`glintfx` follows [Semantic Versioning](https://semver.org/) with an explicit rule while in the 0.x line:

> While in 0.x: **minor** = any addition or change to the public API/ABI (new method, new header, changed signature); **patch** = only bug fixes, docs, or internal-only changes. Intentional breaking changes are documented in the CHANGELOG's "Changed" section with a migration note. From 1.0 onward: strict SemVer.

This policy applies going forward. Past tags are historical and are not rewritten to match it.

### Licensing of contributions

The project is **Apache-2.0** (tags up to `v0.28.0` stay MPL-2.0 forever for whoever obtained those releases -- see [ADR-0019](docs/adr/0019-license-rotation-apache-2.0.md)). If you plan a substantial contribution, open an issue first so this can be sorted out. Do not copy code from third-party libraries; any reimplementation of their behaviour must be clean-room (from understanding, not from their source).

**As of the license rotation to Apache-2.0** ([ADR-0019](docs/adr/0019-license-rotation-apache-2.0.md); see that ADR for the full context, this note documents the policy it establishes once the rotation lands, without repeating the reasoning here):

- **Inbound = outbound, no CLA.** Apache-2.0 §5 ("Submission of Contributions") already states that, unless you say otherwise in writing, anything you submit for inclusion is licensed to the project under the very same Apache-2.0 terms, automatically, with no separate signature. This is precisely the CLA-shaped gap the single-author window (ADR-0007, carried forward by ADR-0019) existed to close: once the project is Apache-2.0, external contributions no longer need a CLA or a one-off relicensing consent to be merged. The "open an issue first" advice above still stands for coordination, not for licensing paperwork.
- **§4(b) ("modified files must carry prominent notice"): the project's position is that the git commit history is that notice.** Apache-2.0 does not mandate an in-file changelog comment; a commit's diff, author, date, and message already state, prominently and permanently, per file, that a change was made and by whom. We rely on that record rather than asking contributors to hand-annotate every touched file.

### Reporting issues

Open an issue on [GitHub](https://github.com/petrinhu/glintfx). For security-sensitive reports, follow [`SECURITY.md`](SECURITY.md) instead.

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

RmlUi 6.3 é baixado automaticamente em tempo de configure; o loader GL próprio do glintfx (`src/gl_loader.h`/`.c`, gerado a partir do registro Khronos `gl.xml`) é vendorizado no repo.

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

A suíte padrão tem **102 testes** com `-DGLINTFX_BACKEND_GLFW=ON` (o default) e **71 testes** num build embed-only (`-DGLINTFX_BACKEND_GLFW=OFF`), incluindo `window_smoke`, `render_smoke`, `engine_smoke`, `app_smoke` e `render_sanity`. O `bootstrap_smoke` de releases anteriores foi substituído pelo `engine_smoke` quando o `Bootstrap::init` parou de aceitar `WindowGlfw` diretamente. `ctest -N` lista todo teste registrado atualmente. O `golden_test` pixel-exato é opt-in (`-DGLINTFX_GOLDEN_TEST=ON`) e é flaky sob GL de software; rode só em GPU real. Os efeitos são validados visualmente em GPU real.

Antes de enviar, builde também o `consumer-example/` para confirmar que o caminho drop-in continua funcionando.

### Padrões de código

- **Header SPDX em todo arquivo de código:** primeira linha `SPDX-License-Identifier: Apache-2.0`, no estilo de comentário da linguagem (`//` C/C++, `;` NASM, `#` CMake). Não adicione SPDX em arquivos `.md`.
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

### Versionamento

O `glintfx` segue [Semantic Versioning](https://semver.org/) com uma regra explícita enquanto estiver na linha 0.x:

> Enquanto 0.x: **minor** = qualquer adição ou mudança de API/ABI pública (método novo, header novo, assinatura alterada); **patch** = só correção de bug, doc, interno. Breaking intencional documentado na seção "Changed" do CHANGELOG com nota de migração. A partir de 1.0: SemVer estrito.

Esta política vale daqui pra frente. Tags passadas são históricas e não são reescritas para se adequar a ela.

### Licença das contribuições

O projeto é **Apache-2.0** (as tags até a `v0.28.0` seguem MPL-2.0 para sempre, para quem obteve aquelas releases -- ver [ADR-0019](docs/adr/0019-license-rotation-apache-2.0.md)). Se planeja uma contribuição substancial, abra uma issue primeiro para resolver isso. Não copie código de bibliotecas de terceiros; qualquer reimplementação do comportamento delas deve ser clean-room (a partir do entendimento, não do código-fonte delas).

**A partir da rotação de licença para Apache-2.0** ([ADR-0019](docs/adr/0019-license-rotation-apache-2.0.md); ver esse ADR para o contexto completo, esta nota registra a política que ele estabelece assim que a rotação acontecer, sem repetir a fundamentação aqui):

- **Inbound = outbound, sem CLA.** O §5 da Apache-2.0 ("Submission of Contributions") já diz que, a menos que você declare o contrário por escrito, qualquer coisa que você envie para inclusão é licenciada ao projeto sob os MESMOS termos Apache-2.0, automaticamente, sem assinatura separada. É exatamente a lacuna em forma de CLA que a janela de autor único (ADR-0007, herdada pelo ADR-0019) existia para fechar: a partir do momento em que o projeto é Apache-2.0, contribuições externas não precisam mais de CLA nem de consentimento avulso de relicenciamento para serem mergeadas. O conselho de "abra uma issue primeiro", acima, continua valendo para coordenação, não para burocracia de licença.
- **§4(b) ("arquivos modificados devem carregar aviso proeminente"): a posição do projeto é que o histórico de commits do git É esse aviso.** A Apache-2.0 não exige um comentário de changelog dentro do arquivo; o diff, autor, data e mensagem de um commit já declaram, de forma proeminente, permanente e por-arquivo, que uma mudança aconteceu e quem a fez. Contamos com esse registro em vez de pedir que contribuidores anotem à mão cada arquivo tocado.

### Reportar problemas

Abra uma issue no [GitHub](https://github.com/petrinhu/glintfx). Para reportes sensíveis de segurança, siga o [`SECURITY.md`](SECURITY.md).
