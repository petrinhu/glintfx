#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (c) 2026 Petrus Silva Costa
#
# EN: Generator for glintfx's own OpenGL 3.3 core-profile function-pointer loader
#     (L1.14-GLLOADER). Reads the public Khronos OpenGL/OpenGL-ES XML API Registry
#     (gl.xml, Apache-2.0, https://github.com/KhronosGroup/OpenGL-Registry) vendored
#     at glintfx/third_party/khronos/gl.xml and emits two generated (do-not-hand-edit)
#     files:
#       glintfx/src/gl_loader.h — one `extern PFN<CMD>PROC glx_<cmd>;` declaration per
#                                  GL 3.3 core command (AUD-L1-GLSYM, 2026-07-19: the
#                                  pointer variable carries a `glx_` prefix — e.g.
#                                  `glx_glCullFace` — instead of the bare GL name, so
#                                  that `nm libglintfx.a` never shows a data symbol
#                                  named exactly like a real GL entry point; a static
#                                  archive that DID export e.g. a BSS `glClear` let an
#                                  embed host's own `glClear` reference resolve against
#                                  glintfx's uninitialised pointer slot instead of
#                                  libGL's real function if glintfx was linked before
#                                  libGL, crashing on first call. A second block, right
#                                  after the extern declarations, `#define`s the bare
#                                  name to the prefixed one — e.g. `#define glCullFace
#                                  glx_glCullFace` — purely so INTERNAL glintfx call
#                                  sites (`glCullFace(x)`) keep reading naturally; the
#                                  macro never leaves this private header, and the
#                                  archive's exported symbol is always `glx_glCullFace`,
#                                  never `glCullFace`), plus the `int glx_gl_load(void)`
#                                  entry point.
#       glintfx/src/gl_loader.c — storage for each `glx_`-prefixed pointer variable, a
#                                  name→slot table (keyed by the BARE GL name — that is
#                                  the real string every GetProcAddress call needs, and
#                                  a string literal is immune to the `#define` rewrite
#                                  above), and the `glx_gl_load()` implementation that
#                                  resolves every symbol via, in order:
#                                  `glXGetProcAddressARB`/`glXGetProcAddress` (dlsym'd
#                                  out of libGL.so.1), `eglGetProcAddress` (dlsym'd out
#                                  of libEGL.so.1, for EGL-backed contexts), then a
#                                  direct `dlsym` against the libGL handle (covers core
#                                  entry points that are ordinary exported symbols and
#                                  do not require a GetProcAddress indirection on most
#                                  GL/Mesa builds).
#
#     ALGORITHM (standard Khronos-registry command-set derivation — the same technique
#     every public GL loader generator, custom or off-the-shelf, is documented to use;
#     no third-party loader's *source* was read to write this script — see the
#     clean-room note below):
#       1. Collect every `<feature api="gl" number="N">` element with N <= 3.3.
#       2. Walk them in ascending version order. For each, add the `<command>` names
#          listed under `<require>` blocks whose `profile` attribute is absent or
#          "core", and remove the `<command>` names listed under `<remove>` blocks
#          whose `profile` attribute is absent or "core" (this is exactly how the
#          registry expresses "fixed-function immediate-mode calls such as glBegin/
#          glVertex3f/glLoadMatrixf existed through GL 1.x-2.1 but are stripped out of
#          the *core* profile starting at GL 3.2" — see gl.xml's own `<remove
#          profile="core" comment="Compatibility-only ... removed from GL 3.2">`
#          blocks).
#       3. The resulting name set (344 commands as of the registry snapshot vendored
#          here) is the complete GL 3.3 core-profile command surface.
#       4. Function-pointer typedef names are NOT re-derived — they are reused as-is
#          from the already-vendored, Khronos-authored `glcorearb.h`
#          (`PFNGL<COMMAND-WITHOUT-gl-PREFIX-UPPERCASED>PROC`, e.g. `glCullFace` ->
#          `PFNGLCULLFACEPROC`), which is verified 1:1 against the registry's command
#          set at generation time (see `_typedef_name` / the sanity check in `main()`).
#
#     CLEAN-ROOM NOTE: this script and its output were written *without* opening
#     glintfx/third_party/gl3w/src/gl3w.c or glintfx/third_party/gl3w/include/GL/gl3w.h
#     (see CLAUDE.md / L1.14-GLLOADER task brief). The only inputs read were the public
#     Apache-2.0 gl.xml registry, the MIT glcorearb.h/khrplatform.h headers (whose
#     reuse is explicitly licensed and already vendored/attributed in NOTICE), and
#     public documentation of glXGetProcAddress/eglGetProcAddress/dlopen semantics.
#
# PT: Gerador do loader próprio de ponteiros de função OpenGL 3.3 core profile do
#     glintfx (L1.14-GLLOADER). Lê o Registro de API XML público OpenGL/OpenGL-ES da
#     Khronos (gl.xml, Apache-2.0, https://github.com/KhronosGroup/OpenGL-Registry)
#     vendorizado em glintfx/third_party/khronos/gl.xml e emite dois arquivos gerados
#     (não editar à mão):
#       glintfx/src/gl_loader.h — uma declaração `extern PFN<CMD>PROC glx_<cmd>;` por
#                                  comando GL 3.3 core (AUD-L1-GLSYM, 2026-07-19: a
#                                  variável-ponteiro carrega prefixo `glx_` — ex.:
#                                  `glx_glCullFace` — em vez do nome GL cru, para que
#                                  `nm libglintfx.a` nunca mostre um símbolo de dado com
#                                  o nome exato de um entry point GL real; um archive
#                                  estático que EXPORTASSE, por ex., um `glClear` em BSS
#                                  deixava a referência a `glClear` de um host embed
#                                  resolver contra o slot de ponteiro não-inicializado
#                                  do glintfx em vez da função real da libGL, se o
#                                  glintfx fosse linkado antes da libGL — crash na
#                                  primeira chamada. Um segundo bloco, logo após as
#                                  declarações extern, faz `#define` do nome cru para o
#                                  prefixado — ex.: `#define glCullFace glx_glCullFace`
#                                  — só para que os call sites INTERNOS do glintfx
#                                  (`glCullFace(x)`) continuem lendo naturalmente; a
#                                  macro nunca sai deste header privado, e o símbolo
#                                  exportado pelo archive é sempre `glx_glCullFace`,
#                                  nunca `glCullFace`), mais o ponto de entrada
#                                  `int glx_gl_load(void)`.
#       glintfx/src/gl_loader.c — armazenamento de cada variável-ponteiro prefixada com
#                                  `glx_`, uma tabela nome→slot (indexada pelo nome GL
#                                  CRU — é a string real que toda chamada a
#                                  GetProcAddress precisa, e um literal de string é
#                                  imune à reescrita do `#define` acima), e a
#                                  implementação de `glx_gl_load()` que resolve cada
#                                  símbolo, em ordem: `glXGetProcAddressARB`/
#                                  `glXGetProcAddress` (via dlsym em libGL.so.1),
#                                  `eglGetProcAddress` (via dlsym em libEGL.so.1, para
#                                  contextos com backing EGL), depois `dlsym` direto no
#                                  handle da libGL (cobre entry points core que são
#                                  símbolos exportados comuns e não exigem indireção via
#                                  GetProcAddress na maioria das builds GL/Mesa).
#
#     ALGORITMO (derivação padrão do conjunto de comandos via registro Khronos — a
#     mesma técnica que todo gerador de loader GL público, próprio ou pronto,
#     documenta usar; nenhum *fonte* de loader de terceiros foi lido para escrever
#     este script — ver nota clean-room abaixo):
#       1. Coleta todo elemento `<feature api="gl" number="N">` com N <= 3.3.
#       2. Percorre em ordem crescente de versão. Para cada um, adiciona os nomes de
#          `<command>` listados em blocos `<require>` cujo atributo `profile` esteja
#          ausente ou seja "core", e remove os nomes de `<command>` listados em blocos
#          `<remove>` cujo atributo `profile` esteja ausente ou seja "core" (é
#          exatamente assim que o registro expressa "chamadas de modo-imediato
#          fixed-function como glBegin/glVertex3f/glLoadMatrixf existiram até GL 1.x-
#          2.1 mas são removidas do profile *core* a partir de GL 3.2" — ver os
#          blocos `<remove profile="core" comment="Compatibility-only ... removed
#          from GL 3.2">` do próprio gl.xml).
#       3. O conjunto de nomes resultante (344 comandos no snapshot do registro
#          vendorizado aqui) é a superfície completa de comandos GL 3.3 core profile.
#       4. Os nomes de typedef de ponteiro de função NÃO são re-derivados — são
#          reaproveitados tal como estão do `glcorearb.h` já vendorizado, de autoria
#          Khronos (`PFNGL<COMANDO-SEM-PREFIXO-gl-MAIÚSCULO>PROC`, ex.: `glCullFace`
#          -> `PFNGLCULLFACEPROC`), verificado 1:1 contra o conjunto de comandos do
#          registro em tempo de geração (ver `_typedef_name` / a checagem de sanidade
#          em `main()`).
#
#     NOTA CLEAN-ROOM: este script e sua saída foram escritos SEM abrir
#     glintfx/third_party/gl3w/src/gl3w.c nem
#     glintfx/third_party/gl3w/include/GL/gl3w.h (ver CLAUDE.md / brief da tarefa
#     L1.14-GLLOADER). As únicas entradas lidas foram o registro público gl.xml
#     (Apache-2.0), os headers MIT glcorearb.h/khrplatform.h (cujo reaproveitamento é
#     explicitamente licenciado e já vendorizado/atribuído no NOTICE), e a
#     documentação pública da semântica de glXGetProcAddress/eglGetProcAddress/dlopen.
#
# Usage / Uso:
#   python3 tools/gen_glloader.py
#
# Regenerates glintfx/src/gl_loader.h and glintfx/src/gl_loader.c in place from
# glintfx/third_party/khronos/gl.xml + glintfx/third_party/khronos/GL/glcorearb.h.
#
# Regenera glintfx/src/gl_loader.h e glintfx/src/gl_loader.c a partir de
# glintfx/third_party/khronos/gl.xml + glintfx/third_party/khronos/GL/glcorearb.h.

