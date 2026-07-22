#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (c) 2026 Petrus Silva Costa
#
# EN: Generator for A4-EMOJI's committed `COLR` v0 + `CPAL` v0 test fixture
#     (tests/assets/colr_fixture.ttf). Precedent: tools/gen_gamepad_db.py/tools/gen_glloader.py --
#     a small, dev-time-only, offline/reproducible generator committed next to its OUTPUT, not run
#     at build time (neither `make build`/`make test` (Layer 0) nor glintfx's CMake invoke this
#     script -- it is a manual, documented re-generation step, same posture
#     third_party/gamecontrollerdb/README.md already documents for its own generator). Uses
#     `fontTools` (https://github.com/fonttools/fonttools, MIT-licensed, already on this machine --
#     `pip install fonttools`), the SAME independent-oracle library already used (offline, never as
#     a source of implementation code) to cross-check every hand-built synthetic font in
#     tests/test_sfnt.c -- see that file's own header for the clean-room statement this reuses.
#     Unlike tests/test_sfnt.c's own hand-built-byte-array `kSyntheticFontColr` (a minimal,
#     C1-harness-only blob with no 'name'/'post'/'OS-2' tables -- see that file's own fixture
#     comment), THIS script emits a genuinely well-formed TTF via `fontTools.fontBuilder
#     .FontBuilder` (the library's own OWN compiler, not a raw byte-patch) -- meant for
#     glintfx's own (S2's, `L1.19-FONTENG`) pixel-proof test, which loads a REAL file the same way
#     tests/fixtures/opensans_regular.ttf already is (see the Makefile's `$(OBJ)/opensans_ttf.o`
#     rule's own comment for that precedent's rationale) -- a font consumer should not have to
#     special-case "the fixture skips tables a real font always has".
#
#     Output: tests/assets/colr_fixture.ttf -- ~1KB, 5 glyphs (`.notdef` empty; `triangle`/`square`,
#     ordinary simple outlines, ALSO reused as the two COLR layer shapes -- a COLR layer is
#     spec-defined to be an ordinary `glyf` outline, no separate "layer glyph" concept exists;
#     `emoji_two_layer`, no outline of its own, COLR base glyph = triangle@palette-entry-0 (opaque
#     red) UNDER square@palette-entry-1 (opaque blue); `emoji_fg_sentinel`, no outline of its own,
#     COLR base glyph = triangle@palette-index `0xFFFF`, the spec sentinel for "use the text/
#     foreground colour, not a palette entry" -- exercising the "um glifo usando paleta 0xFFFF"
#     fixture requirement this ticket's brief names explicitly, same design as
#     tests/test_sfnt.c's own `kSyntheticFontColr`). `cmap`: `U+E000` (Private Use Area, never a
#     real assigned codepoint -- deliberately avoids colliding with any real emoji block) maps to
#     `emoji_two_layer`, `U+E001` to `emoji_fg_sentinel`, so a pixel-proof test can resolve either
#     glyph by ordinary codepoint lookup instead of a hardcoded gid. License: ours (MPL-2.0,
#     synthetic, no vendored/third-party glyph data).
#
#     Deterministic: `fontTools.fontBuilder.FontBuilder` with no timestamp/version fields left to
#     chance produces the SAME bytes for the SAME fontTools version across runs -- a re-run only
#     changes tests/assets/colr_fixture.ttf if this script itself, or the installed fontTools
#     version, changed.
# PT: Gerador da fixture de teste `COLR` v0 + `CPAL` v0 commitada do A4-EMOJI
#     (tests/assets/colr_fixture.ttf). Precedente: tools/gen_gamepad_db.py/tools/gen_glloader.py --
#     um gerador pequeno, só-dev-time, offline/reproduzível, commitado ao lado da própria SAÍDA,
#     não rodado em tempo de build (nem o `make build`/`make test` (Camada 0) nem o CMake do
#     glintfx invocam este script -- é um passo manual, documentado, de re-geração, mesma postura
#     que o third_party/gamecontrollerdb/README.md já documenta pro próprio gerador dele). Usa o
#     `fontTools` (https://github.com/fonttools/fonttools, licença MIT, já nesta máquina --
#     `pip install fonttools`), a MESMA biblioteca-oráculo independente já usada (offline, nunca
#     como fonte de código de implementação) pra cruzar toda fonte sintética construída à mão no
#     tests/test_sfnt.c -- ver o cabeçalho próprio daquele arquivo pra declaração clean-room que
#     este script reusa. Diferente do próprio array-de-byte-construído-à-mão `kSyntheticFontColr`
#     do tests/test_sfnt.c (um blob minimalista, só-pro-harness-C1, sem tabelas
#     'name'/'post'/'OS-2' -- ver o comentário de fixture próprio daquele arquivo), ESTE script
#     emite um TTF genuinamente bem-formado via `fontTools.fontBuilder.FontBuilder` (o próprio
#     compilador da biblioteca, não um remendo-de-byte cru) -- pensado pro teste de prova-de-pixel
#     do próprio glintfx (do S2, `L1.19-FONTENG`), que carrega um arquivo DE VERDADE do mesmo jeito
#     que o tests/fixtures/opensans_regular.ttf já é (ver o comentário próprio da regra
#     `$(OBJ)/opensans_ttf.o` do Makefile pro racional desse precedente) -- quem consome uma fonte
#     não deveria precisar tratar como caso especial "a fixture pula tabela que uma fonte de
#     verdade sempre tem".
#
#     Saída: tests/assets/colr_fixture.ttf -- ~1KB, 5 glyphs (`.notdef` vazio; `triangle`/`square`,
#     outlines simples comuns, TAMBÉM reusados como as duas formas de camada COLR -- uma camada
#     COLR é, pela spec, um outline `glyf` comum, não existe conceito separado de "glyph de
#     camada"; `emoji_two_layer`, sem outline próprio, glyph-base COLR = triângulo@entrada-de-
#     paleta-0 (vermelho opaco) SOB quadrado@entrada-de-paleta-1 (azul opaco); `emoji_fg_sentinel`,
#     sem outline próprio, glyph-base COLR = triângulo@índice-de-paleta `0xFFFF`, o sentinela da
#     spec pra "usa a cor de texto/foreground, não uma entrada de paleta" -- exercitando o
#     requisito de fixture "um glifo usando paleta 0xFFFF" que o brief desta tarefa nomeia
#     explicitamente, mesmo desenho do próprio `kSyntheticFontColr` do tests/test_sfnt.c). `cmap`:
#     `U+E000` (Área de Uso Privado, nunca um codepoint real atribuído -- deliberadamente evita
#     colidir com bloco de emoji real nenhum) mapeia pro `emoji_two_layer`, `U+E001` pro
#     `emoji_fg_sentinel`, pra um teste de prova-de-pixel conseguir resolver qualquer um dos dois
#     glyphs por lookup de codepoint comum em vez de um gid cravado. Licença: nossa (MPL-2.0,
#     sintética, nenhum dado de glyph vendorizado/de-terceiro).
#
#     Determinístico: `fontTools.fontBuilder.FontBuilder` sem campo de timestamp/versão deixado ao
#     acaso produz os MESMOS bytes pra MESMA versão de fontTools entre execuções -- rodar de novo
#     só muda o tests/assets/colr_fixture.ttf se este script, ou a versão instalada de fontTools,
#     mudou.
import os

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_PATH = os.path.join(REPO_ROOT, "tests", "assets", "colr_fixture.ttf")

