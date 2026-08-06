#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""
UIX-RCSS-CENSO-NO-REPO -- re-derivable RCSS corpus census.

EN: Reads every git-tracked `.rcss` file and every `<style>` block inside
every git-tracked `.rml` file, parses them with a real brace/string-aware
tokenizer (never a line-oriented regex -- RCSS selectors and comments
routinely cross line boundaries in this corpus), and prints a JSON census
to stdout. This is the rederivation script `docs/uix-rcss-censo.md` cites;
run it again any time the corpus changes and the numbers in that document
should be regenerated from this script's output, not hand-edited.

PT: Le todo arquivo `.rcss` rastreado pelo git e todo bloco `<style>` dentro
de todo `.rml` rastreado, faz o parse com um tokenizador de verdade
(sensivel a chaves e a strings, nunca uma regex orientada a linha -- nesta
casa, seletor e comentario RCSS cruzam quebra de linha com frequencia), e
imprime um censo em JSON no stdout. Este e o script de rederivacao que o
`docs/uix-rcss-censo.md` cita; rode de novo sempre que o corpus mudar, e
regenere os numeros do documento a partir da saida deste script, nunca a
mao.

Usage / Uso:
    python3 tools/rcss_census.py [--repo-root PATH] [--include-hpp PATH ...]
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path


# ---------------------------------------------------------------------------
# 1. Corpus enumeration / Enumeracao do corpus
# ---------------------------------------------------------------------------

def git_ls_files(repo_root: Path, *patterns: str) -> list[str]:
    out = subprocess.run(
        ["git", "ls-files", *patterns],
        cwd=repo_root, capture_output=True, text=True, check=True,
    )
    return [line for line in out.stdout.splitlines() if line.strip()]


STYLE_TAG_RE = re.compile(r"<style\b[^>]*>", re.IGNORECASE)
STYLE_CLOSE_RE = re.compile(r"</style\s*>", re.IGNORECASE)


XML_COMMENT_RE = re.compile(r"<!--.*?-->", re.DOTALL)


def blank_xml_comments(text: str) -> str:
    """Blank `<!-- ... -->` spans (keep length/newlines) before tag-matching.

    EN: Measured bug, not a hypothetical: `difficulty_menu__lista_hardcore_
    bloqueado.rml:19` has an XML comment whose PROSE contains the literal
    substring `<style>` (documenting the RML lexer's own limitation around
    that tag) -- a naive `<style>...</style>` regex search matches that
    fake open tag first, then keeps scanning for the *next* `</style>` and
    swallows everything up to and including the real block as "RCSS text",
    corrupting the parse with markup and English/Portuguese prose. Same
    family of bug as the brief's own "comment counted as selector" case,
    one layer up (XML comment instead of CSS comment).
    PT: Bug medido, nao hipotetico: `difficulty_menu__lista_hardcore_
    bloqueado.rml:19` tem um comentario XML cuja PROSA contem a substring
    literal `<style>` (documentando uma limitacao do proprio lexer de RML
    perto dessa tag) -- uma busca ingenua por `<style>...</style>` casa essa
    tag-abertura falsa primeiro, continua procurando o PROXIMO `</style>` e
    engole tudo ate o bloco real, inclusive, como "texto RCSS", corrompendo
    o parse com markup e prosa em ingles/portugues. Mesma familia de bug do
    caso "comentario contado como seletor" do proprio brief, uma camada
    acima (comentario XML em vez de comentario CSS).
    """
    return XML_COMMENT_RE.sub(lambda m: "".join("\n" if c == "\n" else " " for c in m.group(0)), text)


def extract_style_blocks_from_rml(text: str) -> list[tuple[str, int]]:
    """Return [(inner_text, start_line_1based), ...] for every <style> block.

    EN: Line-number tracking here is only used for citation/reporting
    (pointing a human at file:line), never for deciding where a rule or a
    selector begins/ends -- that decision is made by the brace/string
    scanner below, which is line-agnostic by construction.
    PT: O rastreio de numero de linha aqui so serve pra citacao/relatorio
    (apontar humano pra arquivo:linha), nunca pra decidir onde uma regra ou
    um seletor comeca/termina -- essa decisao e' do scanner de chave/string
    abaixo, que e' alheio a linha por construcao.
    """
    scan_text = blank_xml_comments(text)
    blocks: list[tuple[str, int]] = []
    pos = 0
    while True:
        m_open = STYLE_TAG_RE.search(scan_text, pos)
        if not m_open:
            break
        m_close = STYLE_CLOSE_RE.search(scan_text, m_open.end())
        if not m_close:
            break
        inner = text[m_open.end():m_close.start()]
        start_line = text.count("\n", 0, m_open.end()) + 1
        blocks.append((inner, start_line))
        pos = m_close.end()
    return blocks


# ---------------------------------------------------------------------------
# 2. Comment stripping (string-aware, multi-line safe)
#    Remocao de comentario (sensivel a string, seguro multi-linha)
# ---------------------------------------------------------------------------

def strip_comments(text: str) -> str:
    """Blank out `/* ... */` AND `// ...` comments, string-aware.

    EN: A real scanner: tracks whether we are inside a single/double-quoted
    string (so a `/*`-looking sequence inside a `content:"/* not a comment
    */"`-style string, however unlikely in RCSS, is not misread), and does
    not require the `*/` on the same source line as the `/*` -- exactly the
    line-boundary bug this task's brief warns measuring this way by regex
    produced twice in one day in this house. `//`-to-end-of-line is also
    stripped: measured, not assumed -- this corpus's own RCSS (RmlUi's
    dialect, not plain CSS) uses `//` line comments for real, e.g.
    `save_load_menu__modo_carregar_dois_slots_ocupados.rml:121-125`, a
    5-line `//` comment block sitting directly between two real rules. A
    first pass of this script, missing this, mis-swallowed that prose as a
    3-part comma-list selector -- the exact "prose read as a selector"
    failure mode the brief itself names, one comment syntax later.
    PT: Um scanner de verdade: rastreia se estamos dentro de uma string
    aspas-simples/duplas (pra uma sequencia parecida com `/*` dentro de uma
    string nao ser lida errado), e nao exige o `*/` na mesma linha do `/*`
    -- exatamente o bug de fronteira-de-linha que o brief desta tarefa avisa
    ter acontecido, por regex, duas vezes num dia so nesta casa. `//` ate
    fim-de-linha tambem e removido: medido, nao suposto -- o RCSS deste
    corpus (dialeto do RmlUi, nao CSS puro) usa comentario `//` de verdade,
    ex. `save_load_menu__modo_carregar_dois_slots_ocupados.rml:121-125`, um
    bloco de comentario `//` de 5 linhas entre duas regras reais. Uma
    primeira passada deste script, sem isto, engoliu essa prosa como um
    seletor comma-list de 3 partes -- exatamente o modo de falha "prosa lida
    como seletor" que o proprio brief nomeia, numa sintaxe de comentario a
    mais.
    """
    out = []
    i = 0
    n = len(text)
    in_string: str | None = None  # holds the quote char, or None
    while i < n:
        c = text[i]
        if in_string:
            out.append(c)
            if c == "\\" and i + 1 < n:
                out.append(text[i + 1])
                i += 2
                continue
            if c == in_string:
                in_string = None
            i += 1
            continue
        if c in ("'", '"'):
            in_string = c
            out.append(c)
            i += 1
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            end = text.find("*/", i + 2)
            if end == -1:
                # Unterminated comment: blank to end of text.
                span = n - i
                out.append(" " * span)
                i = n
                break
            span = (end + 2) - i
            # Preserve newlines inside the comment so line-tracking (if any
            # downstream code wants it) stays correct.
            out.append("".join("\n" if ch == "\n" else " " for ch in text[i:end + 2]))
            i = end + 2
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            end = text.find("\n", i + 2)
            if end == -1:
                out.append(" " * (n - i))
                i = n
                break
            out.append(" " * (end - i))
            i = end  # leave the '\n' itself to be copied on next loop turn
            continue
        out.append(c)
        i += 1
    return "".join(out)


