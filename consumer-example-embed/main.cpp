// SPDX-License-Identifier: MPL-2.0
// EN: consumer-example-embed — drop-in proof for the EMBED contract (TST-L1-CONTRACT): an
//     external host consumes glintfx built with GLINTFX_BACKEND_GLFW=OFF and includes ONLY
//     <glintfx/ui_layer.hpp>. If any GL/GLFW/RmlUi type leaked through glintfx's public
//     headers in embed-only mode, this file would fail to compile.
//
//     glfw is linked DIRECTLY by this consumer (find_package(glfw3) in this project's own
//     CMakeLists.txt), never by glintfx itself -- proving the "host owns its own window/GL
//     context, glintfx never creates one in embed mode" half of the contract documented in
//     docs/embed-integration.md. A real host (e.g. GusWorld, SDL3) would use its own
//     windowing library instead; GLFW is used here only because it is the dependency this
//     repository's CI already provisions and exercises under Xvfb for every other embed test
//     (glintfx/tests/ui_layer_sanity.cpp and friends) -- reusing a proven, zero-new-CI-deps
//     path was a deliberate choice over EGL surfaceless (see this project's TST-L1-CONTRACT
//     hand-off notes for the two-option comparison).
//
//     Unlike glintfx/tests/*.cpp (which are compiled INSIDE the glintfx build tree and may
//     use glintfx's own private test-only GL context helper, window_glfw.cpp), this file is a
//     wholly separate CMake project: it proves the PUBLIC contract from outside, the same way
//     any third-party consumer would.
//
//     *** FINDING (TST-L1-CONTRACT, severity HIGH, NOT fixed here -- QA scope is proving the
//     contract, not patching the library): libglintfx.a leaks GLOBAL, externally-linked BSS
//     symbols with the EXACT SAME NAMES as real OpenGL functions (glClearColor, glClear,
//     glReadBuffer, glPixelStorei, glReadPixels, and -- per glintfx/src/gl_loader.h's own
//     doc-comment -- "one extern PFN<CMD>PROC <cmd> per GL 3.3 command", i.e. essentially the
//     WHOLE GL 3.3 core API). This is the gl3w-derived clean-room loader from L1.14-GLLOADER;
//     its header (gl_loader.h) IS correctly kept private (not installed under include/), but
//     the OBJECT CODE compiled from gl_loader.c into libglintfx.a still exports those global
//     symbols at the ARCHIVE/LINKER level -- invisible to the "does it leak a type through a
//     public header" check the embed contract is normally worried about, but a real leak
//     nonetheless, at the ABI/symbol-table layer instead of the header layer.
//     REPRODUCTION: an earlier version of this file called the standard library functions
//     (glClearColor/glReadBuffer/glPixelStorei/glReadPixels) directly by name, exactly as any
//     host managing its own GL context normally would (and exactly as GusWorld's own
//     SDL3+GL rendering code plausibly does for ITS OWN scene, independent of glintfx). The
//     linker resolved those undefined references to libglintfx.a's BSS data objects (function
//     POINTER variables, uninitialised zero-fill) INSTEAD of the real code symbols in
//     libGL.so, because the static archive was searched and satisfied the reference before
//     ld ever consulted the shared library. The result: a hard SIGSEGV inside "glClearColor"
//     at a low, non-PIE, in-executable address -- confirmed with gdb (crash PC pointed
//     straight at the BSS slot's own address, i.e. the CPU tried to EXECUTE the zeroed
//     pointer storage as machine code) and with `nm libglintfx.a | grep -w glClearColor` (shows
//     `B glClearColor`, a bss-section DATA symbol, not `T`/code).
//     IMPACT: any embed host that links glintfx::glintfx AND calls so much as one raw GL
//     function by its standard name anywhere in the host's OWN rendering code -- an
//     assumption the WHOLE embed contract rests on ("host owns its own GL context", which
//     necessarily means the host draws its own scene with its own GL calls before compositing
//     the UI) -- risks exactly this class of silent symbol hijack for every name
//     gl_loader.h declares. Whether it manifests depends only on link order and on which of
//     the two definitions (real libGL.so code vs. glintfx's own BSS slot) the linker happens
//     to resolve first for that particular symbol in that particular host binary -- i.e. it
//     is a LATENT, not merely local, hazard: a host that has not yet hit it is not proven
//     safe, only proven not-yet-unlucky. This was NOT caught by glintfx's own internal test
//     suite because every internal embed test links against glintfx's PRIVATE window_glfw.cpp
//     helper, which itself resolves all of its GL calls through glintfx's OWN loader
//     (gl_loader.h) rather than the system's <GL/gl.h> -- so internal tests never generate an
//     undefined reference to the bare symbol name from a SEPARATE translation unit the way an
//     external consumer's own rendering code does.
//     WORKAROUND applied in THIS file (see the ResolvedGl block below): resolve the 5 GL
//     functions this file needs via glfwGetProcAddress() into LOCAL function-pointer
//     variables, so this translation unit never emits an undefined reference to the bare
//     symbol names glClearColor/glClear/glReadBuffer/glPixelStorei/glReadPixels -- sidestepping
//     the archive-symbol collision entirely. This is a defensive pattern any real embed host
//     should currently adopt for ALL of its own GL calls (or namespace its own GL loader's
//     symbols, e.g. GLAD's `glad_glXxx` + `#define glXxx glad_glXxx` convention) until the
//     underlying leak is fixed upstream in gl_loader.c/.h (e.g. `-fvisibility=hidden` /
//     `objcopy --localize-symbol` on the archive, or renaming the loader's own globals off the
//     bare GL names, mirroring the `glx_` prefix convention ADR-0009 already establishes for
//     the internalization boundary). Reported for escalation -- not something a QA contract
//     test should silently patch around without flagging; suggested TODO item to file:
//     something in the LW-TST/AUD-* family, e.g. AUD-L1-GLSYM or similar, owned by
//     software-architect/backend-engineer.
//
// PT: consumer-example-embed — prova de drop-in para o contrato EMBED (TST-L1-CONTRACT): um
//     host externo consome a glintfx compilada com GLINTFX_BACKEND_GLFW=OFF e inclui SÓ
//     <glintfx/ui_layer.hpp>. Se algum tipo de GL/GLFW/RmlUi vazasse pelos headers públicos da
//     glintfx em modo embed-only, este arquivo falharia ao compilar.
//
//     glfw é linkado DIRETAMENTE por este consumidor (find_package(glfw3) no próprio
//     CMakeLists.txt deste projeto), nunca pela própria glintfx -- provando a metade "o host é
//     dono da própria janela/contexto GL, a glintfx nunca cria uma em modo embed" do contrato
//     documentado em docs/embed-integration.md. Um host real (ex.: GusWorld, SDL3) usaria sua
//     própria lib de janelamento; GLFW é usado aqui só porque é a dependência que o CI deste
//     repositório já provisiona e exercita sob Xvfb para todo outro teste embed
//     (glintfx/tests/ui_layer_sanity.cpp e afins) -- reusar um caminho comprovado e sem novas
//     deps de CI foi uma escolha deliberada frente a EGL surfaceless (ver as notas de entrega
//     do TST-L1-CONTRACT deste projeto para a comparação das duas opções).
//
//     Diferente de glintfx/tests/*.cpp (compilados DENTRO da árvore de build da glintfx e que
//     podem usar o helper de contexto GL privado só-de-teste da própria glintfx,
//     window_glfw.cpp), este arquivo é um projeto CMake inteiramente separado: prova o
//     contrato PÚBLICO de fora, do mesmo jeito que qualquer consumidor terceiro faria.
//
//     *** ACHADO (TST-L1-CONTRACT, severidade ALTA, NÃO corrigido aqui -- o escopo de QA é
//     provar o contrato, não remendar a lib): libglintfx.a vaza símbolos BSS GLOBAIS, com
//     linkage externa, com o MESMO NOME EXATO de funções OpenGL reais (glClearColor, glClear,
//     glReadBuffer, glPixelStorei, glReadPixels e -- pelo próprio doc-comment de
//     glintfx/src/gl_loader.h -- "um extern PFN<CMD>PROC <cmd> por comando GL 3.3", ou seja,
//     essencialmente a API GL 3.3 core INTEIRA). É o loader clean-room derivado do gl3w do
//     L1.14-GLLOADER; o header dele (gl_loader.h) É corretamente mantido privado (não
//     instalado sob include/), mas o CÓDIGO OBJETO compilado de gl_loader.c dentro de
//     libglintfx.a ainda exporta esses símbolos globais no nível de ARQUIVO/LINKER -- invisível
//     ao cheque "vaza algum tipo por header público" que o contrato embed normalmente se
//     preocupa, mas um vazamento real do mesmo jeito, na camada de ABI/tabela-de-símbolos em
//     vez da camada de header.
//     REPRODUÇÃO: uma versão anterior deste arquivo chamava as funções padrão da lib
//     (glClearColor/glReadBuffer/glPixelStorei/glReadPixels) diretamente pelo nome, exatamente
//     como qualquer host gerenciando seu próprio contexto GL normalmente faria (e exatamente
//     como o código de renderização SDL3+GL do próprio GusWorld plausivelmente faz para a
//     PRÓPRIA cena, independente da glintfx). O linker resolveu essas referências
//     indefinidas para os objetos de dado BSS de libglintfx.a (variáveis PONTEIRO-DE-FUNÇÃO,
//     zero-fill não inicializadas) EM VEZ dos símbolos de código reais em libGL.so, porque o
//     archive estático foi buscado e satisfez a referência antes do ld sequer consultar a
//     biblioteca compartilhada. O resultado: um SIGSEGV duro dentro de "glClearColor" num
//     endereço baixo, não-PIE, dentro do próprio executável -- confirmado com gdb (o PC do
//     crash apontava direto pro endereço do próprio slot BSS, ou seja, a CPU tentou EXECUTAR o
//     armazenamento de ponteiro zerado como código de máquina) e com
//     `nm libglintfx.a | grep -w glClearColor` (mostra `B glClearColor`, um símbolo de DADO da
//     seção bss, não `T`/código).
//     IMPACTO: qualquer host embed que linka glintfx::glintfx E chama sequer UMA função GL
//     crua pelo nome padrão em qualquer lugar do código de renderização PRÓPRIO do host -- uma
//     premissa sobre a qual o contrato embed INTEIRO se apoia ("o host é dono do próprio
//     contexto GL", o que necessariamente significa que o host desenha sua própria cena com
//     suas próprias chamadas GL antes de compor a UI) -- corre o risco exato desse sequestro
//     silencioso de símbolo para todo nome que gl_loader.h declara. Se se manifesta depende só
//     da ordem de link e de qual das duas definições (código real de libGL.so vs. o próprio
//     slot BSS da glintfx) o linker resolve primeiro para aquele símbolo específico naquele
//     binário host específico -- ou seja, é um risco LATENTE, não meramente local: um host que
//     ainda não bateu nisso não está provado seguro, só provado ainda-não-azarado. Isso NÃO
//     foi capturado pela própria suíte de teste interna da glintfx porque todo teste embed
//     interno linka contra o helper PRIVADO window_glfw.cpp da própria glintfx, que por sua vez
//     resolve todas as suas chamadas GL através do PRÓPRIO loader da glintfx (gl_loader.h) em
//     vez do <GL/gl.h> do sistema -- então testes internos nunca geram uma referência
//     indefinida ao nome de símbolo cru a partir de uma translation unit SEPARADA do jeito que
//     o código de renderização próprio de um consumidor externo gera.
//     WORKAROUND aplicado NESTE arquivo (ver o bloco ResolvedGl abaixo): resolve as 5 funções
//     GL que este arquivo precisa via glfwGetProcAddress() em variáveis PONTEIRO-DE-FUNÇÃO
//     LOCAIS, para que esta translation unit nunca emita uma referência indefinida aos nomes
//     de símbolo crus glClearColor/glClear/glReadBuffer/glPixelStorei/glReadPixels --
//     contornando a colisão de símbolo do archive inteiramente. Este é um padrão defensivo que
//     qualquer host embed real deveria adotar hoje para TODAS as suas próprias chamadas GL (ou
//     namespacar os símbolos do próprio loader GL, ex.: a convenção `glad_glXxx` +
//     `#define glXxx glad_glXxx` do GLAD) até que o vazamento subjacente seja corrigido a
//     montante em gl_loader.c/.h (ex.: `-fvisibility=hidden` / `objcopy --localize-symbol` no
//     archive, ou renomear os globais do próprio loader para fora dos nomes GL crus,
//     espelhando a convenção de prefixo `glx_` que o ADR-0009 já estabelece para a fronteira
//     de internalização). Reportado para escalonamento -- não é algo que um teste de contrato
//     de QA deva remendar em silêncio sem sinalizar; item de TODO sugerido a arquivar: algo na
//     família LW-TST/AUD-*, ex. AUD-L1-GLSYM ou similar, de responsabilidade do
//     software-architect/backend-engineer.
// Copyright (c) 2026 Petrus Silva Costa

