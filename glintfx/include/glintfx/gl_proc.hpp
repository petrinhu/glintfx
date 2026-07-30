// SPDX-License-Identifier: MPL-2.0
// EN: DOC-GLCOHAB (framework-2D, GL cohabitation contract) -- a single, thin free function: the
//     SDL_GL_GetProcAddress-equivalent a host cohabiting glintfx's GL context needs to bring its
//     own GL renderer without linking SDL, GLFW, or any other loader library itself.
//
//     Available in BOTH build modes -- resolves against whichever GL context is CURRENT on the
//     calling thread at call time, regardless of who made it current:
//       - glintfx::App (App-mode, set_frame_callback hook, app.hpp): the single context
//         glfwMakeContextCurrent makes current once inside WindowGlfw::create() and never
//         switches away for the App's lifetime -- see glintfx/src/window_glfw.hpp's own
//         "CONTEXT (D-ORDER)" doc-comment for the single-context invariant this relies on.
//       - glintfx::UiLayer (embed/guest mode, ui_layer.hpp): the HOST's own context, already
//         current by construction (docs/embed-integration.md section 0's compose-only contract
//         requires the host to have its GL context current before touching glintfx at all).
//
//     ⚠ GLPROC-EMBED (2026-07-30, reported by a real consumer migrating to embed mode): this
//     function used to be compiled out (`#error`) in embed-only builds
//     (GLINTFX_BACKEND_GLFW=OFF), on the reasoning "a UiLayer host already owns its own GL
//     context and its own loader, so there is nothing to resolve FOR in that mode". Premise
//     right, conclusion backwards: the host IS the one that owns the context and loader in embed
//     mode, which is EXACTLY why it needs a way to resolve its own function pointers without
//     linking a second loader library (SDL/GLFW/glad/gl3w) purely for that. The gate was never
//     technical -- the private C helper this delegates to (glx_gl_get_proc_address(),
//     gl_loader.c) has zero GLFW dependency in either the POSIX or the Win32 branch and is
//     compiled unconditionally into every `glintfx` build configuration regardless of
//     GLINTFX_BACKEND_GLFW (see that function's own `DOC-GLPROC-CLAIM` doc-comment in
//     gl_loader.h for the `nm`-verified proof); the `#error` gated the PUBLIC C++ surface only.
//     Relaxing it destravates an embed host from that second loader dependency; it does NOT hand
//     the host GL types/enums/function prototypes -- a glad/gl3w/hand-rolled header remains the
//     host's own responsibility for those (see WHAT THIS DOES NOT EXPOSE below; this was
//     confirmed explicitly by the consumer that requested this fatia).
//
//     WHY A FREE FUNCTION, NOT AN App::/UiLayer:: METHOD (design note, DOC-GLCOHAB): kept out of
//     both classes' own surfaces deliberately -- resolving a GL function pointer needs only the
//     process-wide GL loader state glx_gl_load() already lazily initialises
//     (glintfx/src/gl_loader.c), not any per-instance App/UiLayer state, so tying it to an
//     instance would suggest a per-instance scoping this call does not actually have (there is
//     only ever one GL context current per thread anyway). A free function in its own header
//     also mirrors this library's existing precedent for a capability that is conceptually
//     process-wide rather than per-object: glintfx::version() (version.hpp) is the same shape
//     for the exact same reason.
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
// PT: DOC-GLCOHAB (framework-2D, contrato de coabitação GL) -- uma única função livre, fina: o
//     equivalente a SDL_GL_GetProcAddress que um host coabitando o contexto GL da glintfx
//     precisa pra trazer o próprio renderer GL sem linkar SDL, GLFW, ou qualquer outra lib de
//     loader por conta própria.
//
//     Disponível nos DOIS modos de build -- resolve contra qualquer que seja o contexto GL
//     CORRENTE na thread chamadora no momento da chamada, independente de quem o tornou corrente:
//       - glintfx::App (modo App, hook set_frame_callback, app.hpp): o contexto único que
//         glfwMakeContextCurrent torna corrente uma vez dentro de WindowGlfw::create() e nunca
//         troca pelo resto da vida do App -- ver o próprio doc-comment "CONTEXT (D-ORDER)" de
//         glintfx/src/window_glfw.hpp pro invariante de contexto único do qual isto depende.
//       - glintfx::UiLayer (modo embed/guest, ui_layer.hpp): o PRÓPRIO contexto do HOST, já
//         corrente por construção (a seção 0 do contrato compose-only de
//         docs/embed-integration.md exige que o host já tenha o próprio contexto GL corrente
//         antes de tocar a glintfx de qualquer forma).
//
//     ⚠ GLPROC-EMBED (2026-07-30, reportado por um consumidor real migrando pro modo embed):
//     esta função costumava ser compilada fora (`#error`) em builds embed-only
//     (GLINTFX_BACKEND_GLFW=OFF), com o raciocínio "um host UiLayer já é dono do próprio
//     contexto GL e do próprio loader, então não há nada pra resolver PARA nesse modo". Premissa
//     certa, conclusão invertida: o host É quem é dono do contexto e do loader no modo embed, e é
//     EXATAMENTE por isso que ele precisa de uma forma de resolver os próprios ponteiros de
//     função sem linkar uma segunda lib de loader (SDL/GLFW/glad/gl3w) só pra isso. A guarda
//     nunca foi técnica -- o helper C privado pro qual isto delega (glx_gl_get_proc_address(),
//     gl_loader.c) não tem dependência de GLFW nenhuma em nenhum dos dois ramos (POSIX ou
//     Win32) e é compilado incondicionalmente em toda configuração de build da `glintfx`
//     independente de GLINTFX_BACKEND_GLFW (ver o próprio doc-comment `DOC-GLPROC-CLAIM` daquela
//     função em gl_loader.h pra prova verificada via `nm`); o `#error` gateava só a superfície
//     C++ PÚBLICA. Relaxá-lo destrava um host embed dessa segunda dependência de loader; NÃO
//     entrega ao host tipos/enums/protótipos de função GL -- um header glad/gl3w/próprio segue
//     sendo responsabilidade do host pra isso (ver O QUE ISTO NÃO EXPÕE abaixo; isto foi
//     confirmado explicitamente pelo consumidor que pediu esta fatia).
//
//     POR QUE UMA FUNÇÃO LIVRE, NÃO UM MÉTODO DE App::/UiLayer:: (nota de desenho, DOC-GLCOHAB):
//     mantida fora da própria superfície das duas classes deliberadamente -- resolver um
//     ponteiro de função GL só precisa do estado de loader GL de escopo de processo que o
//     glx_gl_load() já inicializa preguiçosamente (glintfx/src/gl_loader.c), não de nenhum
//     estado por-instância de App/UiLayer, então amarrar isto a uma instância sugeriria um
//     escopo por-instância que esta chamada não tem de fato (só existe um contexto GL corrente
//     por thread de qualquer forma). Uma função livre em header próprio também espelha o
//     precedente já existente desta biblioteca pra uma capacidade que é conceitualmente de
//     escopo de processo, não por-objeto: glintfx::version() (version.hpp) tem a mesma forma
//     pelo mesmo motivo exato.
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

