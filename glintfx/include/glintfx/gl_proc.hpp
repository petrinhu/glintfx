// SPDX-License-Identifier: Apache-2.0
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

// EN: REQUIRES (read this before the "returns nullptr" guarantee below): `glx_gl_load()` must
//     have run at least once in this process before this function can resolve anything for real.
//     That happens as a side effect of ANY of: App's own construction (WindowGlfw::create(),
//     src/window_glfw.cpp), UiLayer's own construction with the default `Config::load_gl = true`
//     (src/ui_layer.cpp), or Draw2d::init() (src/draw2d.cpp) -- all three call the same
//     glx_gl_load() internally, and it is idempotent, so any ONE of them, anywhere earlier in
//     this same process, is enough. A HOST EMBEDDING GLINTFX VIA UiLayer WITH THE DEFAULT
//     `Config::load_gl = true` ALREADY SATISFIES THIS -- no `App` needed, embed mode does not
//     leave this function stranded.
//
// EN: Resolve a single GL function pointer by name against the CURRENT GL context -- whichever
//     context glintfx::App or the host embedding glintfx::UiLayer owns and makes current (see
//     this header's own top comment). Returns nullptr, never crashes, in these cases: `name` is
//     nullptr or ""; or this is called before the REQUIRES precondition above has been met.
//     ⚠ ASKING TO SKIP `glx_gl_load()` (`Config::load_gl = false`, or its 2026-08-04 rename
//     `Config::assume_gl_loaded = true` -- see below) AS THE FIRST GLINTFX ENTITY IN A PROCESS
//     DOES NOT BUILD A WORKING `UiLayer` -- FIXED TO FAIL CLEAN (`GLPROC-CRASH`, W22, 2026-07-30;
//     this paragraph replaces an earlier version of itself that documented a real, since-fixed
//     SIGSEGV, MEASURED BY EXECUTION via gdb -- see `render_gl3.cpp`'s own `GLPROC-CRASH` comment
//     for the guard and its own mutation proof): a `UiLayer` host that constructs asking to skip
//     the loader (declaring "I already loaded my own GL pointers, skip yours") skips glintfx's own
//     `glx_gl_load()` call. If NOTHING ELSE in the REQUIRES list above has ALREADY run earlier in
//     this same process, `RenderGl3::init()` now REFUSES (`return false`) the moment it detects
//     an unpopulated loader (`glx_glCreateShader == nullptr`, the sentinel proved -- not assumed
//     -- to be the exact pointer this path reads first), BEFORE the RmlUi-side constructor that
//     used to dereference it. `Engine::attach()` propagates that `false`, so the `UiLayer`
//     constructor now leaves `ok() == false` -- a documented, catchable failure, never a crash.
//     Unrelated to this function's own gate: this is a guard inside `RenderGl3::init()` itself,
//     the ONE public entry point across `Engine`/`UiLayer`/`App`/`Draw2d` that used to be
//     unguarded (every sibling already checks `ok()`/`impl_`/`initialized` before touching GL --
//     `init()` is structurally the one place a constructor touches GL before any instance guard
//     can exist yet to stop it). ⚠ FAILING CLEAN IS NOT THE SAME AS BEING USEFUL: a class-wide
//     sweep (2026-07-30) found NO scenario where asking to skip the loader is simultaneously SAFE
//     and USEFUL -- as the FIRST glintfx entity it is a trap (never builds a working `UiLayer`,
//     guard or not); AFTER another entity has already run the loader it works, but saves only one
//     cheap, idempotent call over leaving the default (loader runs). ✅ RESOLVED 2026-08-04
//     (`SEED-LOADGL-NOME`, líder decision): the field itself WAS renamed --
//     `UiLayerConfig::load_gl` is now deprecated (kept functional for one version, source
//     compatibility only) in favour of `UiLayerConfig::assume_gl_loaded`, whose polarity is
//     INVERTED (`load_gl = false` <=> `assume_gl_loaded = true`). This finding (no safe+useful
//     scenario) is UNCHANGED by the rename -- the rename only makes the caller's INTENT
//     nameable ("I already populated the table elsewhere") instead of misleadingly shaped like a
//     performance knob. See `UiLayerConfig`'s own doc-comment (`ui_layer.hpp`) for the exact
//     mechanical contract (both fields participate: the loader runs iff
//     `load_gl && !assume_gl_loaded`).
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
// PT: EXIGE (leia isto antes da garantia de "retorna nullptr" abaixo): o `glx_gl_load()` precisa
//     ter rodado ao menos uma vez neste processo antes desta função conseguir resolver algo de
//     verdade. Isso acontece como efeito colateral de QUALQUER um destes: a própria construção
//     do App (WindowGlfw::create(), src/window_glfw.cpp), a própria construção do UiLayer com o
//     `Config::load_gl = true` padrão (src/ui_layer.cpp), ou o Draw2d::init() (src/draw2d.cpp) --
//     os três chamam internamente o mesmo glx_gl_load(), que é idempotente, então QUALQUER um
//     deles, em qualquer ponto anterior deste mesmo processo, basta. UM HOST QUE EMBARCA A
//     GLINTFX VIA UiLayer COM O `Config::load_gl = true` PADRÃO JÁ SATISFAZ ISTO -- nenhum `App`
//     é necessário, o modo embed não deixa esta função encalhada.
//
// PT: Resolve um único ponteiro de função GL por nome contra o contexto GL CORRENTE -- qualquer
//     que seja o contexto que o glintfx::App ou o host que embarca o glintfx::UiLayer possui e
//     torna corrente (ver o próprio comentário de topo deste header). Retorna nullptr, nunca
//     crasha, nestes casos: `name` é nullptr ou ""; ou isto é chamado antes da precondição EXIGE
//     acima ter sido satisfeita.
//     ⚠ PEDIR PRA PULAR O `glx_gl_load()` (`Config::load_gl = false`, ou o rename dela de
//     2026-08-04 `Config::assume_gl_loaded = true` -- ver abaixo) COMO PRIMEIRA ENTIDADE GLINTFX
//     DE UM PROCESSO NÃO CONSTRÓI UM `UiLayer` FUNCIONAL -- CONSERTADO PRA FALHAR LIMPO
//     (`GLPROC-CRASH`, W22, 2026-07-30; este parágrafo substitui uma versão anterior de si mesmo
//     que documentava um SIGSEGV real, desde então consertado, MEDIDO POR EXECUÇÃO via gdb -- ver
//     o próprio comentário `GLPROC-CRASH` de `render_gl3.cpp` pro guard e a própria prova de
//     mutação dele): um host UiLayer que constrói pedindo pra pular o loader (declarando "já
//     carreguei meus próprios ponteiros GL, pule os seus") pula a própria chamada de
//     `glx_gl_load()` da glintfx. Se NADA MAIS da lista EXIGE acima já rodou antes neste mesmo
//     processo, o `RenderGl3::init()` agora RECUSA (`return false`) no momento em que detecta um
//     loader não populado (`glx_glCreateShader == nullptr`, a sentinela provada -- não presumida
//     -- de ser o ponteiro exato que este caminho lê primeiro), ANTES do construtor do lado RmlUi
//     que costumava desreferenciá-lo. O `Engine::attach()` propaga esse `false`, então o
//     construtor do `UiLayer` agora deixa `ok() == false` -- uma falha documentada, capturável,
//     nunca um crash. Sem relação com a própria guarda desta função: é um guard dentro do próprio
//     `RenderGl3::init()`, o ÚNICO ponto de entrada público de toda a superfície
//     `Engine`/`UiLayer`/`App`/`Draw2d` que costumava ficar desguardado (todo irmão já checa
//     `ok()`/`impl_`/`initialized` antes de tocar GL -- `init()` é estruturalmente o único lugar
//     onde um construtor toca GL antes de qualquer guard de instância existir pra impedi-lo).
//     ⚠ FALHAR LIMPO NÃO É O MESMO QUE SER ÚTIL: uma varredura de classe (2026-07-30) não achou
//     NENHUM cenário onde pedir pra pular o loader é ao mesmo tempo SEGURO e ÚTIL -- como PRIMEIRA
//     entidade glintfx é uma armadilha (nunca constrói um `UiLayer` funcional, guard ou não);
//     DEPOIS de outra entidade já ter rodado o loader funciona, mas economiza só uma chamada
//     idempotente e barata sobre deixar o padrão (loader roda). ✅ RESOLVIDO 2026-08-04
//     (`SEED-LOADGL-NOME`, decisão do líder): o próprio campo FOI renomeado --
//     `UiLayerConfig::load_gl` agora é deprecated (mantido funcional por uma versão, só por
//     compatibilidade de fonte) em favor de `UiLayerConfig::assume_gl_loaded`, cuja polaridade é
//     INVERTIDA (`load_gl = false` <=> `assume_gl_loaded = true`). Este achado (nenhum cenário
//     seguro+útil) fica INALTERADO pelo rename -- ele só torna a INTENÇÃO do chamador nomeável
//     ("eu já populei a tabela em outro lugar") em vez de enganosamente moldada como botão de
//     performance. Ver o próprio doc-comment de `UiLayerConfig` (`ui_layer.hpp`) pro contrato
//     mecânico exato (os dois campos participam: o loader roda sse
//     `load_gl && !assume_gl_loaded`).
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