# ---------------------------------------------------------------------------
# 3. Brace-matching block scanner (string-aware)
#    Scanner de blocos por casamento de chave (sensivel a string)
# ---------------------------------------------------------------------------

@dataclass
class Block:
    prelude: str
    body: str
    kind: str  # "rule" | "at-font-face" | "at-keyframes" | "at-other"
    children: list["Block"] = field(default_factory=list)


def scan_top_level_blocks(text: str) -> tuple[list[Block], str]:
    """Split `text` into a flat list of top-level {prelude {body}} blocks.

    EN: Depth-counts `{`/`}` while skipping over quoted strings, so a `{`
    or `}` inside a string value never desyncs the match (RCSS values in
    this corpus do not contain literal braces in strings, but the scanner
    does not assume that -- it is correct even if one someday does).
    Returns (blocks, leftover_text) where leftover_text is whatever was
    outside any block (should be whitespace only for a well-formed sheet;
    reported, never silently dropped).
    PT: Conta profundidade de `{`/`}` pulando string entre aspas, entao um
    `{`/`}` dentro de uma string nunca desincroniza o casamento. Devolve
    (blocos, sobra) -- sobra deveria ser so espaco em branco numa folha
    bem formada; reportada, nunca descartada em silencio.
    """
    blocks: list[Block] = []
    i = 0
    n = len(text)
    leftover_parts = []
    while i < n:
        brace = text.find("{", i)
        if brace == -1:
            leftover_parts.append(text[i:])
            break
        prelude = text[i:brace]
        depth = 1
        j = brace + 1
        in_string: str | None = None
        while j < n and depth > 0:
            c = text[j]
            if in_string:
                if c == "\\" and j + 1 < n:
                    j += 2
                    continue
                if c == in_string:
                    in_string = None
                j += 1
                continue
            if c in ("'", '"'):
                in_string = c
                j += 1
                continue
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
            j += 1
        if depth != 0:
            # EN: Ran out of text before the brace closed -- a TRUNCATED block, not a
            # real one. Only surfaces on deliberately-malformed fixtures (this repo's
            # own .rcss/.rml corpus is always well-formed) or on a C++-embedded
            # fragment cut short by `<<` interpolation -- never silently treated as a
            # closed rule with fabricated content. The unclosed span becomes leftover,
            # tagged so a caller can tell "truncated" apart from "just whitespace".
            # PT: Acabou o texto antes da chave fechar -- um bloco TRUNCADO, nao um de
            # verdade. So aparece em fixture deliberadamente malformada (o corpus
            # .rcss/.rml deste repo e sempre bem-formado) ou num fragmento embutido em
            # C++ cortado por interpolacao `<<` -- nunca tratado em silencio como regra
            # fechada com conteudo fabricado. O trecho nao-fechado vira sobra, marcada
            # pra quem chama distinguir "truncado" de "so espaco em branco".
            leftover_parts.append(f"\x00TRUNCATED\x00{text[i:]}")
            i = n
            break
        body = text[brace + 1:j - 1]
        leftover_parts.append(prelude if prelude.strip() == "" else "")
        blocks.append(_classify(prelude, body))
        i = j
    leftover = "".join(leftover_parts)
    return blocks, leftover


def _classify(prelude: str, body: str) -> Block:
    p = prelude.strip()
    low = p.lower()
    if low.startswith("@font-face"):
        return Block(prelude=p, body=body, kind="at-font-face")
    if low.startswith("@keyframes"):
        return Block(prelude=p, body=body, kind="at-keyframes")
    if low.startswith("@"):
        return Block(prelude=p, body=body, kind="at-other")
    return Block(prelude=p, body=body, kind="rule")


def parse_stylesheet(text: str) -> tuple[list[Block], str]:
    clean = strip_comments(text)
    blocks, leftover = scan_top_level_blocks(clean)
    for b in blocks:
        if b.kind == "at-keyframes":
            children, _leftover2 = scan_top_level_blocks(b.body)
            b.children = children
    return blocks, leftover


