// SPDX-License-Identifier: MPL-2.0
// EN: DOC-GLCOHAB (framework-2D, App-mode GL cohabitation contract) -- a single, thin free
//     function: the SDL_GL_GetProcAddress-equivalent a host cohabiting glintfx::App's GL
//     context (glintfx/include/glintfx/app.hpp's set_frame_callback) needs to bring its own GL
//     renderer without linking SDL, GLFW, or any other loader library itself.
//
//     App-mode only, same gate as app.hpp: glintfx::App is the one that owns the window and
//     the GL context this resolves symbols against (glfwMakeContextCurrent, called once inside
//     WindowGlfw::create() and never switched away for the App's lifetime -- see
//     glintfx/src/window_glfw.hpp's own "CONTEXT (D-ORDER)" doc-comment for the single-context
//     invariant this relies on). NOT available in an embed-only build (GLINTFX_BACKEND_GLFW=OFF)
//     -- a UiLayer host already owns its own GL context and its own loader by construction
//     (docs/embed-integration.md section 0), so there is nothing for this function to resolve
//     symbols FOR in that mode.
//
//     WHY A FREE FUNCTION, NOT AN App:: METHOD (design note, DOC-GLCOHAB): kept out of App's own
//     class surface deliberately -- resolving a GL function pointer needs only the process-wide
//     GL loader state glx_gl_load() already lazily initialises (glintfx/src/gl_loader.c), not
//     any per-instance App state, so tying it to an App object would suggest a per-instance
//     scoping this call does not actually have (there is only ever one App/GL context per
//     process anyway, App's own class-level "GLOBAL STATE (N3)" doc-comment). A free function in
//     its own header also mirrors this library's existing precedent for a capability that is
//     conceptually process-wide rather than per-object: glintfx::version() (version.hpp) is the
//     same shape for the exact same reason.
//
//     WHAT THIS DOES NOT EXPOSE (the "thin" half of the design decision, from the líder):
//     glintfx's own internal GL 3.3 core-profile function-pointer table and its batch loader
//     (glx_gl_load(), glintfx/src/gl_loader.h) stay entirely private -- never installed under
//     glintfx/include/glintfx/, never reachable through this header. This function is the ONE
//     seam that crosses that boundary, resolving exactly the ONE symbol a caller names, so
//     publishing it does not pin the internal loader's own format/table -- which is itself a
//     future clean-room-internalisation target (ADR-0009/ADR-0013), not something this library's
//     public API should freeze in place. See glintfx/src/gl_loader.h's own doc-comment on
//     glx_gl_get_proc_address() (the private C helper this delegates to) for the exact
//     glX->EGL->dlsym / wglGetProcAddress->GetProcAddress resolution chain on POSIX/Win32.
//
// PT: DOC-GLCOHAB (framework-2D, contrato de coabitação GL no modo App) -- uma única função
//     livre, fina: o equivalente a SDL_GL_GetProcAddress que um host coabitando o contexto GL
//     do glintfx::App (set_frame_callback de glintfx/include/glintfx/app.hpp) precisa pra trazer
//     o próprio renderer GL sem linkar SDL, GLFW, ou qualquer outra lib de loader por conta
//     própria.
//
//     Só modo App, mesma guarda do app.hpp: o glintfx::App é quem é dono da janela e do
//     contexto GL contra o qual isto resolve símbolos (glfwMakeContextCurrent, chamado uma vez
//     dentro de WindowGlfw::create() e nunca trocado pelo resto da vida do App -- ver o próprio
//     doc-comment "CONTEXT (D-ORDER)" de glintfx/src/window_glfw.hpp pro invariante de contexto
//     único do qual isto depende). NÃO disponível num build embed-only
//     (GLINTFX_BACKEND_GLFW=OFF) -- um host UiLayer já é dono do próprio contexto GL e do
//     próprio loader por construção (docs/embed-integration.md seção 0), então não há nada pra
//     esta função resolver símbolo PARA nesse modo.
//
//     POR QUE UMA FUNÇÃO LIVRE, NÃO UM MÉTODO DE App:: (nota de desenho, DOC-GLCOHAB): mantida
//     fora da própria superfície de classe do App deliberadamente -- resolver um ponteiro de
//     função GL só precisa do estado de loader GL de escopo de processo que o glx_gl_load() já
//     inicializa preguiçosamente (glintfx/src/gl_loader.c), não de nenhum estado por-instância
//     do App, então amarrar isto a um objeto App sugeriria um escopo por-instância que esta
//     chamada não tem de fato (só existe um App/contexto GL por processo de qualquer forma, o
//     próprio doc-comment "GLOBAL STATE (N3)" de nível de classe do App). Uma função livre em
//     header próprio também espelha o precedente já existente desta biblioteca pra uma
//     capacidade que é conceitualmente de escopo de processo, não por-objeto: glintfx::version()
//     (version.hpp) tem a mesma forma pelo mesmo motivo exato.
//
//     O QUE ISTO NÃO EXPÕE (a metade "fina" da decisão de desenho, do líder): a própria tabela
//     interna de ponteiros de função GL 3.3 core profile da glintfx e o próprio loader em lote
//     dela (glx_gl_load(), glintfx/src/gl_loader.h) permanecem inteiramente privados -- nunca
//     instalados sob glintfx/include/glintfx/, nunca alcançáveis por este header. Esta função é a
//     ÚNICA costura que cruza essa fronteira, resolvendo exatamente o ÚNICO símbolo que um
//     chamador nomeia, então publicá-la não fixa o próprio formato/tabela do loader interno --
//     que é ele mesmo um alvo futuro de internalização clean-room (ADR-0009/ADR-0013), não algo
//     que a API pública desta biblioteca deva congelar no lugar. Ver o próprio doc-comment de
//     glx_gl_get_proc_address() (o helper C privado pro qual isto delega) em
//     glintfx/src/gl_loader.h pra cadeia de resolução exata glX->EGL->dlsym /
//     wglGetProcAddress->GetProcAddress no POSIX/Win32.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <glintfx/config.hpp>
#if !GLINTFX_BACKEND_GLFW
#error "glintfx::gl_proc_address requires GLINTFX_BACKEND_GLFW=ON (App-mode GL cohabitation; see DOC-GLCOHAB in docs/embed-integration.md)"
#endif