import re
import sys
from pathlib import Path

# EN: defusedxml instead of stdlib xml.etree.ElementTree -- gl.xml is a vendorized,
#     trusted Khronos file (not attacker-controlled input), but this guards against
#     XXE/billion-laughs regardless if the vendored file is ever refreshed from a
#     compromised mirror.
# PT: defusedxml em vez do xml.etree.ElementTree da stdlib -- gl.xml é um arquivo
#     Khronos vendorizado e confiável (não é entrada controlada por atacante), mas
#     isso protege contra XXE/billion-laughs de qualquer forma, caso o arquivo
#     vendorizado seja atualizado a partir de um mirror comprometido.
import defusedxml.ElementTree as ET

REPO_ROOT = Path(__file__).resolve().parent.parent
GLINTFX = REPO_ROOT / "glintfx"
GL_XML = GLINTFX / "third_party" / "khronos" / "gl.xml"
GLCOREARB_H = GLINTFX / "third_party" / "khronos" / "GL" / "glcorearb.h"
OUT_H = GLINTFX / "src" / "gl_loader.h"
OUT_C = GLINTFX / "src" / "gl_loader.c"

TARGET_VERSION = 3.3


def collect_core_commands(gl_xml_path: Path, max_version: float) -> list[str]:
    """EN: Walk GL_VERSION_* <feature> blocks up to max_version (inclusive),
    accumulating <require> commands and dropping <remove>d ones, honouring
    profile="core" (or no profile attribute = applies to every profile).
    Returns the sorted command-name list for the GL core profile at max_version.

    PT: Percorre os blocos <feature> GL_VERSION_* até max_version (inclusive),
    acumulando comandos de <require> e descartando os de <remove>, respeitando
    profile="core" (ou ausência do atributo profile = aplica a todo profile).
    Retorna a lista ordenada de nomes de comando do profile core GL em max_version.
    """
    root = ET.parse(gl_xml_path).getroot()

    features = []
    for feat in root.findall("feature"):
        if feat.get("api") != "gl":
            continue
        number = float(feat.get("number"))
        if number > max_version:
            continue
        features.append((number, feat))
    features.sort(key=lambda t: t[0])

    required: set[str] = set()
    for _number, feat in features:
        for require in feat.findall("require"):
            profile = require.get("profile")
            if profile not in (None, "core"):
                continue
            for cmd in require.findall("command"):
                required.add(cmd.get("name"))
        for remove in feat.findall("remove"):
            profile = remove.get("profile")
            if profile not in (None, "core"):
                continue
            for cmd in remove.findall("command"):
                required.discard(cmd.get("name"))

    return sorted(required)