# ---------------------------------------------------------------------------
# 3b. C++-embedded RCSS extraction -- THE THIRD ORIGIN
#     Extracao de RCSS embutida em C++ -- A TERCEIRA ORIGEM
# ---------------------------------------------------------------------------
#
# EN: RCSS in this repo lives in three places, not two: `.rcss` files, `<style>`
# blocks inside `.rml` files (both handled above), and C++ string/raw-string
# literals in `.cpp`/`.hpp` files -- either production (`glintfx/src/ua_stylesheet.hpp`,
# the base stylesheet applied to EVERY document glintfx loads) or test/demo fixtures
# built at runtime (`f << "body { margin: 0; ... }\n" ...`). The original census
# folded this third origin in; this census's first pass (commit `0de859f`) did not,
# because the task that produced it scoped the surviving corpus as ".rml + .rcss"
# only. This section closes that gap -- added on the coordinator's own explicit
# correction, not discovered independently the first time (see `docs/uix-rcss-censo.md`
# section 5 for the full account of why that mattered: 3 of the census's own 15
# comma-list selector instances live ONLY in `ua_stylesheet.hpp`).
#
# PT: RCSS neste repo mora em tres lugares, nao dois: arquivos `.rcss`, blocos
# `<style>` dentro de `.rml` (os dois tratados acima), e literais de string/raw-string
# de C++ em `.cpp`/`.hpp` -- produção (`glintfx/src/ua_stylesheet.hpp`, a folha base
# aplicada a TODO documento que a glintfx carrega) ou fixture de teste/demo construída
# em runtime (`f << "body { margin: 0; ... }\n" ...`). O censo original incluía essa
# terceira origem; a primeira passada deste censo (commit `0de859f`) não, porque a
# tarefa que a produziu escopou o corpus sobrevivente só como ".rml + .rcss". Esta
# seção fecha essa lacuna -- acrescentada por correção explícita do coordenador, não
# descoberta de forma independente da primeira vez (ver `docs/uix-rcss-censo.md`
# seção 5 pro relato completo de por que isso importou: 3 das próprias 15 instâncias
# de seletor comma-list do censo só vivem em `ua_stylesheet.hpp`).
#
# Classification rule (reproducible, structural -- NOT filename-pattern guessing,
# and NOT a hand-typed path list). A candidate is "test/demo" if its path is under
# `glintfx/demos/`, OR its basename is a source argument of some `add_executable(...)`
# in any git-tracked `glintfx/tests/**/CMakeLists.txt` -- i.e. it is actually compiled
# into a CTest binary, which is the real, structural ground truth for "this is a
# test", independent of which directory the source file physically lives in or what
# its filename happens to end with.
#
# EN: this rule was tightened after a real miss: the first version matched filenames
# ending `_sanity`/`_smoke`/`_bench`/`_test`, and silently misclassified
# `glintfx/src/rml/dom_dump_spec_conformance.cpp` as "production" -- its own header
# doc-comment says "byte-exact oracle", i.e. it plainly IS a test, but its name ends
# in `_conformance`, not one of the four guessed suffixes. Checked directly:
# `grep -l dom_dump_spec_conformance glintfx/tests/CMakeLists.txt` finds it registered
# as an `add_executable` source there (alongside its sibling
# `dom_dump_determinism_sanity.cpp`) even though the `.cpp` itself lives in
# `glintfx/src/rml/` by this repo's own convention (the RMLX-1 DOM-dump oracle tests
# are co-located with the implementation they test). A name-pattern guess is exactly
# the kind of "looks right, isn't verified against the build system" shortcut this
# whole census exists to replace with structural fact -- so the CMake-registration
# check, not the filename, is now the rule.
# PT: esta regra foi apertada depois de um erro real: a primeira versao casava nome
# de arquivo terminado em `_sanity`/`_smoke`/`_bench`/`_test`, e classificou em
# silencio `glintfx/src/rml/dom_dump_spec_conformance.cpp` como "producao" -- o
# proprio doc-comment de cabecalho dele diz "oraculo byte-exato", ou seja, e'
# claramente um teste, mas o nome termina em `_conformance`, nao um dos quatro
# sufixos chutados. Conferido direto: `grep -l dom_dump_spec_conformance
# glintfx/tests/CMakeLists.txt` acha ele registrado como fonte de `add_executable`
# la (ao lado do irmao `dom_dump_determinism_sanity.cpp`) mesmo o `.cpp` em si
# morando em `glintfx/src/rml/` pela propria convencao deste repo (os testes de
# oraculo do dump de DOM da RMLX-1 ficam ao lado da implementacao que testam). Um
# chute por padrao de nome e' exatamente o tipo de atalho "parece certo, nao foi
# verificado contra o sistema de build" que este censo inteiro existe pra trocar por
# fato estrutural -- entao a checagem de registro no CMake, nao o nome, e' a regra
# agora.

_CMAKE_TEST_SOURCES_CACHE: set[str] | None = None


_ADD_EXECUTABLE_RE = re.compile(r"add_executable\s*\(([^)]*)\)", re.IGNORECASE | re.DOTALL)


def _cmake_test_executable_basenames(repo_root: Path) -> set[str]:
    """Return every basename that appears as a SOURCE ARGUMENT of an
    `add_executable(...)` call in any `glintfx/tests/**/CMakeLists.txt`.

    EN: Deliberately narrower than "the basename appears anywhere in the CMake
    file" -- `ua_stylesheet.hpp`'s own basename appears in
    `glintfx/tests/uix_style/CMakeLists.txt`, but only inside a
    `target_compile_definitions(... GLINTFX_UIX_STYLE_UA_STYLESHEET="...ua_stylesheet.hpp")`
    string (a path handed to a test at runtime, not a source the test is BUILT
    from) -- a plain substring search would have misclassified it as test/demo.
    Parsing `add_executable(...)`'s own argument list specifically avoids that.
    PT: Deliberadamente mais estreito que "o nome aparece em algum lugar do
    arquivo CMake" -- o nome de `ua_stylesheet.hpp` aparece em
    `glintfx/tests/uix_style/CMakeLists.txt`, mas só dentro de uma string de
    `target_compile_definitions(... GLINTFX_UIX_STYLE_UA_STYLESHEET="...ua_stylesheet.hpp")`
    (um caminho passado a um teste em runtime, não um fonte do qual o teste é
    CONSTRUÍDO) -- uma busca de substring simples teria classificado errado como
    teste/demo. Parsear especificamente a lista de argumentos do próprio
    `add_executable(...)` evita isso.
    """
    global _CMAKE_TEST_SOURCES_CACHE
    if _CMAKE_TEST_SOURCES_CACHE is None:
        all_files = git_ls_files(repo_root, "*CMakeLists.txt")
        cmake_files = [f for f in all_files if f.startswith("glintfx/tests/")]
        basenames: set[str] = set()
        for rel in cmake_files:
            try:
                text = (repo_root / rel).read_text(encoding="utf-8")
            except (UnicodeDecodeError, OSError):
                continue
            for m in _ADD_EXECUTABLE_RE.finditer(text):
                for tok in m.group(1).split():
                    if tok.endswith(".cpp") or tok.endswith(".hpp"):
                        basenames.add(Path(tok).name)
        _CMAKE_TEST_SOURCES_CACHE = basenames
    return _CMAKE_TEST_SOURCES_CACHE

