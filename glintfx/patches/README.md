<!-- SPDX-License-Identifier: MPL-2.0 -->

# glintfx/patches

**EN:** Explicit, tracked source patches applied to third-party dependencies fetched via
CMake `FetchContent`, via each dependency's `PATCH_COMMAND` (see `glintfx/CMakeLists.txt`,
the `FetchContent_Declare(RmlUi ...)` block). This is **not** a silent fork: every patch
here is atomic, reviewed, tied to the exact upstream pinned commit it was written against,
and (per the líder's 2026-07-10 decision) meant to be **temporary**: reported/submitted to
the upstream project and removed once upstream merges an equivalent fix and this repo bumps
the pinned `GIT_TAG` past it. If you bump a `GIT_TAG` in `CMakeLists.txt`, re-check every
patch listed below: it may no longer apply cleanly, may already be fixed upstream (drop it),
or may still be needed (re-verify the line numbers/context still match).

**PT:** Patches de fonte explícitos e rastreados, aplicados a dependências de terceiro
fetchadas via `FetchContent` do CMake, através do `PATCH_COMMAND` de cada dependência (ver
`glintfx/CMakeLists.txt`, o bloco `FetchContent_Declare(RmlUi ...)`). Isto **não** é um fork
silencioso: todo patch aqui é atômico, revisado, atado ao commit exato pinado do upstream
contra o qual foi escrito, e (conforme decisão do líder de 2026-07-10) pensado para ser
**temporário**: reportado/submetido ao projeto upstream e removido assim que o upstream
mesclar uma correção equivalente e este repositório avançar o `GIT_TAG` pinado para além
dele. Se você avançar um `GIT_TAG` em `CMakeLists.txt`, reverifique todo patch listado
abaixo: ele pode não aplicar mais de forma limpa, pode já estar corrigido no upstream (nesse
caso, remova-o), ou pode ainda ser necessário (reverifique se os números de linha/contexto
ainda batem).

## `rmlui-2cd28864-teardown-ub.patch`

**EN:** Fixes 2 distinct, genuine undefined-behavior findings in RmlUi's own element/document
teardown order, both caught by `GLINTFX_SANITIZE=ON` (UBSan) and previously worked around by
3 suppression entries in `glintfx/tests/ubsan_suppressions.txt` (now removed: this patch
fixes the root cause instead of suppressing the symptom):

1. **`vptr:ElementDocument`**: `Element::SetOwnerDocument` (called recursively from
   `~Element()` while a document tears down its own children) dereferences
   `owner_document->GetContext()` through a pointer whose `ElementDocument`-derived part has
   already finished destructing at that point in the call chain, a use of a pointer to a
   derived object whose derived part is already destroyed (strict UB). Fixed by giving
   `ElementDocument` a real destructor body (`Source/Core/ElementDocument.cpp`) that clears
   `owner_document` on every direct child (which itself recurses over the whole subtree via
   the existing `SetOwnerDocument` implementation) **while `this` is still a fully valid
   `ElementDocument`**, pre-empting the later, unsafe recursive clear from the base
   `~Element()` teardown. Requires `ElementDocument` to reach `Element::meta`/internals as a
   `friend` (`Include/RmlUi/Core/Element.h`).
2. **`vptr:*WidgetScroll.cpp*` / `vptr:*RemoveEventListener*`**: the same root-cause pattern,
   inside RmlUi's own internal scrollbar widget: `~WidgetScroll()` calls
   `RemoveEventListener()` on up to 4 `Element*` it holds to its own generated scrollbar
   children, which by then may already be mid-destruction in the same recursive teardown
   cascade. Fixed by reordering `Element::~Element()` (`Source/Core/Element.cpp`) to release
   `meta` (which owns the `WidgetScroll`, whose destructor is what touches those sibling
   `Element*`s) **before** `children.clear()` runs, instead of after: `meta`'s destructor
   chain no longer observes already-destroyed children.

Both were caught live under `GLINTFX_SANITIZE=ON` (ASan+UBSan), see `glintfx/CHANGELOG.md`
`[Unreleased]` and `TESTES.md` `TST-L1-MEM` for the original discovery/triage. Validated: with
this patch applied and the 3 suppression entries above removed, the full suite (both
`GLINTFX_BACKEND_GLFW=ON`/`OFF`) runs clean under UBSan: zero `vptr:*` findings for these 3
patterns (if the patch failed to apply, the suite would fail loudly instead of silently
staying green, which is the point of removing the suppressions rather than just leaving them
in place alongside the fix).

**Pinned against:** RmlUi commit `2cd28864ae25ed345b70598751703a5433b12356` (the `GIT_TAG` in
`glintfx/CMakeLists.txt` at the time this patch was written). **Status:** not yet reported
upstream (mikke89/RmlUi), reporting/submitting upstream is the intended next step; this
patch should be dropped from this repo once upstream carries an equivalent fix and the pinned
`GIT_TAG` is bumped past it.

**PT:** Corrige 2 achados distintos e genuínos de undefined behavior na própria ordem de
teardown de elemento/documento do RmlUi, ambos capturados por `GLINTFX_SANITIZE=ON` (UBSan) e
antes contornados por 3 entradas de suppression em `glintfx/tests/ubsan_suppressions.txt`
(agora removidas: este patch corrige a causa raiz em vez de suprimir o sintoma):

1. **`vptr:ElementDocument`**: `Element::SetOwnerDocument` (chamado recursivamente a partir
   de `~Element()` enquanto um documento desmonta os próprios filhos) desreferencia
   `owner_document->GetContext()` através de um ponteiro cuja parte derivada `ElementDocument`
   já terminou de se destruir naquele ponto da cadeia de chamada, um uso de ponteiro para um
   objeto derivado cuja parte derivada já foi destruída (UB estrito). Corrigido dando ao
   `ElementDocument` um corpo real de destrutor (`Source/Core/ElementDocument.cpp`) que limpa
   `owner_document` em cada filho direto (que por sua vez recursa sobre toda a subárvore via a
   implementação já existente de `SetOwnerDocument`) **enquanto `this` ainda é um
   `ElementDocument` totalmente válido**, antecipando-se à limpeza recursiva posterior e
   insegura do teardown da base `~Element()`. Exige que `ElementDocument` alcance
   `Element::meta`/internos como `friend` (`Include/RmlUi/Core/Element.h`).
2. **`vptr:*WidgetScroll.cpp*` / `vptr:*RemoveEventListener*`**: o mesmo padrão de causa
   raiz, dentro do próprio widget interno de scrollbar do RmlUi: `~WidgetScroll()` chama
   `RemoveEventListener()` em até 4 `Element*` que guarda para os próprios filhos gerados de
   scrollbar, que naquele momento já podem estar no meio da própria destruição na mesma
   cascata recursiva de teardown. Corrigido reordenando `Element::~Element()`
   (`Source/Core/Element.cpp`) para liberar `meta` (que é dono do `WidgetScroll`, cujo
   destrutor é o que toca aqueles `Element*` irmãos) **antes** de `children.clear()` rodar, em
   vez de depois: a cadeia de destrutor de `meta` não observa mais filhos já destruídos.

Ambos foram capturados ao vivo sob `GLINTFX_SANITIZE=ON` (ASan+UBSan), ver
`glintfx/CHANGELOG.md` `[Unreleased]` e `TESTES.md` `TST-L1-MEM` para a descoberta/triagem
original. Validado: com este patch aplicado e as 3 entradas de suppression acima removidas, a
suíte completa (`GLINTFX_BACKEND_GLFW=ON`/`OFF`) roda limpa sob UBSan: zero achados
`vptr:*` para estes 3 padrões (se o patch falhasse ao aplicar, a suíte falharia ruidosamente
em vez de continuar verde silenciosamente, que é o motivo de remover as suppressions em vez
de só deixá-las junto com a correção).

**Pinado contra:** commit `2cd28864ae25ed345b70598751703a5433b12356` do RmlUi (o `GIT_TAG` em
`glintfx/CMakeLists.txt` no momento em que este patch foi escrito). **Status:** ainda não
reportado ao upstream (mikke89/RmlUi), reportar/submeter ao upstream é o próximo passo
pretendido; este patch deve ser removido deste repositório assim que o upstream carregar uma
correção equivalente e o `GIT_TAG` pinado avançar para além dela.