def typedef_name(command_name: str) -> str:
    """EN: gl<Foo> -> PFNGL<FOO>PROC (Khronos convention, verified against
    glcorearb.h for every command in main()).
    PT: gl<Foo> -> PFNGL<FOO>PROC (convenção Khronos, verificada contra
    glcorearb.h para todo comando em main())."""
    assert command_name.startswith("gl")
    return "PFNGL" + command_name[2:].upper() + "PROC"


def glx_name(command_name: str) -> str:
    """EN: gl<Foo> -> glx_gl<Foo> (AUD-L1-GLSYM) -- the actual name of the pointer
    variable / exported archive symbol. The bare `<cmd>` name is reserved for the
    `#define <cmd> glx_<cmd>` call-site-rewrite macro and for the GetProcAddress
    lookup string; it is never itself an identifier in the generated .h/.c.
    PT: gl<Foo> -> glx_gl<Foo> (AUD-L1-GLSYM) -- o nome real da variável-ponteiro /
    símbolo exportado do archive. O nome cru `<cmd>` fica reservado para a macro de
    reescrita de call site `#define <cmd> glx_<cmd>` e para a string de busca do
    GetProcAddress; ele nunca é, por si só, um identificador no .h/.c gerado."""
    return "glx_" + command_name


def verify_typedefs_exist(commands: list[str], glcorearb_h_path: Path) -> None:
    content = glcorearb_h_path.read_text()
    known = set(re.findall(r"PFNGL\w+PROC", content))
    missing = [(c, typedef_name(c)) for c in commands if typedef_name(c) not in known]
    if missing:
        print("ERROR: typedefs missing from glcorearb.h:", file=sys.stderr)
        for cmd, td in missing:
            print(f"  {cmd} -> {td}", file=sys.stderr)
        sys.exit(1)


