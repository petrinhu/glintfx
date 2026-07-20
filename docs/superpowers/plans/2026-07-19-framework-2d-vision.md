# glintfx as a 2D Game Framework -- Vision & Options / glintfx como Framework de Jogos 2D -- Visão e Opções

> **EN (summary):** Options document (NOT a decision) for the lider: turning glintfx from
> "an embeddable UI layer for a host (SDL3/GLFW)" into a self-sufficient **2D game framework**
> for simple 2D games (no 3D, no editor) -- the single dependency a dev needs, instead of
> gluing SDL + Qt + N libs. Born from real pain: building GusWorld (consumer #1) required
> assembling SDL3 + several engines. Goal: GusWorld running on glintfx alone; then ship it
> as one more alternative to SDL/Qt for simple 2D. HARD architectural constraint from the
> lider: **atomized/modular** -- every capability is an independent, opt-in module composed
> a la carte (subsystem granularity; module map in section 2.5), never an all-or-nothing
> monolith. This doc inventories have-vs-gap, summarizes
> community pain research with sources, lays out 3 phasing options with effort/risk, is frank
> about scale, and closes with the CTO recommendation. The lider decides.
>
> **PT:** Documento de OPÇÕES (não decisão) para o líder. Autor: Caetano (CTO), 2026-07-19.
> Status: aguardando decisão do líder (faseamento + escopo de áudio + GLFW dentro/fora).
> **Rev. 2 (2026-07-19):** incorporada a RESTRIÇÃO DE ARQUITETURA do líder (requisito duro):
> **arquitetura atomizada/modular** -- cada capacidade é um módulo independente e opt-in,
> composto à la carte (seção 2.5). Granularidade: por SUBSISTEMA, não por função.
> **Rev. 3 (2026-07-19) -- DECIDIDO pelo líder:** (1) começar pela fatia **A1** (input + loop
> no `App` standalone; plano de execução em
> [`2026-07-19-framework2d-A1-input.md`](2026-07-19-framework2d-A1-input.md)); (2) áudio será
> **miniaudio vendorizado** quando a fase A3 chegar (opção (a) da tabela); (3) a arquitetura
> **atomizada por subsistema** está RATIFICADA e formalizada como decisão canônica no
> **[ADR-0015](../../adr/0015-framework-2d-atomized-architecture.md)** -- em divergência,
> o ADR manda; este doc permanece como registro de opções e pesquisa.

---

## 1. Posicionamento / Positioning

**O que o glintfx VIRA:** um **framework de jogos 2D "batteries-included" para Linux x86-64**
(alvo atual): a única peça de que um dev precisa para fazer um **jogo 2D simples** -- janela,
loop de frame, input (teclado/mouse/gamepad), áudio, imagem, texto (com emoji colorido),
tempo, e o diferencial que ninguém nesse nicho tem: **UI declarativa HTML/CSS com efeitos GL
data-driven** (glow, gradiente, blur, mask, ripple, polygon) e data-binding.

**Requisito duro de arquitetura (decisão do líder): ATOMIZADA/MODULAR.** Cada capacidade
(janela, input, áudio, texto/fonte, imagem, UI, efeitos GL, loop, tempo) é um **módulo
independente e opt-in**, com fronteira limpa, buildável/linkável/testável isolado. O
consumidor compõe à la carte -- usa só o que precisa, sem arrastar o resto. NÃO é monolito
"tudo-ou-nada". O embrião já existe: `GLINTFX_BACKEND_GLFW=OFF` produz hoje o build
embed-only sem GLFW; `GLINTFX_OWN_FONT_ENGINE` liga/desliga o motor de fonte. O framework
mode generaliza esse padrão para TODA capacidade (mapa de módulos na seção 2.5).

**O que o glintfx NÃO é (fronteiras explícitas, anti-scope-creep):**

- **Não é engine 3D.** Nada de câmera 3D, mesh, PBR. GL3 é meio, não fim.
- **Não é editor.** Sem GUI de autoria, sem scene editor. O "editor" é o par `.rml`/`.rcss`
  num editor de texto -- essa é a proposta.
- **Não é engine completa** (classe Godot/Unity): sem física embutida, sem ECS obrigatório,
  sem asset pipeline, sem scripting embarcado. Régua: **as funções do SDL/Qt para 2D, e mais**
  (a UI declarativa + efeitos), não as da Godot.
- **Não abandona o embed mode.** `UiLayer` continua sendo o produto para quem já tem host.
  O framework mode é o modo NOVO, aditivo, construído sobre o `App`.

**Narrativa de origem:** desenvolver o GusWorld (jogo SDL3, consumidor nº 1) exigiu colar
SDL (janela/input/áudio) + Qt + vários motores + o próprio glintfx. Essa cola é exatamente a
dor que a comunidade indie relata (seção 3). Meta de validação: **o GusWorld rodar só com
glintfx, sem SDL nem Qt**. Se der certo com o consumidor real, distribui-se como MAIS UMA
alternativa ao SDL/Qt para 2D simples.

---

## 2. Inventário have-vs-gap (lido do código, não assumido)

Fontes: `glintfx/include/glintfx/*.hpp`, `glintfx/src/`, `CLAUDE.md`, `TODO.md`
(épico `L1-INTERNALIZE`), ADR-0006/0008/0009.

| Capability | glintfx tem hoje? | Quem dá isso no SDL/Qt | Esforço de absorver |
|---|---|---|---|
| **Janela + contexto GL** | Parcial: `App` cria janela via GLFW (`window_glfw.cpp`, wrapper de 48 linhas; option `GLINTFX_BACKEND_GLFW`) | `SDL_CreateWindow` / `QWindow` | Já resolvido VIA GLFW. Internalizar (X11+Wayland+EGL/GLX) = GRANDE (ver seção 4.4) |
| **Loop de frame** | Parcial: `App::run()` (poll -> update -> render), bloqueante, sem callback de frame para o host desenhar a CENA | `SDL_PollEvent` loop / `QEventLoop` | PEQUENO: expor frame callback / passo de loop com contexto GL corrente |
| **Input teclado/mouse (standalone)** | **NÃO** -- achado desta análise: `App::poll_events()` só chama `glfwPollEvents()`; **zero** `glfwSet*Callback` em `src/` e nenhum `process_event` no `App`. Mouse/teclado físicos hoje NÃO chegam à UI no modo standalone (só no embed, via `UiEvent` injetado pelo host) | `SDL_Event` / `QEvent` | MÉDIO-PEQUENO: registrar callbacks GLFW e rotear para o mesmo caminho `UiEvent` já existente + expor ao jogo |
| **Input gamepad + hotplug** | **NÃO** (contrato atual: host mapeia d-pad -> `Key`, `ui_event.hpp`) | `SDL_Gamepad` (força do SDL: mapping DB + hotplug) / Qt Gamepad (morto) | MÉDIO (Linux-only): evdev + libudev/inotify para hotplug; DB de mapeamento é dado (formato SDL, zlib) |
| **Áudio** | **NÃO** -- decisão deliberada e documentada ("glintfx does NOT play audio itself", callbacks de scroll/hover/etc são ganchos p/ o host tocar som) | `SDL_Audio`+`SDL_mixer` / Qt Multimedia | GRANDE clean-room; MÉDIO se vendorizar (seção 4.3). A API de callbacks existente já é a metade "quando tocar"; falta a metade "tocar" |
| **Desenho 2D da CENA (sprites/shapes fora do DOM)** | Parcial: decorators RCSS (`polygon`, `image-tint`, ripple...) vivem DENTRO do documento; não há API imediata de sprite/batch para o mundo do jogo | `SDL_Renderer` / `QPainter` / raylib `DrawTexture` | Fase 1: ZERO (host desenha GL cru no frame callback, como já faz no embed). API de sprite própria: MÉDIO-GRANDE, adiar |
| **Texto** | **SIM, e próprio**: motor de fonte clean-room default (`font_engine_own.cpp`, 1727 linhas, ADR-0011), FreeType como rollback runtime | `SDL_ttf` / Qt | -- |
| **Emoji colorido (COLR/CBDT)** | **NÃO** (pedido explícito do líder; não sai de graça nem de manter FreeType -- é peça própria em cima do motor de fonte) | Qt só ganhou emoji decente no 6.9 (2025!); SDL_ttf não tem | MÉDIO: CBDT/CBLC primeiro (bitmaps PNG -- o decode stb_image JÁ está vendorizado); COLRv1 vetorial depois (seção 4.5) |
| **Imagem (PNG/JPG/TGA)** | SIM (stb_image vendorizado, premultiply in-place) | `SDL_image` / QImage | -- |
| **Tempo/clock** | SIM (`SystemClock`) | `SDL_GetTicks` / QElapsedTimer | -- |
| **UI declarativa + data-binding + efeitos GL** | **SIM -- é o diferencial.** Ninguém no nicho (SDL/raylib/SFML/LÖVE) tem UI HTML/CSS com efeitos data-driven | SDL: NADA (colar Dear ImGui/etc); Qt: QML (mas aí é Qt inteiro) | -- |
| **Rede** | NÃO | `SDL_net` / Qt Network | FORA de escopo (adiar indefinidamente) |
| **Física / ECS** | NÃO | SDL/Qt também NÃO dão | Fora da régua ("mesmas funções do SDL/Qt") -- não é gap |

Leitura honesta do quadro: **o gap real do framework mode é (1) input no standalone,
(2) frame callback para a cena, (3) gamepad, (4) áudio, (5) emoji colorido.** Janela e loop
já existem via GLFW; texto/imagem/tempo/UI já são fortes.

---

## 2.5 Arquitetura atomizada: mapa de módulos (requisito do líder)

> **Ratificada (2026-07-19).** Esta seção foi formalizada como decisão canônica no
> **[ADR-0015 -- Framework 2D: arquitetura atomizada por subsistema](../../adr/0015-framework-2d-atomized-architecture.md)**:
> os 9 átomos abaixo, o par acoplado ui+fx, a regra de DAG sem ciclo, a mecânica
> flags `GLINTFX_MODULE_*` + alvos + presets (fase A0), e a disciplina de suíte de teste +
> parecer de auditoria POR MÓDULO. Em divergência entre esta tabela e o ADR, o ADR manda.

Granularidade: **por SUBSISTEMA/capacidade** (nível confirmado pelo líder), não por função.
Proposta de mecânica (a ratificar): família de options CMake `GLINTFX_MODULE_*` + **um alvo
CMake por módulo** (`glintfx::audio`, `glintfx::input`, ...) mais o alvo agregador
`glintfx::glintfx` que preserva o consumo atual. Flag OFF = módulo nem compila nem vira dep
de link. `GLINTFX_BACKEND_GLFW` e `GLINTFX_OWN_FONT_ENGINE` continuam como aliases dos
módulos correspondentes (zero quebra para consumidores existentes).

| Módulo (átomo) | Flag proposta | Expõe (fronteira pública) | Depende de | Estado hoje | Independência real |
|---|---|---|---|---|---|
| **core** | sempre ON (base) | tipos comuns, `version()`, `SystemClock` (tempo), convenções de erro fail-high | nada | existe (`version.hpp`, `system_clock.cpp`) | total (é a raiz) |
| **image** | `GLINTFX_MODULE_IMAGE` | decode PNG/JPG/TGA -> pixels RGBA (premultiply opcional) | core | existe (stb_image vendorizado), hoje colado na UI | **alta** -- zero GL, zero janela |
| **text** | `GLINTFX_MODULE_TEXT` | rasterização de glyphs -> bitmap/atlas + métricas; emoji colorido (CBDT, depois COLR) entra AQUI como feature do subsistema, sem flag própria | core, image (emoji) | existe (`font_engine_own.cpp`), hoje exposto SÓ via `FontEngineInterface` do RmlUi -- a fronteira própria (raster -> atlas) é trabalho da atomização | **média** -- o raster é isolável; o plug no RmlUi vira adapter fino do módulo UI |
| **audio** | `GLINTFX_MODULE_AUDIO` | init device, mixer, play/stop/volume, decode WAV/OGG | core (SÓ) | não existe (fase A3) | **total -- o átomo mais independente da casa**: sem GL, sem janela, sem UI; buildável/testável em CI leve sem Xvfb |
| **window** (inclui o **loop**) | `GLINTFX_MODULE_WINDOW` (alias do atual `GLINTFX_BACKEND_GLFW`) | criar janela+contexto GL, poll, swap, should_close, frame step | core, gl-loader (interno) | existe (`window_glfw.cpp`) | média -- janela e loop de eventos são **inseparáveis por natureza** (poll/swap são operações DA janela); tratados como UM átomo |
| **input** | `GLINTFX_MODULE_INPUT` | `UiEvent`/`Key` (formato neutro, já existe) + polling/callbacks p/ o jogo; backend teclado/mouse-da-janela + backend **gamepad evdev/udev** (Linux, hotplug) | core; backend teclado/mouse depende de **window**; backend gamepad NÃO (evdev direto) | formato neutro existe (`ui_event.hpp`); captura física não existe (achado da seção 2) | **dividida e honesta**: gamepad é independente de janela; teclado/mouse físico não tem como ser (os eventos nascem na janela). No embed, input continua vindo do host via `UiEvent` -- prova de que a fronteira neutra funciona |
| **ui** | `GLINTFX_MODULE_UI` | `UiLayer`, documentos RML/RCSS, data-binding, DOM r/w, callbacks | core, fx, text, image | existe (o produto atual) | baixa p/ dentro do par ui+fx (ver abaixo), alta p/ fora (não puxa window/input/audio -- o embed-only de hoje É a prova) |
| **fx** (render GL3 + efeitos/decorators) | `GLINTFX_MODULE_FX` | renderer GL3, efeitos RCSS (glow/blur/mask/ripple/polygon) | core, gl-loader | existe (`render_gl3.cpp` + decorators) | **acoplamento inevitável com ui hoje**: o renderer implementa a `RenderInterface` do RmlUi e os efeitos são decorators DO documento. Separável de verdade só no endgame de internalização (RmlUi core próprio). Declarar ui+fx como **par acoplado** é mais honesto que fingir dois átomos |
| **app** (composição framework) | `GLINTFX_MODULE_APP` | `App`: compõe window+input+ui(+audio) no modo "jogo dentro do glintfx" | window, input, ui | existe parcial (`App` sem input) | é o AGREGADOR à la carte, não um átomo de capacidade |

Composições à la carte que o desenho habilita (exemplos de valor):

- **embed-only** (o GusWorld de hoje): ui+fx+text+image. Já existe -- vira o preset formal.
- **audio-only**: um jogo SDL que só quer o mixer do glintfx. Custo marginal zero de deps.
- **gamepad-only**: host X11 cru que quer hotplug de gamepad sem SDL.
- **framework completo**: app = window+input+ui+audio (o GusWorld do endgame).

Regras de fronteira (propostas como contrato): (1) nenhum módulo inclui header de outro
módulo que não esteja na sua coluna "depende de"; (2) nenhum tipo de terceiro cruza fronteira
pública de módulo (regra que a lib inteira já segue); (3) cada módulo com flag OFF remove
código E dependência de link; (4) cada módulo tem suíte de teste que roda com os demais OFF
(o par ui+fx testa junto). O gate de encapsulamento include-based que já existe no CI é o
enforcement natural de (1)/(2).

---

## 3. Pesquisa: dores da comunidade e comparativo (com fontes)

### 3.1 Dores com SDL e Qt para 2D indie

- **SDL é low-level de propósito**: não dá UI, nem texto (SDL_ttf à parte), nem imagem
  (SDL_image à parte), nem mixer de verdade (SDL_mixer à parte) -- o dev 2D simples acaba
  gerenciando 4-6 bibliotecas satélites + as próprias abstrações. É exatamente a dor de
  origem do GusWorld. O comparativo do próprio autor do raylib documenta essa fragmentação
  ([raylib vs SDL, raysan5](https://gist.github.com/raysan5/17392498d40e2cb281f5d09c0a4bf798)).
- **Transição SDL2 -> SDL3 dolorida**: regressões de áudio (ex.: [áudio distorcido via
  sdl2-compat](https://github.com/libsdl-org/SDL/issues/12150)) e problemas de HiDPI/scaling
  reportados; setup/compilação seguem fonte recorrente de fricção
  ([troubleshooting SDL](https://mindfulchase.com/explore/troubleshooting-tips/game-development-tools/troubleshooting-sdl-common-issues-and-solutions.html)).
- **Qt para jogos = licença e peso**: LGPL exige linkagem dinâmica substituível (sem static
  link) e conflita com regras de app stores (iOS exige licença comercial)
  ([Qt LGPL obligations](https://www.qt.io/development/open-source-lgpl-obligations),
  [forum Qt indie](https://forum.qt.io/topic/41639/how-does-qt-work-with-a-indie-developer),
  [análise de licenciamento](https://extenly.com/2024/02/07/qt-licensing-gpl-vs-lgpl-vs-commercial-choosing-the-right-fit-for-you/)).
  Além disso Qt é um framework de APLICAÇÃO: event loop próprio, dezenas de MB, módulos
  GPL-only -- desproporcional para um jogo 2D. O glintfx MPL-2.0 (copyleft fraco por-arquivo,
  link estático livre, apps proprietários OK) é objetivamente mais simples de adotar.
- **Emoji é dor recente e real**: o próprio Qt só entregou emoji colorido decente em 2025
  ([Emoji in Qt 6.9](https://www.qt.io/blog/emoji-in-qt-6.9)); SDL_ttf não suporta; projetos
  como EasyRPG têm issues abertas pedindo
  ([TTF color font support](https://github.com/EasyRPG/Player/issues/2857)).
  Chat/nomes de jogador com emoji é caso de uso comum hoje.

### 3.2 O que um framework 2D mínimo entrega (Raylib / SFML / LÖVE / SDL)

| | Janela | Input+gamepad | Áudio | Imagem | Texto | 2D draw | UI declarativa | Efeitos data-driven |
|---|---|---|---|---|---|---|---|---|
| **SDL3** | sim | sim (o melhor gamepad do mercado) | sim (cru; mixer à parte) | à parte | à parte | `SDL_Renderer` básico | **não** | **não** |
| **raylib** | sim | sim | sim | sim | sim (básico) | sim, ótima | **não** | **não** |
| **SFML** | sim | sim | sim | sim | sim (básico) | sim | **não** | **não** |
| **LÖVE** | sim | sim | sim | sim | sim | sim | **não** (é Lua, não é lib C/C++ linkável) | **não** |
| **glintfx hoje** | via GLFW | só embed, sem gamepad | **não** | sim | **sim, motor próprio** | via DOM/decorators | **SIM** | **SIM** |

Onde cada um brilha/falha: raylib brilha em simplicidade/zero-deps e falha em UI séria;
SFML brilha em API C++ limpa e falha em cadência/gamepad datado; LÖVE brilha em produtividade
Lua e falha para quem quer C/C++ nativo; SDL brilha em plataforma/input e falha em ser
low-level (tudo é trabalho seu). Fontes:
[Slant SFML vs raylib](https://www.slant.co/versus/1100/20505/~sfml_vs_raylib),
[GameDev.net: experiências SFML/raylib/SDL2](https://gamedev.net/forums/topic/717635-game-dev-experiences-with-sfml-raylib-and-sdl2-share-your-insights/),
[love2d.org](https://www.love2d.org/), [discourse SDL vs SFML](https://discourse.libsdl.org/t/why-would-i-choose-sdl2-over-sfml/28473).

### 3.3 Diferenciação ("e MAIS")

Nenhum dos quatro dá o que o glintfx já tem: **UI HTML/CSS declarativa com data-binding,
efeitos GL declarados em RCSS, motor de fonte próprio, e injeção de segurança na fronteira
(escape de RML, fail-high em toda a superfície)**. A tese de produto: *"o único framework 2D
em que menu, HUD e diálogo são um arquivo .rml estilizado com glow/blur/ripple em CSS, e o
jogo é um callback GL"*. Somando emoji colorido, o glintfx passa Qt 6.8-, SDL_ttf e raylib
em texto. Isso é diferencial defensável, não paridade.

**A arquitetura atomizada é diferencial declarado por si** (requisito do líder que também
vende): raylib é deliberadamente monolítico; SDL3 tem subsistemas (`SDL_InitSubSystem`) mas
compila e linka como UMA lib -- desligar áudio não tira o áudio do binário; Qt é modular mas
cada módulo é pesado e o núcleo (QtCore/QtGui) vem sempre junto. O precedente honesto é a
SFML (libs `sfml-window`/`sfml-audio`/`sfml-graphics` separadas) -- só que a SFML não tem a
UI declarativa nem os efeitos. A combinação **módulos opt-in de verdade (flag OFF = nem
compila, nem linka) + UI HTML/CSS + efeitos data-driven + MPL-2.0 com link estático livre**
não existe em nenhum dos quatro. E o glintfx já provou o padrão em produção: o build
embed-only sem GLFW é exatamente isso desde a v0.2.1.

### 3.4 Peças de implementação pesquisadas (para as fases)

- **Gamepad Linux sem SDL**: evdev (`/dev/input/eventX`, read+poll) + libudev/netlink (ou
  fallback inotify) para hotplug -- caminho documentado e batido
  ([ArchWiki Gamepad](https://wiki.archlinux.org/title/Gamepad),
  [Our Machinery: gamepad on Linux](https://ruby0x1.github.io/machinery_blog_archive/post/gamepad-implementation-on-linux/index.html),
  [Handmade Network: evdev gamepad](https://handmade.network/forums/t/3673-modern_way_to_read_gamepad_input_with_c_on_linux)).
  A base de mapeamentos SDL_gamecontrollerdb é DADO em formato texto (licença zlib) --
  consumível sem linkar SDL.
- **Áudio**: [miniaudio](https://miniaud.io/) ([GitHub](https://github.com/mackron/miniaudio))
  é single-file, domínio público, backends ALSA/PulseAudio/PipeWire, mixer + decode
  WAV/FLAC/MP3 embutidos -- o análogo de áudio do stb_image que já vendorizamos. Panorama das
  APIs Linux: [0pointer: Linux Sound API Jungle](https://0pointer.de/blog/projects/guide-to-sound-apis.html).
- **Emoji colorido**: formatos COLR/CPAL (vetor, v0 simples/v1 com gradientes), CBDT/CBLC
  (bitmaps PNG -- Noto Color Emoji), sbix (Apple)
  ([FontLab: color fonts 2026](https://blog.fontlab.com/2026/05/03/color-fonts-in-2026/),
  [EmojiFYI: rendering across platforms](https://emojifyi.com/stories/emoji-rendering-across-platforms/)).
  CBDT é o caminho curto para nós: parse de 2 tabelas + decode PNG (stb_image JÁ no repo) +
  upload como glyph texture no atlas do motor próprio. COLRv1 (vetorial, escala limpa) é a
  fase 2 do emoji.

---

## 4. Opções de faseamento

Nomenclatura de esforço: P = dias/1 onda · M = 1-3 ondas · G = muitas ondas/semanas ·
GG = época (meses+).

### Opção A -- "Casco primeiro" (host-first; GLFW fica)

Fatia 1 = tornar o `App` um host de jogo de verdade; o jogo desenha a própria cena em GL cru
dentro do frame, invertendo o embed (em vez de glintfx dentro do host, o host-jogo dentro do
glintfx). Áudio e emoji vêm depois, sobre o casco já validado pelo GusWorld. **Cada fase
entrega um ÁTOMO da seção 2.5 atrás da própria flag `GLINTFX_MODULE_*`, com suíte isolada**
-- a atomização não é uma fase à parte: é a FORMA de entregar cada fase.

| Fase | Conteúdo | Átomo/flag entregue | Esforço | Risco | Destrava |
|---|---|---|---|---|---|
| **A0 Esqueleto modular** | Formalizar a família `GLINTFX_MODULE_*` + alvos CMake por módulo + presets (embed-only/full); aliases dos flags atuais; gate include-based estendido a fronteiras entre módulos | a mecânica de módulos (sem capability nova) | P-M | Baixo (refactor de build, zero API) | Tudo que vem depois nasce atômico em vez de ser re-fatiado |
| **A1 Input+loop** | Callbacks GLFW -> rota `UiEvent` existente no `App`; API de input pro JOGO (polling/callback, além da UI); frame callback com contexto GL corrente (`on_frame(dt)`); `run()` vira o loop do jogo | `GLINTFX_MODULE_INPUT` (backend janela) + `MODULE_APP` | M | Baixo (tudo sobre código existente; embed intocado) | GusWorld larga a janela/input SDL |
| **A2 Gamepad** | evdev+udev hotplug, mapeamento via DB formato SDL (dado), exposto como gamepad cru + mapeado p/ `Key` (rota UI já existe) | backend gamepad do `MODULE_INPUT` (dep udev só aqui; utilizável SEM window -- composição "gamepad-only") | M | Baixo-médio (device zoo; mas escopo Linux) | GusWorld larga o input SDL por completo |
| **A3 Áudio** | Decisão do líder embutida: (a) **vendorizar miniaudio** (como stb_image; single-file, domínio público, mixer+decode prontos) ou (b) **clean-room ALSA/PipeWire + mixer próprio** (soberania, alinhado à Camada 0, MUITO mais caro) | `GLINTFX_MODULE_AUDIO` -- **o átomo mais limpo**: dep só de core, testável sem Xvfb/GL; é o teste de fogo da atomização | (a) M / (b) G-GG | (a) baixo / (b) alto | GusWorld larga o áudio SDL -> **SDL some do GusWorld**; composição "audio-only" p/ terceiros |
| **A4 Emoji colorido** | CBDT/CBLC no `font_engine_own` (parse tabelas + stb_image + atlas); COLRv0/v1 depois | feature do `MODULE_TEXT` (sem flag própria -- granularidade é por subsistema) | M (CBDT) + M-G (COLRv1) | Médio (formato bem documentado; risco é atlas/escala) | Diferencial de texto acima de Qt<6.9/SDL_ttf/raylib |
| **A5 (adiado)** | API de sprite/batch 2D própria; janela própria sem GLFW; Windows/macOS; rede; separação real do par ui+fx (gated pelo endgame de internalização do RmlUi) | -- | G-GG | -- | -- |

### Opção B -- "Soberania primeiro" (internalizar janela antes de tudo)

Sair do GLFW já: janela própria X11+Wayland+EGL/GLX, alinhada ao épico `L1-INTERNALIZE`.

- **Contra (e é forte):** Wayland é um pântano (protocolos, decorações client-side,
  fractional scaling); X11+Wayland em paralelo dobra o custo; a recomendação registrada do
  auditor no épico é **NÃO internalizar GLFW** (`TODO.md` L1-INTERNALIZE / `AUDIT_FIND.md`
  §5.5); e nada disso entrega valor novo ao GusWorld -- a janela GLFW já funciona.
- **A favor:** coerência com a "loucura" de longo prazo (Camada 0).
- **Veredito do CTO:** não recomendo como início. A soberania de janela continua no endgame
  do épico, sem prazo. **Critério objetivo GLFW-dentro-ou-fora** (proposto para ratificação):
  GLFW fica ENQUANTO (1) não bloquear nenhuma feature do framework mode (gamepad NÃO
  bloqueia: entra por evdev, fora do GLFW), (2) permanecer dep leve/zlib sem conflito de
  licença, e (3) o custo X11+Wayland não se pagar com demanda real. Reavaliar no marco em
  que FreeType cair (FT-F4), quando o resto da pilha já for próprio.

### Opção C -- "Consumer-driven estrito" (o GusWorld ordena a fila)

Mesmo backlog da Opção A, mas a ORDEM é ditada pela dor rankeada do GusWorld (relay do líder,
cadência consumer-driven já validada no projeto). Ex.: se a dor nº 1 deles for áudio, A3
antecipa A1/A2.

- **Contra:** A1 (input+loop) é pré-requisito técnico de qualquer "GusWorld sem SDL" -- sem
  janela+input glintfx, dropar SDL é impossível mesmo com áudio pronto. A ordenação livre é
  parcialmente ilusória.
- **A favor:** é a cadência que já funciona; evita construir na ordem errada.
- **Uso recomendado:** híbrido -- A1 é fixo como primeiro passo; o líder faz relay ao GusWorld
  para rankear A2 vs A3 vs A4.

---

## 5. Riscos e anti-over-engineering (a escala real, sem maquiagem)

1. **Esta é potencialmente a maior expansão de escopo da história do projeto.** A barra
   "MESMAS funções do SDL/Qt E MAIS", lida literalmente (cross-platform, todos os backends
   de áudio, câmera, sensores, RT threads), é trabalho de ANOS de equipe. O recorte que a
   torna executável: **paridade funcional para "jogo 2D simples em LINUX x86-64"** (o alvo
   declarado do repo), com o resto explicitamente adiado. Windows/macOS multiplicam cada
   linha desta tabela; o codepath `_WIN32` do loader GL (v0.9.2) existe, mas prometer
   framework-mode Windows agora seria irresponsável.
2. **Áudio é o item mais perigoso.** Clean-room cross-platform é ENORME (o guia 0pointer
   mostra o pântano de APIs). Mitigação: Linux-only + (miniaudio vendorizado OU ALSA/PipeWire
   direto). A decisão (a)/(b) da fase A3 é do líder -- nova dep, mesmo vendorizada, exige
   decisão dele por regra do repo (`NOTICE`).
3. **Gamepad device zoo**: hotplug/quirks de dispositivos variados; mitigado pela DB de
   mapeamento e por escopo evdev-only. Superfície de INPUT NÃO-CONFIÁVEL nova (arquivo de
   DB parseado, devices) -- gatilho (d) do `.bigtech-porte` fica de vigília (Narciso).
4. **Não quebrar o produto atual**: `UiLayer`/embed é o modo em produção; todo o framework
   mode é ADITIVO no `App` (e `App` já é `GLINTFX_BACKEND_GLFW=ON` only). Zero risco de
   regressão embed se a fronteira for respeitada; a suíte embed (GLFW=OFF) vira o guarda.
5. **Emoji**: CBDT primeiro (barato, bitmaps) e COLRv1 depois; NÃO começar por COLRv1/SVG.
   Shaping complexo (ligaturas ZWJ de emoji, bidi, HarfBuzz-class) fica explicitamente FORA
   da fase 1 de emoji -- codepoint único primeiro, sequências ZWJ como fase própria.
6. **Sprite/batch API própria** é sedutora e adiável: o GusWorld já desenha a cena com GL
   próprio; o frame callback resolve. Construir API de desenho sem consumidor pedindo é
   over-engineering clássico.
7. **A atomização tem os próprios anti-padrões.** (a) Granularidade fina demais: flag por
   função ou por efeito seria explosão combinatória de builds no CI -- a régua confirmada
   pelo líder é POR SUBSISTEMA (9 átomos na seção 2.5, ponto). (b) Repos/libs distribuídas
   separadas (estilo N pacotes): não -- é UM repo, UMA build, alvos/flags; a separação é de
   fronteira e link, não de distribuição. (c) Fingir independência onde não há: janela+loop
   são um átomo só, ui+fx são um par acoplado até a internalização do RmlUi -- documentado
   como tal em vez de inventar interface prematura entre eles (interface especulativa entre
   ui e fx AGORA seria over-engineering puro, o RmlUi já define essa fronteira). (d) Matriz
   de CI: testar 2^N combinações é inviável; testar presets nomeados (embed-only, full,
   audio-only, gamepad-only) + o build de cada átomo isolado é suficiente e linear.

---

## 6. Recomendação do CTO

**Opção A com ordenação híbrida da C.** Concretamente:

0. **A0 (esqueleto modular) primeiro, curto e barato.** A restrição de atomização do líder
   é mais barata de instalar AGORA (formalizar flags/alvos/presets sobre o embrião
   `GLINTFX_BACKEND_GLFW` existente) do que re-fatiar depois de 3 módulos novos nascerem
   colados. É refactor de build sem API nova; 1 onda.
1. **Fatia mínima que já dá valor: A1 (input + frame callback no `App`).** É pequena, é
   toda sobre código existente (a rota `UiEvent` do embed é reaproveitada), destrava o
   primeiro "GusWorld sem janela/input SDL" e valida a tese do framework mode com o
   consumidor real antes de qualquer aposta cara. O achado desta análise (o `App` hoje não
   tem NENHUMA rota de input física) mostra que A1 é, na prática, terminar o `App`.
2. **A2 (gamepad evdev) em seguida** -- salvo o GusWorld rankear áudio acima (relay do líder).
3. **A3 (áudio) com decisão explícita do líder** entre miniaudio vendorizado (recomendo:
   é o stb_image do áudio, e a cláusula de internalização de longo prazo continua valendo
   como no resto da pilha) e clean-room ALSA (mais caro em ordem de magnitude).
4. **A4 (emoji CBDT)** como primeiro diferencial "e MAIS" visível de marketing.
5. **Declaradamente adiado:** janela própria (GLFW FICA, critério na Opção B), sprite/batch
   API, Windows/macOS no framework mode, rede, física, shaping ZWJ completo, editor (nunca).

Perguntas abertas que só o líder responde: (1) ratificar o critério GLFW-dentro-ou-fora da
seção 4 (Opção B) -- **AINDA ABERTA**; (2) a escolha (a)/(b) do áudio -- **RESPONDIDA
(Rev. 3): miniaudio vendorizado, na fase A3**; (3) ratificar o mapa de módulos da seção 2.5
(os 9 átomos, o par acoplado ui+fx, e a mecânica flags+alvos+presets) -- **RESPONDIDA
(Rev. 3): ratificado, formalizado no [ADR-0015](../../adr/0015-framework-2d-atomized-architecture.md)**.