namespace glintfx {

// EN: Resolve a single GL function pointer by name against the CURRENT GL context -- the same
//     context glintfx::App owns and makes current for the whole process lifetime (see this
//     header's own top comment). Returns nullptr, never crashes, in these cases: `name` is
//     nullptr or ""; or this is called before any glintfx::App has been constructed at least
//     once in this process (the underlying loader state is lazily populated by App's own
//     construction -- WindowGlfw::create(), src/window_glfw.cpp -- so a call made earlier has
//     nothing to resolve against yet).
//     ⚠ MEASURED, NOT ASSUMED (do not rely on the opposite): a NAME THAT NAMES NO REAL SYMBOL
//     does NOT reliably return nullptr on every driver. Confirmed empirically under this
//     library's own Mesa/llvmpipe CI environment: `gl_proc_address("totally_bogus_symbol")`
//     returned a NON-NULL pointer. This is not a glintfx bug -- it is `GLX_ARB_get_proc_address`
//     itself, documented to permit an implementation to hand back a non-NULL pointer for a name
//     it does not actually recognise (the caller is expected to have already confirmed the
//     extension/function exists, e.g. against the GL version or an extension string, BEFORE
//     trusting the returned pointer -- this function does no such confirmation on the caller's
//     behalf). Only call this with a name you already know should exist for the GL 3.3
//     core-profile context glintfx requests (see WindowGlfw::create(), window_glfw.cpp) or a
//     core/EXT/ARB extension you have separately confirmed is present; calling an
//     invented/misspelled name's returned pointer is undefined behaviour, exactly as it would be
//     calling `SDL_GL_GetProcAddress`/`glXGetProcAddressARB` directly for the same reason.
//     Safe to call every frame from inside App::set_frame_callback's hook, or once at
//     renderer-setup time -- this delegates to the SAME resolver glintfx's own internal loader
//     uses per-symbol (glx_gl_get_proc_address(), glintfx/src/gl_loader.h), so it is exactly as
//     cheap/expensive as that (a handful of pointer-typed calls, no allocation, no caching kept
//     here -- cache the result yourself if calling this every frame for the same name matters to
//     your renderer).
// PT: Resolve um único ponteiro de função GL por nome contra o contexto GL CORRENTE -- o mesmo
//     contexto que o glintfx::App possui e torna corrente pela vida inteira do processo (ver o
//     próprio comentário de topo deste header). Retorna nullptr, nunca crasha, nestes casos:
//     `name` é nullptr ou ""; ou isto é chamado antes de qualquer glintfx::App ter sido
//     construído ao menos uma vez neste processo (o estado do loader subjacente é populado
//     preguiçosamente pela própria construção do App -- WindowGlfw::create(),
//     src/window_glfw.cpp -- então uma chamada feita antes não tem nada
//     pra resolver ainda).
//     ⚠ MEDIDO, NÃO PRESUMIDO (não conte com o oposto): um NOME QUE NÃO NOMEIA SÍMBOLO NENHUM
//     NÃO retorna nullptr de forma confiável em todo driver. Confirmado empiricamente sob o
//     próprio ambiente de CI Mesa/llvmpipe desta biblioteca:
//     `gl_proc_address("totally_bogus_symbol")` retornou um ponteiro NÃO-NULO. Isto não é um
//     bug da glintfx -- é o próprio `GLX_ARB_get_proc_address`, documentado a permitir que uma
//     implementação devolva um ponteiro não-nulo pra um nome que não reconhece de fato (o
//     chamador é esperado já ter confirmado que a extensão/função existe, ex.: contra a versão
//     do GL ou uma string de extensão, ANTES de confiar no ponteiro devolvido -- esta função não
//     faz confirmação nenhuma disso em nome do chamador). Só chame isto com um nome que você já
//     sabe que deveria existir pro contexto GL 3.3 core-profile que a glintfx pede (ver
//     WindowGlfw::create(), window_glfw.cpp) ou uma extensão core/EXT/ARB que você confirmou
//     separadamente estar presente; chamar o ponteiro devolvido de um nome
//     inventado/digitado-errado é comportamento indefinido, exatamente como seria chamar
//     `SDL_GL_GetProcAddress`/`glXGetProcAddressARB` direto pelo mesmo motivo.
//     Seguro chamar todo frame de dentro do hook de
//     App::set_frame_callback, ou uma vez no setup do renderer -- isto delega pro MESMO
//     resolvedor que o próprio loader interno da glintfx usa por-símbolo
//     (glx_gl_get_proc_address(), glintfx/src/gl_loader.h), então é exatamente tão barato/caro
//     quanto aquilo (um punhado de chamadas tipadas em ponteiro, sem alocação, sem cache
//     mantido aqui -- faça cache do resultado você mesmo se chamar isto todo frame pro mesmo
//     nome importar pro seu renderer).
void* gl_proc_address(const char* name);

} // namespace glintfx
