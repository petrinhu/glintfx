// SPDX-License-Identifier: MPL-2.0
// EN: TST-L1-GLLEAK / GLLEAK-2 -- endurance-lite proof that glintfx does not leak GL driver
//     HANDLES (texture/FBO/renderbuffer/buffer/VAO/program/shader) under repeated document
//     reload. document_reload_leak.cpp already proves heap/document-count hygiene via
//     Rml::Context::GetNumDocuments() + ASan/LSan (TST-L1-MEM) -- but ASan operates at the
//     process heap allocator level and is BLIND to GL object names, which live in the GPU
//     driver's own namespace (glGenTextures/glDeleteTextures etc. never touch malloc/free
//     directly from the test's point of view). A driver-side handle leak is a REAL risk for a
//     long-running host (GusWorld, the first real consumer, runs for hours) that reload()s
//     documents on every menu-screen navigation -- exactly the pattern this file drives.
//
//     METHOD (TESTES.md TST-L1-GLLEAK, "endurance-lite direcionado, não soak completo"):
//     count LIVE objects per category via glIsTexture/glIsFramebuffer/glIsRenderbuffer/
//     glIsBuffer/glIsVertexArray/glIsProgram/glIsShader swept over name range [1,4096]
//     (cheap under llvmpipe -- ~28k boolean queries total per scan) before and after N=20
//     reload cycles, plus one full UiLayer destroy+recreate cycle. A LEAKING category grows
//     monotonically with N; a leak-free one returns to baseline exactly.
//
//     WARMUP: exactly 1 load+render cycle runs BEFORE the baseline scan. RmlUi lazily builds
//     caches on first use (font glyph atlas texture, compiled static geometry for the
//     document's boxes/backgrounds) that are still cold on the very first render -- without
//     this warmup cycle, that first-use allocation would look identical to a "leak" relative
//     to a scan taken before ANY render ever happened, a false positive this file must not
//     produce. Empirically (see the tolerance rationale below, updated after the first local
//     run of this file) RmlUi's caches for a document this small are fully warm after ONE
//     render -- reloading the SAME two documents (never new content/glyphs) 20x after that
//     allocates nothing further.
//
//     TOLERANCE: 0 for every category after the N-cycle loop. If a future document/asset needs
//     a >0 tolerance (a genuine lazy cache proven stable-but-nonzero), it must be measured
//     empirically (print the actual post-loop counts, run locally, read the numbers) and
//     justified IN THIS COMMENT with the category, the count, and why -- never guessed to make
//     the test pass silently.
// PT: TST-L1-GLLEAK / GLLEAK-2 -- prova endurance-lite de que a glintfx não vaza HANDLES de
//     driver GL (textura/FBO/renderbuffer/buffer/VAO/programa/shader) sob reload repetido de
//     documento. document_reload_leak.cpp já prova higiene de heap/contagem-de-documento via
//     Rml::Context::GetNumDocuments() + ASan/LSan (TST-L1-MEM) -- mas o ASan opera no nível do
//     alocador de heap do processo e é CEGO a nomes de objeto GL, que vivem no namespace do
//     PRÓPRIO driver da GPU (glGenTextures/glDeleteTextures etc. nunca tocam malloc/free
//     diretamente do ponto de vista do teste). Um leak de handle do lado do driver é um risco
//     REAL para um host de longa duração (GusWorld, o primeiro consumidor real, roda por
//     horas) que faz reload() de documentos a cada navegação de tela de menu -- exatamente o
//     padrão que este arquivo exercita.
//
//     MÉTODO (TESTES.md TST-L1-GLLEAK, "endurance-lite direcionado, não soak completo"): conta
//     objetos VIVOS por categoria via glIsTexture/glIsFramebuffer/glIsRenderbuffer/glIsBuffer/
//     glIsVertexArray/glIsProgram/glIsShader varrendo o range de nomes [1,4096] (barato sob
//     llvmpipe -- ~28 mil queries booleanas por varredura) antes e depois de N=20 ciclos de
//     reload, mais um ciclo completo de destroy+recreate do UiLayer. Uma categoria VAZANDO
//     cresce monotonicamente com N; uma sem leak volta ao baseline exatamente.
//
//     WARMUP: exatamente 1 ciclo de load+render roda ANTES da varredura de baseline. O RmlUi
//     constrói caches preguiçosamente no primeiro uso (atlas de glifos de fonte, geometria
//     estática compilada das caixas/backgrounds do documento) ainda frios no primeiríssimo
//     render -- sem este ciclo de warmup, essa alocação de primeiro-uso pareceria idêntica a
//     um "leak" frente a uma varredura tirada antes de QUALQUER render acontecer, um falso
//     positivo que este arquivo não deve produzir. Empiricamente (ver a justificativa de
//     tolerância abaixo, atualizada após a primeira rodada local deste arquivo) os caches do
//     RmlUi para um documento deste tamanho já estão totalmente quentes após UM render --
//     recarregar os MESMOS dois documentos (nunca conteúdo/glifo novo) 20x depois disso não
//     aloca mais nada.
//
//     TOLERÂNCIA: 0 para toda categoria após o loop de N ciclos. Se um documento/asset futuro
//     precisar de tolerância >0 (um cache preguiçoso genuíno provado estável-mas-não-zero),
//     deve ser medido empiricamente (imprimir as contagens reais pós-loop, rodar localmente,
//     ler os números) e justificado NESTE COMENTÁRIO com a categoria, a contagem e o motivo --
//     nunca chutado para fazer o teste passar em silêncio.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp" // EN: includes gl_loader.h. PT: inclui gl_loader.h.
#include <cstdio>
#include <memory>

