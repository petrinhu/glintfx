# A4-EMOJI — COLR emoji (vector) · minimal defensible slice / fatia mínima defensável

Status: PLANNED (Caetano/CTO scoping 2026-07-21, awaiting go from líder) · Branch: `feat/framework2d-emoji-colr` · Target tag: minor.

---

## EN

### 1. Verdict / scope decision

**GO, with the minimal slice: COLRv0 + CPAL + single-codepoint emoji + VS15/VS16 skip. COLRv1, ZWJ/GSUB shaping, CBDT/CBLC and sbix are DEFERRED with explicit triggers.**

Why the slice is small enough for a small workflow (<5 agents): the audit of the current pipeline found that **most of the expensive groundwork already exists**:

| Need | Status found (file:line) |
|---|---|
| RGBA atlas | **Already RGBA8** — `FaceInstance::atlas_rgba` is 4 bytes/texel, coverage stored as premultiplied white `(cov,cov,cov,cov)` (`font_engine_own.cpp:1171,1217-1234`). A color glyph blits real premultiplied RGBA into the SAME atlas. **No new texture, no shader change, no renderer change.** |
| Text render integration | Vertex colour multiplies texels (`GenerateString`, `font_engine_own.cpp:1661`). Color glyph = emit its quad with vertex colour **white scaled by alpha** `(a,a,a,a)` instead of the text colour → `white × emoji_rgba = emoji_rgba`, opacity/fade still works. ~10 lines + an `is_color` flag in `GlyphInfo`. |
| Codepoints > U+FFFF | `cmap` **format 12 already parsed and preferred over format 4** (`include/core/sfnt.h:324-336`); UTF-8 walk is `Rml::StringIteratorU8` (4-byte sequences OK, `font_engine_own.cpp:1414,1473`). Single-codepoint emoji already resolves to a gid today — it just renders as a grayscale outline glyph or `.notdef`. |
| Emoji font next to a text font | **Per-glyph fallback shipped in v0.10.0 era** (FT-F4, `BakeGlyph` walks `fallback_faces_`, scales by the source face's own upem). Registering a COLR font with `fallback_face=true` is the exact consumer scenario and already works for the lookup half. |
| Layer rasterization | COLRv0 layers are **plain `glyf` glyphs** — each layer reuses `glx_sfnt_glyph_outline` + `glx_rasterize_outline` (coverage) unchanged. Zero new raster features. Tint coverage by the CPAL colour, composite premultiplied-OVER into a staging RGBA, blit. |

What is genuinely missing (the actual work):
1. **SOV-SFNT: parse `COLR` v0 + `CPAL` v0** (Layer 0, C, clean-room from the public OpenType spec). Two optional tables, soft-fail posture like `kern` (`kern_len==0` precedent). New API: enumerate `(layer_gid, palette_index)` for a base gid + fetch a palette entry as RGBA. ~250-350 lines + hostile tests.
2. **FontEngineOwn: color bake path** — in `BakeGlyph`/`RasterizeGlyph`, if the source face has COLR layers for the gid: per-layer coverage raster → tint → OVER-composite → blit RGBA; mark `GlyphInfo::is_color`; `GenerateString` emits white-vertex quads for those. Union the layers' bboxes for the cell size.
3. **VS15/VS16 (U+FE0E/U+FE0F) skip** in `IterateGlyphs`/`EnsureGlyphs`: treat as zero-width ignorables so `☺️` doesn't produce a tofu after the emoji. Cheap, high value.
4. **Test fixture**: synthetic, hand-built COLR/CPAL font (see §4). No third-party font vendored.

### 2. COLRv0 vs COLRv1 — the gap, measured

- **COLRv0**: base glyph → ordered list of (glyph, palette colour) layers. Consumes SOV-RAST as-is. Parser is two flat arrays (`BaseGlyphRecord[]`, `LayerRecord[]`) + CPAL colour records (BGRA). Bounded, hostile-input-friendly, same `rd_u16/rd_u32` hardening idiom as the rest of `sfnt.c`.
- **COLRv1**: a paint **DAG** — solid, linear/radial/sweep gradients, affine transforms, 26 composite/blend modes, variable-font deltas. That is a mini 2D vector compositor: per-pixel gradient evaluation, transform stacks, blend modes SOV-RAST has no notion of, cycle-safe DAG traversal on hostile input. Estimated **10-20× the COLRv0 effort**. **Deferred.**
- Honest cost of the slice: **the flagship modern emoji font (Noto Color Emoji) is CBDT/CBLC or COLRv1 — NOT COLRv0.** Real COLRv0 coverage exists (Twemoji Mozilla ~full emoji set, CC-BY; Bungee Color, OFL, decorative), so the slice is real and consumable, but "any modern emoji font just works" is NOT this slice.
- **Triggers to revisit**: (a) a consumer needs a COLRv1-only font → re-scope COLRv1 as its own epic; (b) a consumer needs broad photographic emoji coverage cheaply → consider **CBDT/CBLC first** (bitmap PNG glyphs; `stb_image` is already vendored, so that path is plausibly CHEAPER than COLRv1 and covers Noto).

### 3. Shaping: OUT (single-codepoint only)

ZWJ sequences (👨‍👩‍👧), skin tones, flags = multiple codepoints → one glyph via GSUB ligature lookups. We have no GSUB; the i18n module explicitly deferred complex shaping and emoji ZWJ is the same class. **Deferred, same trigger as i18n shaping.** Degradation is the Unicode-sanctioned one: a ZWJ family renders as its individual member emoji; a tone-modified emoji renders base + (unmapped) modifier. VS15/16 skip (in scope) removes the ugliest artifact.

### 4. Test fixture (license + size)

**Synthetic hand-built font, no vendoring.** Precedent: `tests/test_sfnt.c` already hand-builds a synthetic cmap-format-12 SFNT blob. Extend the technique: a tiny TTF (2 base glyphs; one COLR'd into 2-3 layers — e.g. a square + triangle in distinct palette colours, one glyph with palette index `0xFFFF`) either (a) built in-memory by a shared fixture builder used by the C tests, and (b) committed as a ~1-2 KB `tests/assets/colr_fixture.ttf` generated by a `tools/gen_colr_fixture.py` (fontTools, dev-time only, already on the machine) for the glintfx pixel test. License: ours (MPL-2.0, synthetic). Pixel proof: render the fixture glyph via `UiLayer`, read back the framebuffer, assert exact layer colours at known pixels (llvmpipe-safe — solid fills, no gradients).

### 5. Known limitations (documented, accepted)

- Palette index `0xFFFF` (text foreground) bakes as **opaque white** (atlas is baked colour-independent; per-text-colour bake would break the atlas cache). Documented; trigger: consumer complaint.
- `kMaxOutlinePoints = 768` applies **per layer** (layers are simple glyphs) — enough in practice; `BUFFER_TOO_SMALL` on a hostile/fat layer degrades to "glyph skipped", never UB.
- First palette only (`CPAL` palette 0). Multiple palettes deferred.
- No CBDT/sbix; no COLRv1; no ZWJ (above).

### 6. Execution — 3 agents, strict pipeline (1 heavy build at a time)

Constraints: build in `/var/tmp` (`TMPDIR=/var/tmp`), 1 heavy build at a time (RAM), `windows-atoms.yml` untouched, local static gates (clang-tidy + cppcheck + gitleaks) replicated before push, **vendor sync**: every change to `src/sfnt.c`/`include/core/sfnt.h` is mirrored byte-identical to `glintfx/vendor/core/` (gate: `diff -r`).

- **S1 — `backend-engineer` (C, Layer 0)**: owns `src/sfnt.c`, `include/core/sfnt.h`, `tests/test_sfnt.c`, `tools/gen_colr_fixture.py`, + vendor mirror. Implements `COLR` v0 + `CPAL` v0 parsing, clean-room from the public OpenType spec (bilingual doc-comments; hardening posture identical to `kern`: optional table, soft-fail to "no COLR", every read via `rd_*`, no `offset+size` overflow, layer-range vs `numLayerRecords` validated, `numBaseGlyphRecords` lying → clean error, binary-search over sorted BaseGlyphRecords with the same hostile-unsorted degradation posture as KERN-BSEARCH). New API sketch: `glx_sfnt_colr_layers(face, gid, glx_sfnt_colr_layer* out, uint16_t cap, uint16_t* count_out)` (`{uint16_t gid; uint16_t palette_index;}`) + `glx_sfnt_cpal_rgba(face, uint16_t entry_index, uint8_t rgba_out[4])` returning straight-alpha sRGB. TDD under the C1 harness: happy path + ≥6 hostile cases (truncated COLR, truncated CPAL, layer index out of range, lying counts, unsorted records, colour index out of range). Gate: `make test` + `nm -u` (no libc/libm symbol) + sanitize leg.
- **S2 — `engine-graphics-programmer` (C++, glintfx)**: owns `glintfx/src/font_engine_own.{cpp,hpp}`, `glintfx/tests/` (new `fonteng_colr_sanity`). After S1 lands + vendor sync. Color bake path (per-layer coverage → CPAL tint → premultiplied OVER into staging RGBA → blit; bbox = union of layer bboxes; `0xFFFF` → white), `GlyphInfo::is_color` + white-vertex `(a,a,a,a)` quads in `GenerateString`, VS15/16 skip in the shared `IterateGlyphs`/`EnsureGlyphs` walk (width==mesh invariance preserved — both share the walk), fallback-face plumbing (color glyph from a fallback COLR face bakes into the primary instance's atlas, scaled by the source face's upem — existing FT-F4 mechanics). Pixel-proof test with the committed fixture via `UiLayer` readback (exact colours at known pixels; also proves text colour does NOT tint the emoji and that a grayscale glyph in the same string still tints). Gate: full ctest matrix (ON GLFW / ON embed / OFF), static gates, single heavy build.
- **S3 — `qa-engineer` (adversarial, EXECUTES)**: distinct from both implementers. Runs the pixel proof + mutation-style attacks: corrupt the fixture's COLR/CPAL bytes (truncation, layer-gid=0xFFFF, palette index OOB, cyclic-ish/lying counts) through the FULL engine under ASan+UBSan in the `glintfx-ci:f42` container; verifies clean degradation (no crash/UAF/OOB, glyph skipped or tofu-less blank); re-checks width==mesh, dedup/reload (does the color atlas survive 50 reloads), and the domino question ("does this guard exist in the sibling path?"). Report with file:line claims; orchestrator re-verifies build + spot-checks claims before accepting.

Orchestrator (main thread) does the fan-out, captures outputs, re-verifies each deliverable (agent report is not proof), cites `A4-EMOJI` in commits, NO push without the líder's order.

Estimated size: S1 ≈ 250-350 lines C + tests; S2 ≈ 200-300 lines C++ + test; S3 report. Fits one small wave.

---

## PT

### 1. Veredito / decisão de escopo

**SEGUIR, com a fatia mínima: COLRv0 + CPAL + emoji de codepoint único + skip de VS15/VS16. COLRv1, shaping ZWJ/GSUB, CBDT/CBLC e sbix ADIADOS com gatilhos explícitos.**

Por que a fatia cabe num workflow pequeno: a auditoria do pipeline atual mostrou que **a maior parte do chão caro já existe** — o atlas JÁ é RGBA8 premultiplicado (`atlas_rgba`, branco-premultiplicado `(cov,cov,cov,cov)`, `font_engine_own.cpp:1171,1217-1234`), então glifo colorido = blitar RGBA real no MESMO atlas, **sem shader novo, sem textura nova, sem tocar o renderer**; a integração no texto é trocar a cor de vértice do quad do emoji por branco×alpha (`GenerateString:1661`, ~10 linhas + flag `is_color`); cmap formato 12 (fora do BMP) **já é parseado e preferido** (`sfnt.h:324-336`) e o decode UTF-8 de 4 bytes já funciona (`Rml::StringIteratorU8`); e o **fallback por-glyph do FT-F4 já resolve** o cenário "fonte emoji ao lado da fonte de texto". Camadas COLRv0 são glifos `glyf` comuns — o SOV-RAST rasteriza cada camada sem NENHUMA feature nova; só falta tingir pela cor do CPAL e compor OVER premultiplicado.

O que falta de verdade: (1) parser `COLR` v0 + `CPAL` v0 no SOV-SFNT (C, clean-room da spec pública, postura soft-fail idêntica ao `kern`); (2) caminho de bake colorido no `FontEngineOwn` + quads de vértice branco; (3) skip de VS15/VS16 no laço de glifos; (4) fixture sintética de teste.

### 2. COLRv0 vs COLRv1

COLRv0 = dois arrays planos + cores BGRA — tratável, mesmo idioma de hardening do resto do `sfnt.c`. COLRv1 = grafo de paint (gradientes linear/radial/sweep, transforms afins, 26 modos de composição, var-fonts) = um mini-compositor vetorial 2D, **10-20× o esforço** — adiado. **Custo honesto da fatia:** a Noto Color Emoji moderna é CBDT ou COLRv1, NÃO COLRv0; cobertura COLRv0 real existe (Twemoji Mozilla, Bungee Color), mas "qualquer fonte emoji moderna funciona" NÃO é esta fatia. Gatilhos: consumidor precisando de fonte só-COLRv1 → épico COLRv1; consumidor precisando de cobertura ampla barata → avaliar **CBDT/CBLC primeiro** (PNG bitmap; o stb_image já está vendorizado — provavelmente mais barato que COLRv1 e cobre a Noto).

### 3. Shaping: FORA (só codepoint único)

ZWJ/tons de pele/bandeiras exigem GSUB (mesma classe do shaping complexo adiado no i18n) — adiado com o mesmo gatilho. Degradação é a sancionada pelo Unicode (família ZWJ vira os emoji individuais). O skip de VS15/16 (em escopo) elimina o artefato mais feio (tofu depois de `☺️`).

### 4. Fixture de teste

**Fonte sintética construída à mão, nada vendorizado** (precedente: fixture cmap-12 sintética no `tests/test_sfnt.c`). TTF de ~1-2 KB commitado em `tests/assets/` + gerador `tools/gen_colr_fixture.py` (fontTools, só dev-time), 2-3 camadas em cores sólidas distintas + um glifo com paleta `0xFFFF`. Licença nossa (MPL-2.0). Prova de pixel via readback do `UiLayer` (cores sólidas exatas, seguro no llvmpipe).

### 5. Limitações documentadas

`0xFFFF` (foreground) vira branco opaco (bake é independente de cor de texto — preserva o cache de atlas); `kMaxOutlinePoints=768` vale POR CAMADA (degrada pra glifo pulado, nunca UB); só a paleta 0; sem CBDT/sbix/COLRv1/ZWJ.

### 6. Execução — 3 agentes, pipeline estrito

S1 `backend-engineer` (C, Camada 0: COLR/CPAL + testes hostis C1 + espelho `glintfx/vendor/core/` byte-idêntico, gate `diff -r` + `nm -u`) → S2 `engine-graphics-programmer` (C++: bake colorido, `is_color`, vértice branco×alpha, skip VS15/16, fallback-face, teste `fonteng_colr_sanity` com prova de pixel) → S3 `qa-engineer` adversarial (EXECUTA: corrompe COLR/CPAL da fixture no motor completo sob ASan+UBSan no container `glintfx-ci:f42`, verifica degradação limpa + dominó + width==mesh + 50 reloads). Posse disjunta de arquivos; 1 build pesado por vez em `/var/tmp`; `windows-atoms.yml` intocado; gates estáticos locais antes de qualquer push; commits citam `A4-EMOJI`; push só com ordem do líder. Orquestrador (thread principal) faz o fan-out e re-verifica cada entregável.