# EN: Known RCSS/RmlUi property names, used only as a HINT to decide whether an
# already-extracted, already-brace-balanced C++ string fragment is plausibly real
# RCSS worth feeding to the parser -- never used to find rule/selector boundaries
# (that is exclusively `scan_top_level_blocks`'s job, string/brace-aware, comment-
# blind-to-line-boundary). A doc-comment sentence that merely MENTIONS a property
# name in prose (e.g. "an author who writes `decorator: image-tint();`") does not
# pass this filter's structural half (see `require_hint` usage below) unless the
# surrounding text is *also* brace-balanced, which prose almost never is.
# PT: Nomes de propriedade RCSS/RmlUi conhecidos, usados so como DICA pra decidir se
# um fragmento de string C++ ja extraido e ja balanceado em chave e' plausivelmente
# RCSS real que vale a pena mandar pro parser -- nunca usado pra achar fronteira de
# regra/seletor (isso e' so trabalho do `scan_top_level_blocks`, sensivel a
# chave/string, cego a fronteira de linha). Uma frase de doc-comment que so MENCIONA
# um nome de propriedade em prosa nao passa a metade estrutural deste filtro (ver uso
# de `require_hint` abaixo) a menos que o texto ao redor tambem esteja balanceado em
# chave, o que prosa quase nunca esta.
_KNOWN_PROP_HINT = re.compile(
    r"(?<![A-Za-z-])(color|width|height|display|decorator|box-shadow|background|margin|"
    r"padding|position|font-family|font-size|ripple-[a-z-]+|transform|border-radius|"
    r"opacity|image-tint|mask-image|filter)\s*:"
)


def is_cpp_test_or_demo(rel_path: str, repo_root: Path) -> bool:
    if rel_path.startswith("glintfx/tests/") or rel_path.startswith("glintfx/demos/"):
        return True
    basename = Path(rel_path).name
    return basename in _cmake_test_executable_basenames(repo_root)


@dataclass
class CppToken:
    kind: str  # "string" | "raw" | "other"
    text: str  # decoded content for string/raw, raw source span for "other"


def tokenize_cpp_strings(code: str) -> list[CppToken]:
    """Walk already-comment-stripped C++ source, yielding STRING/RAW/OTHER tokens.

    EN: A `"..."` token decodes `\\n`/`\\t`/`\\\\`/`\\"` (the escapes this corpus's
    fixtures actually use; anything else passes through the backslashed character
    unchanged, which is always at least as safe as guessing). A raw-string token
    `R"delim(...)delim"` is captured with its delimiter and un-escaped content
    verbatim (raw strings do not process escapes by definition). Every other run of
    source (operators, identifiers, whitespace) becomes one OTHER token, so a caller
    can tell "two literals separated only by whitespace/newline" (true C++ adjacent-
    literal concatenation, byte-for-byte identical to one literal) apart from "two
    literals separated by real code" (e.g. `<< some_variable <<`, a value the source
    computes at runtime -- never guessable from static text).
    PT: Percorre fonte C++ ja sem comentario, gerando tokens STRING/RAW/OTHER. Um
    token `"..."` decodifica `\\n`/`\\t`/`\\\\`/`\\"` (os escapes que as fixtures
    deste corpus de fato usam; qualquer outro passa o caractere apos a barra sem
    mudar, o que e' sempre pelo menos tao seguro quanto chutar). Um token raw-string
    `R"delim(...)delim"` e' capturado com seu delimitador e conteudo sem-escape
    verbatim (raw string nao processa escape por definicao). Todo outro trecho de
    fonte (operador, identificador, espaco) vira um token OTHER, pra quem chama
    distinguir "dois literais separados so por espaco/quebra" (concatenacao
    adjacente de C++ de verdade, byte-a-byte identica a um literal so) de "dois
    literais separados por codigo real" (ex. `<< alguma_variavel <<`, um valor que a
    fonte computa em runtime -- nunca chutavel a partir de texto estatico).
    """
    tokens: list[CppToken] = []
    i, n = 0, len(code)
    other_buf: list[str] = []

    def flush_other():
        if other_buf:
            tokens.append(CppToken("other", "".join(other_buf)))
            other_buf.clear()

    raw_start_re = re.compile(r'R"([A-Za-z_]{0,16})\(')
    while i < n:
        m = raw_start_re.match(code, i)
        if m:
            delim = m.group(1)
            close = f'){delim}"'
            end = code.find(close, m.end())
            if end == -1:
                other_buf.append(code[i:])
                i = n
                break
            flush_other()
            tokens.append(CppToken("raw", code[m.end():end]))
            i = end + len(close)
            continue
        c = code[i]
        if c == '"':
            j = i + 1
            buf = []
            esc = {"n": "\n", "t": "\t", "\\": "\\", '"': '"'}
            while j < n and code[j] != '"':
                if code[j] == "\\" and j + 1 < n:
                    buf.append(esc.get(code[j + 1], code[j + 1]))
                    j += 2
                    continue
                buf.append(code[j])
                j += 1
            flush_other()
            tokens.append(CppToken("string", "".join(buf)))
            i = j + 1
            continue
        other_buf.append(c)
        i += 1
    flush_other()
    return tokens


@dataclass
class CppRcssRun:
    text: str          # merged, decoded content of the run
    is_raw: bool        # True if the run was a single raw-string literal
    gap_before: bool     # True if this run is preceded by real (non-whitespace) code
    gap_after: bool      # True if this run is followed by real (non-whitespace) code


def merge_adjacent_literal_runs(tokens: list[CppToken]) -> list[CppRcssRun]:
    """Group STRING/RAW tokens separated only by whitespace into single runs.

    EN: Two adjacent C++ string literals with nothing but whitespace between them
    ARE, by the language's own rule, one literal -- merging them loses no
    information and is not a guess. A `raw` token never merges with a neighbour (a
    raw string is already one complete literal on its own). Any OTHER token whose
    stripped content is non-empty breaks the run and is recorded as a `gap` on both
    sides of the break, so a caller knows this run may be a TRUNCATED fragment of a
    larger, runtime-interpolated value, not full text.
    PT: Dois literais de string C++ adjacentes com nada alem de espaco entre eles
    SAO, pela propria regra da linguagem, um literal so -- juntar os dois nao perde
    informacao nenhuma e nao e' chute. Um token `raw` nunca se junta a um vizinho
    (raw string ja e' um literal completo sozinho). Qualquer token OTHER cujo
    conteudo sem espaco nao seja vazio quebra o run e fica registrado como um `gap`
    dos dois lados da quebra, pra quem chama saber que este run pode ser um
    fragmento TRUNCADO de um valor maior, interpolado em runtime, nao o texto
    inteiro.
    """
    runs: list[CppRcssRun] = []
    cur: list[str] = []
    cur_gap_before = False
    pending_gap = False
    for tok in tokens:
        if tok.kind == "other":
            if tok.text.strip():
                if cur:
                    runs.append(CppRcssRun("".join(cur), False, cur_gap_before, True))
                    cur = []
                pending_gap = True
            continue
        if tok.kind == "raw":
            if cur:
                runs.append(CppRcssRun("".join(cur), False, cur_gap_before, pending_gap))
                cur = []
            runs.append(CppRcssRun(tok.text, True, pending_gap, False))
            pending_gap = False
            cur_gap_before = False
            continue
        # tok.kind == "string"
        if not cur:
            cur_gap_before = pending_gap
        cur.append(tok.text)
        pending_gap = False
    if cur:
        runs.append(CppRcssRun("".join(cur), False, cur_gap_before, False))
    return runs