namespace {

// EN: Name range swept per category. Cheap under llvmpipe; comfortably above the handful of
//     objects a small test document + the glintfx engine itself ever allocate.
// PT: Range de nomes varrido por categoria. Barato sob llvmpipe; folgadamente acima do punhado
//     de objetos que um documento de teste pequeno + o próprio motor glintfx chegam a alocar.
constexpr GLuint kMaxName = 4096;

struct LiveCounts {
  int textures = 0;
  int framebuffers = 0;
  int renderbuffers = 0;
  int buffers = 0;
  int vertex_arrays = 0;
  int programs = 0;
  int shaders = 0;
};

// EN: Sweep [1, kMaxName] and count live objects per category. glIsTexture/glIsProgram/
//     glIsShader are defined to only ever return GL_TRUE for a name that is a CURRENTLY VALID
//     object of that exact type -- a deleted-and-reused name, or a name never generated, both
//     correctly read back false. This makes the sweep an exact live-object census, not a
//     high-water-mark proxy.
// PT: Varre [1, kMaxName] e conta objetos vivos por categoria. glIsTexture/glIsProgram/
//     glIsShader são definidos para só retornar GL_TRUE para um nome que é ATUALMENTE um
//     objeto válido daquele tipo exato -- um nome deletado-e-reusado, ou um nome nunca gerado,
//     ambos corretamente retornam false. Isso torna a varredura um censo exato de objetos
//     vivos, não um proxy de marca d'água.
LiveCounts scan_live_objects() {
  LiveCounts c;
  for (GLuint name = 1; name <= kMaxName; ++name) {
    if (glIsTexture(name)) ++c.textures;
    if (glIsFramebuffer(name)) ++c.framebuffers;
    if (glIsRenderbuffer(name)) ++c.renderbuffers;
    if (glIsBuffer(name)) ++c.buffers;
    if (glIsVertexArray(name)) ++c.vertex_arrays;
    if (glIsProgram(name)) ++c.programs;
    if (glIsShader(name)) ++c.shaders;
  }
  return c;
}

// EN: One reload-on-navigate cycle: load a document, drive the frame (update() actually
//     destroys the PREVIOUSLY loaded document -- see document_reload_leak.cpp's own comment on
//     Close()/UnloadDocument() deferring to the next Context::Update()), then render() so any
//     GL resources the newly-shown document needs (geometry, textures) are actually realised,
//     not just parsed.
// PT: Um ciclo de reload-on-navigate: carrega um documento, dirige o frame (update() de fato
//     destrói o documento CARREGADO ANTERIORMENTE -- ver o próprio comentário de
//     document_reload_leak.cpp sobre Close()/UnloadDocument() diferirem para o próximo
//     Context::Update()), depois render() para que quaisquer recursos GL que o documento
//     recém-mostrado precise (geometria, texturas) sejam de fato realizados, não só parseados.
bool reload_cycle(glintfx::UiLayer& ui, const char* path) {
  if (!ui.load(path)) return false;
  ui.update();
  ui.render();
  return true;
}

// EN: Prints one row of the before/after table; returns true if the category diverged from
//     baseline (a leak/growth) or shrank (an over-eager double-free/UAF, also a bug).
// PT: Imprime uma linha da tabela antes/depois; retorna true se a categoria divergiu do
//     baseline (leak/crescimento) ou encolheu (double-free/UAF precipitado, também um bug).
bool report_row(const char* name, int before, int after) {
  std::printf("gl_resource_leak [%-13s]: baseline=%d post-cycles=%d %s\n",
              name, before, after, before == after ? "OK" : "MISMATCH");
  return before != after;
}

} // namespace

