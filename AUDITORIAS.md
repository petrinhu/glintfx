# AUDITORIAS — loucura_c_asm

Manual de auditorias do projeto. Auditoria é **downstream** de código + teste: só audita o que já foi implementado e testado. Stack: C + Assembly puros, sem libc, Linux x86-64 (Camada 0) mais C++/RmlUi/GL3 (Camada 1). Itens `AUD-*` na tabela. **Dossiê consolidado (vereditos, achados por severidade, sumário executivo):** [`docs/auditoria/README.md`](docs/auditoria/README.md). **Checklist pré-tag** (o gate que evita drift entre releases, motivado por 3 drifts reais já ocorridos): [`docs/RELEASE_CHECKLIST.md`](docs/RELEASE_CHECKLIST.md).

## Catálogo aplicável

**Estado (2026-07-19): `AUD-ABI`/`AUD-SEC` re-emitidas como delta na Onda 4** (`AUD-ABI-Δ`/`AUD-SEC-Δ`, `docs/auditoria/AUD-C0-PLAN.md`), cobrindo tudo que pousou na Camada 0 depois da auditoria original de 2026-07-09 (`core-v0.1.0`) — alocador free-list reescrito, `ftoa`/`atof`/`%f`, a fronteira `libcore.a` (ADR-0009), SFNT `kern`/bsearch, `src/hint.c` (1ª auditoria, gate `sanitize-hint` novo). Ambos vereditos **CONFORME-COM-RESSALVAS**, 1 achado 🟠 IMPORTANTE cada, **ambos encontrados E resolvidos dentro da própria onda** (custo PIE/ASLR aceito por adendo à ADR-0009; overflow de mantissa do `atof` corrigido em 2 rodadas com 2 reviews adversariais). Zero 🔴 CRÍTICO. Relatórios em `docs/auditoria/AUD-ABI.md`/`AUD-SEC.md` (seção "Delta re-audit 2026-07-19"); índice consolidado em `docs/auditoria/README.md`. `REL-TAG core-v0.4.0` segue como decisão go/no-go do líder, ver `TODO.md` (linhas `AUD-ABI-Δ`/`AUD-SEC-Δ`).

| ID | Auditoria | O que verifica | Pré-requisito |
| :--- | :--- | :--- | :--- |
| AUD-ABI | Conformidade ABI / ASM | Convenção de chamada System V AMD64 ([ADR-0003](docs/adr/0003-internal-abi.md)): preservação de registradores callee-saved (`rbx,rbp,r12-r15`), alinhamento de stack 16B em todo `call`, uso correto de `r10` no `syscall`, ausência de clobber indevido | Código de bootstrap + libc-núcleo testados |
| AUD-SEC | Segurança | Validação de argumentos de syscall, checagem de bounds em mem/string, overflow inteiro em conversões e no alocador, ausência de UB explorável; superfície de ataque (entrada não-confiável) | Núcleo testado |

## Camada 1 (glintfx)

Auditorias da biblioteca C++ (RmlUi 6.3 + renderer GL3), stack distinta da Camada 0 (C++17→23, deps de terceiros, Linux x86-64, MPL-2.0). Prefixo `AUD-L1-*` para não colidir com `AUD-ABI`/`AUD-SEC`. Catálogo **podado** ao perfil real: biblioteca desktop **sem rede, sem banco, sem PII, sem authN/authZ, sem cripto** — a única entrada não-confiável plausível é **decodificação de arquivo de asset** (imagem via stb_image, `.rml`/`.rcss` via RmlUi) quando o host carrega conteúdo de origem não totalmente confiável (mod, download, UGC). As seções **"Severidade dos achados"** e **"Regra"** abaixo valem para as duas camadas; nenhum item vai para uma tag de release do glintfx (`vX.Y.Z`) com achado CRÍTICO aberto.

