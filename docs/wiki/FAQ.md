# FAQ

## English

**Is this one project or two?**
One repository, two independent tracks ("layers"): [glintfx](Layer-1-glintfx) (Layer 1, released, active product) and [loucura_c_asm](Layer-0-Core) (Layer 0, experimental, long-term). They do not link into each other; see [ADR-0006](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0006-layered-hybrid-architecture.md).

**Why is the repository named `loucura_c_asm` but the released library called `glintfx`?**
The repository predates glintfx; it started as the Layer-0 "madness" experiment. glintfx was added later, became the active product, and got its own name, but the repository folder name and git history were never renamed.

**Does glintfx use the loucura_c_asm runtime internally?**
Not today. glintfx is pure, ordinary C++ that links normal system libraries. Layer 0 is a separate, unreleased experiment. Per [ADR-0009](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0009-internalization-boundary.md), the long-term plan is a narrow, incremental static link of specific pieces, not a wholesale merge.

**What platforms are supported?**
Linux on x86-64 only, for both layers. No macOS, no ARM. See the "Known limitations" section of the main [`README.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/README.md). (On Windows specifically, see the next question -- there is a real but narrow exception.)

**Does glintfx build on Windows?**
Not officially, and not end-to-end -- Linux x86-64 remains the one supported platform (previous question). What changed in `v0.9.2`: the generated GL loader (the internal piece that finds the graphics driver's drawing functions at runtime) gained a Windows codepath and was proven to cross-compile cleanly for Windows from a Linux machine. That is one necessary ingredient toward a Windows build, not a claim that the whole library (windowing, RmlUi, FreeType, the demo/test suite) has been built or run on Windows. If you need this, ask on the issue tracker before assuming it works.

**Can I switch which font rasteriser glintfx uses?**
Yes. Since `v0.10.0`, glintfx's own clean-room font engine is the default (`FontEngine::Own`), but RmlUi's FreeType-backed engine is still fully linked and one config line away: set `font_engine = glintfx::FontEngine::FreeType` on `AppConfig`/`UiLayerConfig` before construction -- no rebuild needed. See [ADR-0011](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0011-soft-font-flip.md).

**Is Layer 0 ready to use in a real project?**
No. It is implementation-complete but has not passed its final audit, and it is a learning/craftsmanship track with no promised stability or support. glintfx (Layer 1) is the supported, released piece.

**What license is this under?**
MPL-2.0 (Mozilla Public License 2.0) for the whole repository. See [`LICENSE`](https://codeberg.org/petrinhu/glintfx/src/branch/main/LICENSE) and third-party credits in [`NOTICE`](https://codeberg.org/petrinhu/glintfx/src/branch/main/NOTICE).

**I found a mistake in the docs or the wiki. What do I do?**
Open an issue on [Codeberg](https://codeberg.org/petrinhu/glintfx/issues) or [GitHub](https://github.com/petrinhu/glintfx/issues) describing what you read and what actually happened.

**I see UBSan warnings from RmlUi (`vptr:ElementDocument`, `WidgetScroll`) when I run my host under sanitizers. Is that a glintfx bug?**
No -- and if you build glintfx from source (`FetchContent`/`add_subdirectory`), you should not see them at all anymore: glintfx ships a small, tracked patch (`glintfx/patches/`) that fixes two genuine RmlUi teardown-order bugs at the root, reported upstream as a contribution back to RmlUi. If you still see them, you are likely linking your own separate copy of RmlUi, or an older glintfx release; see [`docs/embed-integration.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/embed-integration.md) section 18 for the full explanation and the fallback suppressions.

---

## Português

**É um projeto só ou dois?**
Um repositório, duas trilhas independentes ("camadas"): [glintfx](Layer-1-glintfx) (Camada 1, lançada, produto ativo) e [loucura_c_asm](Layer-0-Core) (Camada 0, experimental, longo prazo). Elas não se linkam uma na outra; ver [ADR-0006](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0006-layered-hybrid-architecture.md).

