# Wiki source files / Arquivos-fonte da Wiki

> **This is NOT the Wiki itself.** This folder holds the *source* Markdown pages for the project's Codeberg/GitHub Wiki. The Wiki is a **separate git repository** (`<repo>.wiki.git`), not part of this repository's own commit history: publishing it is a deliberate, separate action, out of scope for a documentation-only commit. This page tells whoever does the publishing (the project lead or an orchestrator agent with remote-push authority) exactly how.
>
> **Isto NÃO é a Wiki em si.** Esta pasta guarda as páginas Markdown *fonte* da Wiki do projeto no Codeberg/GitHub. A Wiki é um **repositório git separado** (`<repo>.wiki.git`), fora do histórico de commits deste repositório: publicá-la é uma ação deliberada e separada, fora do escopo de um commit só-de-documentação. Esta página diz a quem for publicar (o líder do projeto ou um agent orquestrador com autoridade de push em remoto) exatamente como.

## English

### Why the Wiki is a separate git repository

Codeberg (Forgejo) and GitHub both implement their Wiki feature as an ordinary git repository living at a fixed URL next to the main one: `<url>/<owner>/<repo>.wiki.git`. Cloning it, editing files, and pushing is identical to working with any other git repository; there is no special web-only editing requirement. Because it is a **separate git history**, this repository's own commits (including the one that added this folder) never touch it directly: a human (or an authorized agent) has to explicitly clone it and push.

### Pages in this folder

