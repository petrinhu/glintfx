# Security Policy

> **EN:** How to report a vulnerability in glintfx. Bilingual: English first, then Português.
> **PT:** Como reportar uma vulnerabilidade no glintfx. Bilíngue: inglês primeiro, depois português.

---

## English

### Supported versions

`glintfx` is at an early stage. Only the latest release line receives fixes.

| Version | Supported |
| :--- | :--- |
| 0.1.x | Yes |
| < 0.1.0 | No |

### Reporting a vulnerability

Please report security issues **privately**, not in a public issue, so a fix can be prepared before disclosure.

- **Preferred:** open a confidential / private security report on [Codeberg](https://codeberg.org/petrinhu/glintfx) or use [GitHub private vulnerability reporting](https://github.com/petrinhu/glintfx/security/advisories).
- **Alternative:** open a minimal public issue asking for a private contact channel, without disclosing the details.

Please include, where possible:

- A description of the issue and its impact.
- Steps to reproduce (a minimal `.rml`/`.rcss` or code snippet helps).
- Affected version and environment (distro, GPU/driver, clang/CMake versions).

You can expect an acknowledgement within a reasonable time. Coordinated disclosure is appreciated: give the maintainer time to ship a fix before going public.

### Scope

`glintfx` is a **Linux x86-64 desktop library**, not a network service. Keep these boundaries in mind:

- **In scope:** memory-safety bugs in glintfx's own code (M0-M3 facade, window/render glue), unsafe handling of `.rml`/`.rcss` or asset paths supplied to the library, and issues in the build that could compromise a consumer.
- **Out of scope:** vulnerabilities in third-party dependencies (RmlUi, GLFW, FreeType, Mesa/libGL, gl3w) should be reported upstream; the `mask` crash under Mesa/llvmpipe is a known Mesa software-renderer bug, not a glintfx vulnerability (see the limitations in [`README.md`](README.md)).

> Loading untrusted `.rml`/`.rcss` documents runs the full RmlUi parser and the GL3 shader paths; treat untrusted UI documents with the same caution as any untrusted input.

---

## Português

### Versões suportadas

O `glintfx` está em estágio inicial. Apenas a última linha de release recebe correções.

| Versão | Suportada |
| :--- | :--- |
| 0.1.x | Sim |
| < 0.1.0 | Não |

### Reportar uma vulnerabilidade

Reporte problemas de segurança de forma **privada**, não em issue pública, para que a correção seja preparada antes da divulgação.

- **Preferido:** abra um reporte de segurança confidencial / privado no [Codeberg](https://codeberg.org/petrinhu/glintfx) ou use o [reporte privado de vulnerabilidade do GitHub](https://github.com/petrinhu/glintfx/security/advisories).
- **Alternativa:** abra uma issue pública mínima pedindo um canal de contato privado, sem revelar os detalhes.

Inclua, quando possível:

- Uma descrição do problema e seu impacto.
- Passos para reproduzir (um `.rml`/`.rcss` mínimo ou snippet de código ajuda).
- Versão afetada e ambiente (distro, GPU/driver, versões de clang/CMake).

Você pode esperar uma confirmação em tempo razoável. Divulgação coordenada é apreciada: dê tempo ao mantenedor para lançar uma correção antes de tornar público.

### Escopo

O `glintfx` é uma **biblioteca desktop Linux x86-64**, não um serviço de rede. Tenha estas fronteiras em mente:

- **No escopo:** bugs de memory-safety no código do próprio glintfx (fachada M0-M3, cola de janela/render), tratamento inseguro de `.rml`/`.rcss` ou caminhos de asset passados à biblioteca, e problemas no build que possam comprometer um consumidor.
- **Fora do escopo:** vulnerabilidades em dependências de terceiros (RmlUi, GLFW, FreeType, Mesa/libGL, gl3w) devem ser reportadas upstream; o crash do `mask` sob Mesa/llvmpipe é um bug conhecido do renderer de software do Mesa, não uma vulnerabilidade do glintfx (ver as limitações no [`README.md`](README.md)).

> Carregar documentos `.rml`/`.rcss` não confiáveis executa o parser completo do RmlUi e os caminhos de shader GL3; trate documentos de UI não confiáveis com a mesma cautela de qualquer entrada não confiável.