#include <glintfx/ui_layer.hpp>

// EN: GLFW's own header transitively includes the platform's legacy <GL/gl.h> (OpenGL 1.1)
//     by default (unless GLFW_INCLUDE_NONE is defined, which this file does not do). It
//     supplies the GLenum/GLbitfield/GLint/GLsizei/GLclampf types and the GL_* constants this
//     file uses -- but NOT the 5 function CALLS themselves: see the top-of-file FINDING above
//     for why those are resolved through glfwGetProcAddress() instead of called by their bare
//     names.
// PT: O próprio header do GLFW inclui transitivamente o <GL/gl.h> legado da plataforma
//     (OpenGL 1.1) por padrão (a menos que GLFW_INCLUDE_NONE seja definido, o que este arquivo
//     não faz). Ele supre os tipos GLenum/GLbitfield/GLint/GLsizei/GLclampf e as constantes
//     GL_* que este arquivo usa -- mas NÃO as 5 CHAMADAS de função em si: ver o ACHADO no topo
//     do arquivo para o porquê delas serem resolvidas via glfwGetProcAddress() em vez de
//     chamadas pelo nome cru.
#include <GLFW/glfw3.h>

#include <cstdio>
#include <vector>

namespace {
constexpr int kWidth = 320;
constexpr int kHeight = 240;

// ---------------------------------------------------------------------------
// EN: ResolvedGl — the 5 legacy GL 1.0/1.1 functions this file needs, resolved by pointer via
//     glfwGetProcAddress() rather than called by their bare standard names. See the
//     top-of-file FINDING comment for the full rationale: libglintfx.a exports GLOBAL BSS
//     symbols under these EXACT names (its own internal GL loader, gl_loader.h/.c), which
//     would otherwise hijack a direct call from this (or any other external consumer's)
//     translation unit. Populated once in main() right after glfwMakeContextCurrent();
//     every call site below goes through these pointers, never the bare names.
// PT: ResolvedGl — as 5 funções GL 1.0/1.1 legadas que este arquivo precisa, resolvidas por
//     ponteiro via glfwGetProcAddress() em vez de chamadas pelos nomes padrão crus. Ver o
//     comentário de ACHADO no topo do arquivo para a racional completa: libglintfx.a exporta
//     símbolos BSS GLOBAIS sob esses nomes EXATOS (o próprio loader GL interno da glintfx,
//     gl_loader.h/.c), que de outra forma sequestrariam uma chamada direta desta (ou de
//     qualquer outra) translation unit de consumidor externo. Preenchido uma vez em main()
//     logo após glfwMakeContextCurrent(); todo call site abaixo passa por estes ponteiros,
//     nunca pelos nomes crus.
// ---------------------------------------------------------------------------
struct ResolvedGl {
  using PFN_ClearColor = void (*)(GLclampf, GLclampf, GLclampf, GLclampf);
  using PFN_Clear = void (*)(GLbitfield);
  using PFN_ReadBuffer = void (*)(GLenum);
  using PFN_PixelStorei = void (*)(GLenum, GLint);
  using PFN_ReadPixels = void (*)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*);