**Por que o repositório se chama `loucura_c_asm` mas a biblioteca lançada se chama `glintfx`?**
O repositório é anterior ao glintfx; começou como o experimento "loucura" da Camada 0. O glintfx foi adicionado depois, virou o produto ativo, e ganhou nome próprio, mas o nome da pasta do repositório e o histórico git nunca foram renomeados.

**O glintfx usa o runtime loucura_c_asm internamente?**
Não hoje. O glintfx é C++ puro e comum, que linka bibliotecas de sistema normais. A Camada 0 é um experimento separado e não lançado. Pela [ADR-0009](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0009-internalization-boundary.md), o plano de longo prazo é um link estático estreito e incremental de peças específicas, não uma fusão total.

**Quais plataformas são suportadas?**
Só Linux em x86-64, pras duas camadas. Sem macOS, sem ARM. Ver a seção "Limitações conhecidas" do [`README.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/README.md) principal. (No Windows especificamente, ver a próxima pergunta -- há uma exceção real, mas estreita.)

**A glintfx builda no Windows?**
Não oficialmente, e não de ponta a ponta -- Linux x86-64 continua sendo a única plataforma suportada (pergunta anterior). O que mudou na `v0.9.2`: o loader GL gerado (a peça interna que encontra as funções de desenho do driver gráfico em runtime) ganhou um codepath Windows e foi comprovado que cross-compila limpo pra Windows a partir de uma máquina Linux. Isso é um ingrediente necessário rumo a um build Windows, não uma afirmação de que a biblioteca inteira (janela, RmlUi, FreeType, a suíte de demo/teste) já foi buildada ou rodada no Windows. Se você precisa disso, pergunte no issue tracker antes de assumir que funciona.

**Dá pra trocar qual rasterizador de fonte a glintfx usa?**
Sim. Desde a `v0.10.0`, o motor de fonte próprio clean-room da glintfx é o default (`FontEngine::Own`), mas o motor apoiado em FreeType do RmlUi continua plenamente linkado e a uma linha de config de distância: defina `font_engine = glintfx::FontEngine::FreeType` em `AppConfig`/`UiLayerConfig` antes da construção -- sem rebuild. Ver [ADR-0011](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/adr/0011-soft-font-flip.md).

**A Camada 0 está pronta pra usar num projeto de verdade?**
Não. Está com a implementação completa mas ainda não passou pela auditoria final, e é uma trilha de aprendizado/artesania sem estabilidade ou suporte prometidos. O glintfx (Camada 1) é a peça suportada e lançada.

**Qual licença isto usa?**
MPL-2.0 (Mozilla Public License 2.0) pro repositório inteiro. Ver [`LICENSE`](https://codeberg.org/petrinhu/glintfx/src/branch/main/LICENSE) e créditos de terceiros em [`NOTICE`](https://codeberg.org/petrinhu/glintfx/src/branch/main/NOTICE).

**Achei um erro na documentação ou na wiki. O que eu faço?**
Abra uma issue no [Codeberg](https://codeberg.org/petrinhu/glintfx/issues) ou [GitHub](https://github.com/petrinhu/glintfx/issues) descrevendo o que você leu e o que de fato aconteceu.

**Vejo avisos de UBSan vindos do RmlUi (`vptr:ElementDocument`, `WidgetScroll`) quando rodo meu host sob sanitizers. Isso é um bug do glintfx?**
Não -- e se você builda o glintfx do fonte (`FetchContent`/`add_subdirectory`), você não deveria mais ver isso: o glintfx embarca um pequeno patch rastreado (`glintfx/patches/`) que corrige dois bugs genuínos de ordem-de-teardown do RmlUi na raiz, reportado ao upstream como contribuição de volta ao RmlUi. Se ainda assim você vê os avisos, provavelmente está linkando sua própria cópia separada do RmlUi, ou uma release mais antiga do glintfx; ver [`docs/embed-integration.md`](https://codeberg.org/petrinhu/glintfx/src/branch/main/docs/embed-integration.md) seção 18 pra explicação completa e as supressões de fallback.