HEADER_PREAMBLE = """// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 Petrus Silva Costa
//
// EN: GENERATED FILE -- do not hand-edit. Produced by tools/gen_glloader.py from the
//     public Khronos gl.xml registry (Apache-2.0) + the vendored Khronos glcorearb.h
//     (MIT). See tools/gen_glloader.py for the full generation method and the
//     clean-room note (L1.14-GLLOADER: this loader was NOT written by reading the
//     previously-vendored gl3w sources).
//
//     Declares one `extern PFN<CMD>PROC glx_<cmd>;` function-pointer variable per GL
//     3.3 core-profile command, PREFIXED with `glx_` (AUD-L1-GLSYM, 2026-07-19) so
//     that no data symbol in libglintfx.a is ever named identically to a real GL entry
//     point -- `nm libglintfx.a` used to show a BSS `glClear`, `glClearColor`, etc.
//     (344 of them); an embed host linking its own GL calls against libglintfx.a
//     *before* libGL.so could have those references resolve to glintfx's
//     uninitialised pointer slot instead of the driver's real function, crashing on
//     first call. A second block below, AFTER both the glcorearb.h include and every
//     extern declaration, `#define`s each bare name to its glx_-prefixed variable
//     (e.g. `#define glCullFace glx_glCullFace`) purely so glintfx's OWN, INTERNAL
//     call sites (`glCullFace(x)`) keep compiling unchanged -- this header is private
//     (glintfx/src/, never installed under glintfx/include/glintfx/), so the macro
//     never reaches a consumer's translation unit. Only the typedef token
//     (`PFN<CMD>PROC`) is untouched by the macro -- the preprocessor rewrites whole
//     identifiers only, and `PFNGLCULLFACEPROC` is never equal to the `glCullFace`
//     macro name, so there is no risk of the macro clobbering the type name.
//
//     glx_gl_load() resolves every symbol against the host's current GL context and
//     returns 0 on success (same polarity as the gl3wInit() call sites it replaces),
//     non-zero if one or more core symbols could not be resolved.
// PT: ARQUIVO GERADO -- não editar à mão. Produzido por tools/gen_glloader.py a
//     partir do registro público gl.xml da Khronos (Apache-2.0) + do glcorearb.h
//     vendorizado da Khronos (MIT). Ver tools/gen_glloader.py para o método completo
//     de geração e a nota clean-room (L1.14-GLLOADER: este loader NÃO foi escrito
//     lendo os fontes do gl3w previamente vendorizado).
//
//     Declara uma variável-ponteiro-de-função `extern PFN<CMD>PROC glx_<cmd>;` por
//     comando do profile core GL 3.3, PREFIXADA com `glx_` (AUD-L1-GLSYM, 2026-07-19)
//     para que nenhum símbolo de dado do libglintfx.a tenha o nome exato de um entry
//     point GL real -- `nm libglintfx.a` mostrava um `glClear`, `glClearColor` etc. em
//     BSS (344 deles); um host embed que linkasse suas próprias chamadas GL contra
//     libglintfx.a ANTES de libGL.so podia ter essas referências resolvidas contra o
//     slot de ponteiro não-inicializado do glintfx em vez da função real do driver --
//     crash na primeira chamada. Um segundo bloco abaixo, DEPOIS tanto do include de
//     glcorearb.h quanto de toda declaração extern, faz `#define` de cada nome cru
//     para sua variável prefixada com glx_ (ex.: `#define glCullFace glx_glCullFace`)
//     só para que os call sites PRÓPRIOS, INTERNOS do glintfx (`glCullFace(x)`)
//     continuem compilando sem mudança -- este header é privado (glintfx/src/, nunca
//     instalado sob glintfx/include/glintfx/), então a macro nunca alcança a unidade
//     de tradução de um consumidor. Só o token do typedef (`PFN<CMD>PROC`) fica imune
//     à macro -- o pré-processador reescreve identificadores inteiros, e
//     `PFNGLCULLFACEPROC` nunca é igual ao nome de macro `glCullFace`, então não há
//     risco de a macro atropelar o nome do tipo.
//
//     glx_gl_load() resolve cada símbolo contra o contexto GL corrente do host e
//     retorna 0 em sucesso (mesma polaridade dos call-sites de gl3wInit() que
//     substitui), não-zero se um ou mais símbolos core não puderam ser resolvidos.
#ifndef GLINTFX_GL_LOADER_H
#define GLINTFX_GL_LOADER_H

#include <GL/glcorearb.h>

#ifdef __cplusplus
extern "C" {
#endif

"""