  PFN_ClearColor clear_color = nullptr;
  PFN_Clear clear = nullptr;
  PFN_ReadBuffer read_buffer = nullptr;
  PFN_PixelStorei pixel_storei = nullptr;
  PFN_ReadPixels read_pixels = nullptr;

  // EN: Returns false (all pointers left null) if any of the 5 symbols failed to resolve --
  //     should never happen for GL 1.0/1.1 core against any conformant driver, but checked
  //     explicitly rather than assumed (AUD-TEC-5 fail-high spirit, consistent with the rest
  //     of this codebase's public API).
  // PT: Retorna false (todos os ponteiros deixados nulos) se algum dos 5 símbolos falhar ao
  //     resolver -- não deveria nunca acontecer para GL 1.0/1.1 core contra qualquer driver
  //     conformante, mas verificado explicitamente em vez de assumido (espírito fail-high
  //     AUD-TEC-5, consistente com o resto da API pública deste repositório).
  bool resolve() {
    clear_color = reinterpret_cast<PFN_ClearColor>(glfwGetProcAddress("glClearColor"));
    clear = reinterpret_cast<PFN_Clear>(glfwGetProcAddress("glClear"));
    read_buffer = reinterpret_cast<PFN_ReadBuffer>(glfwGetProcAddress("glReadBuffer"));
    pixel_storei = reinterpret_cast<PFN_PixelStorei>(glfwGetProcAddress("glPixelStorei"));
    read_pixels = reinterpret_cast<PFN_ReadPixels>(glfwGetProcAddress("glReadPixels"));
    return clear_color && clear && read_buffer && pixel_storei && read_pixels;
  }
};
} // namespace

