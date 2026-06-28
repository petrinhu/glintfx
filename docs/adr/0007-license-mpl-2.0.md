# ADR-0007 — License: MPL-2.0 (relicensed from AGPL-3.0) / Licença: MPL-2.0 (relicenciada da AGPL-3.0)

- **Status:** Accepted (2026-06-28)
- **Deciders:** petrus (líder), claudio-clo (orientação técnica, não vinculante)
- **Tags:** legal, scope, one-way-door
- **Supersedes:** the AGPL-3.0 choice recorded in commit `77a3826`.

## Context / Contexto

**EN:** Camada 1 is a library meant for **external adoption** by other developers. AGPL-3.0 treats linking as a derivative/combined work and adds a network clause (§13) — poison for an adopted library: almost no consumer (proprietary or even permissive FOSS) will link it. The linked dependencies (RmlUi MIT, gl3w public domain, FreeType FTL, libGL MIT/SGI) are compatible with **any** outbound license because we **link, not copy** (and internalization is clean-room).

**PT:** A Camada 1 é uma biblioteca destinada à **adoção externa** por outros devs. A AGPL-3.0 trata o link como obra derivada/combinada e adiciona a cláusula de rede (§13) — veneno para uma lib adotada: quase nenhum consumidor (proprietário ou mesmo FOSS permissivo) vai linká-la. As dependências linkadas (RmlUi MIT, gl3w domínio público, FreeType FTL, libGL MIT/SGI) são compatíveis com **qualquer** licença de saída porque **linkamos, não copiamos** (e a internalização é clean-room).

## Decision / Decisão

**EN:** Relicense the **whole project** from `AGPL-3.0-or-later` to **`MPL-2.0`** (Mozilla Public License 2.0). Unifying both layers avoids the per-layer AGPL-propagation trap (AGPL is viral through linking). MPL-2.0 gives **weak per-file copyleft** (modifications to *our* files stay open) + **patent grant**, while letting third parties — including proprietary apps — link the library freely. Dependency attributions live in `NOTICE`. Every source file carries the SPDX header `SPDX-License-Identifier: MPL-2.0` (item `STD-SPDX`).

**PT:** Relicenciar o **projeto inteiro** de `AGPL-3.0-or-later` para **`MPL-2.0`** (Mozilla Public License 2.0). Unificar as duas camadas evita a armadilha de propagação da AGPL (que é viral via link). A MPL-2.0 dá **copyleft fraco por-arquivo** (modificações nos *nossos* arquivos seguem abertas) + **grant de patente**, permitindo que terceiros — inclusive apps proprietários — linkem a lib livremente. As atribuições das dependências ficam em `NOTICE`. Todo arquivo-fonte leva o header SPDX `SPDX-License-Identifier: MPL-2.0` (item `STD-SPDX`).

## Consequences / Consequências

**EN:** External devs can adopt/link (even proprietary) without copyleft on their own app; our files remain open under MPL. Relicensing is clean **now** because petrus is the sole author — it must happen **before** accepting external contributions (otherwise it needs consent from all contributors or a CLA). Formalizing license/dual-license decisions may warrant human-lawyer review.

**PT:** Devs externos podem adotar/linkar (inclusive proprietário) sem copyleft no app deles; nossos arquivos seguem abertos sob MPL. Relicenciar é limpo **agora** porque o petrus é autor único — precisa acontecer **antes** de aceitar contribuições externas (senão exige consentimento de todos ou um CLA). Formalizar decisões de licença/dual-license pode exigir revisão de advogado humano.

## Options considered / Opções consideradas

- **MPL-2.0 (chosen / escolhida)** — equilíbrio soberania × adoção + patente.
- Apache-2.0 — permissiva pura, máxima adoção, sem copyleft. Rejected.
- LGPL-3.0 — copyleft forte na lib, mas atrita com link estático (`-static`). Rejected.
- Keep AGPL-3.0 / Manter AGPL — mata a adoção externa. Rejected.

Cross-ref: [[0006-layered-hybrid-architecture]].