| File here | Becomes wiki page | Purpose |
| :--- | :--- | :--- |
| `Home.md` | `Home` (the wiki's landing page) | Orientation plus links to everything else, both wiki pages and the real docs in `docs/`. |
| `Getting-Started.md` | `Getting-Started` | Short redirect-style page pointing at the two real tutorials (developer-level and total-beginner-level). |
| `Layer-1-glintfx.md` | `Layer-1-glintfx` | One-screen overview of the active product (glintfx) with links to its docs. |
| `Layer-0-Core.md` | `Layer-0-Core` | One-screen overview of the experimental C+ASM runtime with links to its docs. |
| `FAQ.md` | `FAQ` | Short answers to the questions this project gets asked most often. |

Every page here is intentionally **short** (a wiki page should be skimmable in under a minute) and **link-heavy**: none of them duplicate the content of `docs/getting-started.md`, `docs/GUIA_INICIANTE.md`, `docs/effects.md`, the ADRs, etc.; they point at those files as the single source of truth.

### How to publish (step by step)

Run these commands from any machine with push access to the repository's remote (Codeberg is the primary remote; mirror to GitHub the same way if desired). **This step is a remote-visible action (a push) and, per this project's own working rules, requires the project lead's explicit authorization before running.** Do not run it as a routine follow-up to a documentation commit.

```sh
# 1. Clone the wiki repo (note the .wiki.git suffix, a URL distinct from the main repo)
git clone https://codeberg.org/petrinhu/glintfx.wiki.git glintfx-wiki
cd glintfx-wiki

# 2. Copy the prepared pages from the main repo's working tree into the wiki clone
#    (adjust the source path to wherever your main-repo checkout lives)
cp /path/to/loucura_c_asm/docs/wiki/Home.md .
cp /path/to/loucura_c_asm/docs/wiki/Getting-Started.md .
cp /path/to/loucura_c_asm/docs/wiki/Layer-1-glintfx.md .
cp /path/to/loucura_c_asm/docs/wiki/Layer-0-Core.md .
cp /path/to/loucura_c_asm/docs/wiki/FAQ.md .
# NOTE: do not copy this README.md itself, it is instructions for publishing,
# not a wiki page; a wiki repo with no README.md falls back to Home.md as its
# landing page, which is what we want.

# 3. Commit and push to the wiki repo (a repo of its own, normal git workflow)
git add Home.md Getting-Started.md Layer-1-glintfx.md Layer-0-Core.md FAQ.md
git commit -m "docs(wiki): sync wiki pages from docs/wiki/ in main repo"
git push origin main   # or "master": check `git branch` after the clone, Forgejo/Gitea wikis default to "master" on older instances

# 4. (Optional) Mirror the same content to the GitHub wiki
git clone https://github.com/petrinhu/glintfx.wiki.git glintfx-wiki-github
# repeat steps 2-3 against this second clone/remote
```

### Keeping it in sync later

There is no automation wiring `docs/wiki/` in this repository to the live Wiki (deliberately: a Wiki push is a remote action requiring explicit authorization, not something CI should do unattended). When a future change updates `docs/wiki/*.md` in a normal commit, re-run the four `cp` + `git commit` + `git push` steps above against the wiki clone to bring the live Wiki back in sync. If this becomes a frequent chore, consider proposing a manually-triggered CI job for it, but that is a new decision, not something this document authorizes on its own.

---

## Português

### Por que a Wiki é um repositório git separado

Tanto o Codeberg (Forgejo) quanto o GitHub implementam a funcionalidade de Wiki como um repositório git comum, vivendo numa URL fixa ao lado da principal: `<url>/<dono>/<repo>.wiki.git`. Cloná-lo, editar arquivos e dar push é idêntico a trabalhar com qualquer outro repositório git; não há exigência especial de edição só-pela-web. Por ser um **histórico git separado**, os próprios commits deste repositório (incluindo o que adicionou esta pasta) nunca o tocam diretamente: uma pessoa (ou um agent autorizado) precisa clonar e dar push explicitamente.

### Páginas nesta pasta

| Arquivo aqui | Vira a página da wiki | Propósito |
| :--- | :--- | :--- |
| `Home.md` | `Home` (página inicial da wiki) | Orientação mais links pra tudo mais, tanto páginas da wiki quanto os docs de verdade em `docs/`. |
| `Getting-Started.md` | `Getting-Started` | Página curta tipo redirecionamento, apontando pros dois tutoriais reais (nível-dev e nível-iniciante-total). |
| `Layer-1-glintfx.md` | `Layer-1-glintfx` | Visão geral de uma tela do produto ativo (glintfx) com links pros docs dele. |
| `Layer-0-Core.md` | `Layer-0-Core` | Visão geral de uma tela do runtime experimental C+ASM com links pros docs dele. |
| `FAQ.md` | `FAQ` | Respostas curtas às perguntas mais frequentes deste projeto. |

Toda página aqui é deliberadamente **curta** (uma página de wiki deve ser lida por cima em menos de um minuto) e **cheia de links**: nenhuma delas duplica o conteúdo de `docs/getting-started.md`, `docs/GUIA_INICIANTE.md`, `docs/effects.md`, os ADRs, etc.; elas apontam pra esses arquivos como fonte única de verdade.

### Como publicar (passo a passo)

Rode estes comandos de qualquer máquina com acesso de push ao remoto do repositório (Codeberg é o remoto principal; espelhar pro GitHub do mesmo jeito se desejado). **Este passo é uma ação visível em remoto (um push) e, pelas regras de trabalho deste projeto, exige autorização explícita do líder antes de rodar.** Não rode como consequência de rotina de um commit só-de-documentação.

```sh
# 1. Clone o repo da wiki (note o sufixo .wiki.git, uma URL diferente do repo principal)
git clone https://codeberg.org/petrinhu/glintfx.wiki.git glintfx-wiki
cd glintfx-wiki

# 2. Copie as páginas preparadas da working tree do repo principal pro clone da wiki
#    (ajuste o caminho de origem pra onde estiver seu checkout do repo principal)
cp /caminho/para/loucura_c_asm/docs/wiki/Home.md .
cp /caminho/para/loucura_c_asm/docs/wiki/Getting-Started.md .
cp /caminho/para/loucura_c_asm/docs/wiki/Layer-1-glintfx.md .
cp /caminho/para/loucura_c_asm/docs/wiki/Layer-0-Core.md .
cp /caminho/para/loucura_c_asm/docs/wiki/FAQ.md .
# NOTA: não copie este README.md, ele é instrução de publicação, não uma página
# de wiki; uma wiki sem README.md cai de volta pro Home.md como página inicial,
# que é o que queremos.

# 3. Commit e push pro repo da wiki (um repo próprio, fluxo git normal)
git add Home.md Getting-Started.md Layer-1-glintfx.md Layer-0-Core.md FAQ.md
git commit -m "docs(wiki): sincroniza paginas da wiki a partir de docs/wiki/ no repo principal"
git push origin main   # ou "master": confira `git branch` depois do clone, wikis Forgejo/Gitea mais antigas usam "master" por padrão

# 4. (Opcional) Espelhar o mesmo conteúdo pra wiki do GitHub
git clone https://github.com/petrinhu/glintfx.wiki.git glintfx-wiki-github
# repita os passos 2-3 contra este segundo clone/remoto
```

### Mantendo sincronizado depois

Não há automação ligando `docs/wiki/` deste repositório à Wiki ao vivo (deliberadamente: um push de Wiki é uma ação em remoto que exige autorização explícita, não algo que o CI deveria fazer sem supervisão). Quando uma mudança futura atualizar `docs/wiki/*.md` num commit normal, rerode os quatro passos de `cp` + `git commit` + `git push` acima contra o clone da wiki pra trazê-la de volta à sincronia. Se isso virar uma tarefa frequente, considere propor um job de CI disparado manualmente pra isso, mas essa é uma decisão nova, não algo que este documento autoriza por conta própria.
