# Security Policy

> **EN:** How to report a vulnerability in glintfx. Bilingual: English first, then Português.
> **PT:** Como reportar uma vulnerabilidade no glintfx. Bilíngue: inglês primeiro, depois português.

---

## English

### Supported versions

`glintfx` is at an early stage. Only the latest release line receives fixes.

| Version | Supported |
| :--- | :--- |
| 0.13.x | Yes |
| < 0.13.0 | No |

> **Process note (AUD-L1-PARSE, 2026-07-16):** this table drifted for several minor releases (it named `0.4.x` while the project had already reached `0.11.0` -- the same drift class as `AUD-TEC-8`). **Bump this table's version line on every release** (whichever tag is current becomes the sole "Yes" row) as part of the release checklist, not as an afterthought caught by a later audit.

### Reporting a vulnerability

Please report security issues **privately**, not in a public issue, so a fix can be prepared before disclosure.

- **Preferred:** open a confidential / private security report on [Codeberg](https://codeberg.org/petrinhu/glintfx) or use [GitHub private vulnerability reporting](https://github.com/petrinhu/glintfx/security/advisories).
- **Alternative:** open a minimal public issue asking for a private contact channel, without disclosing the details.

Please include, where possible:

- A description of the issue and its impact.
- Steps to reproduce (a minimal `.rml`/`.rcss` or code snippet helps).
- Affected version and environment (distro, GPU/driver, clang/CMake versions).

You can expect an acknowledgement within a reasonable time. Coordinated disclosure is appreciated: give the maintainer time to ship a fix before going public.

### Fixed in previous releases

A short, honest record of the security-relevant findings closed by prior releases (full detail in [`CHANGELOG.md`](CHANGELOG.md) and the audit reports linked from [`docs/auditoria/README.md`](docs/auditoria/README.md)):

- **`v0.11.2` (`AUD-L1-GLSYM`):** the generated GL loader exported 344 function-pointer variables as ordinary global data symbols named identically to the GL functions they stand in for (`glClear`, etc.). A host that calls GL by its raw name and links `libglintfx.a` before `libGL.so` had that reference resolved to glintfx's uninitialised BSS slot instead of the real driver function, a null-pointer call that crashed on the very first GL call. Fixed by prefixing every exported slot `glx_<cmd>`; see [ADR-0013](docs/adr/0013-gl-symbol-boundary.md).
- **`v0.11.1` (`AUD-L1-PARSE`):** no size ceiling on `@font-face` asset loading could let a hostile/corrupt oversized font file crash the host with an allocation failure. Fixed with a 256 MiB ceiling in `BaseUrlFileInterface::Open()`, covering both image and font loading.
- **Layer 0 `core-v0.4.0` (`AUD-SEC-Δ`):** `atof` could overflow to `+Inf` on legitimate decimal literals within its own documented anti-DoS cap. Fixed with a separate fractional-digit accumulator, validated against glibc's `strtod` on 26 cross-checked cases. This is Layer 0 (`loucura_c_asm`), the separate zero-libc runtime described in the repository's [`CLAUDE.md`](CLAUDE.md); it does not link into glintfx (see [ADR-0009](docs/adr/0009-internalization-boundary.md)) and is out of the scope defined below except where a piece of it (e.g. the font engine) is vendored into glintfx.
- **Layer 0 `core-v0.4.0`, accepted risk, not a fix (`AUD-ABI-Δ`):** linking `libcore.a` (built `-fno-pic`, a deliberate freestanding constraint) into a hosted C++ consumer silently forces the whole consumer binary to `ET_EXEC`, losing ASLR. Not memory corruption (confirmed via `readelf -h`, all 16 `libcore_consumer` checks pass). Documented and accepted as a cost rather than fixed, pending a real consumer that needs a `-fpic` variant; see the [ADR-0009 addendum](docs/adr/0009-internalization-boundary.md#addendum-2026-07-19--pieaslr-cost-of-the--fno-pic-archive--custo-pieaslr-do-archive--fno-pic).

### Scope

`glintfx` is a **Linux x86-64 desktop library**, not a network service. Keep these boundaries in mind:

- **In scope:** memory-safety bugs in glintfx's own code (M0-M3 facade, window/render glue), unsafe handling of `.rml`/`.rcss` or asset paths supplied to the library, and issues in the build that could compromise a consumer.
- **Out of scope:** vulnerabilities in third-party dependencies (RmlUi, GLFW, FreeType, Mesa/libGL) should be reported upstream; the `mask` crash under Mesa/llvmpipe is a known Mesa software-renderer bug, not a glintfx vulnerability (see the limitations in [`README.md`](README.md)). Note: `glintfx/src/gl_loader.{h,c}` (the GL 3.3 core-profile loader, L1.14-GLLOADER) is glintfx's own code, not a third-party dependency, so bugs there are **in scope**.

> Loading untrusted `.rml`/`.rcss` documents runs the full RmlUi parser and the GL3 shader paths; treat untrusted UI documents with the same caution as any untrusted input.

---

## Português

### Versões suportadas

O `glintfx` está em estágio inicial. Apenas a última linha de release recebe correções.

| Versão | Suportada |
| :--- | :--- |
| 0.13.x | Sim |
| < 0.13.0 | Não |

> **Nota de processo (AUD-L1-PARSE, 2026-07-16):** esta tabela desalinhou por várias releases minor (nomeava `0.4.x` enquanto o projeto já estava em `0.11.0` -- a mesma classe de drift do `AUD-TEC-8`). **Atualize a linha de versão desta tabela a cada release** (a tag corrente vira a única linha "Sim") como parte do checklist de release, não como algo pego depois por uma auditoria futura.

### Reportar uma vulnerabilidade

Reporte problemas de segurança de forma **privada**, não em issue pública, para que a correção seja preparada antes da divulgação.

- **Preferido:** abra um reporte de segurança confidencial / privado no [Codeberg](https://codeberg.org/petrinhu/glintfx) ou use o [reporte privado de vulnerabilidade do GitHub](https://github.com/petrinhu/glintfx/security/advisories).
- **Alternativa:** abra uma issue pública mínima pedindo um canal de contato privado, sem revelar os detalhes.

Inclua, quando possível:

- Uma descrição do problema e seu impacto.
- Passos para reproduzir (um `.rml`/`.rcss` mínimo ou snippet de código ajuda).
- Versão afetada e ambiente (distro, GPU/driver, versões de clang/CMake).

Você pode esperar uma confirmação em tempo razoável. Divulgação coordenada é apreciada: dê tempo ao mantenedor para lançar uma correção antes de tornar público.

### Corrigido em releases anteriores

Um registro curto e honesto dos achados relevantes de segurança fechados por releases anteriores (detalhe completo no [`CHANGELOG.md`](CHANGELOG.md) e nos relatórios de auditoria linkados de [`docs/auditoria/README.md`](docs/auditoria/README.md)):

- **`v0.11.2` (`AUD-L1-GLSYM`):** o loader GL gerado exportava 344 variáveis-ponteiro de função como símbolos de dado globais comuns, com nome idêntico às funções GL que substituíam (`glClear`, etc.). Um host que chama GL pelo nome cru e linka o `libglintfx.a` antes do `libGL.so` tinha essa referência resolvida pro slot BSS não-inicializado da glintfx em vez da função real do driver, uma chamada por ponteiro nulo que crashava na primeiríssima chamada GL. Corrigido prefixando todo slot exportado com `glx_<cmd>`; ver [ADR-0013](docs/adr/0013-gl-symbol-boundary.md).
- **`v0.11.1` (`AUD-L1-PARSE`):** nenhum teto de tamanho no carregamento de asset `@font-face` podia deixar um arquivo de fonte gigante/hostil/corrompido derrubar o host com falha de alocação. Corrigido com um teto de 256 MiB em `BaseUrlFileInterface::Open()`, cobrindo carregamento de imagem e de fonte.
- **Camada 0 `core-v0.4.0` (`AUD-SEC-Δ`):** o `atof` podia estourar pra `+Inf` em literais decimais legítimos dentro do próprio cap anti-DoS documentado. Corrigido com um acumulador de dígito fracionário separado, validado contra o `strtod` da glibc em 26 casos cruzados. Isto é a Camada 0 (`loucura_c_asm`), o runtime zero-libc separado descrito no [`CLAUDE.md`](CLAUDE.md) do repositório; ela não linka no glintfx (ver [ADR-0009](docs/adr/0009-internalization-boundary.md)) e fica fora do escopo definido abaixo, exceto onde uma peça dela (ex.: o motor de fonte) é vendorizada no glintfx.
- **Camada 0 `core-v0.4.0`, risco aceito, não uma correção (`AUD-ABI-Δ`):** linkar o `libcore.a` (buildado `-fno-pic`, uma restrição freestanding deliberada) num consumidor C++ hosted derruba em silêncio o binário-consumidor inteiro pra `ET_EXEC`, perdendo ASLR. Não é corrupção de memória (confirmado via `readelf -h`, os 16 checks do `libcore_consumer` passam). Documentado e aceito como custo em vez de corrigido, até que um consumidor real precise de uma variante `-fpic`; ver o [adendo da ADR-0009](docs/adr/0009-internalization-boundary.md#addendum-2026-07-19--pieaslr-cost-of-the--fno-pic-archive--custo-pieaslr-do-archive--fno-pic).

### Escopo

O `glintfx` é uma **biblioteca desktop Linux x86-64**, não um serviço de rede. Tenha estas fronteiras em mente:

- **No escopo:** bugs de memory-safety no código do próprio glintfx (fachada M0-M3, cola de janela/render), tratamento inseguro de `.rml`/`.rcss` ou caminhos de asset passados à biblioteca, e problemas no build que possam comprometer um consumidor.
- **Fora do escopo:** vulnerabilidades em dependências de terceiros (RmlUi, GLFW, FreeType, Mesa/libGL) devem ser reportadas upstream; o crash do `mask` sob Mesa/llvmpipe é um bug conhecido do renderer de software do Mesa, não uma vulnerabilidade do glintfx (ver as limitações no [`README.md`](README.md)). Nota: `glintfx/src/gl_loader.{h,c}` (o loader GL 3.3 core profile, L1.14-GLLOADER) é código próprio do glintfx, não uma dependência de terceiro, então bugs ali estão **dentro do escopo**.

> Carregar documentos `.rml`/`.rcss` não confiáveis executa o parser completo do RmlUi e os caminhos de shader GL3; trate documentos de UI não confiáveis com a mesma cautela de qualquer entrada não confiável.
