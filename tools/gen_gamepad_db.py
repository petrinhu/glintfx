#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (c) 2026 Petrus Silva Costa
#
# EN: Generator for glintfx's embedded, Linux-only gamepad mapping database
#     (A2-GAMEPAD, ADR-0015 (b) "gamepad", ADR-0016). Reads the already-vendored, already
#     platform-filtered text file
#     glintfx/third_party/gamecontrollerdb/gamecontrollerdb_linux.txt (see that directory's
#     README.md for the fetch/filter recipe -- a manual, documented step, exactly like
#     glintfx/third_party/khronos/gl.xml is fetched by hand rather than by tools/gen_glloader.py)
#     and emits ONE generated (do-not-hand-edit) file:
#       glintfx/third_party/gamecontrollerdb/gamecontrollerdb_linux.inc -- a `static const char
#       glintfx_gamecontrollerdb_linux[]` byte array holding the exact bytes of the vendored
#       .txt, as a sequence of adjacent (C/C++ auto-concatenated) string-literal chunks each
#       kept well under the 64 KB single-literal-length limit some compilers still enforce
#       (conservative 4096-byte raw chunk size, before escaping expansion), plus a computed
#       `glintfx_gamecontrollerdb_linux_len` (== `sizeof(...) - 1`, excluding the implicit
#       trailing NUL every string literal carries).
#
#     This script does NOT touch the network, does NOT fetch anything from GitHub, and does
#     NOT perform the platform:Linux filtering itself -- both of those are the manual,
#     documented "re-syncing" recipe in third_party/gamecontrollerdb/README.md. Its ONLY input
#     is the committed .txt; its ONLY output is the generated .inc. Deterministic: the same
#     input .txt byte-for-byte always produces the same output .inc byte-for-byte.
#
#     Loose ("grosseira") sanity validation only (this is TRUSTED, vendored data, not hostile
#     input -- the real hostile-input hardening lives in glintfx's own runtime parser,
#     MappingDb::parse_text, glintfx/src/gamepad_mapping.cpp): every non-comment, non-blank
#     line must contain at least 2 commas (the minimum shape of `guid,name,...`). This catches
#     an accidental corrupt/truncated re-sync (e.g. a broken filter that let non-comma-separated
#     garbage through) without re-implementing the runtime parser's full grammar here.
#
# PT: Gerador do banco de dados de mapeamento de gamepad, só-Linux, embutido no glintfx
#     (A2-GAMEPAD, ADR-0015 (b) "gamepad", ADR-0016). Lê o arquivo de texto já vendorizado, já
#     filtrado por plataforma,
#     glintfx/third_party/gamecontrollerdb/gamecontrollerdb_linux.txt (ver o README.md desse
#     diretório pela receita de fetch/filtro -- um passo manual, documentado, exatamente como o
#     glintfx/third_party/khronos/gl.xml é baixado à mão em vez de pelo tools/gen_glloader.py)
#     e emite UM arquivo gerado (não editar à mão):
#       glintfx/third_party/gamecontrollerdb/gamecontrollerdb_linux.inc -- um array de bytes C
#       `static const char glintfx_gamecontrollerdb_linux[]` guardando os bytes exatos do .txt
#       vendorizado, como uma sequência de pedaços de literal-de-string adjacentes
#       (concatenados automaticamente pelo C/C++) cada um mantido bem abaixo do limite de 64 KB
#       por literal que alguns compiladores ainda impõem (tamanho de pedaço cru conservador de
#       4096 bytes, antes da expansão de escape), mais um `glintfx_gamecontrollerdb_linux_len`
#       calculado (== `sizeof(...) - 1`, excluindo o NUL final implícito que todo literal de
#       string carrega).
#
#     Este script NÃO toca a rede, NÃO busca nada do GitHub, e NÃO faz a filtragem
#     platform:Linux ele mesmo -- os dois são a receita manual, documentada, de
#     "re-sincronização" em third_party/gamecontrollerdb/README.md. Sua ÚNICA entrada é o .txt
#     commitado; sua ÚNICA saída é o .inc gerado. Determinístico: a mesma entrada .txt
#     byte-a-byte sempre produz a mesma saída .inc byte-a-byte.
#
#     Validação de sanidade só "grosseira" (é dado CONFIÁVEL, vendorizado, não é entrada
#     hostil -- o hardening real de entrada hostil vive no parser próprio de runtime do
#     glintfx, MappingDb::parse_text, glintfx/src/gamepad_mapping.cpp): toda linha
#     não-comentário, não-vazia precisa conter pelo menos 2 vírgulas (a forma mínima de
#     `guid,name,...`). Isso pega um re-sync corrompido/truncado por acidente (ex.: um filtro
#     quebrado que deixou passar lixo sem vírgula) sem reimplementar a gramática completa do
#     parser de runtime aqui.
#
# Usage / Uso:
#   python3 tools/gen_gamepad_db.py
#
# Regenerates glintfx/third_party/gamecontrollerdb/gamecontrollerdb_linux.inc in place from
# glintfx/third_party/gamecontrollerdb/gamecontrollerdb_linux.txt.
#
# Regenera glintfx/third_party/gamecontrollerdb/gamecontrollerdb_linux.inc a partir de
# glintfx/third_party/gamecontrollerdb/gamecontrollerdb_linux.txt.