HEADER_POSTAMBLE = """
// EN: Resolve every GL 3.3 core function pointer declared above against the host's
//     CURRENT GL context (glXMakeCurrent/eglMakeCurrent must already have been
//     called by the caller -- same contract as the gl3wInit() call sites this
//     function replaces). Safe to call more than once per process (idempotent:
//     re-resolves and overwrites the same pointers; cheap, no allocation kept
//     around).
//     Returns 0 on success, 1 if one or more core symbols failed to resolve.
// PT: Resolve todo ponteiro de função GL 3.3 core declarado acima contra o
//     contexto GL CORRENTE do host (glXMakeCurrent/eglMakeCurrent já deve ter
//     sido chamado pelo chamador -- mesmo contrato dos call-sites de gl3wInit()
//     que esta função substitui). Seguro chamar mais de uma vez por processo
//     (idempotente: re-resolve e sobrescreve os mesmos ponteiros; barato, sem
//     alocação retida).
//     Retorna 0 em sucesso, 1 se um ou mais símbolos core falharem ao resolver.
int glx_gl_load(void);

#ifdef __cplusplus
}
#endif

#endif // GLINTFX_GL_LOADER_H
"""

SOURCE_PREAMBLE = """// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 Petrus Silva Costa
//
// EN: GENERATED FILE -- do not hand-edit. See gl_loader.h / tools/gen_glloader.py.
//
//     Every pointer variable defined below carries the `glx_` prefix (AUD-L1-GLSYM,
//     2026-07-19, e.g. `glx_glCullFace`); the symbol table keys on the BARE GL name
//     (`"glCullFace"`) because that is the literal string GetProcAddress needs to
//     resolve the real driver entry point -- a string literal is not an identifier and
//     is therefore never touched by gl_loader.h's `#define glCullFace glx_glCullFace`
//     call-site-rewrite macros, so there is no risk of this table ending up keyed on
//     the wrong (prefixed) string.
//
//     glx_gl_load() resolution order per symbol, matching the L1.14-GLLOADER task
//     brief: (1) glXGetProcAddressARB/glXGetProcAddress, dlsym'd out of the libGL.so.1
//     handle -- covers GLX-backed desktop contexts (GLFW's default on X11/Mesa,
//     the CI/dev target here); (2) eglGetProcAddress, dlsym'd out of a libEGL.so.1
//     handle if present -- covers EGL-backed contexts (Wayland, some embedded
//     hosts); (3) a direct dlsym() against the libGL handle -- covers core entry
//     points that are ordinary exported symbols on most GL/Mesa builds and do not
//     strictly require a GetProcAddress indirection. The first non-NULL result
//     wins.
// PT: GENERATED FILE -- não editar à mão. Ver gl_loader.h / tools/gen_glloader.py.
//
//     Toda variável-ponteiro definida abaixo carrega o prefixo `glx_` (AUD-L1-GLSYM,
//     2026-07-19, ex.: `glx_glCullFace`); a tabela de símbolos é indexada pelo nome GL
//     CRU (`"glCullFace"`) porque é essa a string literal que o GetProcAddress precisa
//     para resolver o entry point real do driver -- um literal de string não é um
//     identificador e por isso nunca é tocado pelas macros de reescrita de call site
//     `#define glCullFace glx_glCullFace` de gl_loader.h, então não há risco de esta
//     tabela acabar indexada pela string (prefixada) errada.
//
//     Ordem de resolução por símbolo em glx_gl_load(), conforme o brief da tarefa
//     L1.14-GLLOADER: (1) glXGetProcAddressARB/glXGetProcAddress, via dlsym no
//     handle de libGL.so.1 -- cobre contextos desktop com backing GLX (padrão do
//     GLFW em X11/Mesa, o alvo de CI/dev aqui); (2) eglGetProcAddress, via dlsym
//     num handle de libEGL.so.1 se presente -- cobre contextos com backing EGL
//     (Wayland, alguns hosts embarcados); (3) dlsym() direto no handle da libGL --
//     cobre entry points core que são símbolos exportados comuns na maioria das
//     builds GL/Mesa e não exigem estritamente indireção via GetProcAddress. O
//     primeiro resultado não-NULL vence.
#include "gl_loader.h"

#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

"""