int main() {
  // ---------------------------------------------------------------------------
  // EN: Step 1 -- host creates its OWN hidden window + GL context. glintfx::UiLayer never
  //     creates a window in embed mode -- this is the "host owns the context" half of the
  //     contract (docs/embed-integration.md). GLFW_VISIBLE=false: same headless-under-Xvfb
  //     convention as glintfx/tests/ui_layer_sanity.cpp's hidden host window.
  // PT: Passo 1 -- o host cria sua PRÓPRIA janela oculta + contexto GL. glintfx::UiLayer nunca
  //     cria janela em modo embed -- esta é a metade "o host é dono do contexto" do contrato
  //     (docs/embed-integration.md). GLFW_VISIBLE=false: mesma convenção headless-sob-Xvfb da
  //     janela host oculta de glintfx/tests/ui_layer_sanity.cpp.
  // ---------------------------------------------------------------------------
  if (!glfwInit()) {
    std::puts("contract_consume FAIL: glfwInit failed");
    return 1;
  }
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* win = glfwCreateWindow(kWidth, kHeight, "consumer-example-embed", nullptr, nullptr);
  if (!win) {
    std::puts("contract_consume FAIL: glfwCreateWindow failed");
    glfwTerminate();
    return 2;
  }
  glfwMakeContextCurrent(win);

  ResolvedGl gl;
  if (!gl.resolve()) {
    std::puts("contract_consume FAIL: glfwGetProcAddress failed to resolve a GL 1.1 symbol");
    glfwDestroyWindow(win);
    glfwTerminate();
    return 6;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 2 -- the ONLY glintfx type this file names is UiLayer (from
  //     <glintfx/ui_layer.hpp>). load_gl=true: glintfx loads its own extended GL function
  //     pointers against the context this consumer just created above.
  // PT: Passo 2 -- o ÚNICO tipo da glintfx que este arquivo nomeia é UiLayer (de
  //     <glintfx/ui_layer.hpp>). load_gl=true: a glintfx carrega seus próprios ponteiros de
  //     função GL estendidos contra o contexto que este consumidor acabou de criar acima.
  // ---------------------------------------------------------------------------
  glintfx::UiLayer ui({.logical_width = kWidth, .logical_height = kHeight, .load_gl = true});
  if (!ui.ok()) {
    std::puts("contract_consume FAIL: UiLayer attach failed");
    glfwDestroyWindow(win);
    glfwTerminate();
    return 3;
  }

  // ---------------------------------------------------------------------------
  // EN: Step 3 -- host clears FBO 0 to black (known baseline), loads the scene, renders two
  //     warm-up frames (same convention as glintfx/tests/ui_layer_sanity.cpp: frame 1 settles
  //     RmlUi's internal layout/render-layer allocation, frame 2 is the stable state read
  //     back below).
  // PT: Passo 3 -- o host limpa o FBO 0 a preto (linha de base conhecida), carrega a cena,
  //     renderiza dois frames de aquecimento (mesma convenção de
  //     glintfx/tests/ui_layer_sanity.cpp: o frame 1 estabiliza a alocação interna de
  //     layout/render-layer do RmlUi, o frame 2 é o estado estável lido abaixo).
  // ---------------------------------------------------------------------------
  gl.clear_color(0.f, 0.f, 0.f, 0.f);
  gl.clear(GL_COLOR_BUFFER_BIT);

  if (!ui.load("contract_scene.rml")) {
    std::puts("contract_consume FAIL: load(contract_scene.rml) failed");
    glfwDestroyWindow(win);
    glfwTerminate();
    return 4;
  }
  ui.update();
  ui.render();
  ui.update();
  ui.render();

  // ---------------------------------------------------------------------------
  // EN: Step 4 -- read back FBO 0 and assert NON-DEGENERATE output (rendered something, not
  //     all-black) -- proves the library compiled-and-linked from OUTSIDE the glintfx source
  //     tree actually produces pixels, not just "it links". contract_scene.rcss fills the
  //     whole viewport with a single vivid solid colour (#ff3366, mean channel ~136 of 255),
  //     so a coarse brightness threshold is enough -- no llvmpipe-tolerant multi-check
  //     statistics needed (unlike ui_layer_sanity's gradient/glow scene, which this consumer
  //     deliberately does not replicate: keeping this scene a single flat fill is what makes a
  //     single threshold assertion sufficient and robust).
  // PT: Passo 4 -- lê de volta o FBO 0 e verifica saída NÃO-DEGENERADA (renderizou algo, não
  //     tudo-preto) -- prova que a lib compilada-e-linkada de FORA da árvore de fonte da
  //     glintfx realmente produz pixels, não só "linka". contract_scene.rcss preenche a
  //     viewport inteira com uma única cor sólida vívida (#ff3366, canal médio ~136 de 255),
  //     então um limiar grosseiro de brilho basta -- sem precisar das estatísticas
  //     multi-check tolerantes a llvmpipe da cena de gradiente/glow do ui_layer_sanity
  //     (deliberadamente não replicada aqui: manter esta cena um preenchimento chapado único é
  //     o que torna uma única asserção de limiar suficiente e robusta).
  // ---------------------------------------------------------------------------
  std::vector<unsigned char> px(static_cast<std::size_t>(kWidth) * kHeight * 3);
  gl.read_buffer(GL_BACK);
  gl.pixel_storei(GL_PACK_ALIGNMENT, 1);
  gl.read_pixels(0, 0, kWidth, kHeight, GL_RGB, GL_UNSIGNED_BYTE, px.data());

  const std::size_t total = static_cast<std::size_t>(kWidth) * kHeight;
  long long brightness_sum = 0;
  for (std::size_t i = 0; i < total; ++i) {
    brightness_sum += px[i * 3 + 0] + px[i * 3 + 1] + px[i * 3 + 2];
  }
  const double mean_brightness = static_cast<double>(brightness_sum) / (static_cast<double>(total) * 3.0);

  glfwDestroyWindow(win);
  glfwTerminate();

  std::printf("contract_consume: %dx%d mean_brightness=%.1f\n", kWidth, kHeight, mean_brightness);

  // EN: Threshold 50: contract_scene's #ff3366 fill yields mean_channel ~136, comfortably
  //     above any llvmpipe/Xvfb noise floor (an all-black failure would read ~0). Same
  //     methodology, coarser single threshold, as ui_layer_sanity's check 1.
  // PT: Limiar 50: o preenchimento #ff3366 da contract_scene produz mean_channel ~136,
  //     confortavelmente acima de qualquer piso de ruído do llvmpipe/Xvfb (uma falha
  //     tudo-preto leria ~0). Mesma metodologia, limiar único mais grosseiro, do cheque 1 do
  //     ui_layer_sanity.
  if (mean_brightness < 50.0) {
    std::fprintf(stderr,
                 "contract_consume FAIL: mean_brightness=%.1f < 50 -- appears all-black (render did not happen)\n",
                 mean_brightness);
    return 5;
  }

  std::puts("contract_consume: PASS");
  return 0;
}