namespace glintfx {

// EN: Resolve a single GL function pointer by name against the CURRENT GL context -- whichever
//     context glintfx::App or the host embedding glintfx::UiLayer owns and makes current (see
//     this header's own top comment). Returns nullptr, never crashes, in these cases: `name` is
//     nullptr or ""; or this is called before the underlying loader state has been populated at
//     least once in this process. That population happens as a side effect of ANY of: App's own
//     construction (WindowGlfw::create(), src/window_glfw.cpp), UiLayer's own construction with
//     the default `Config::load_gl = true` (src/ui_layer.cpp), or Draw2d::init()
//     (src/draw2d.cpp) -- all three call the same glx_gl_load() internally, and it is idempotent,
//     so any one of them is enough. ⚠ EMBED CAVEAT: a UiLayer host that constructs with
//     `Config::load_gl = false` (declaring "I already loaded my own GL pointers, skip yours")
//     causes glintfx's own glx_gl_load() to never run in that process unless something else in
//     this list also runs -- in that specific configuration this function reliably returns
//     nullptr for every name, because the glX/EGL/dlsym resolver state it depends on is only
//     ever populated by glx_gl_load() itself, regardless of what the host's OWN loader resolved.
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
//     Safe to call every frame from inside App::set_frame_callback's hook, from a UiLayer host's
//     own render step, or once at renderer-setup time -- this delegates to the SAME resolver
//     glintfx's own internal loader uses per-symbol (glx_gl_get_proc_address(),
//     glintfx/src/gl_loader.h), so it is exactly as cheap/expensive as that (a handful of
//     pointer-typed calls, no allocation, no caching kept here -- cache the result yourself if
//     calling this every frame for the same name matters to your renderer).
// PT: Resolve um único ponteiro de função GL por nome contra o contexto GL CORRENTE -- qualquer
//     que seja o contexto que o glintfx::App ou o host que embarca o glintfx::UiLayer possui e
//     torna corrente (ver o próprio comentário de topo deste header). Retorna nullptr, nunca
//     crasha, nestes casos: `name` é nullptr ou ""; ou isto é chamado antes do estado do loader
//     subjacente ter sido populado ao menos uma vez neste processo. Essa população acontece como
//     efeito colateral de QUALQUER um destes: a própria construção do App (WindowGlfw::create(),
//     src/window_glfw.cpp), a própria construção do UiLayer com o `Config::load_gl = true`
//     padrão (src/ui_layer.cpp), ou o Draw2d::init() (src/draw2d.cpp) -- os três chamam
//     internamente o mesmo glx_gl_load(), que é idempotente, então qualquer um deles basta.
//     ⚠ RESSALVA DE EMBED: um host UiLayer que constrói com `Config::load_gl = false`
//     (declarando "já carreguei meus próprios ponteiros GL, pule os seus") faz com que o próprio
//     glx_gl_load() da glintfx nunca rode nesse processo, a menos que outra coisa desta lista
//     também rode -- nessa configuração específica esta função retorna nullptr de forma
//     confiável para todo nome, porque o estado do resolvedor glX/EGL/dlsym do qual ela depende
//     só é populado pelo próprio glx_gl_load(), independente do que o loader PRÓPRIO do host
//     tenha resolvido.
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
//     Seguro chamar todo frame de dentro do hook de App::set_frame_callback, do próprio passo de
//     render de um host UiLayer, ou uma vez no setup do renderer -- isto delega pro MESMO
//     resolvedor que o próprio loader interno da glintfx usa por-símbolo
//     (glx_gl_get_proc_address(), glintfx/src/gl_loader.h), então é exatamente tão barato/caro
//     quanto aquilo (um punhado de chamadas tipadas em ponteiro, sem alocação, sem cache
//     mantido aqui -- faça cache do resultado você mesmo se chamar isto todo frame pro mesmo
//     nome importar pro seu renderer).
void* gl_proc_address(const char* name);

} // namespace glintfx
