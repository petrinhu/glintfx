# glintfx framework 2D -- módulo `i18n`: opções de escopo / i18n module: scoping options

> **EN (summary):** Options document (NOT a decision) for the lider, scoping an opt-in
> `GLINTFX_MODULE_I18N` for the 2D game framework ([ADR-0015](../../adr/0015-framework-2d-atomized-architecture.md)).
> Verified against the actual code (not assumed): RmlUi 6.3 core plumbs `dir`/`lang` HTML
> attributes into a `direction`/`language` computed style and even threads them into a
> `TextShapingContext` reaching the font engine -- but NEITHER RmlUi's default FreeType engine
> NOR glintfx's own clean-room engine (`font_engine_own.cpp`) actually consumes `direction`
> for bidi reordering or `language` for contextual shaping; both read only `letter_spacing`
> and `font_kerning` from that struct. There is zero bidi (UAX#9) and zero CSS logical
> properties anywhere in the pinned RmlUi tree; the only precedent for real text shaping is a
> non-core HarfBuzz *sample* that replaces the whole `FontEngineInterface`. Glyph COVERAGE,
> by contrast, is in decent shape: the own engine lazily top-up-bakes any codepoint present in
> a registered face's cmap on first appearance, plus a per-glyph fallback walk across
> registered fallback faces (`L1.20-FONTFLIP` FT-F4 sub-phase 2B) -- `embed-integration.md`
> section 17's "fixed codepoint set" bullet is stale against this. Net: **translation catalog
> + plurals + locale-aware formatting + runtime language switch is a clean, high-ROI slice
> that rides entirely on infrastructure that already exists** (`bind_string`/`set_string`,
> per-glyph fallback for extra scripts); **complex-script shaping and full bidi reordering are
> the dangerous, open-ended slice** (HarfBuzz-class engine, ICU-class bidi library), explicitly
> NOT recommended for this pass. Recommended module boundary: `i18n` depends on `core` ONLY
> (no GL, no window, no RmlUi type crossing its public header -- testable without Xvfb, same
> independence class as the audio atom); it feeds translated strings into `ui` through the
> EXISTING data-binding surface, no new coupling needed.
>
> **PT:** Documento de OPÇÕES (não decisão) para o líder. Autor: i18n-l10n-specialist,
> 2026-07-19. Branch: `feat/framework2d`. Escopo: só este documento, em
> `docs/superpowers/plans/`; nenhum código tocado (outro agente, `a0-skeleton`, builda o CMake
> em paralelo no mesmo branch). Baseado em leitura verificada do código-fonte pinado (RmlUi
> 6.3 em `examples/RmlUi/`) e do motor de fonte próprio (`glintfx/src/font_engine_own.*`), não
> em suposição.

---

## 1. Estado atual: o que a glintfx já entrega de graça vs o que falta / Have-vs-gap

### 1.1 UI declarativa (RmlUi 6.3) -- o que existe, verificado por leitura

| Peça | Existe? | Evidência |
|---|---|---|
| Atributos `dir="ltr\|rtl\|auto"` e `lang="..."` no RML | Sim, parseados | `examples/RmlUi/Source/Core/Element.cpp:1773-1798` -- vira `PropertyId::RmlUi_Direction` / `RmlUi_Language` |
| `direction`/`language` computados e propagados a um `TextShapingContext` | Sim | `examples/RmlUi/Include/RmlUi/Core/TextShapingContext.h`; usado em `DecoratorText.cpp:67`, `ElementText.cpp:243,487`, `ElementUtilities.cpp:109` |
| O engine de fonte de fato USA `direction`/`language` do `TextShapingContext` (reordenação bidi, formas contextuais) | **NÃO** | `FontFaceHandleDefault.cpp` (FreeType, o engine default do RmlUi) só lê `text_shaping_context.letter_spacing` (linhas 80, 239) e `text_shaping_context.font_kerning` (linha 340) daquela struct -- `direction`/`language` chegam e são ignorados. O motor próprio da glintfx segue o MESMO contrato de interface (`FontEngineInterface`), então herda a mesma lacuna. |
| Algoritmo de bidi (UAX#9) em qualquer forma (fribidi, ICU ubidi, implementação própria) | **NÃO, em lugar nenhum da árvore** | grep por `bidi\|fribidi\|ubidi` na árvore inteira do RmlUi pinado (`examples/RmlUi/`) não bate em nenhuma implementação, só em nomes de arquivo/campo que citam `direction` por outros motivos (gradiente RCSS, navegação por seta) |
| Propriedades CSS lógicas (`margin-inline-start`, `padding-block-end`, etc.) | **NÃO** | zero ocorrências de `writing-mode`/`WritingMode`/`inline-start` na árvore |
| Shaping de script complexo (HarfBuzz-classe: junção árabe, reordenação índica, ligaturas) | **NÃO no core**; existe como *sample* isolado | `examples/RmlUi/Samples/basic/harfbuzz/` reimplementa o `FontEngineInterface` INTEIRO com HarfBuzz+FreeType puros -- prova que a extensão é possível (a mesma que a glintfx já usou para plugar o motor próprio), mas não é código de produção do core nem resolve bidi (o próprio sample não linka fribidi/ICU) |
| Cobertura de GLYPH fora do conjunto europeu (Cirílico, Grego, CJK, etc.) | **Melhor do que a doc atual sugere** | `font_engine_own.cpp`/`.hpp`: o atlas eager cobre ASCII + Latin-1 Supplement, mas `EnsureGlyphs()`/`BakeGlyph()` faz TOP-UP PREGUIÇOSO aditivo pra QUALQUER codepoint fora desse conjunto, na 1ª vez que aparece numa string, contanto que a face registrada tenha o glyph no cmap (`font_engine_own.hpp:177-220`) |
| Fallback de glyph entre faces registradas (uma face CJK cobre o que a face latina não tem) | **Sim** | `font_engine_own.hpp:390-598`: `fallback_faces_` percorrida em ordem de registro, pega a primeira face que resolve um gid não-zero -- espelha o próprio laço de fallback do RmlUi (`FontFaceHandleDefault.cpp`, "fallback_font_faces em ordem") |
| `FontEngine::FreeType` como saída de emergência (rollback de 1 linha, sem rebuild) | Sim | `AppConfig::font_engine`/`UiLayerConfig::font_engine`, `docs/embed-integration.md` seção 17. Cobertura Unicode do FreeType é a padrão da indústria (per-glyph sob demanda, sem atlas fixo) -- útil como fallback se um locale precisar de mais cobertura do que o motor próprio ainda cobre |
| Modo de escrita vertical (CJK vertical) | **NÃO** | zero `writing-mode` na árvore |
| Data-binding pra empurrar string traduzida pra UI | Sim, maduro | `docs/embed-integration.md` seção 6: `create_data_model` -> `bind_string(key)` -> `load()` -> `set_string(key, valor)`; troca de idioma em runtime = re-chamar `set_string` pra cada chave, infraestrutura ZERO nova |

**Achado colateral (fora do escopo deste doc, mas vale registrar):** `docs/embed-integration.md` seção 17, bullet "v1 scope, documented" descreve o motor próprio como limitado a um "conjunto FIXO de codepoint" que "silenciosamente não tem glyph" fora dele. Isso descreve o comportamento de ANTES do sub-phase 2B (`L1.20-FONTFLIP` FT-F4); o código atual já faz top-up preguiçoso + fallback entre faces. A doc está desatualizada nesse bullet especificamente -- sinalizo pro time, não é bug deste módulo, é drift de doc pré-existente.

### 1.2 Infraestrutura de i18n hoje

Zero. `grep -rli "locale\|gettext\|i18n\|l10n\|icu" glintfx/` não bate em nada relevante (os 3 hits são falsos-positivos no `image_tint`, sem relação). Não há `TODO.md`/INBOX com item de i18n hoje. Greenfield completo -- não há débito a herdar, só a decidir.

---

## 2. O que "boa integração com i18n" entrega, concretamente, pra um jogo 2D

Na régua do projeto ("as funções do SDL/Qt e mais"): nem SDL nem raylib têm QUALQUER coisa de i18n embutida (é 100% responsabilidade do jogo); Qt tem `QTranslator`+Qt Linguist, que é a barra de comparação mais próxima. O que entregaria paridade-ou-mais:

1. **Catálogo de tradução carregado em runtime**, por locale (`pt-BR.json`, `en-US.json`, ...), chaveado por string estável (`hud.score.label`), com um `defaultMessage` de fallback embutido no binário pro locale-base (nunca crash/tela em branco se faltar arquivo).
2. **Troca de idioma em runtime, sem reiniciar o jogo.** Trocar o catálogo ativo + varrer as chaves bindadas e chamar `set_string` de novo -- a superfície JÁ EXISTE (seção 1.1). Isso é o item de MAIOR alavancagem: zero API nova na `ui`, só um consumidor novo do que já está lá.
3. **Plural via regra CLDR (subconjunto ICU MessageFormat), nunca `if (n == 1)`.** Ex.: `{count, plural, one {# item} other {# itens}}`. Sem isso, qualquer jogo com contador (moedas, inimigos, munição) quebra em locale com regra de plural diferente do inglês.
4. **`select` pra gênero/variante quando o jogo precisa** (ex.: "Ela venceu" vs "Ele venceu" vs "Você venceu" em pt-br, que tem concordância de gênero que o inglês não tem) -- subconjunto pequeno do ICU `select`.
5. **Formatação locale-aware de número/data/hora/moeda** (placar, cronômetro, preço de loja in-game): separador decimal/milhar, ordem dd/mm/aaaa vs mm/dd/aaaa, símbolo de moeda antes/depois -- pro recorte de locales que o jogo realmente ship (não CLDR inteiro, ver seção 3).
6. **Aproveitar o fallback-por-glyph já existente** (seção 1.1) como a resposta de cobertura de script pra scripts SEM shaping complexo: cirílico, grego, vietnamita (com diacríticos, mas sem junção), a maior parte do CJK (cada caractere é seu próprio glyph -- ver ressalva de shaping na seção 3). O jogo registra uma face adicional por script (ex.: uma face Noto Sans CJK como fallback) e o glyph aparece, sem trabalho novo no engine de fonte.
7. **RTL básico como opt-out honesto:** herdar o que o RmlUi já expõe (`dir="rtl"` no documento, `text-align` seguindo a direção) SEM prometer reordenação bidi de verdade (ver seção 3) -- documentar como "funciona para strings puramente RTL sem mistura de números/pontuação LTR", que é exatamente onde a lacuna de bidi dói menos.

---

## 3. As partes difíceis, com honestidade de escala (a linha que separa "cabe numa fatia" de "poço sem fundo")

### 3.1 Shaping de script complexo -- PERIGOSO, não recomendado nesta fase

Árabe (formas contextuais inicial/medial/final/isolada + ligaduras obrigatórias), a maioria dos scripts índicos (reordenação de vogal, conjuntos consonantais, matras), e ligaduras latinas avançadas (fi/fl como um glyph, se o jogo quiser tipografia fina) exigem um motor de shaping classe HarfBuzz. Isso não é polimento: sem shaping, árabe renderiza como letras isoladas sem se conectar (ilegível pro leitor nativo), e hindi/tâmil/etc. renderizam com a ordem visual errada. Duas rotas, ambas caras:

- **Vendorizar HarfBuzz de verdade:** dep nova pesada (C++ moderno, várias dezenas de milhares de linhas, mantém tabelas Unicode próprias) -- precisa de aval do líder pelo `NOTICE` (regra do CLAUDE.md: Camada 1 pode linkar libs listadas lá, mas novas deps exigem decisão explícita). O sample do RmlUi (seção 1.1) já mostra QUE PONTO plugar (substituir `FontEngineInterface` inteiro), então o esforço de integração RmlUi é conhecido; o esforço de VENDORIZAR+MANTER o HarfBuzz é que é grande.
- **Shaping clean-room reimplementado:** na régua "reimplementar do zero" que o projeto já pratica pro motor de fonte (`SOV-SFNT`/`SOV-RAST` da Camada 0), um shaping engine clean-room é trabalho de ANOS de uma equipe dedicada só nisso (é literalmente o que a equipe do HarfBuzz faz em tempo integral desde 2010). Não é uma fatia, é um épico à parte, sem prazo -- na mesma classe do "internalizar RmlUi/FreeType/GLFW" que o `CLAUDE.md` já trata como meta de longuíssimo prazo, sem pressa.

**Recomendação:** NÃO entra nesta fase. Registrar como gap conhecido e documentado (não escondido), do mesmo jeito que o próprio RmlUi core deixa em aberto.

### 3.2 Bidi (reordenação visual de texto misto LTR/RTL, UAX#9) -- também perigoso, mas menor que shaping

Mesmo sem shaping contextual, textos MISTOS (frase em árabe com um número ou uma palavra em inglês no meio) precisam do algoritmo de bidi (UAX#9) pra decidir a ORDEM VISUAL dos runs. Isso é mais tratável que shaping completo (o algoritmo é bem especificado, ~mil linhas de referência, tipo fribidi), mas ainda é trabalho de MESES, não de uma fatia, e sem ele, qualquer UI mista mostra a ordem errada (número indo pro lado errado da frase, por exemplo). Vendorizar `fribidi` (leve, C puro, licença LGPL -- checar compatibilidade de licença com MPL-2.0 antes) é a rota mais barata SE o líder decidir que RTL de verdade é prioridade; ainda assim entra depois de catálogo+plural+formatação (seção 5), nunca antes.

### 3.3 CLDR completo (todos os locales, todos os calendários, todas as moedas) -- poço sem fundo declarado

O CLDR oficial (fonte do ICU) é dezenas de MB de dados XML cobrindo ~700 locales, múltiplos calendários (hijri, persa, budista, japonês com eras), todas as 180+ moedas ISO 4217 com histórico de símbolo. Vendorizar o ICU inteiro (`libicu`, tipicamente 20-30MB de dados) é uma dep pesada desproporcional pra "jogo 2D simples" -- nenhum concorrente do nicho (raylib, SFML, LÖVE) faz isso. **Não reimplementar o CLDR inteiro.** A rota certa pro porte deste projeto: tabelas PRÓPRIAS, pequenas, curadas por locale REALMENTE suportado (a régua já usada no motor de fonte: começa pequeno -- ASCII+Latin-1 -- e estende por demanda real, nunca por especulação). Regra de plural + padrão de número/data por locale cabem em poucas dezenas de linhas de dado por locale; extrair isso do CLDR público (que é aberto) pra alimentar as tabelas próprias é trabalho de HORAS por locale, não de meses -- bem diferente de reimplementar o motor do ICU.

---

## 4. Desenho do módulo `i18n` na arquitetura atomizada (ADR-0015)

Seguindo a granularidade por subsistema já ratificada:

| Item | Valor |
|---|---|
| **Flag** | `GLINTFX_MODULE_I18N` |
| **Expõe (fronteira pública)** | Carregamento de catálogo por locale; resolução de chave -> string formatada (plural/select via ICU-lite); troca de locale ativo em runtime; formatação locale-aware de número/data/hora/moeda pro conjunto de locales curados. Tipo público, nenhum tipo de terceiro cruzando o header (mesma regra de todo módulo). |
| **Depende de** | **`core` SOMENTE.** Zero GL, zero janela, zero tipo do RmlUi no header público -- é dado puro (parse de catálogo + tabelas de regra pequenas + formatação de string). Mesma classe de independência que o átomo `audio` da tabela do ADR-0015: testável sem Xvfb, sem GPU, em CI leve. |
| **Quem consome o módulo** | O JOGO (chama `i18n::translate("hud.score", n)` e recebe a string já formatada) e, opcionalmente, a `ui` -- mas por PADRÃO DE USO com a API que já existe (`bind_string`/`set_string`), não por acoplamento novo em tempo de compilação. `i18n` não precisa incluir nada de `ui`; é a aplicação que faz a ponte chamando os dois. |
| **Existe hoje** | Não (módulo novo, greenfield) |
| **Independência real** | Alta -- na mesma prateleira do `audio`: nenhuma dependência de GL/janela/RmlUi, um consumidor pode usar `i18n` sozinho até fora de um jogo glintfx (ex.: uma ferramenta CLI que só formata número por locale) |
| **O que fica FORA do módulo (declarado, não escondido)** | Shaping de script complexo e bidi completo (seção 3) NÃO entram no `i18n`; se um dia entrarem, são extensão da `text`/`ui` (o ponto de plug já demonstrado pelo sample HarfBuzz do RmlUi), não do `i18n`, que continua sendo só dado+regra, nunca renderização |

**Por que não colar no `text` ou na `ui`:** o `text` module (seção 2.5 do ADR-0015) é raster de glyph + métricas; a `ui` é layout+HTML/CSS. Nenhum dos dois PRECISA saber que uma string é "traduzida" -- eles só recebem a string final (já resolvida pelo `i18n`) via `bind_string`/`set_string`, exatamente como recebem qualquer outra string hoje. Colar tradução dentro de `ui` violaria a regra (d) do ADR ("um módulo só inclui headers dos módulos na sua coluna depende-de") sem necessidade -- `i18n` não precisa aparecer na coluna "depende de" da `ui`, e a `ui` não precisa aparecer na do `i18n`. Isso é o mesmo padrão de independência que a separação `fx -> ui` já estabeleceu: dependência de mão única, e aqui nem isso -- zero dependência entre os dois, só um consumidor comum (o jogo).

---

## 5. Faseamento por ROI (fatias, seguindo o princípio (g) do ADR-0015: mais fácil + melhor ROI primeiro)

| Fatia | Conteúdo | Esforço | Risco | Depende de | ROI |
|---|---|---|---|---|---|
| **i18n-1 (recomendada primeiro)** | Catálogo de tradução por locale (formato próprio, pequeno, sem dep nova -- ou JSON via lib header-only pequena, decisão do líder) + resolução de chave + plural via regra CLDR-lite (tabela própria curada, pt-BR/en-US pra começar) + troca de locale em runtime via `set_string` existente | P-M | Baixo -- puro dado, zero GL/RmlUi, testável isolado | `GLINTFX_MODULE_I18N` sozinho, sobre `core` | Alto -- cobre a maioria absoluta dos jogos 2D simples (placar, HUD, menu, diálogo curto) |
| **i18n-2** | `select` pra gênero/variante (ICU-lite); formatação locale-aware de número/data/hora/moeda pro conjunto curado de locales; extensão do conjunto de locales por demanda real (nunca especulativa) | M | Baixo-médio -- mais tabelas curadas, mesma classe de trabalho da i18n-1 | i18n-1 | Alto -- fecha o gap de "placar/preço/relógio formatado errado" |
| **i18n-3 (documentar, não implementar agora)** | RTL básico usável (aproveitar `dir="rtl"` já plumbed no RmlUi + orientação de layout, SEM bidi de texto misto) pra locales majoritariamente-RTL sem mistura pesada de LTR | M | Médio -- depende de quanto o RmlUi de fato faz com `direction()` fora de texto (não verificado em profundidade neste doc; mirror de ícone/margem é trabalho de CSS/host, RmlUi não tem propriedades lógicas) | i18n-1 | Médio -- só vale se houver demanda real de locale RTL (consumer-driven, como o resto do projeto já pratica) |
| **i18n-4 (adiado, épico à parte, sem prazo)** | Bidi completo (fribidi ou próprio) + shaping de script complexo (HarfBuzz vendorizado ou clean-room) | G-GG | Alto -- dep nova pesada OU anos de trabalho clean-room | i18n-1..3 + decisão de dep do líder | Baixo-pro-porte-atual -- só se um consumidor real pedir árabe/hindi/etc. de verdade (cadência consumer-driven já validada no projeto) |

**Recomendação da fatia mínima:** **i18n-1**. É o mesmo padrão de "casca primeiro" que a fase A1 (input) já validou: entrega o item que TODO jogo 2D simples precisa (trocar "Score: 10" por "Pontuação: 10" com plural certo, em runtime, sem rebuild), sobre infraestrutura que já existe (`bind_string`/`set_string`), com um módulo do tamanho e independência do `audio` (o átomo mais limpo da casa, segundo o próprio ADR-0015).

---

## 6. Riscos / anti-over-engineering

1. **Não prometer CLDR inteiro nem todos os locales.** Tabela curada, cresce por demanda real (mesma régua do motor de fonte: ASCII+Latin-1 primeiro, resto por top-up).
2. **Não prometer shaping nem bidi nesta fase.** Declarar o gap explicitamente na doc pública (`embed-integration.md` ganharia uma seção "i18n" no mesmo estilo das seções 6/17 já existentes) em vez de deixar o consumidor descobrir sozinho que árabe não conecta.
3. **Locale != idioma.** Mesmo na tabela curada, distinguir `pt-BR` de `pt-PT`, `en-US` de `en-GB` desde o dia 1 -- é dado barato de acertar agora e caro de corrigir depois (toda chave de catálogo já nasce com o locale completo, nunca só o idioma).
4. **Formato de catálogo: decisão do líder.** Duas rotas legítimas -- (a) formato próprio minúsculo (zero dep nova, alinhado à filosofia clean-room do projeto, mas reinventa um parser); (b) JSON via lib header-only pequena (menos código próprio, mas é uma dep nova, ainda que leve, sujeita à regra do `NOTICE`). Nenhuma das duas é óbvia o suficiente pra decidir sem o líder -- ver seção 7.
5. **Alinhar com "Linux x86-64 primeiro".** Nenhuma dependência de API de locale do SO (`setlocale`/`LC_*`) -- o jogo roda determinístico independente do ambiente do usuário, na mesma régua de portabilidade que o resto do framework já persegue (tabelas próprias > `libc` locale).

---

## 7. Perguntas em aberto pro líder

1. **Confirma a fatia mínima i18n-1** (catálogo + plural + troca runtime, zero RTL/shaping) como primeiro corte, na ordem de ROI da seção 5?
2. **Formato de catálogo:** formato próprio minúsculo (zero dep) ou JSON via lib header-only pequena (dep nova, precisa constar no `NOTICE`)? Recomendação: formato próprio pra fatia i18n-1 (é pouco código, mantém zero-dep-nova nesta fase; reavaliar se o catálogo crescer o bastante pra JSON compensar).
3. **RTL/bidi/shaping (i18n-3/i18n-4):** ficam explicitamente fora do roadmap até um consumidor real pedir (cadência consumer-driven já validada no projeto), ou o líder quer reservar um slot de pesquisa mesmo sem demanda hoje?
4. **Onde entra na sequência A0-A5** do faseamento geral do framework (seção 4 do doc de visão)? Sugestão: depois de A1 (input básico) e independente de A2-A4 (gamepad/áudio/emoji não bloqueiam nem são bloqueados por i18n-1) -- pode rodar em paralelo a qualquer uma delas, dado que `i18n` só depende de `core`.