UNITS_PER_EM = 1000
GLYPH_ORDER = [".notdef", "triangle", "square", "emoji_two_layer", "emoji_fg_sentinel"]


def build_glyphs():
    glyphs = {}

    pen = TTGlyphPen(None)
    glyphs[".notdef"] = pen.glyph()

    # EN: triangle -- 3 on-curve points, reused verbatim as the RED (palette entry 0) layer shape.
    # PT: triangle -- 3 pontos on-curve, reusado ao pé da letra como a forma de camada VERMELHA
    #     (entrada de paleta 0).
    pen = TTGlyphPen(None)
    pen.moveTo((0, 0))
    pen.lineTo((100, 0))
    pen.lineTo((50, 100))
    pen.closePath()
    glyphs["triangle"] = pen.glyph()

    # EN: square -- 4 on-curve points, reused verbatim as the BLUE (palette entry 1) layer shape.
    # PT: square -- 4 pontos on-curve, reusado ao pé da letra como a forma de camada AZUL (entrada
    #     de paleta 1).
    pen = TTGlyphPen(None)
    pen.moveTo((0, 0))
    pen.lineTo((100, 0))
    pen.lineTo((100, 100))
    pen.lineTo((0, 100))
    pen.closePath()
    glyphs["square"] = pen.glyph()

    # EN: The two COLR base glyphs have NO outline of their own (empty, same shape as `.notdef` --
    #     COLR layers are what actually paints, see this file's own header).
    # PT: Os dois glyphs-base COLR NÃO têm outline próprio (vazio, mesma forma do `.notdef` -- as
    #     camadas COLR são quem de fato pinta, ver o cabeçalho próprio deste arquivo).
    glyphs["emoji_two_layer"] = TTGlyphPen(None).glyph()
    glyphs["emoji_fg_sentinel"] = TTGlyphPen(None).glyph()

    return glyphs


def main():
    fb = FontBuilder(UNITS_PER_EM, isTTF=True)
    fb.setupGlyphOrder(GLYPH_ORDER)
    fb.setupCharacterMap({0xE000: "emoji_two_layer", 0xE001: "emoji_fg_sentinel"})
    fb.setupGlyf(build_glyphs())
    fb.setupHorizontalMetrics({name: (1000, 0) for name in GLYPH_ORDER})
    fb.setupHorizontalHeader(ascent=1000, descent=-200)
    fb.setupNameTable({"familyName": "GlintfxColrFixture", "styleName": "Regular"})
    fb.setupOS2()
    fb.setupPost()

    fb.setupCOLR(
        {
            "emoji_two_layer": [("triangle", 0), ("square", 1)],
            "emoji_fg_sentinel": [("triangle", 0xFFFF)],
        }
    )
    # EN: fontTools' own CPAL API takes floats in [0, 1] per channel (RGBA) -- (1,0,0,1)=opaque
    #     red, (0,0,1,1)=opaque blue, matching kSyntheticFontColr's own two palette entries
    #     (tests/test_sfnt.c) so both fixtures describe the SAME visible scenario.
    # PT: A própria API CPAL do fontTools recebe float em [0, 1] por canal (RGBA) --
    #     (1,0,0,1)=vermelho opaco, (0,0,1,1)=azul opaco, casando com as próprias duas entradas de
    #     paleta do kSyntheticFontColr (tests/test_sfnt.c) pra as duas fixtures descreverem o MESMO
    #     cenário visível.
    fb.setupCPAL([[(1.0, 0.0, 0.0, 1.0), (0.0, 0.0, 1.0, 1.0)]])

    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    fb.save(OUT_PATH)
    print(f"wrote {OUT_PATH} ({os.path.getsize(OUT_PATH)} bytes)")


if __name__ == "__main__":
    main()
