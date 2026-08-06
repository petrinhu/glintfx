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
        body = text[brace + 1:j - 1]
        leftover_parts.append(prelude if prelude.strip() == "" else "")
        if prelude.strip() != "":
            pass  # handled by caller via classify_prelude / leftover check below
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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo-root", default=".", type=Path)
    ap.add_argument("--include-hpp", nargs="*", default=[],
                     help="extra .hpp/.cpp paths with R\"rcss(...)rcss\" raw strings to fold in, labeled separately")
    ap.add_argument("--out", default=None, help="write JSON to this path instead of stdout")
    args = ap.parse_args()

    repo_root = args.repo_root.resolve()
    census = run(repo_root, args.include_hpp)
    payload = to_jsonable(census)
    text = json.dumps(payload, indent=2, ensure_ascii=False, sort_keys=False)
    if args.out:
        Path(args.out).write_text(text, encoding="utf-8")
    else:
        print(text)


if __name__ == "__main__":
    main()