@dataclass
class CppEmbedFile:
    path: str
    category: str  # "production" | "test-demo"
    extracted_blocks: int = 0
    extracted_declarations: int = 0
    truncated_runs: int = 0
    truncated_bytes: int = 0
    truncated_examples: list[str] = field(default_factory=list)
    notes: list[str] = field(default_factory=list)


def find_cpp_embedded_rcss_files(repo_root: Path) -> list[str]:
    """Discover every git-tracked `.cpp`/`.hpp` file (excluding the two forbidden
    implementation paths) that contains at least one self-contained, brace-balanced
    fragment matching a known RCSS property -- the automated re-derivation this
    section's module docstring promises, not a hand-typed list.
    """
    all_files = sorted(set(
        git_ls_files(repo_root, "*.cpp") + git_ls_files(repo_root, "*.hpp")
    ))
    candidates = [
        f for f in all_files
        if not f.startswith("glintfx/src/uix/style/")
        and not f.startswith("glintfx/src/rml/rcss_dump")
    ]
    found = []
    for rel in candidates:
        p = repo_root / rel
        try:
            text = p.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        code = strip_comments(text)
        runs = merge_adjacent_literal_runs(tokenize_cpp_strings(code))
        for run in runs:
            if run.text.count("{") >= 1 and _KNOWN_PROP_HINT.search(run.text):
                found.append(rel)
                break
    return found


def extract_cpp_embedded_rcss(census: "Census", repo_root: Path, files: list[str]) -> list[CppEmbedFile]:
    """Parse each candidate file's merged literal runs; fold CLOSED (non-truncated)
    blocks into `census` under a distinct `origin` tag; report truncated/gap runs
    honestly instead of guessing their content.
    """
    results: list[CppEmbedFile] = []
    for rel in files:
        p = repo_root / rel
        text = p.read_text(encoding="utf-8")
        code = strip_comments(text)
        runs = merge_adjacent_literal_runs(tokenize_cpp_strings(code))
        entry = CppEmbedFile(path=rel, category="test-demo" if is_cpp_test_or_demo(rel, repo_root) else "production")
        for run in runs:
            if not (run.text.count("{") >= 1 and _KNOWN_PROP_HINT.search(run.text)):
                continue
            blocks, leftover = parse_stylesheet(run.text)
            for b in blocks:
                entry.extracted_blocks += 1
                process_block(census, b, f"{rel}#cpp-literal", top_level=True)
                entry.extracted_declarations += len(split_declarations(b.body))
            if "\x00TRUNCATED\x00" in leftover or run.gap_before or run.gap_after:
                truncated_text = leftover.replace("\x00TRUNCATED\x00", "")
                if truncated_text.strip() or run.gap_before or run.gap_after:
                    entry.truncated_runs += 1
                    entry.truncated_bytes += len(truncated_text)
                    reason = "runtime-interpolated (`<<`) value adjacent to this fragment" \
                        if (run.gap_before or run.gap_after) else "brace never closed within this run"
                    if len(entry.truncated_examples) < 3:
                        entry.truncated_examples.append(f"{reason}: {truncated_text.strip()[:120]!r}")
        if entry.extracted_blocks == 0 and entry.truncated_runs == 0:
            entry.notes.append("candidate matched by hint but no run reached the parser (should not happen)")
        results.append(entry)
    return results


# ---------------------------------------------------------------------------
# 4. Declaration splitting (top-level ';', paren-aware)
#    Corte de declaracao (';' de topo, sensivel a parenteses)
# ---------------------------------------------------------------------------

def split_declarations(body: str) -> list[str]:
    parts = []
    depth = 0
    cur = []
    in_string: str | None = None
    for c in body:
        if in_string:
            cur.append(c)
            if c == in_string:
                in_string = None
            continue
        if c in ("'", '"'):
            in_string = c
            cur.append(c)
            continue
        if c == "(":
            depth += 1
            cur.append(c)
            continue
        if c == ")":
            depth -= 1
            cur.append(c)
            continue
        if c == ";" and depth == 0:
            parts.append("".join(cur))
            cur = []
            continue
        cur.append(c)
    tail = "".join(cur).strip()
    if tail:
        parts.append(tail)
    return [p.strip() for p in parts if p.strip()]


def split_declaration(decl: str) -> tuple[str, str] | None:
    idx = decl.find(":")
    if idx == -1:
        return None
    prop = decl[:idx].strip().lower()
    val = decl[idx + 1:].strip()
    return prop, val


# ---------------------------------------------------------------------------
# 5. Selector splitting (top-level ',', paren-aware) + form classification
#    Corte de seletor (',' de topo) + classificacao de forma
# ---------------------------------------------------------------------------

def split_top_level(s: str, sep: str) -> list[str]:
    parts = []
    depth = 0
    cur = []
    for c in s:
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        if c == sep and depth == 0:
            parts.append("".join(cur))
            cur = []
            continue
        cur.append(c)
    parts.append("".join(cur))
    return parts


PSEUDO_RE = re.compile(r":([a-zA-Z-]+)(\([^)]*\))?")
COMPOUND_TOKEN_RE = re.compile(
    r"(?P<class>\.[A-Za-z0-9_-]+)"
    r"|(?P<id>#[A-Za-z0-9_-]+)"
    r"|(?P<attr>\[[^\]]*\])"
    r"|(?P<universal>\*)"
    r"|(?P<pseudo>:[A-Za-z-]+(\([^)]*\))?)"
    r"|(?P<tag>[A-Za-z][A-Za-z0-9_-]*)"
)