import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
GAMECONTROLLERDB_DIR = REPO_ROOT / "glintfx" / "third_party" / "gamecontrollerdb"
SRC_TXT = GAMECONTROLLERDB_DIR / "gamecontrollerdb_linux.txt"
OUT_INC = GAMECONTROLLERDB_DIR / "gamecontrollerdb_linux.inc"

# EN: Raw-byte chunk size, BEFORE escaping expansion. Worst case every byte escapes to a
#     4-character octal sequence (`\ooo`), so 4096 raw bytes never exceeds ~16 KB of escaped
#     literal text -- far under the 64 KB per-literal limit, with ample headroom.
# PT: Tamanho de pedaço de bytes crus, ANTES da expansão de escape. No pior caso todo byte
#     escapa pra uma sequência octal de 4 caracteres (`\ooo`), então 4096 bytes crus nunca
#     excede ~16 KB de texto literal escapado -- bem abaixo do limite de 64 KB por literal,
#     com folga generosa.
CHUNK_SIZE = 4096


def validate_lines(text: str) -> tuple[int, int]:
    """EN: Loose sanity check -- returns (data_lines, comment_or_blank_lines); exits non-zero
    if any non-comment, non-blank line looks structurally broken (fewer than 2 commas).
    PT: Checagem de sanidade solta -- retorna (linhas_de_dado, linhas_comentario_ou_vazias);
    sai com código não-zero se alguma linha não-comentário, não-vazia parecer estruturalmente
    quebrada (menos de 2 vírgulas)."""
    data_lines = 0
    other_lines = 0
    broken = []
    for lineno, line in enumerate(text.splitlines(), start=1):
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            other_lines += 1
            continue
        if stripped.count(",") < 2:
            broken.append((lineno, line))
            continue
        data_lines += 1
    if broken:
        print("ERROR: structurally broken line(s) in gamecontrollerdb_linux.txt "
              "(fewer than 2 commas -- not guid,name,... shape):", file=sys.stderr)
        for lineno, line in broken[:10]:
            print(f"  line {lineno}: {line!r}", file=sys.stderr)
        if len(broken) > 10:
            print(f"  ... and {len(broken) - 10} more", file=sys.stderr)
        sys.exit(1)
    return data_lines, other_lines


def c_escape_chunk(raw: bytes) -> str:
    """EN: Escape a raw byte chunk into the body of a C/C++ string literal (without the
    surrounding quotes). `"` and `\\` are backslash-escaped; `\n`/`\t`/`\r` get their named
    escapes; any other byte outside printable ASCII (0x20-0x7E) is emitted as a fixed
    3-digit octal escape (`\\ooo`) -- fixed-width so it can never accidentally swallow a
    following hex/octal-looking digit into itself (a `\\xHH`-style hex escape is greedy and
    would have that hazard; `\\ooo` is capped at exactly 3 digits by construction here).
    PT: Escapa um pedaço de bytes crus pro corpo de um literal de string C/C++ (sem as aspas
    ao redor). `"` e `\\` levam escape de barra invertida; `\n`/`\t`/`\r` ganham seus escapes
    nomeados; qualquer outro byte fora do ASCII imprimível (0x20-0x7E) é emitido como um
    escape octal de 3 dígitos fixo (`\\ooo`) -- largura fixa para nunca engolir sem querer um
    dígito hex/octal seguinte (um escape hex `\\xHH` é guloso e teria esse risco; o `\\ooo`
    aqui é limitado a exatamente 3 dígitos por construção)."""
    out = []
    for byte in raw:
        ch = chr(byte)
        if ch == '"':
            out.append('\\"')
        elif ch == "\\":
            out.append("\\\\")
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\t":
            out.append("\\t")
        elif ch == "\r":
            out.append("\\r")
        elif 0x20 <= byte <= 0x7E:
            out.append(ch)
        else:
            out.append(f"\\{byte:03o}")
    return "".join(out)