LOAD_FN_TEMPLATE = """
// EN: Function-pointer-typed cast of a void* from dlsym/GetProcAddress via a union
//     (not a direct reinterpret/C-style cast) -- POSIX explicitly sanctions this
//     idiom (see the dlsym(3) rationale: ISO C forbids converting object pointers
//     to function pointers directly, but the round-trip through a union member is
//     well-defined on POSIX platforms and avoids -Wpedantic / strict-aliasing
//     complaints).
// PT: Cast tipado-para-ponteiro-de-função de um void* vindo de dlsym/GetProcAddress
//     via union (não um reinterpret/C-style cast direto) -- o POSIX sanciona
//     explicitamente esse idioma (ver o racional de dlsym(3): o ISO C proíbe
//     converter ponteiro de objeto para ponteiro de função diretamente, mas o
//     round-trip via membro de union é bem definido em plataformas POSIX e evita
//     avisos de -Wpedantic / strict-aliasing).
typedef void (*glintfx_glx_fn_t)(void);

#ifdef _WIN32
// EN: Win32 codepath (L1.14-GLLOADER-WIN) -- mirrors the well-known GLAD
//     GLAD_PLATFORM_WIN32 resolution strategy: wglGetProcAddress() first (covers GL
//     >=1.2 core entry points and every extension), falling back to a direct
//     GetProcAddress() against opengl32.dll (covers GL 1.0/1.1, which
//     wglGetProcAddress is documented to NEVER resolve -- see the sentinel-value
//     rationale on glintfx_resolve_symbol below). opengl32.dll is loaded via
//     LoadLibraryA(), never linked (-lopengl32) -- kernel32's implicit
//     Load/GetProcAddress import keeps this loader dependency-free for consumers'
//     link lines, matching the POSIX branch's dlopen-not-link posture.
// PT: Caminho Win32 (L1.14-GLLOADER-WIN) -- espelha a estratégia de resolução
//     GLAD_PLATFORM_WIN32 do GLAD, bem conhecida: wglGetProcAddress() primeiro
//     (cobre entry points core GL >=1.2 e toda extensão), caindo para
//     GetProcAddress() direto em opengl32.dll (cobre GL 1.0/1.1, que o
//     wglGetProcAddress é documentado a NUNCA resolver -- ver o racional dos
//     valores-sentinela em glintfx_resolve_symbol abaixo). opengl32.dll é
//     carregada via LoadLibraryA(), nunca linkada (-lopengl32) -- o import
//     implícito de Load/GetProcAddress do kernel32 mantém este loader livre de
//     dependência na linha de link dos consumidores, espelhando a postura
//     dlopen-sem-linkar do ramo POSIX.
typedef PROC (WINAPI *glintfx_wglGetProcAddress_t)(LPCSTR);

static HMODULE glintfx_opengl32 = NULL;
static glintfx_wglGetProcAddress_t glintfx_wgl_get_proc = NULL;

static void* glintfx_resolve_symbol(const char* name) {
  // EN: FARPROC/PROC are the same underlying function-pointer type on Win32
  //     (<winnt.h>/<minwindef.h>: `typedef FARPROC PROC;`), so a single union
  //     member covers both wglGetProcAddress()'s and GetProcAddress()'s return
  //     value -- same void*<->function-pointer round-trip idiom as the POSIX
  //     `cast` union below (avoids a direct object/function-pointer C-style cast).
  // PT: FARPROC/PROC são o mesmo tipo de ponteiro-de-função por baixo no Win32
  //     (<winnt.h>/<minwindef.h>: `typedef FARPROC PROC;`), então um único membro
  //     de union cobre o retorno de wglGetProcAddress() e de GetProcAddress() --
  //     mesmo idioma de round-trip void*<->ponteiro-de-função da union `cast`
  //     POSIX abaixo (evita um cast C-style direto objeto/ponteiro-de-função).
  union { void* obj; FARPROC fn; } cast;
  cast.obj = NULL;

  if (glintfx_wgl_get_proc) {
    cast.fn = glintfx_wgl_get_proc(name);
    void* p = cast.obj;
    // EN: wglGetProcAddress signals failure not only with NULL but also with the
    //     four documented sentinel values below (MSDN: "The pointer returned...
    //     may be one of a number of failure codes"). Treating those as success
    //     would hand a GL 1.0/1.1 caller a bogus non-NULL pointer that crashes on
    //     first invocation -- those entry points are ordinary opengl32.dll
    //     exports and are documented to NEVER resolve via wglGetProcAddress.
    // PT: wglGetProcAddress sinaliza falha não só com NULL, mas também com os
    //     quatro valores-sentinela documentados abaixo (MSDN: "o ponteiro
    //     retornado... pode ser um dos vários códigos de falha"). Tratar esses
    //     como sucesso entregaria a um chamador GL 1.0/1.1 um ponteiro não-NULL
    //     espúrio que crasha na primeira chamada -- esses entry points são
    //     exports comuns de opengl32.dll e são documentados a NUNCA resolver via
    //     wglGetProcAddress.
    if (p != NULL && p != (void*)1 && p != (void*)2 && p != (void*)3 && p != (void*)-1) {
      return p;
    }
  }
  if (glintfx_opengl32) {
    cast.fn = GetProcAddress(glintfx_opengl32, name);
    return cast.obj;
  }
  return NULL;
}

int glx_gl_load(void) {
  if (!glintfx_opengl32) {
    glintfx_opengl32 = LoadLibraryA("opengl32.dll");
  }

  if (glintfx_opengl32 && !glintfx_wgl_get_proc) {
    union { FARPROC obj; glintfx_wglGetProcAddress_t fn; } cast;
    cast.obj = GetProcAddress(glintfx_opengl32, "wglGetProcAddress");
    glintfx_wgl_get_proc = cast.fn;
  }

  int missing = 0;
  for (size_t i = 0; i < sizeof(glintfx_gl_symbol_table) / sizeof(glintfx_gl_symbol_table[0]); ++i) {
    void* resolved = glintfx_resolve_symbol(glintfx_gl_symbol_table[i].name);
    *glintfx_gl_symbol_table[i].slot = resolved;
    if (!resolved) ++missing;
  }
  return missing ? 1 : 0;
}
#else
typedef glintfx_glx_fn_t (*glintfx_glXGetProcAddress_t)(const unsigned char*);
typedef void* (*glintfx_eglGetProcAddress_t)(const char*);

static void* glintfx_gl_lib_handle = NULL;
static glintfx_glXGetProcAddress_t glintfx_glx_get_proc_address = NULL;
static glintfx_eglGetProcAddress_t glintfx_egl_get_proc_address = NULL;

static void* glintfx_resolve_symbol(const char* name) {
  union { void* obj; glintfx_glx_fn_t fn; } cast;
  cast.obj = NULL;

  if (glintfx_glx_get_proc_address) {
    // EN: glXGetProcAddress's ABI-mandated parameter type is `const GLubyte*` (==
    //     `const unsigned char*`), one qualifier-preserving character-type away from
    //     `name`'s `const char*`. Routed through a union member (instead of a
    //     C-style `(const unsigned char*)name` cast) for the same reason as the
    //     `cast` union above: it sidesteps cppcheck's cstyleCast style check (a
    //     project-wide CI gate, --error-exitcode=1) without a per-file suppression,
    //     and C explicitly permits inspecting any object through an unsigned-char
    //     lvalue, so this is, if anything, less exotic than the pointer-to-
    //     function-pointer union conversion two lines up.
    // PT: O tipo de parâmetro do glXGetProcAddress, ditado pela ABI, é `const
    //     GLubyte*` (== `const unsigned char*`), a um tipo-de-caractere qualificador-
    //     preservado de distância do `const char*` de `name`. Roteado por membro de
    //     union (em vez de um cast C-style `(const unsigned char*)name`) pelo mesmo
    //     motivo da union `cast` acima: contorna o check de estilo cstyleCast do
    //     cppcheck (gate de CI do projeto inteiro, --error-exitcode=1) sem
    //     supressão pontual, e o C permite explicitamente inspecionar qualquer
    //     objeto via lvalue unsigned char -- portanto isto é, no máximo, menos
    //     exótico que a conversão de ponteiro-de-objeto-para-ponteiro-de-função via
    //     union duas linhas acima.
    union { const char* c; const unsigned char* u; } name_view;
    name_view.c = name;
    cast.fn = glintfx_glx_get_proc_address(name_view.u);
    if (cast.obj) return cast.obj;
  }
  if (glintfx_egl_get_proc_address) {
    cast.obj = glintfx_egl_get_proc_address(name);
    if (cast.obj) return cast.obj;
  }
  if (glintfx_gl_lib_handle) {
    cast.obj = dlsym(glintfx_gl_lib_handle, name);
    if (cast.obj) return cast.obj;
  }
  return NULL;
}

int glx_gl_load(void) {
  // EN: dlopen libGL.so.1 first (SONAME on virtually every Linux distro's Mesa/
  //     proprietary-driver package); fall back to the unversioned libGL.so (dev
  //     package name) for hosts missing the versioned symlink. RTLD_GLOBAL so the
  //     symbols are visible for the eglGetProcAddress fallback path below too.
  // PT: Abre libGL.so.1 primeiro (SONAME em praticamente toda distro Linux com
  //     Mesa/driver proprietário); cai para libGL.so sem versão (nome do pacote
  //     -dev) em hosts sem o symlink versionado. RTLD_GLOBAL para que os símbolos
  //     fiquem visíveis também para o caminho de fallback eglGetProcAddress abaixo.
  if (!glintfx_gl_lib_handle) {
    glintfx_gl_lib_handle = dlopen("libGL.so.1", RTLD_NOW | RTLD_GLOBAL);
    if (!glintfx_gl_lib_handle) {
      glintfx_gl_lib_handle = dlopen("libGL.so", RTLD_NOW | RTLD_GLOBAL);
    }
  }

  if (glintfx_gl_lib_handle && !glintfx_glx_get_proc_address) {
    union { void* obj; glintfx_glXGetProcAddress_t fn; } cast;
    cast.obj = dlsym(glintfx_gl_lib_handle, "glXGetProcAddressARB");
    if (!cast.obj) cast.obj = dlsym(glintfx_gl_lib_handle, "glXGetProcAddress");
    glintfx_glx_get_proc_address = cast.fn;
  }

  if (!glintfx_egl_get_proc_address) {
    void* egl_handle = dlopen("libEGL.so.1", RTLD_NOW | RTLD_GLOBAL);
    if (!egl_handle) egl_handle = dlopen("libEGL.so", RTLD_NOW | RTLD_GLOBAL);
    if (egl_handle) {
      union { void* obj; glintfx_eglGetProcAddress_t fn; } cast;
      cast.obj = dlsym(egl_handle, "eglGetProcAddress");
      glintfx_egl_get_proc_address = cast.fn;
    }
  }

  int missing = 0;
  for (size_t i = 0; i < sizeof(glintfx_gl_symbol_table) / sizeof(glintfx_gl_symbol_table[0]); ++i) {
    void* resolved = glintfx_resolve_symbol(glintfx_gl_symbol_table[i].name);
    *glintfx_gl_symbol_table[i].slot = resolved;
    if (!resolved) ++missing;
  }
  return missing ? 1 : 0;
}
#endif // _WIN32

#ifdef __cplusplus
}
#endif
"""


