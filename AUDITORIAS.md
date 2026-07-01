# AUDITORIAS — loucura_c_asm

Manual de auditorias do projeto. Auditoria é **downstream** de código + teste: só audita o que já foi implementado e testado. Stack: C + Assembly puros, sem libc, Linux x86-64. Itens `AUD-*` na tabela.

## Catálogo aplicável

| ID | Auditoria | O que verifica | Pré-requisito |
| :--- | :--- | :--- | :--- |
| AUD-ABI | Conformidade ABI / ASM | Convenção de chamada System V AMD64 ([ADR-0003](docs/adr/0003-internal-abi.md)): preservação de registradores callee-saved (`rbx,rbp,r12-r15`), alinhamento de stack 16B em todo `call`, uso correto de `r10` no `syscall`, ausência de clobber indevido | Código de bootstrap + libc-núcleo testados |
| AUD-SEC | Segurança | Validação de argumentos de syscall, checagem de bounds em mem/string, overflow inteiro em conversões e no alocador, ausência de UB explorável; superfície de ataque (entrada não-confiável) | Núcleo testado |

## Camada 1 (glintfx)

Auditorias da biblioteca C++ (RmlUi 6.3 + renderer GL3), stack distinta da Camada 0 (C++17→23, deps de terceiros, Linux x86-64, MPL-2.0). Prefixo `AUD-L1-*` para não colidir com `AUD-ABI`/`AUD-SEC`. Catálogo **podado** ao perfil real: biblioteca desktop **sem rede, sem banco, sem PII, sem authN/authZ, sem cripto** — a única entrada não-confiável plausível é **decodificação de arquivo de asset** (imagem via stb_image, `.rml`/`.rcss` via RmlUi) quando o host carrega conteúdo de origem não totalmente confiável (mod, download, UGC). As seções **"Severidade dos achados"** e **"Regra"** abaixo valem para as duas camadas; nenhum item vai para uma tag de release do glintfx (`vX.Y.Z`) com achado CRÍTICO aberto.

| ID | Auditoria | O que verifica | Pré-requisito |
| :--- | :--- | :--- | :--- |
| AUD-L1-SUPPLY | Supply-chain / CVE de dependências | Versões pinadas vs CVEs conhecidos: RmlUi (pin por commit SHA imutável `2cd28864…` via FetchContent), gl3w e **stb_image v2.30** vendorizados, FreeType/GLFW/libGL do sistema. Confere imutabilidade/verificabilidade dos pins, ausência de CVE explorável na configuração usada, superfície mínima (só `STBI_ONLY_PNG/JPEG/TGA` compilado), licenças rastreadas e processo de acompanhamento de advisory no bump. Ferramenta: `osv-scanner`/`trivy fs` sobre a árvore fetchada+vendorizada | L1-BUILD (grafo de deps/FetchContent + stb vendorizado) |
| AUD-L1-PARSE | Robustez de parsing de asset não-confiável | Decode de imagem malformada/hostil (PNG/JPG/TGA em `render_gl3.cpp::LoadTexture` via `stbi_load_from_memory`) e de `.rml`/`.rcss` não trava nem corrompe memória: harness de **fuzzing** (libFuzzer/AFL++) sobre decode + laço de premultiply; ausência de cap de dimensão (`STBI_MAX_DIMENSIONS` não definido hoje); overflow inteiro no bound `i < w*h` (`int`); DoS por alocação gigante (`vector<uchar> buf(len)` + saída stb). Categoria com histórico real de CVE em decoders de imagem | L1-BACKEND (render) + feature de texturas (v0.2.3); L2-EMBED (UiLayer é 2ª entrada ao render) |
| AUD-L1-MEM | Memory-safety de recursos GL/buffers manuais | Sem leak/UAF/double-free na gestão manual de textura/FBO/buffer GL e do buffer do stb (RAII via `unique_ptr` já corrigido em `render_gl3.cpp`); run limpo de **ASan+UBSan** sobre a suíte de testes; revisão do ciclo de vida de recurso GL (`GenerateTexture`/`GlStateGuard`) e da UB de overflow `w*h` no premultiply. Classe validada por bug real (leak do buffer stb em exceção de `GenerateTexture`, achado em code review) | L1-BACKEND, L2-EMBED (GlStateGuard + estado GL manual) |
| AUD-L1-LICENSE | Conformidade de licença / NOTICE (pós-MPL-2.0) | Header SPDX `MPL-2.0` presente em todo arquivo de fonte; `NOTICE` completo e correto (linkadas + vendorizadas + assets: RmlUi MIT, gl3w/stb domínio público, FreeType FTL, Khronos MIT, OpenSans Apache-2.0); ausência de resíduo AGPL pós-relicenciamento (2026-06-28); compatibilidade de cada licença com o copyleft fraco por-arquivo do MPL-2.0. Checagem rápida (grep SPDX + diff NOTICE↔deps reais), **não** parecer jurídico | L1-DOCS (NOTICE/LICENSE/SECURITY) + relicenciamento MPL-2.0 |
| AUD-L1-BUILD | Gates de segurança no CI + hardening de build | Artefato compilado com flags de hardening (`-D_FORTIFY_SOURCE=3`, `-D_GLIBCXX_ASSERTIONS`, `-fstack-protector-strong`, `-Wl,-z,relro,-z,now`) e scanners como **gate** no pipeline (SCA/`osv-scanner` + job ASan/UBSan + fuzz opcional) nos dois CIs (GitHub Actions + Forgejo). Estado atual: nenhum gate de segurança, sem dependabot/renovate. Valida com `forgejo-runner exec` antes de mexer nos workflows. **Mais deferível do catálogo** — pode ser dobrado em SUPPLY+MEM+PARSE se o líder preferir o corte mínimo | L1.2-CI (CI versionado) + L1-BUILD |

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