def classify_selector_instance(sel: str) -> dict:
    """Classify one comma-separated selector (may itself have combinators).

    EN: This mirrors, as closely as an independent re-derivation can, the
    dimensions `docs/rmlx-subset.md` section 6.2's own table already
    reports (per-instance simple-selector components: .class/#id/tag/
    pseudo-class, plus combinator presence) -- but every count below is
    produced by this scanner, not copied from that table.
    """
    s = sel.strip()
    has_child = ">" in s
    has_sibling_adjacent = ("+" in s) or ("~" in s)
    # crude compound split on combinators for structural forms
    compounds = re.split(r"\s*>\s*|\s*\+\s*|\s*~\s*|\s+", s)
    compounds = [c for c in compounds if c]
    is_descendant = len(compounds) > 1 and not has_child and not has_sibling_adjacent
    pseudo_matches = PSEUDO_RE.findall(s)
    pseudo_names = [m[0].lower() for m in pseudo_matches]
    tokens = COMPOUND_TOKEN_RE.finditer(s)
    kinds = Counter()
    for m in tokens:
        for k, v in m.groupdict().items():
            if v:
                kinds[k] += 1
    return {
        "raw": s,
        "has_child": has_child,
        "has_sibling_adjacent": has_sibling_adjacent,
        "is_descendant": is_descendant,
        "n_compounds": len(compounds),
        "pseudo_classes": pseudo_names,
        "n_class": kinds.get("class", 0),
        "n_id": kinds.get("id", 0),
        "n_tag": kinds.get("tag", 0),
        "n_attr": kinds.get("attr", 0),
        "n_universal": kinds.get("universal", 0),
        "n_pseudo": kinds.get("pseudo", 0),
        "is_compound_no_combinator": (len(compounds) == 1 and (kinds.get("class", 0) + kinds.get("id", 0) + kinds.get("tag", 0)) >= 2),
    }


# ---------------------------------------------------------------------------
# 6. Value tokenization: numbers+units, hex colors, named colors, functions
#    Tokenizacao de valor: numero+unidade, cor hex, cor nomeada, funcao
# ---------------------------------------------------------------------------

NUMBER_UNIT_RE = re.compile(
    r"(?<![A-Za-z0-9_#])"
    r"(-?\d+(?:\.\d+)?)"
    r"(dp|px|em|rem|vw|vh|deg|rad|grad|turn|ms|s|%)?"
    r"(?![A-Za-z0-9_])"
)
HEX_COLOR_RE = re.compile(r"#([0-9A-Fa-f]{3,8})\b")
FUNC_CALL_RE = re.compile(r"([A-Za-z][A-Za-z0-9_-]*)\s*\(")

NAMED_COLOR_TABLE = {
    "transparent", "black", "white", "red", "green", "blue", "yellow",
    "cyan", "magenta", "gray", "grey", "silver", "maroon", "olive",
    "purple", "teal", "navy", "lime", "aqua", "fuchsia", "orange",
}

COLOR_FUNCS = {"rgb", "rgba", "hsl", "hsla", "lab", "lch", "oklab", "oklch"}


def tokenize_numbers(value_text: str) -> list[tuple[str, str]]:
    """Return [(number_str, unit_or_empty), ...] for every numeric literal.

    EN: Applied only to an already-isolated declaration VALUE (never to
    raw file text, never to decide rule/selector boundaries) -- this is
    the sub-tokenization step, not the structural parse, so a regex here
    does not repeat the line-oriented mistake the brief warns about.
    """
    out = []
    for m in NUMBER_UNIT_RE.finditer(value_text):
        out.append((m.group(1), m.group(2) or ""))
    return out


# ---------------------------------------------------------------------------
# 7. Driver: walk the corpus, accumulate everything
#    Driver: percorre o corpus, acumula tudo
# ---------------------------------------------------------------------------

@dataclass
class Census:
    files_rcss: list[str] = field(default_factory=list)
    files_rml: list[str] = field(default_factory=list)
    files_rml_with_style: list[str] = field(default_factory=list)
    files_unreadable: list[str] = field(default_factory=list)
    style_sources: int = 0  # number of <style>/.rcss text chunks parsed
    top_level_blocks: int = 0
    rule_blocks: int = 0
    at_font_face_blocks: int = 0
    at_keyframes_blocks: int = 0
    at_keyframes_children: int = 0
    at_other_blocks: list[str] = field(default_factory=list)
    leftover_nonblank: list[tuple[str, str]] = field(default_factory=list)
    declarations_total: int = 0
    declarations_rule: int = 0
    declarations_font_face: int = 0
    declarations_keyframe: int = 0
    property_counts: Counter = field(default_factory=Counter)
    property_files: dict = field(default_factory=lambda: defaultdict(set))
    selector_instances: int = 0
    rules_with_selectors: int = 0
    comma_list_rules: int = 0
    comma_list_examples: list[dict] = field(default_factory=list)
    selector_form_counts: Counter = field(default_factory=Counter)
    pseudo_class_counts: Counter = field(default_factory=Counter)
    pseudo_class_bare: Counter = field(default_factory=Counter)
    pseudo_class_composite: Counter = field(default_factory=Counter)
    unit_counts: Counter = field(default_factory=Counter)
    unit_examples: dict = field(default_factory=lambda: defaultdict(list))
    number_max_by_unit: dict = field(default_factory=dict)
    hex_len_counts: Counter = field(default_factory=Counter)
    hex_examples: dict = field(default_factory=lambda: defaultdict(list))
    named_color_counts: Counter = field(default_factory=Counter)
    color_func_counts: Counter = field(default_factory=Counter)
    decorator_func_counts: Counter = field(default_factory=Counter)
    percent_family_a_props: Counter = field(default_factory=Counter)
    percent_by_property: Counter = field(default_factory=Counter)
    box_shadow_layer_count: int = 0
    box_shadow_spread_omitted: int = 0
    box_shadow_spread_present: int = 0
    shorthand_value_count_hist: dict = field(default_factory=lambda: defaultdict(Counter))
    files_with_comma_list: set = field(default_factory=set)
    cpp_embedded_summary: list = field(default_factory=list)  # populated only by --cpp-embedded


FAMILY_A_PROPS = {
    "width", "height", "top", "right", "bottom", "left",
    "margin-top", "margin-right", "margin-bottom", "margin-left",
    "padding-top", "padding-right", "padding-bottom", "padding-left",
    "min-width", "min-height", "max-width", "max-height", "flex-basis",
    "margin", "padding",
}

COMPOSITE_FUNC_PROPS = {"decorator", "mask-image", "filter", "backdrop-filter"}

SHORTHAND_BOX_PROPS = {"margin", "padding", "border-radius", "border-color"}