**Estado (2026-07-16): as 5 auditorias abaixo estão FECHADAS** (onda `LW-AUD`, `security-engineer` + `compliance-legal`, 4 vereditos **CONFORME** sem achado bloqueante + 1 resolvida por dobra). Relatórios individuais em `docs/auditoria/AUD-L1-*.md`; índice consolidado com sumário executivo e achados-por-severidade em [`docs/auditoria/README.md`](docs/auditoria/README.md). Ver `TODO.md` pelas linhas `AUD-L1-*` (Status/Estado Auditado).

| ID | Auditoria | O que verifica | Pré-requisito |
| :--- | :--- | :--- | :--- |
| AUD-L1-SUPPLY | Supply-chain / CVE de dependências | Versões pinadas vs CVEs conhecidos: RmlUi (pin por commit SHA imutável `2cd28864…` via FetchContent), **stb_image v2.30** vendorizado, FreeType/GLFW/libGL do sistema. Confere imutabilidade/verificabilidade dos pins, ausência de CVE explorável na configuração usada, superfície mínima (só `STBI_ONLY_PNG/JPEG/TGA` compilado), licenças rastreadas e processo de acompanhamento de advisory no bump. Ferramenta: `grype`/`osv.dev` sobre a árvore fetchada+vendorizada. **Nota:** o loader GL não é mais o `gl3w` de terceiro desde a v0.8.0 (`L1.14-GLLOADER`) — é `glintfx/src/gl_loader.{h,c}`, código MPL-2.0 próprio, gerado clean-room do registro Khronos `gl.xml`; não entra no escopo de CVE de dependência de terceiro | L1-BUILD (grafo de deps/FetchContent + stb vendorizado) |
| AUD-L1-PARSE | Robustez de parsing de asset não-confiável | Decode de imagem malformada/hostil (PNG/JPG/TGA em `render_gl3.cpp::LoadTexture` via `stbi_load_from_memory`) e de `.rml`/`.rcss` (incl. `@font-face`) não trava nem corrompe memória: mutation testing sobre as guardas de tamanho, cap central de 256MiB em `BaseUrlFileInterface::Open()` (imagem+fonte, os 2 motores de fonte), ausência de overflow inteiro, DoS por alocação gigante. Categoria com histórico real de CVE em decoders de imagem | L1-BACKEND (render) + feature de texturas (v0.2.3); L2-EMBED (UiLayer é 2ª entrada ao render) |
| AUD-L1-MEM | Memory-safety de recursos GL/buffers manuais | Sem leak/UAF/double-free na gestão manual de textura/FBO/buffer GL e do buffer do stb (RAII via `unique_ptr` já corrigido em `render_gl3.cpp`); run limpo de **ASan+UBSan+LSan** sobre a suíte de testes (julgamento downstream do gate noturno, sem re-executar); revisão do ciclo de vida de recurso GL (`GenerateTexture`/`GlStateGuard`) e da UB de overflow `w*h` no premultiply. Classe validada por 3 bugs reais históricos (leak stb, UAF do `bind_*`, UAF de reentrância do click-callback) | L1-BACKEND, L2-EMBED (GlStateGuard + estado GL manual) |
| AUD-L1-LICENSE | Conformidade de licença / NOTICE (pós-MPL-2.0) | Header SPDX `MPL-2.0` presente em todo arquivo de fonte; `NOTICE` completo e correto (linkadas + vendorizadas + assets: RmlUi MIT, stb domínio público, FreeType FTL, Khronos MIT/Apache-2.0, OpenSans Apache-2.0; loader GL próprio MPL-2.0, **não** `gl3w` desde a v0.8.0); ausência de resíduo AGPL pós-relicenciamento (2026-06-28); compatibilidade de cada licença com o copyleft fraco por-arquivo do MPL-2.0. Checagem rápida (grep SPDX + diff NOTICE↔deps reais), **não** parecer jurídico | L1-DOCS (NOTICE/LICENSE/SECURITY) + relicenciamento MPL-2.0 |
| AUD-L1-BUILD | Gates de segurança no CI + hardening de build | **Resolvido por dobra 2026-07-16** (decisão autônoma do Caetano/CTO, sugerida pelo `security-engineer`): virou recomendação documentada de flags de hardening (`-D_FORTIFY_SOURCE=3`, `-D_GLIBCXX_ASSERTIONS`, `-fstack-protector-strong`, `-Wl,-z,relro,-z,now`) pra empacotadores/consumidores na seção "AUD-L1-BUILD fold-in" de `docs/auditoria/AUD-L1-SUPPLY.md`, não um gate de CI deste repo (a `glintfx` é distribuída como fonte; impor flags de linker/compilador ao consumidor seria regressão de correção pra alguns casos). Gap real de higiene de build fechado em paralelo: `-Wall -Wextra` no alvo `glintfx` (`45209e5`) | L1.2-CI (CI versionado) + L1-BUILD |