INC_PREAMBLE = """// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 Petrus Silva Costa
//
// EN: GENERATED FILE -- do not hand-edit. Produced by tools/gen_gamepad_db.py from
//     glintfx/third_party/gamecontrollerdb/gamecontrollerdb_linux.txt (see that file and this
//     directory's README.md for upstream provenance: pinned commit, fetch date, license). This
//     is DATA, not code -- the byte content is the vendored, Linux-filtered subset of the
//     community SDL_GameControllerDB mapping text (`guid,name,field:value,...,platform:Linux`
//     lines), consumed by glintfx's own clean-room parser (MappingDb::parse_text,
//     glintfx/src/gamepad_mapping.cpp) at runtime -- nothing here executes on its own.
//
//     `#include`d only from glintfx/src/gamepad.cpp (Agent B's device layer), under
//     `#if GLINTFX_MODULE_GAMEPAD`, gated further at runtime by
//     `GamepadsConfig::use_builtin_db` (default true). Module OFF removes this array and its
//     symbol from the archive entirely (same discipline as every other generated/vendored
//     data blob in this repository -- compare glintfx/src/gl_loader.{h,c}).
// PT: ARQUIVO GERADO -- não editar à mão. Produzido por tools/gen_gamepad_db.py a partir de
//     glintfx/third_party/gamecontrollerdb/gamecontrollerdb_linux.txt (ver esse arquivo e o
//     README.md deste diretório pela procedência upstream: commit pinado, data do fetch,
//     licença). Isto é DADO, não código -- o conteúdo de bytes é o subconjunto vendorizado,
//     filtrado por Linux, do texto de mapeamento da comunidade SDL_GameControllerDB (linhas
//     `guid,name,campo:valor,...,platform:Linux`), consumido pelo parser clean-room próprio do
//     glintfx (MappingDb::parse_text, glintfx/src/gamepad_mapping.cpp) em runtime -- nada aqui
//     executa por conta própria.
//
//     Incluído (`#include`) só a partir de glintfx/src/gamepad.cpp (camada de device do Agente
//     B), sob `#if GLINTFX_MODULE_GAMEPAD`, gateado ainda em runtime por
//     `GamepadsConfig::use_builtin_db` (default true). Módulo OFF remove este array e o
//     símbolo dele do archive por completo (mesma disciplina de todo outro blob de dado
//     gerado/vendorizado deste repositório -- compare glintfx/src/gl_loader.{h,c}).
//
// EN: <cstddef> included here (not assumed from the including TU) so this file is
//     self-contained regardless of what glintfx/src/gamepad.cpp includes before this line.
// PT: <cstddef> incluído aqui (não presumido da TU que inclui) para este arquivo ser
//     autocontido independente do que glintfx/src/gamepad.cpp inclui antes desta linha.
#include <cstddef>
"""


def main() -> None:
    if not SRC_TXT.exists():
        print(f"ERROR: {SRC_TXT} not found. See {GAMECONTROLLERDB_DIR / 'README.md'} "
              "for the fetch/filter recipe (this generator does not fetch anything itself).",
              file=sys.stderr)
        sys.exit(1)

    raw_bytes = SRC_TXT.read_bytes()
    text = raw_bytes.decode("utf-8")

    data_lines, other_lines = validate_lines(text)

    lines = [INC_PREAMBLE, "\n"]
    lines.append("static const char glintfx_gamecontrollerdb_linux[] =\n")
    for offset in range(0, len(raw_bytes), CHUNK_SIZE):
        chunk = raw_bytes[offset:offset + CHUNK_SIZE]
        lines.append(f'  "{c_escape_chunk(chunk)}"\n')
    lines[-1] = lines[-1].rstrip("\n") + ";\n\n"
    lines.append("// EN: Length in bytes, EXCLUDING the implicit trailing NUL every C string\n"
                 "//     literal carries -- computed from the array itself (sizeof of an\n"
                 "//     adjacent-string-literal-concatenated array already reflects the FULL\n"
                 "//     concatenated content plus one trailing NUL, per the C/C++ translation-\n"
                 "//     phase-6 concatenation rule), so this stays correct even if CHUNK_SIZE\n"
                 "//     or the input file size changes on a future re-sync.\n"
                 "// PT: Tamanho em bytes, EXCLUINDO o NUL final implícito que todo literal de\n"
                 "//     string C carrega -- calculado a partir do próprio array (sizeof de um\n"
                 "//     array concatenado por literais-de-string adjacentes já reflete o\n"
                 "//     conteúdo concatenado INTEIRO mais um NUL final, pela regra de\n"
                 "//     concatenação da fase de tradução 6 do C/C++), então isto permanece\n"
                 "//     correto mesmo se o CHUNK_SIZE ou o tamanho do arquivo de entrada\n"
                 "//     mudarem num re-sync futuro.\n"
                 "static const std::size_t glintfx_gamecontrollerdb_linux_len =\n"
                 "  sizeof(glintfx_gamecontrollerdb_linux) - 1;\n")

    OUT_INC.write_text("".join(lines))

    print(f"Generated {OUT_INC.relative_to(REPO_ROOT)} "
          f"({len(raw_bytes)} bytes, {data_lines} data lines, {other_lines} comment/blank "
          f"lines, {len(raw_bytes) // CHUNK_SIZE + 1} literal chunks).")


if __name__ == "__main__":
    main()