def process_declaration(census: Census, prop: str, val: str, file_path: str, kind: str):
    census.property_counts[prop] += 1
    census.property_files[prop].add(file_path)

    if prop == "box-shadow":
        layers = split_top_level(val, ",")
        for layer in layers:
            census.box_shadow_layer_count += 1
            # 6 CSS-order fields is offset/offset/blur/spread/color/inset,
            # but this corpus's own source order is whatever the author
            # wrote; we only ask "how many length tokens" to see if spread
            # was omitted (3 lengths present => spread omitted, in the
            # common case of no split across color/inset differently).
            nums = tokenize_numbers(layer)
            length_like = [u for (_, u) in nums if u in ("px", "dp", "")]
            # length-typed tokens per PropertyParserBoxShadow: only real
            # unit-bearing lengths count (bare numbers are not valid here)
            length_like = [u for (_, u) in nums if u in ("px", "dp")]
            if len(length_like) >= 4:
                census.box_shadow_spread_present += 1
            elif len(length_like) == 3:
                census.box_shadow_spread_omitted += 1

    if prop in COMPOSITE_FUNC_PROPS or prop == "transform" or prop == "animation":
        for fm in FUNC_CALL_RE.finditer(val):
            fname = fm.group(1).lower()
            census.decorator_func_counts[fname] += 1

    # percent family (a) tracking: only for simple box-relative longhand
    # properties (not inside a composite/gradient value, which is handled
    # by the decorator_func_counts / gradient-stop pass instead).
    if prop in FAMILY_A_PROPS:
        nums = tokenize_numbers(val)
        for (_, u) in nums:
            if u == "%":
                census.percent_family_a_props[prop] += 1

    nums = tokenize_numbers(val)
    for (numstr, unit) in nums:
        key = unit if unit else "(unitless)"
        if val.strip() == numstr and unit == "":
            pass
        census.unit_counts[key] += 1
        if len(census.unit_examples[key]) < 3:
            census.unit_examples[key].append(f"{file_path}: {prop}: {val.strip()}")
        try:
            fval = abs(float(numstr))
            prev = census.number_max_by_unit.get(key)
            if prev is None or fval > prev[0]:
                census.number_max_by_unit[key] = (fval, f"{file_path}: {prop}: {val.strip()}")
        except ValueError:
            pass

    for hx in HEX_COLOR_RE.finditer(val):
        digits = hx.group(1)
        census.hex_len_counts[len(digits)] += 1
        if len(census.hex_examples[len(digits)]) < 3:
            census.hex_examples[len(digits)].append(f"{file_path}: {prop}: #{digits}")

    for fm in FUNC_CALL_RE.finditer(val):
        fname = fm.group(1).lower()
        if fname in COLOR_FUNCS:
            census.color_func_counts[fname] += 1

    for tok in re.findall(r"[A-Za-z][A-Za-z0-9_-]*", val):
        low = tok.lower()
        if low in NAMED_COLOR_TABLE:
            census.named_color_counts[low] += 1

    if prop in SHORTHAND_BOX_PROPS:
        toks = split_top_level(val, ",")
        joined = toks[0] if toks else val
        parts = joined.split()
        census.shorthand_value_count_hist[prop][len(parts)] += 1
    if prop == "border":
        parts = val.split()
        census.shorthand_value_count_hist["border"][len(parts)] += 1
    if prop in ("border-top", "border-right", "border-bottom", "border-left"):
        parts = val.split()
        census.shorthand_value_count_hist["border-side"][len(parts)] += 1
    if prop == "background":
        parts = val.split()
        census.shorthand_value_count_hist["background"][len(parts)] += 1


def process_block(census: Census, block: Block, file_path: str, top_level: bool):
    census.top_level_blocks += 1
    if block.kind == "at-font-face":
        census.at_font_face_blocks += 1
        for decl_text in split_declarations(block.body):
            sd = split_declaration(decl_text)
            if not sd:
                continue
            prop, val = sd
            census.declarations_total += 1
            census.declarations_font_face += 1
            process_declaration(census, prop, val, file_path, "font-face")
        return
    if block.kind == "at-keyframes":
        census.at_keyframes_blocks += 1
        for child in block.children:
            census.at_keyframes_children += 1
            for decl_text in split_declarations(child.body):
                sd = split_declaration(decl_text)
                if not sd:
                    continue
                prop, val = sd
                census.declarations_total += 1
                census.declarations_keyframe += 1
                process_declaration(census, prop, val, file_path, "keyframe")
        return
    if block.kind == "at-other":
        census.at_other_blocks.append(f"{file_path}: {block.prelude[:80]!r}")
        return

    # normal rule block
    census.rule_blocks += 1
    census.rules_with_selectors += 1
    selectors = [s.strip() for s in split_top_level(block.prelude, ",") if s.strip()]
    if len(selectors) > 1:
        census.comma_list_rules += 1
        census.files_with_comma_list.add(file_path)
        if len(census.comma_list_examples) < 20:
            census.comma_list_examples.append({
                "file": file_path,
                "n_parts": len(selectors),
                "selector": " , ".join(selectors)[:200],
            })
    for sel in selectors:
        census.selector_instances += 1
        info = classify_selector_instance(sel)
        for pc in info["pseudo_classes"]:
            census.pseudo_class_counts[pc] += 1
            # "bare" = the whole compound is only the pseudo-class (":hover")
            # "composite" = attached to a class/id/tag (".foo:hover")
            stripped = sel.strip()
            if re.fullmatch(r":" + re.escape(pc) + r"(\([^)]*\))?", stripped):
                census.pseudo_class_bare[pc] += 1
            else:
                census.pseudo_class_composite[pc] += 1
        if info["n_universal"]:
            census.selector_form_counts["universal"] += info["n_universal"]
        if info["n_attr"]:
            census.selector_form_counts["attribute"] += info["n_attr"]
        if info["has_sibling_adjacent"]:
            census.selector_form_counts["adjacent_or_sibling"] += 1
        if info["has_child"]:
            census.selector_form_counts["child_combinator"] += 1
        if info["is_descendant"]:
            census.selector_form_counts["descendant_combinator"] += 1
        if info["is_compound_no_combinator"]:
            census.selector_form_counts["compound_no_combinator"] += 1
        census.selector_form_counts["class_token"] += info["n_class"]
        census.selector_form_counts["id_token"] += info["n_id"]
        census.selector_form_counts["tag_token"] += info["n_tag"]
        census.selector_form_counts["pseudo_token"] += info["n_pseudo"]

    for decl_text in split_declarations(block.body):
        sd = split_declaration(decl_text)
        if not sd:
            continue
        prop, val = sd
        census.declarations_total += 1
        census.declarations_rule += 1
        process_declaration(census, prop, val, file_path, "rule")


