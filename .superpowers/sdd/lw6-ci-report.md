# LW6 CI Report — glintfx CI versionado (L1.2-CI)

**Status:** ENTREGUE (matrix GLFW=ON|OFF + warning golden adicionados)  
**Hashes:** `9f43a88` (criação inicial) · `919ab69` (correção Forgejo) · `cd1c47c` (matrix embed-only)  
**Branches:** `chore/post-v0.1` (rounds 1-2) · `fix/glfw-optional-backend` (round 3)  
**Data:** 2026-06-29 / 2026-06-30  
**Agent:** devops-sre  

---

## O que foi criado

### Pipelines de CI

| Arquivo | Plataforma | Runner | Container | Gate? |
| :--- | :--- | :--- | :--- | :--- |
| `.github/workflows/ci.yml` | GitHub Actions | `ubuntu-latest` (gratuito) | sem container (runner tem tudo) | **Sim — gate principal** |
| `.forgejo/workflows/ci.yml` | Codeberg Forgejo Actions | `codeberg-medium` | `ghcr.io/catthehacker/ubuntu:act-latest` | Sim — soberania |

### Docs atualizados

- `README.md` — dois badges de CI (GitHub + Codeberg) adicionados; bullet "Sem CI versionado ainda" substituído por "CI ativo".
- `TESTES.md` — seção glintfx menciona os dois pipelines e o item L1.2-CI.
- `TODO.md` — L1.2-CI: `⏳ Pendente` → `🔍 Pendente verificação`.

---

## Correção Forgejo (commit 919ab69)

O primeiro commit (`9f43a88`) usava `ubuntu-latest` (label inválido no Codeberg — é label do GitHub) e não declarava `container:`. Após leitura das memórias `codeberg-runners`, `forgejo-actions-cookbook` e `reference_ler_falhas_ci_forgejo`, a correção aplicou:

| Campo | Antes (errado) | Depois (correto) | Motivo |
| :--- | :--- | :--- | :--- |
| `runs-on` | `ubuntu-latest` | `codeberg-medium` | Label inválido no Codeberg; `codeberg-medium` é o runner global ativo (4 CPU/8 GB/10 min) |
| `container:` | ausente | `ghcr.io/catthehacker/ubuntu:act-latest` | Runners Codeberg NÃO têm stack pré-instalada — container obrigatório |
| `sudo` nos apt-get | presente | removido | Dentro do container roda como root — sudo desnecessário |
| `git` nas deps | ausente | adicionado | FetchContent precisa de `git` para clonar RmlUi |
| `libegl1-mesa-dev` | ausente | adicionado | Header EGL do GLFW (FindOpenGL CMake) |
| `libxkbcommon-dev` | ausente | adicionado | Header XKB de layout de teclado do GLFW |
| `concurrency:` | ausente | adicionado | Cancelar runs em andamento — poupar fila do pool compartilhado |

---

## Validação local (forgejo-runner exec)

**Ferramenta:** `forgejo-runner` v12.10.0  
**Comando:**
```sh
forgejo-runner exec \
  -W .forgejo/workflows/ci.yml \
  --detect-event \
  -j build-and-test
```
**Imagem usada:** `ghcr.io/catthehacker/ubuntu:act-latest` (Ubuntu 24.04 noble — conforme `container:` no workflow)

### Resultado por step

| Step | Status |
| :--- | :--- |
| checkout | ✅ Success |
| cache rmlui (fetchcontent) | ✅ Success (miss — sem cache local, esperado) |
| install deps | ✅ Success — 63 packages instalados |
| configure | ✅ Success |
| build | ✅ Success |
| test | ✅ **100% tests passed, 0 tests failed out of 5** |
| Post cache rmlui | ✅ Success (save com 403 local — artefato do executor local; no Codeberg real o cache salva normalmente) |
| **Job** | **succeeded (EXIT=0)** |

### Resultado dos 5 testes

```
1/5 Test #1: window_smoke    Passed    0.10 sec
2/5 Test #2: render_smoke    Passed    0.14 sec
3/5 Test #3: bootstrap_smoke Passed    0.11 sec
4/5 Test #4: app_smoke       Passed    0.11 sec
5/5 Test #5: render_sanity   Passed    0.53 sec

100% tests passed, 0 tests failed out of 5
Total Test time (real) =   0.99 sec
```

**Todos os 5 testes passando localmente sob o mesmo container que rodará no Codeberg.**

---

## Decisão de plataforma

### Por que GitHub Actions como gate principal

- Runners `ubuntu-latest` gratuitos, sem configuração adicional.
- O repo já tem `github` como remote rastreado para `chore/post-v0.1`.
- Validação no primeiro push: instantânea.

### Por que Forgejo Actions (não Woodpecker LOCAL)

- O repo vive em remotes **públicos** (Codeberg + GitHub). Woodpecker LOCAL exigiria servidor Woodpecker self-hosted com webhook registrado no Codeberg/GitHub — infraestrutura inexistente.
- Forgejo Actions é o CI **nativo** do Codeberg (mesma instância). Runner `codeberg-medium` serve repos públicos automaticamente com a unidade *Actions* habilitada.
- Sintaxe Forgejo Actions é idêntica à GitHub Actions — manutenção zero entre os dois arquivos (só mudam `runs-on:`, `container:` e `sudo`).

---

## Detalhes técnicos do pipeline

### Trigger (path filter)