### Semente — dívida técnica (não-catalogada ainda)

- ~~**AUD-L1-QUALITY**~~ ✅ **RESOLVIDA 2026-07-19** (levantada na consolidação `LW-AUD` 2026-07-16, puxada na Onda 1 do `chore/l1-quality`): god-class/complexidade ciclomática/dead-code da Camada 1, escopo do `tech-lead` (não do `security-engineer`/`compliance-legal`). Veredito **CONFORME COM RESSALVAS** (`docs/auditoria/AUD-L1-QUALITY.md`): os 3 candidatos (`font_engine_own.cpp` 1727L/`bootstrap.cpp` 1500L/`render_gl3.cpp` 1349L) **não são god-classes** (55–74% do volume é doc-comment bilíngue obrigatório; medidos em código, 469–577L cada); zero dead-code; 2 duplicações reais achadas e corrigidas (`0f29879`/`d6ee6ba`). Detalhe em `TODO.md` INBOX.
- **AUD-C0-PICVAR** (semente nova, levantada pelo achado 🟠 do `AUD-ABI-Δ` 2026-07-19, ver adendo da [ADR-0009](docs/adr/0009-internalization-boundary.md)): variante `-fpic` da `libcore.a` pra consumidor hosted que exigir PIE — deliberadamente NÃO construída sem consumidor real. Detalhe em `TODO.md` INBOX.

### Fora de escopo (descartados — sem superfície neste perfil)

- **AuthN/AuthZ** — a lib não tem login, sessão, token nem controle de acesso.
- **Criptografia / gestão de segredo** — nenhum uso de cripto, TLS, hash ou secret no produto.
- **Rede / TLS / headers HTTP / CORS / CSRF** — biblioteca desktop, sem socket, sem servidor HTTP.
- **PII / privacidade / LGPD-GDPR** — não processa dado pessoal.
- **Injeção SQL/HTML/shell (SAST clássico)** — sem SQL, sem `exec` de shell, sem HTML remoto de usuário; o `.rml` local cai em `AUD-L1-PARSE`.
- **DAST (ZAP/nuclei)** — sem endpoint HTTP para varrer.
- **Container / k8s / IaC hardening** — não faz parte do artefato distribuído (o container só existe no runner de CI).
- **Secret-scanning dedicado (gitleaks)** — sem segredo no repo de uma lib de render; varredura pontual é trivial e não merece gate de release próprio.
- **Rate-limit / anti-DoS de rede** — sem rede; o único DoS realista (imagem gigante) está em `AUD-L1-PARSE`.
- **Gate de encapsulamento (grep include-based da fronteira pública)** — é higiene de ABI/design, não auditoria de segurança; já roda no CI/AGENTS.md.

## Severidade dos achados

- **CRÍTICO** — corrompe memória/estado, viola ABI silenciosamente, ou é explorável. Bloqueia release.
- **IMPORTANTE** — bug latente, invariante frágil, falta de validação. Corrigir antes da tag.
- **COSMÉTICO** — estilo, clareza, doc. Não bloqueia.

## Regra

Nenhum item vai para `REL-TAG` (tag de versão) com achado **CRÍTICO** aberto. Estado Auditado na tabela: `✓` aprovado · `⚠` com ressalvas · `—` não auditado. Auditoria sempre tem como pré-requisito os itens de código + teste que ela cobre (nunca auditar antes de testar).