def run(repo_root: Path, extra_hpp: list[str]) -> Census:
    census = Census()
    rcss_files = sorted(git_ls_files(repo_root, "*.rcss"))
    rml_files = sorted(git_ls_files(repo_root, "*.rml"))
    census.files_rcss = rcss_files
    census.files_rml = rml_files

    for rel in rcss_files:
        p = repo_root / rel
        try:
            text = p.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            census.files_unreadable.append(rel)
            continue
        census.style_sources += 1
        blocks, leftover = parse_stylesheet(text)
        if leftover.strip():
            census.leftover_nonblank.append((rel, leftover.strip()[:120]))
        for b in blocks:
            process_block(census, b, rel, top_level=True)

    for rel in rml_files:
        p = repo_root / rel
        try:
            text = p.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            census.files_unreadable.append(rel)
            continue
        style_blocks = extract_style_blocks_from_rml(text)
        if style_blocks:
            census.files_rml_with_style.append(rel)
        for (inner, _line) in style_blocks:
            census.style_sources += 1
            blocks, leftover = parse_stylesheet(inner)
            if leftover.strip():
                census.leftover_nonblank.append((rel, leftover.strip()[:120]))
            for b in blocks:
                process_block(census, b, rel, top_level=True)

    for rel in extra_hpp:
        p = repo_root / rel
        if not p.exists():
            continue
        text = p.read_text(encoding="utf-8")
        for m in re.finditer(r'R"rcss\((.*?)\)rcss"', text, re.DOTALL):
            inner = m.group(1)
            census.style_sources += 1
            blocks, leftover = parse_stylesheet(inner)
            if leftover.strip():
                census.leftover_nonblank.append((rel, leftover.strip()[:120]))
            for b in blocks:
                process_block(census, b, rel, top_level=True)

    return census


def fold_in_cpp_embedded(census: Census, repo_root: Path) -> None:
    """Discover + extract the third origin (C++ literals) and fold CLOSED blocks
    into `census` in place; always populates `census.cpp_embedded_summary` with a
    per-file report (category, extracted counts, truncated/gap evidence) regardless
    of how many blocks actually made it into the statistics -- a file that
    contributes 0 closed blocks still gets a row, with its truncated bytes/reason
    shown, per this task's own "declare not-extracted with reason" instruction.
    """
    files = find_cpp_embedded_rcss_files(repo_root)
    results = extract_cpp_embedded_rcss(census, repo_root, files)
    census.cpp_embedded_summary = [
        {
            "path": r.path,
            "category": r.category,
            "extracted_blocks": r.extracted_blocks,
            "extracted_declarations": r.extracted_declarations,
            "truncated_runs": r.truncated_runs,
            "truncated_bytes": r.truncated_bytes,
            "truncated_examples": r.truncated_examples,
            "notes": r.notes,
        }
        for r in results
    ]


def to_jsonable(census: Census) -> dict:
    d = {}
    for k, v in census.__dict__.items():
        if isinstance(v, Counter):
            d[k] = dict(sorted(v.items(), key=lambda kv: (-kv[1], str(kv[0]))))
        elif isinstance(v, defaultdict):
            d[k] = {kk: (sorted(vv) if isinstance(vv, set) else vv) for kk, vv in v.items()}
        elif isinstance(v, set):
            d[k] = sorted(v)
        elif isinstance(v, dict):
            out = {}
            for kk, vv in v.items():
                if isinstance(vv, Counter):
                    out[kk] = dict(sorted(vv.items(), key=lambda kv: (-kv[1], str(kv[0]))))
                elif isinstance(vv, set):
                    out[kk] = sorted(vv)
                else:
                    out[kk] = vv
            d[k] = out
        else:
            d[k] = v
    return d


CANONICAL_PRODUCTION_HPP = "glintfx/src/ua_stylesheet.hpp"


def main():
    ap = argparse.ArgumentParser(
        description="RCSS corpus census -- see docs/uix-rcss-censo.md. "
                     "Every table in that document declares its own exact command; "
                     "--scope is the one to copy-paste, the others are lower-level building blocks.")
    ap.add_argument("--repo-root", default=".", type=Path)
    ap.add_argument(
        "--scope", choices=["rml-rcss", "production", "full"], default="rml-rcss",
        help="THE FLAG TO USE. 'rml-rcss' (default): only git-tracked .rcss + <style> in .rml "
             "(85 files at HEAD when this was written) -- markup-and-stylesheet corpus only. "
             "'production': + glintfx/src/ua_stylesheet.hpp (86 files) -- THE CANONICAL SCOPE this "
             "document's headline numbers cite; matches docs/rmlx-subset.md's own published comma-list "
             "figures (15 rules, 13 files) exactly, because that is the file the líder's own comma-list "
             "authorization decision was evidenced by. 'full': + all 15 test/demo files with embedded "
             "RCSS too (101 files) -- informational only, may include verbatim echoes of corpus text "
             "already counted once under 'production' (docs/uix-rcss-censo.md section 5.4) -- never cite "
             "'full' numbers as the corpus census without saying so.")
    ap.add_argument("--include-hpp", nargs="*", default=[],
                     help="advanced/lower-level: fold arbitrary extra .hpp/.cpp raw-string paths in by hand "
                          "(what --scope production does automatically for the one canonical path)")
    ap.add_argument("--cpp-embedded", action="store_true",
                     help="advanced/lower-level: equivalent to --scope full, kept for scripts already using this flag")
    ap.add_argument("--list-cpp-embedded", action="store_true",
                     help="only print the third-origin discovery table (path, category, extracted/truncated "
                          "counts) and exit -- does not fold into the census or print the full JSON")
    ap.add_argument("--out", default=None, help="write JSON to this path instead of stdout")
    args = ap.parse_args()

    repo_root = args.repo_root.resolve()

    if args.list_cpp_embedded:
        census = Census()
        fold_in_cpp_embedded(census, repo_root)
        for row in census.cpp_embedded_summary:
            print(f"{row['category']:12s} {row['path']:60s} "
                  f"blocks={row['extracted_blocks']:2d} decls={row['extracted_declarations']:3d} "
                  f"truncated_runs={row['truncated_runs']} truncated_bytes={row['truncated_bytes']}")
            for ex in row["truncated_examples"]:
                print(f"             not-extracted: {ex}")
        return

    # --scope is sugar over the lower-level flags; an explicit --include-hpp/--cpp-embedded
    # still works standalone (advanced use), and --scope wins if both are given at once.
    include_hpp = list(args.include_hpp)
    want_full = args.cpp_embedded
    if args.scope == "production":
        include_hpp = [CANONICAL_PRODUCTION_HPP]
        want_full = False
    elif args.scope == "full":
        want_full = True
        include_hpp = []
    elif args.scope == "rml-rcss" and (args.include_hpp or args.cpp_embedded):
        pass  # explicit lower-level flags override the rml-rcss default, as documented above

    census = run(repo_root, include_hpp)
    if want_full:
        fold_in_cpp_embedded(census, repo_root)
    payload = to_jsonable(census)
    text = json.dumps(payload, indent=2, ensure_ascii=False, sort_keys=False)
    if args.out:
        Path(args.out).write_text(text, encoding="utf-8")
    else:
        print(text)


if __name__ == "__main__":
    main()