```yaml
on:
  push:
    paths: ["glintfx/**", ".forgejo/workflows/ci.yml"]
  pull_request:
    paths: ["glintfx/**", ".forgejo/workflows/ci.yml"]
```

Só dispara quando código da lib ou o workflow muda. Pushes na Camada 0 (C/ASM) não acionam CI da glintfx.

### Cache RmlUi (FetchContent)

Chave: `rmlui-<runner.os>-<sha256 de glintfx/CMakeLists.txt>`. Invalida automaticamente quando o `GIT_TAG` do RmlUi muda.

### Deps de sistema (Ubuntu — Forgejo container `act-latest`)

| Pacote | Para quê |
| :--- | :--- |
| `cmake g++` | build |
| `git` | FetchContent para clonar RmlUi |
| `libglfw3-dev` | GLFW (sistema) |
| `libfreetype-dev` | FreeType (sistema) |
| `libgl1-mesa-dev` | headers OpenGL + libGL.so (FindOpenGL) |
| `libegl1-mesa-dev` | headers EGL (FindOpenGL, GLFW) |
| `libxkbcommon-dev` | header XKB para GLFW |
| `libgl1-mesa-dri` | llvmpipe — renderer software Mesa (backend GL do Xvfb) |
| `xvfb` | display virtual X + `xvfb-run` |

### Por que `GLINTFX_BUILD_DEMOS=OFF` funciona para os testes

`tests/CMakeLists.txt` copia assets do showcase via `file(COPY ...)` em configure — independente de `GLINTFX_BUILD_DEMOS`. Os executáveis de demo são desnecessários no CI.

### Runner Codeberg — estado oscila

`codeberg-medium` é o runner global ativo em 2026-06-13 (4 CPU / 8 GB / 10 min). O estado dos runners **oscila** — reconfirmar em `codeberg.org/actions/meta` antes do 1º push. Se o job ficar em `waiting` por minutos, é fila do pool compartilhado (não erro de YAML). Fallback: `codeberg-medium-lazy` (até 24h de janela).

---

## Validação no Codeberg real

A validação local com `forgejo-runner exec` confirma o workflow. A validação no runner Codeberg ocorre no primeiro push:

```sh
git push github chore/post-v0.1   # dispara GitHub Actions
git push origin chore/post-v0.1   # dispara Codeberg Forgejo Actions
```

Acompanhar em:
- GitHub: https://github.com/petrinhu/glintfx/actions
- Codeberg: https://codeberg.org/petrinhu/glintfx/actions

**Pré-requisito Codeberg:** habilitar unidade *Actions* em Settings do repo (pode ser via API: `PATCH /repos/{owner}/{repo}` com `has_actions=true`).

---

## Rollback

```sh
git revert 919ab69 9f43a88
```
Remove os dois arquivos de workflow e desfaz alterações de README/TESTES/TODO. Sem efeito colateral em infra ou configuração remota.

---

---

## Round 3 — Matrix embed-only (GLFW=OFF) (commit cd1c47c, branch fix/glfw-optional-backend)

**Problema:** README afirmava que a suíte embed (GLFW=OFF, 5 testes) roda no CI, mas os workflows só testavam GLFW=ON.

**Solução:** matrix `backend: [ON, OFF]` nos dois workflows:

| Config | Build dir | Cache key | Testes |
| :--- | :--- | :--- | :--- |
| GLFW=ON | `glintfx/build-ON` | `rmlui-<os>-glfwON-<hash>` | 10 (5 backend GLFW + 5 embed) |
| GLFW=OFF | `glintfx/build-OFF` | `rmlui-<os>-glfwOFF-<hash>` | 5 (embed: ui_layer_attach/compose/gl_state_guard/events/sanity) |

`libglfw3-dev` instalado em ambas as configs: no modo GLFW=OFF os testes embed usam uma janela GLFW oculta como fixture de contexto GL (dep só-de-teste; a lib não linka GLFW).

**CMake warning:** adicionado em `tests/CMakeLists.txt` — quando `GLINTFX_GOLDEN_TEST=ON` + `GLINTFX_BACKEND_GLFW=OFF` emite `message(WARNING ...)` em vez de aceitar silenciosamente.

### Validação local (round 3)

```
forgejo-runner exec -W .forgejo/workflows/ci.yml --detect-event
```

Matrix identificada: 2 jobs paralelos (`GLFW=ON` e `GLFW=OFF`) com container `act-latest`. Cache 403 esperado (local). `apt-get` de ambos os jobs iniciou com sucesso.

Validação dos 5 testes embed no build-noglfw local:
```
1/5 Test #1: ui_layer_attach  Passed  3.27 sec
2/5 Test #2: ui_layer_compose Passed  3.27 sec
3/5 Test #3: gl_state_guard   Passed  3.27 sec
4/5 Test #4: ui_layer_events  Passed  3.26 sec
5/5 Test #5: ui_layer_sanity  Passed  3.29 sec

100% tests passed, 0 tests failed out of 5
Total Test time (real) = 16.36 sec
```

---

## Próximos passos (pós-push)

1. Verificar CI verde em ambas as configs (ON e OFF) no GitHub → marcar L1.2-CI como `✅ Concluído`.
2. Se Codeberg ficar em `waiting` > 5 min: verificar `codeberg.org/actions/meta` (runners globais) e Settings → Actions do repo (unidade habilitada?).
3. L1.8-FINDPKG e L1.1-ERRSTRAT são os demais itens LW6 em aberto.