int main() {
  // EN: Host creates a hidden window + current GL context (same fixture pattern as
  //     gl_state_guard.cpp/document_reload_leak.cpp).
  // PT: Host cria janela oculta + contexto GL corrente (mesmo padrão de fixture de
  //     gl_state_guard.cpp/document_reload_leak.cpp).
  glintfx::WindowGlfw host;
  if (!host.create("gl_resource_leak_host", 320, 240)) {
    std::puts("FAIL: host window create failed");
    return 1;
  }

  auto ui = std::make_unique<glintfx::UiLayer>(
      glintfx::UiLayerConfig{.logical_width = 320, .logical_height = 240, .load_gl = true});
  if (!ui->ok()) {
    std::puts("FAIL: ui attach failed");
    return 2;
  }

  // EN: Two distinct, already-present documents (same pair document_reload_leak.cpp reuses) --
  //     no new scene assets needed, and reusing the SAME two documents across all 20 cycles is
  //     the point: it proves reload does not leak, without a second variable (new content)
  //     that could itself allocate new, legitimately-kept resources every cycle.
  // PT: Dois documentos distintos já existentes (mesmo par que document_reload_leak.cpp
  //     reutiliza) -- nenhum asset de cena novo necessário, e reutilizar os MESMOS dois
  //     documentos nos 20 ciclos é o ponto: prova que o reload não vaza, sem uma segunda
  //     variável (conteúdo novo) que poderia ela mesma alocar recursos novos e legitimamente
  //     mantidos a cada ciclo.
  constexpr const char* kDocA = "tests/min.rml";
  constexpr const char* kDocB = "tests/min_partial.rml";

  // EN: WARMUP -- 1 cycle, see the file-level comment above for why this must run BEFORE the
  //     baseline scan.
  // PT: WARMUP -- 1 ciclo, ver o comentário de nível de arquivo acima sobre por que isto deve
  //     rodar ANTES da varredura de baseline.
  if (!reload_cycle(*ui, kDocA)) {
    std::puts("FAIL: warmup load/render failed");
    return 3;
  }

  const LiveCounts baseline = scan_live_objects();
  std::printf(
      "gl_resource_leak: baseline after 1 warmup cycle -- tex=%d fbo=%d rbo=%d buf=%d "
      "vao=%d prog=%d shader=%d\n",
      baseline.textures, baseline.framebuffers, baseline.renderbuffers, baseline.buffers,
      baseline.vertex_arrays, baseline.programs, baseline.shaders);

  // EN: N=20 reload-on-navigate cycles, alternating A/B -- the exact GusWorld menu-navigation
  //     pattern (document_reload_leak.cpp's own top comment), repeated enough times that a
  //     genuine per-cycle leak (even a single handle) would show up as a clearly nonzero delta,
  //     not noise.
  // PT: N=20 ciclos de reload-on-navigate, alternando A/B -- o padrão exato de navegação de
  //     menu do GusWorld, repetido vezes suficientes para que um leak genuíno por ciclo (mesmo
  //     um único handle) apareça como um delta claramente não-zero, não ruído.
  constexpr int kCycles = 20;
  for (int i = 0; i < kCycles; ++i) {
    const char* path = (i % 2 == 0) ? kDocB : kDocA;
    if (!reload_cycle(*ui, path)) {
      std::printf("FAIL: reload cycle #%d (%s) failed\n", i, path);
      return 4;
    }
  }

  // EN: Destroy + recreate the WHOLE UiLayer (not just a document reload) -- the harshest
  //     lifecycle event a host can put the engine through (e.g. GusWorld tearing down and
  //     rebuilding its embed layer across a full scene change). Everything the destroyed
  //     UiLayer owned (RmlUi context, glintfx's own GL3 render interface objects: shaders,
  //     VAOs, buffers) must be released; the recreated instance re-establishes its own set from
  //     scratch, which the final scan below folds into the same baseline comparison.
  // PT: Destrói + recria o UiLayer INTEIRO (não só um reload de documento) -- o evento de
  //     lifecycle mais duro que um host pode impor ao motor (ex.: GusWorld desmontando e
  //     reconstruindo sua camada embed numa troca de cena completa). Tudo que o UiLayer
  //     destruído possuía (contexto RmlUi, objetos próprios da interface de render GL3 da
  //     glintfx: shaders, VAOs, buffers) deve ser liberado; a instância recriada reestabelece o
  //     próprio conjunto do zero, o que a varredura final abaixo já inclui na mesma comparação
  //     com o baseline.
  ui.reset();
  ui = std::make_unique<glintfx::UiLayer>(
      glintfx::UiLayerConfig{.logical_width = 320, .logical_height = 240, .load_gl = true});
  if (!ui->ok()) {
    std::puts("FAIL: ui re-attach after destroy failed");
    return 5;
  }
  if (!reload_cycle(*ui, kDocA)) {
    std::puts("FAIL: post-recreate load/render failed");
    return 6;
  }

  const LiveCounts after = scan_live_objects();
  std::printf(
      "gl_resource_leak: post-cycles -- tex=%d fbo=%d rbo=%d buf=%d vao=%d prog=%d "
      "shader=%d\n",
      after.textures, after.framebuffers, after.renderbuffers, after.buffers,
      after.vertex_arrays, after.programs, after.shaders);

  bool mismatch = false;
  mismatch |= report_row("textures", baseline.textures, after.textures);
  mismatch |= report_row("framebuffers", baseline.framebuffers, after.framebuffers);
  mismatch |= report_row("renderbuffers", baseline.renderbuffers, after.renderbuffers);
  mismatch |= report_row("buffers", baseline.buffers, after.buffers);
  mismatch |= report_row("vertex_arrays", baseline.vertex_arrays, after.vertex_arrays);
  mismatch |= report_row("programs", baseline.programs, after.programs);
  mismatch |= report_row("shaders", baseline.shaders, after.shaders);

  ui.reset();

  if (mismatch) {
    std::puts(
        "gl_resource_leak: FAIL -- live GL object count diverged from baseline after "
        "20 reload cycles + 1 UiLayer destroy/recreate cycle (see MISMATCH row(s) above)");
    return 7;
  }
  std::puts("gl_resource_leak: PASS");
  return 0;
}