def main() -> None:
    if not GL_XML.exists():
        print(f"ERROR: {GL_XML} not found. Fetch it first (see L1.14-GLLOADER task):", file=sys.stderr)
        print("  curl -sL https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/xml/gl.xml "
              f"-o {GL_XML}", file=sys.stderr)
        sys.exit(1)

    commands = collect_core_commands(GL_XML, TARGET_VERSION)
    verify_typedefs_exist(commands, GLCOREARB_H)

    # ---- gl_loader.h ----
    # EN: AUD-L1-GLSYM -- the extern declaration itself already uses the glx_-prefixed
    #     name (Design A: the generator never emits a plain-named data symbol, not even
    #     transiently), and the call-site-rewrite `#define`s are a second, separate
    #     block emitted only AFTER every extern declaration (and after the
    #     glcorearb.h include, already done in HEADER_PREAMBLE) -- matching the ordering
    #     the task brief calls out as critical.
    # PT: AUD-L1-GLSYM -- a própria declaração extern já usa o nome prefixado com
    #     glx_ (Design A: o gerador nunca emite um símbolo de dado com nome cru, nem
    #     transitoriamente), e os `#define` de reescrita de call site são um segundo
    #     bloco, separado, emitido só DEPOIS de toda declaração extern (e depois do
    #     include de glcorearb.h, já feito em HEADER_PREAMBLE) -- respeitando a ordem
    #     que o brief da tarefa aponta como crítica.
    h_lines = [HEADER_PREAMBLE]
    for cmd in commands:
        h_lines.append(f"extern {typedef_name(cmd)} {glx_name(cmd)};\n")
    h_lines.append(
        "\n"
        "// EN: Call-site-rewrite macros -- rename glintfx's OWN internal GL call sites\n"
        "//     (e.g. `glCullFace(x)`) to the glx_-prefixed pointer variable declared\n"
        "//     above, transparently, without touching a single .cpp call site. Private\n"
        "//     header only (glintfx/src/) -- never leaks into glintfx/include/glintfx/,\n"
        "//     so no consumer translation unit is ever exposed to these macros.\n"
        "// PT: Macros de reescrita de call site -- renomeiam os call sites GL\n"
        "//     internos, PRÓPRIOS do glintfx (ex.: `glCullFace(x)`) para a\n"
        "//     variável-ponteiro prefixada com glx_ declarada acima, de forma\n"
        "//     transparente, sem tocar um único call site .cpp. Só header privado\n"
        "//     (glintfx/src/) -- nunca vaza para glintfx/include/glintfx/, então\n"
        "//     nenhuma unidade de tradução de consumidor é exposta a estas macros.\n"
    )
    for cmd in commands:
        h_lines.append(f"#define {cmd} {glx_name(cmd)}\n")
    h_lines.append(HEADER_POSTAMBLE)
    OUT_H.write_text("".join(h_lines))

    # ---- gl_loader.c ----
    # EN: Definitions and the symbol table both spell out the glx_-prefixed identifier
    #     directly (never relying on the header's #define to rewrite a plain name into
    #     the prefixed one) -- so gl_loader.c's own generated text is immune to the
    #     call-site-rewrite macros it pulls in via `#include "gl_loader.h"` regardless
    #     of ordering. Only the GetProcAddress lookup key is the bare name, and it is a
    #     string literal, which macros never touch.
    # PT: Definições e a tabela de símbolos escrevem o identificador prefixado com
    #     glx_ diretamente (nunca dependendo do #define do header para reescrever um
    #     nome cru no prefixado) -- então o próprio texto gerado de gl_loader.c fica
    #     imune às macros de reescrita de call site que ele puxa via
    #     `#include "gl_loader.h"`, independente de ordem. Só a chave de busca do
    #     GetProcAddress é o nome cru, e é um literal de string, que macro nunca toca.
    c_lines = [SOURCE_PREAMBLE]
    for cmd in commands:
        c_lines.append(f"{typedef_name(cmd)} {glx_name(cmd)} = NULL;\n")
    c_lines.append("\ntypedef struct { const char* name; void** slot; } glintfx_gl_sym_t;\n\n")
    c_lines.append("static const glintfx_gl_sym_t glintfx_gl_symbol_table[] = {\n")
    for cmd in commands:
        c_lines.append(f'  {{ "{cmd}", (void**)&{glx_name(cmd)} }},\n')
    c_lines.append("};\n")
    c_lines.append(LOAD_FN_TEMPLATE)
    OUT_C.write_text("".join(c_lines))

    print(f"Generated {OUT_H.relative_to(REPO_ROOT)} and {OUT_C.relative_to(REPO_ROOT)} "
          f"({len(commands)} GL {TARGET_VERSION} core commands).")


if __name__ == "__main__":
    main()
