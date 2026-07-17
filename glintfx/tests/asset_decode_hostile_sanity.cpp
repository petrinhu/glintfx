// SPDX-License-Identifier: MPL-2.0
// EN: asset_decode_hostile_sanity (AUD-L1-PARSE / TST-L1-DECODE) -- corpus test for malformed/
//     hostile asset files across BOTH the image path (Gl3RenderInterface::LoadTexture,
//     render_gl3.cpp) and the font path (FontEngineOwn::LoadFontFace / RmlUi's own default
//     FreeType FontProvider::LoadFontFace, both funneled through the single AUD-L1-PARSE size
//     cap in glintfx::BaseUrlFileInterface::Open() -- src/base_url_file_interface.hpp -- added
//     after a 2026-07-16 adversarial review found the font path had no cap of its own; see that
//     class's own top comment for the full single-point-of-enforcement design). Deliberately NOT
//     a fuzz campaign (CTO decision, LW-AUD scope): stb_image is one of the most fuzzed open-
//     source targets in the world (OSS-Fuzz), and glintfx's own code around the size-cap
//     mechanism is a handful of lines. This is a small, FIXED corpus of seven representative
//     hostile shapes (five image-format-malformation cases + two size-DoS discriminator cases,
//     one per asset kind), each proving the oracle: the document still LOADS (a broken <img>/
//     @font-face degrades that ONE resource, not the whole document -- see RmlUi's own base
//     LoadTexture, Backends/RmlUi_Renderer_GL3.cpp, which returns `false`/0 on failure, never
//     aborts), the engine does not crash/UB/OOM, and -- critically -- the UiLayer stays fully
//     usable for the NEXT document afterward (proven PIXEL-LEVEL by a final real-PNG load+decode
//     check, reusing circle_rgba.png and the exact red-pixel-count oracle texture_png_alpha.cpp's
//     own Check A already proves for that fixture -- not merely "did not crash").
//
//     The five image-format-malformation cases:
//       1. png_truncated -- valid PNG signature + valid small (4x4 RGBA) IHDR, then an IDAT chunk
//          that declares more bytes than actually follow before EOF. Exercises stb_image's
//          "outofdata" short-read path (stb_image.h's stbi__getn returning 0 mid-IDAT).
//       2. garbage -- 4096 bytes matching no known image-format magic. Exercises the "unknown
//          image type" rejection path (fails every stb_image format sniff in turn).
//       3. huge_dims -- valid PNG signature + a bare IHDR claiming 100000x100000 RGBA (1e10
//          pixels, ~40 GiB raw) and NOTHING else (no IDAT needed -- see below). Exercises
//          stb_image's own overflow-safe size check, `(1<<30)/img_x/img_n < img_y`
//          (stb_image.h ~line 5121, inside the IHDR chunk case of stbi__parse_png_file): this
//          fires and `return`s BEFORE the chunk loop ever advances to read IDAT, so no IDAT/CRC
//          bytes are needed for this file to be a complete, well-formed test case -- the failure
//          is deterministic and immediate, no multi-gigabyte allocation is ever attempted.
//       4. empty -- 0-byte file. Exercises `len == 0` through the whole LoadTexture path (an
//          empty std::vector<unsigned char> buf(0), then stbi_load_from_memory with a null/empty
//          buffer, both of which must fail gracefully, not read out of bounds).
//       5. oversized -- a SPARSE file one byte past AUD-L1-PARSE's own 256 MiB file-size cap
//          (render_gl3.cpp's `kMaxAssetFileBytes`, private to Gl3RenderInterface -- this test
//          re-derives the same literal value rather than reaching into that private constant, so
//          if the two ever drift the DELTA is exactly what this case would stop catching, not a
//          silent pass). `std::filesystem::resize_file` grows the file via a hole (no real disk
//          blocks allocated for the unwritten range on any sane Linux filesystem -- ext4/btrfs/
//          xfs all support sparse files), so this test consumes ~0 real disk despite the file's
//          *nominal* 256 MiB+1 size. Written under /var/tmp (NOT /tmp -- tmpfs on this machine,
//          see the project's own CLAUDE.md gotcha), though a sparse hole would cost ~0 either way.
//          Exercises the cap-rejection path itself (LoadTexture's guard 1/2): the load must be
//          instant (no attempt to Seek/Read the actual 256 MiB+1 bytes) and must NOT fall through
//          to RmlUi's own un-capped base TGA loader (the specific regression this audit wave's fix
//          closes -- see render_gl3.cpp's own "DELIBERATELY DOES NOT delegate" comment on that
//          guard for the full why).
//
//     Two size-DoS DISCRIMINATOR cases (added 2026-07-16, adversarial review IMPORTANTE #1/#2):
//       6. img_huge_1tib -- same run_case() path as #5 above, but at kHugeFileBytes (2^40+1, see
//          that constant's own comment for why 256 MiB+1 in case #5 is a BOUNDARY-VALUE proof but
//          NOT a real regression guard -- a 256 MiB allocation does not crash an unguarded
//          allocator either, so removing the guard would NOT flip case #5 red). Removing EITHER
//          the central Open()-level cap (base_url_file_interface.hpp) OR render_gl3.cpp's own
//          image-specific guard still leaves this case green (the other layer catches it) --
//          confirmed by mutation testing during this case's own development; only removing BOTH
//          simultaneously reproduces the crash the original adversarial review found.
//       7. font_huge_1tib -- run_font_case() (a "@font-face" probe, not an "<img>" one) at the
//          same kHugeFileBytes size. This is the case that actually discriminates the NEW central
//          guard in glintfx::BaseUrlFileInterface::Open(): the font path has NO OTHER guard, so
//          removing that one guard and rebuilding under GLINTFX_SANITIZE=ON makes this specific
//          case abort under ASan's own allocation-size-too-big trap (confirmed by mutation
//          testing -- see this file's own commit message / the AUD-L1-PARSE PR notes for the
//          exact before/after ctest transcript).
//
//     Runs in BOTH build configs (embed-mode, UiLayer via a WindowGlfw GL-context fixture -- same
//     idiom as input_hardening_sanity.cpp/ripple_sanity.cpp). No GL-entry-point spy needed here
//     (unlike ripple_sanity) -- this test's oracle is "did not crash + still decodes real assets
//     afterward", not a specific GL call count; links ${_embed_dep} glloader (glloader only for
//     offscreen.hpp's <gl_loader.h> include, same pair as input_hardening_sanity.cpp).
// PT: asset_decode_hostile_sanity (AUD-L1-PARSE / TST-L1-DECODE) -- teste de corpus para
//     arquivos de asset malformados/hostis nos DOIS caminhos: imagem (Gl3RenderInterface::
//     LoadTexture, render_gl3.cpp) e fonte (FontEngineOwn::LoadFontFace / o próprio
//     FontProvider::LoadFontFace default do FreeType do RmlUi, ambos canalizados pelo teto único
//     AUD-L1-PARSE em glintfx::BaseUrlFileInterface::Open() -- src/base_url_file_interface.hpp
//     -- adicionado depois de uma review adversarial de 2026-07-16 achar que o caminho de fonte
//     não tinha teto próprio nenhum; ver o próprio comentário do topo daquela classe pro design
//     completo de ponto-único-de-aplicação). Deliberadamente NÃO uma campanha de fuzz (decisão do
//     CTO, escopo LW-AUD): o stb_image é um dos alvos open-source mais fuzzados do mundo
//     (OSS-Fuzz), e o código próprio da glintfx ao redor do mecanismo de teto de tamanho é um
//     punhado de linhas. Este é um corpus pequeno e FIXO de sete formas hostis representativas
//     (cinco casos de malformação de formato de imagem + dois casos discriminadores de DoS de
//     tamanho, um por tipo de asset), cada uma provando o oráculo: o documento ainda CARREGA (um
//     <img>/@font-face quebrado degrada SÓ aquele recurso, não o documento inteiro -- ver o
//     próprio LoadTexture da base do RmlUi, Backends/RmlUi_Renderer_GL3.cpp, que retorna
//     `false`/0 em falha, nunca aborta), o engine não crasha/UB/OOM, e -- criticamente -- a
//     UiLayer permanece totalmente usável para o PRÓXIMO documento depois (provado a NÍVEL DE
//     PIXEL por um check final de load+decode de PNG real, reusando circle_rgba.png e o mesmo
//     oráculo de contagem de pixel vermelho que o próprio Check A de texture_png_alpha.cpp já
//     prova pra essa fixture -- não só "não crashou").
//
//     Os cinco casos de malformação de imagem: ver os parágrafos EN acima (espelhados aqui em
//     conceito) -- PNG truncado no meio do IDAT, lixo puro, dimensões absurdas (100000x100000,
//     rejeitado dentro do próprio parse do IHDR antes de qualquer IDAT ser necessário), arquivo
//     0-byte, e um arquivo esparso um byte acima do teto de 256 MiB do AUD-L1-PARSE (consome ~0
//     disco real via hole).
//
//     Os dois casos discriminadores de DoS de tamanho (adicionados 2026-07-16, review adversarial
//     IMPORTANTE #1/#2): ver os parágrafos EN acima (espelhados aqui em conceito) --
//     img_huge_1tib (mesmo caminho de run_case() do caso #5, mas em kHugeFileBytes = 2^40+1,
//     porque 256 MiB+1 é pequeno demais pra crashar um alocador sem guarda nenhuma -- não
//     discrimina "guarda presente" de "guarda removida") e font_huge_1tib (run_font_case(), a
//     ÚNICA guarda do caminho de fonte é a nova guarda central do Open(), então removê-la sob
//     GLINTFX_SANITIZE=ON faz este caso especificamente abortar sob a trap de
//     allocation-size-too-big do ASan -- confirmado por mutation testing).
//
//     Roda em AMBAS as configs de build (embed-mode, UiLayer via fixture WindowGlfw de contexto
//     GL -- mesmo idioma de input_hardening_sanity.cpp/ripple_sanity.cpp). Sem espião de GL aqui
//     (diferente de ripple_sanity) -- o oráculo deste teste é "não crashou + ainda decodifica
//     assets reais depois", não uma contagem específica de chamada GL; linka ${_embed_dep}
//     glloader (glloader só pelo <gl_loader.h> de offscreen.hpp, mesmo par de
//     input_hardening_sanity.cpp).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "offscreen.hpp"  // EN: includes gl_loader.h (GL already loaded by UiLayer ctor).
                          // PT: inclui gl_loader.h (GL já carregado pelo ctor da UiLayer).
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

// EN: Re-derivation (not a shared header) of render_gl3.cpp's private kMaxAssetFileBytes -- see
//     this file's top comment on why a drift between the two is caught (not silently masked).
// PT: Re-derivação (não um header compartilhado) do kMaxAssetFileBytes privado de render_gl3.cpp
//     -- ver o comentário do topo deste arquivo pro porquê de um desalinhamento entre os dois ser
//     capturado (não mascarado silenciosamente).
constexpr uint64_t kAssetCapBytes = 256ull * 1024ull * 1024ull;

// EN: A size deliberately WAY past any legitimate cap (2^40 = 1 TiB) -- unlike the boundary-value
//     "oversized" case above (256 MiB+1, which is too SMALL to itself crash an allocator even with
//     ZERO guard present -- allocating 256 MiB just... succeeds, gracefully falls through to a
//     decode-failure path either way, so that case alone cannot DISCRIMINATE "guard present" from
//     "guard silently removed", the false-negative this review round's IMPORTANTE #2 found), 1 TiB
//     is chosen specifically to sit at/above AddressSanitizer's own hard-coded allocator ceiling
//     (compiler-rt's allocator refuses any single request beyond roughly this order of magnitude
//     with "requested allocation size ... exceeds maximum supported size", aborting the process
//     immediately -- confirmed empirically during this test's own development under
//     GLINTFX_SANITIZE=ON: a std::vector<unsigned char>/Rml::String/`new byte[]` of this size
//     aborts under ASan with NO guard present, and is rejected gracefully in ~0ms WITH the guard
//     present). A std::filesystem::resize_file() sparse hole makes the on-disk cost of this
//     ~0 real bytes regardless (see write_sparse_file()) -- confirmed empirically: `du` on a
//     freshly resize_file()'d 1 TiB+1 file reports 0 real blocks on this machine's ext4 /var/tmp.
// PT: Um tamanho deliberadamente MUITO além de qualquer teto legítimo (2^40 = 1 TiB) -- diferente
//     do caso de valor-de-fronteira "oversized" acima (256 MiB+1, que é PEQUENO demais pra crashar
//     um alocador sozinho mesmo com ZERO guarda presente -- alocar 256 MiB simplesmente... tem
//     sucesso, cai graciosamente num caminho de falha de decode de qualquer jeito, então aquele
//     caso sozinho não consegue DISCRIMINAR "guarda presente" de "guarda removida
//     silenciosamente", o falso-negativo que o IMPORTANTE #2 desta rodada de review achou), 1 TiB
//     é escolhido especificamente por ficar no/acima do teto hard-coded do próprio alocador do
//     AddressSanitizer (o alocador do compiler-rt recusa qualquer requisição única além
//     aproximadamente desta ordem de grandeza com "requested allocation size ... exceeds maximum
//     supported size", abortando o processo imediatamente -- confirmado empiricamente durante o
//     desenvolvimento deste teste sob GLINTFX_SANITIZE=ON: um std::vector<unsigned char>/
//     Rml::String/`new byte[]` deste tamanho aborta sob ASan SEM a guarda presente, e é rejeitado
//     graciosamente em ~0ms COM a guarda presente). Um hole de std::filesystem::resize_file() faz
//     o custo em disco disto ~0 bytes reais de qualquer forma (ver write_sparse_file()) --
//     confirmado empiricamente: `du` num arquivo de 1 TiB+1 recém resize_file()'d reporta 0 blocos
//     reais no ext4 desta máquina em /var/tmp.
constexpr uint64_t kHugeFileBytes = (1ull << 40) + 1;  // 1 TiB + 1 byte.

void put_be32(std::vector<unsigned char>& v, uint32_t x) {
  v.push_back(static_cast<unsigned char>((x >> 24) & 0xFF));
  v.push_back(static_cast<unsigned char>((x >> 16) & 0xFF));
  v.push_back(static_cast<unsigned char>((x >> 8) & 0xFF));
  v.push_back(static_cast<unsigned char>(x & 0xFF));
}

void put_tag(std::vector<unsigned char>& v, const char (&tag)[5]) {
  v.insert(v.end(), tag, tag + 4);
}

const unsigned char kPngSig[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

// EN: A well-formed IHDR chunk (length + "IHDR" + 13-byte body + a dummy CRC -- stb_image never
//     validates PNG chunk CRCs, see stb_image.h's own "end of PNG chunk, read and skip CRC").
// PT: Um chunk IHDR bem-formado (comprimento + "IHDR" + corpo de 13 bytes + um CRC-dummy --
//     stb_image nunca valida CRCs de chunk PNG, ver o próprio "end of PNG chunk, read and skip
//     CRC" de stb_image.h).
std::vector<unsigned char> build_ihdr_chunk(uint32_t w, uint32_t h, uint8_t depth, uint8_t color) {
  std::vector<unsigned char> data;
  put_be32(data, w);
  put_be32(data, h);
  data.push_back(depth);
  data.push_back(color);
  data.push_back(0);  // compression method
  data.push_back(0);  // filter method
  data.push_back(0);  // interlace method

  std::vector<unsigned char> chunk;
  put_be32(chunk, static_cast<uint32_t>(data.size()));  // == 13
  put_tag(chunk, "IHDR");
  chunk.insert(chunk.end(), data.begin(), data.end());
  put_be32(chunk, 0);  // dummy CRC, never checked
  return chunk;
}

bool write_file(const fs::path& path, const std::vector<unsigned char>& bytes) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f)
    return false;
  if (!bytes.empty())
    f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return f.good();
}

// EN: Case 1 -- PNG truncated mid-IDAT. Signature + valid 4x4 RGBA IHDR + an IDAT chunk header
//     declaring length=100 but only 10 bytes follow before EOF (no CRC, no IEND).
// PT: Caso 1 -- PNG truncado no meio do IDAT. Assinatura + IHDR válido 4x4 RGBA + um header de
//     chunk IDAT declarando length=100 mas só 10 bytes seguem antes do EOF (sem CRC, sem IEND).
std::vector<unsigned char> build_png_truncated() {
  std::vector<unsigned char> f(kPngSig, kPngSig + 8);
  const auto ihdr = build_ihdr_chunk(4, 4, 8, 6);
  f.insert(f.end(), ihdr.begin(), ihdr.end());
  put_be32(f, 100);
  put_tag(f, "IDAT");
  for (int i = 0; i < 10; ++i)
    f.push_back(static_cast<unsigned char>(0xAA ^ i));
  return f;
}

// EN: Case 2 -- pure garbage, matches no known image-format magic.
// PT: Caso 2 -- lixo puro, não casa com nenhuma magic de formato de imagem conhecido.
std::vector<unsigned char> build_garbage() {
  std::vector<unsigned char> f;
  f.reserve(4096);
  for (int i = 0; i < 4096; ++i)
    f.push_back(static_cast<unsigned char>((i * 167 + 31) & 0xFF));
  return f;
}

// EN: Case 3 -- absurd declared dimensions (100000x100000 RGBA, ~40 GiB raw). No IDAT needed --
//     see this file's top comment for why stb_image rejects this from inside IHDR parsing alone.
// PT: Caso 3 -- dimensões declaradas absurdas (100000x100000 RGBA, ~40 GiB brutos). Sem IDAT
//     necessário -- ver o comentário do topo pro porquê do stb_image rejeitar isso já dentro do
//     parse do IHDR sozinho.
std::vector<unsigned char> build_huge_dims() {
  std::vector<unsigned char> f(kPngSig, kPngSig + 8);
  const auto ihdr = build_ihdr_chunk(100000, 100000, 8, 6);
  f.insert(f.end(), ihdr.begin(), ihdr.end());
  return f;
}

// EN: Case 5 (oversized) helper -- grows a file to `size` bytes via a hole (sparse), consuming
//     ~0 real disk on ext4/btrfs/xfs. NOT std::filesystem::resize_file on a fresh empty file --
//     that call requires the file to already exist, so an empty file is created first.
// PT: Helper do caso 5 (oversized) -- cresce um arquivo para `size` bytes via um hole (esparso),
//     consumindo ~0 disco real em ext4/btrfs/xfs. Cria um arquivo vazio primeiro, pois
//     std::filesystem::resize_file exige que o arquivo já exista.
bool write_sparse_file(const fs::path& path, uint64_t size) {
  {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f)
      return false;
  }
  std::error_code ec;
  fs::resize_file(path, size, ec);
  return !ec;
}

// EN: Minimal RML document referencing a single asset (hostile or real) by path, at a
//     caller-chosen dp size. `img_path` is always the asset's BARE FILENAME (never a leading-'/'
//     absolute path -- see run_case's own comment below for why: RmlUi's SystemInterface::
//     JoinPath strips a leading '/' instead of resolving it, so a bare filename resolved
//     relative to the .rml document's own directory is the only reliable way to reference a
//     scratch-dir asset that is not on the process's WORKING_DIRECTORY).
// PT: Documento RML mínimo referenciando um único asset (hostil ou real) por path, num tamanho
//     dp escolhido pelo chamador. `img_path` é sempre o NOME DE ARQUIVO PURO do asset (nunca um
//     path absoluto com '/' inicial -- ver o próprio comentário de run_case abaixo pro porquê: o
//     SystemInterface::JoinPath do RmlUi remove uma '/' inicial em vez de resolvê-la, então um
//     nome de arquivo puro resolvido relativo ao próprio diretório do documento .rml é a única
//     forma confiável de referenciar um asset de scratch-dir que não está no WORKING_DIRECTORY do
//     processo). O check final
//     de asset real (ver main() abaixo) usa um nome relativo simples resolvido via
//     WORKING_DIRECTORY.
bool write_probe_rml(const fs::path& rml_path, const fs::path& img_path, int dp_size) {
  std::ofstream f(rml_path, std::ios::trunc);
  if (!f)
    return false;
  f << "<rml>\n<head>\n<style>\n"
       "body { background: #101010; margin: 0; padding: 0; }\n"
       "img { width: "
    << dp_size << "dp; height: " << dp_size
    << "dp; }\n"
       "</style>\n<title>asset_decode_hostile probe</title>\n</head>\n<body>\n"
       "<img id=\"probe\" src=\""
    << img_path.string() << "\" />\n</body>\n</rml>\n";
  return f.good();
}

// EN: RGB frame readback -- same idiom as input_hardening_sanity.cpp's capture()/count_bright().
// PT: Readback de frame RGB -- mesmo idioma de capture()/count_bright() de
//     input_hardening_sanity.cpp.
std::vector<unsigned char> capture(glintfx::UiLayer& ui, int w, int h) {
  ui.update();
  ui.render();
  std::vector<unsigned char> px(static_cast<size_t>(w) * h * 3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  return px;
}

// EN: Count pixels matching circle_rgba.png's known warm-red circle colour (R=220,G=80,B=80 --
//     same channel bounds texture_png_alpha.cpp's own Check A uses for the identical fixture).
// PT: Conta pixels que casam com a cor vermelha quente conhecida do círculo de
//     circle_rgba.png (R=220,G=80,B=80 -- mesmos limites de canal que o próprio Check A de
//     texture_png_alpha.cpp usa para a fixture idêntica).
size_t count_red(const std::vector<unsigned char>& px, int w, int h) {
  size_t n = 0;
  const size_t total = static_cast<size_t>(w) * h;
  for (size_t i = 0; i < total; ++i)
    if (px[i * 3 + 0] > 150 && px[i * 3 + 1] < 100 && px[i * 3 + 2] < 100)
      ++n;
  return n;
}

// EN: Minimal RML document declaring a "@font-face" pointing at `font_path` (bare filename, same
//     document-relative-resolution reasoning as write_probe_rml's own comment applies here too --
//     RmlUi's StyleSheetParser calls Rml::LoadFontFace() DIRECTLY when it parses the "@font-face"
//     at-rule (Source/Core/StyleSheetParser.cpp), REGARDLESS of whether any element in the
//     document actually uses that font-family -- no text content is needed to trigger the load).
// PT: Documento RML mínimo declarando um "@font-face" apontando pra `font_path` (nome de arquivo
//     puro, mesmo racional de resolução relativa-ao-documento do próprio comentário de
//     write_probe_rml também se aplica aqui -- o StyleSheetParser do RmlUi chama
//     Rml::LoadFontFace() DIRETO quando parseia a at-rule "@font-face" (Source/Core/
//     StyleSheetParser.cpp), INDEPENDENTE de algum elemento do documento de fato usar essa
//     font-family -- nenhum conteúdo de texto é necessário pra disparar o load).
bool write_font_probe_rml(const fs::path& rml_path, const fs::path& font_path) {
  std::ofstream f(rml_path, std::ios::trunc);
  if (!f)
    return false;
  f << "<rml>\n<head>\n<style>\n"
       "@font-face { font-family: \"HostileFont\"; src: \""
    << font_path.string()
    << "\"; }\n"
       "body { background: #101010; margin: 0; padding: 0; }\n"
       "</style>\n<title>asset_decode_hostile font probe</title>\n</head>\n<body>\n</body>\n</rml>\n";
  return f.good();
}

}  // namespace

int main() {
  constexpr int W = 200, H = 200;

  glintfx::WindowGlfw host;
  if (!host.create("asset_decode_hostile_host", W, H)) {
    std::puts("asset_decode_hostile_sanity FAIL: host window create failed");
    return 1;
  }

  glintfx::UiLayer ui({.logical_width = W, .logical_height = H, .load_gl = true});
  if (!ui.ok()) {
    std::puts("asset_decode_hostile_sanity FAIL: ui attach failed");
    return 2;
  }

  // EN: Scratch dir under /var/tmp -- NOT /tmp (tmpfs on this dev machine; see this project's
  //     CLAUDE.md gotcha on build/scratch placement). Sparse files make the disk-usage concern
  //     moot for case 5 specifically, but every fixture lives here for one consistent policy.
  // PT: Dir de scratch sob /var/tmp -- NÃO /tmp (tmpfs nesta máquina de dev; ver o gotcha do
  //     CLAUDE.md deste projeto sobre onde colocar build/scratch). Arquivos esparsos tornam a
  //     preocupação de uso de disco irrelevante especificamente pro caso 5, mas toda fixture vive
  //     aqui por uma política única e consistente.
  char tmpl[] = "/var/tmp/glintfx_hostile_XXXXXX";
  char* tmp_dir_c = mkdtemp(tmpl);
  if (!tmp_dir_c) {
    std::puts("asset_decode_hostile_sanity FAIL: mkdtemp(/var/tmp/glintfx_hostile_XXXXXX) failed");
    return 3;
  }
  const fs::path tmp_dir(tmp_dir_c);
  std::printf("asset_decode_hostile_sanity: scratch dir = %s\n", tmp_dir.c_str());

  int failures = 0;

  // EN: Runs one hostile case end-to-end: write the asset fixture + a probe .rml referencing it,
  //     load the document, render two frames, and assert (a) the document itself still loaded
  //     (a broken texture must degrade the <img>, not the whole document) and (b) the UiLayer
  //     is still `ok()` afterward (no crash/abort reached this line at all, plus no silent
  //     internal-state corruption). Fixtures are removed after each case (best-effort hygiene;
  //     not asserted -- a leftover fixture in a CI-ephemeral /var/tmp is not itself a bug).
  // PT: Roda um caso hostil ponta-a-ponta: escreve a fixture de asset + um .rml de sonda
  //     referenciando-a, carrega o documento, renderiza dois frames, e verifica (a) que o
  //     documento em si ainda carregou (uma textura quebrada deve degradar o <img>, não o
  //     documento inteiro) e (b) que a UiLayer ainda está `ok()` depois (nenhum crash/abort
  //     alcançou esta linha, mais nenhuma corrupção silenciosa de estado interno). Fixtures são
  //     removidas após cada caso (higiene best-effort; não verificada -- uma fixture remanescente
  //     num /var/tmp efêmero de CI não é em si um bug).
  auto run_case = [&](const char* name, const std::vector<unsigned char>* bytes, uint64_t sparse_size) {
    const fs::path asset_path = tmp_dir / (std::string(name) + ".bin");
    const bool wrote = sparse_size != 0 ? write_sparse_file(asset_path, sparse_size)
                                         : write_file(asset_path, *bytes);
    if (!wrote) {
      std::printf("asset_decode_hostile_sanity FAIL [%s]: could not write asset fixture at '%s'\n",
                  name, asset_path.c_str());
      ++failures;
      return;
    }

    const fs::path rml_path = tmp_dir / (std::string(name) + ".rml");
    // EN: Reference the asset by BARE FILENAME, not the full absolute path -- RmlUi's own
    //     SystemInterface::JoinPath (Source/Core/SystemInterface.cpp) STRIPS the leading '/' off
    //     an absolute `src` and returns it AS-IS otherwise (no join with the document's own
    //     directory), turning "/var/tmp/x/y.bin" into the bogus relative path "var/tmp/x/y.bin"
    //     -- which then fails to open relative to the PROCESS's cwd, never even reaching
    //     Gl3RenderInterface::LoadTexture with the file we meant to test (confirmed by
    //     instrumentation during this test's own development). A bare filename hits JoinPath's
    //     OTHER branch instead: it takes the directory of `document_path` (the ABSOLUTE .rml path
    //     this test passes to ui.load() below) and appends the bare filename, correctly
    //     reconstructing the absolute path to the asset written right next to that .rml.
    // PT: Referencia o asset pelo NOME DE ARQUIVO PURO, não o path absoluto completo -- o próprio
    //     SystemInterface::JoinPath do RmlUi (Source/Core/SystemInterface.cpp) REMOVE a '/' inicial
    //     de um `src` absoluto e o retorna COMO-ESTÁ (sem juntar com o diretório do documento),
    //     transformando "/var/tmp/x/y.bin" no path relativo bogus "var/tmp/x/y.bin" -- que então
    //     falha ao abrir relativo ao cwd do PROCESSO, nunca chegando a
    //     Gl3RenderInterface::LoadTexture com o arquivo que pretendíamos testar (confirmado por
    //     instrumentação durante o desenvolvimento deste teste). Um nome de arquivo puro cai no
    //     OUTRO ramo do JoinPath: pega o diretório de `document_path` (o path .rml ABSOLUTO que
    //     este teste passa a ui.load() abaixo) e concatena o nome de arquivo puro, reconstruindo
    //     corretamente o path absoluto do asset escrito ao lado desse .rml.
    if (!write_probe_rml(rml_path, asset_path.filename(), /*dp_size=*/32)) {
      std::printf("asset_decode_hostile_sanity FAIL [%s]: could not write probe .rml\n", name);
      ++failures;
      return;
    }

    const bool ok_load = ui.load(rml_path.c_str());
    if (!ok_load) {
      std::printf("asset_decode_hostile_sanity FAIL [%s]: document load itself failed -- a "
                  "hostile <img> asset must degrade the TEXTURE, not the whole document\n", name);
      ++failures;
    } else {
      ui.update();
      ui.render();
      ui.update();
      ui.render();
      if (!ui.ok()) {
        std::printf("asset_decode_hostile_sanity FAIL [%s]: ui.ok() went false after rendering a "
                    "hostile asset -- the engine did not survive intact\n", name);
        ++failures;
      } else {
        std::printf("asset_decode_hostile_sanity [%s]: loaded+rendered, no crash, ui.ok()=true\n",
                    name);
      }
    }

    std::error_code ec;
    fs::remove(asset_path, ec);
    fs::remove(rml_path, ec);
  };

  // EN: Font-path counterpart of run_case above -- IMPORTANTE #1 (2026-07-16 adversarial review):
  //     "@font-face" loads a font file with NO cap of its own (FontEngineOwn::LoadFontFace via
  //     the base Rml::FileInterface::LoadFile(), and RmlUi's own default FreeType
  //     FontProvider::LoadFontFace) -- closed centrally in
  //     glintfx::BaseUrlFileInterface::Open() (src/base_url_file_interface.hpp, see that class's
  //     own top comment for the full design/why-a-single-point rationale), not per-engine. This
  //     lambda proves that closure end-to-end through whichever font engine is active at runtime
  //     in this build (the guard is engine-agnostic by construction -- it lives below BOTH
  //     engines, at the file layer both of them share).
  // PT: Contraparte de fonte do run_case acima -- IMPORTANTE #1 (review adversarial de
  //     2026-07-16): "@font-face" carrega um arquivo de fonte SEM teto próprio nenhum
  //     (FontEngineOwn::LoadFontFace via o LoadFile() base de Rml::FileInterface, e o próprio
  //     FontProvider::LoadFontFace default do FreeType do RmlUi) -- fechado centralmente em
  //     glintfx::BaseUrlFileInterface::Open() (src/base_url_file_interface.hpp, ver o próprio
  //     comentário do topo daquela classe pro racional completo de por-que-um-ponto-único), não
  //     por-motor. Esta lambda prova esse fechamento ponta-a-ponta através de qualquer motor de
  //     fonte ativo em runtime neste build (a guarda é agnóstica de motor por construção -- vive
  //     abaixo dos DOIS motores, na camada de arquivo que ambos compartilham).
  auto run_font_case = [&](const char* name, uint64_t sparse_size) {
    const fs::path font_path = tmp_dir / (std::string(name) + ".bin");
    if (!write_sparse_file(font_path, sparse_size)) {
      std::printf("asset_decode_hostile_sanity FAIL [%s]: could not write font fixture at '%s'\n",
                  name, font_path.c_str());
      ++failures;
      return;
    }

    const fs::path rml_path = tmp_dir / (std::string(name) + ".rml");
    // EN: Unlike <img src=...> (JoinPath'd document-relative -- see run_case's own comment),
    //     "@font-face { src: ...; }" is passed to Rml::LoadFontFace() COMPLETELY RAW: RmlUi's
    //     StyleSheetParser::ParseFontFaceBlock (Source/Core/StyleSheetParser.cpp) reads `src` via
    //     FontFacePropertyParser (which only StringUtilities::ExpandString()s the comma-separated
    //     value -- no path joining at all) and hands it straight to Rml::LoadFontFace(src, ...),
    //     which itself does nothing but forward to the font engine's LoadFontFace(file_path, ...)
    //     -- confirmed by reading that call chain end-to-end during this test's own development
    //     (a bare filename here resolved relative to the PROCESS's cwd, not this scratch dir,
    //     silently missing the fixture and producing a false-pass with the guard never even
    //     exercised). The ABSOLUTE `font_path` therefore has to be used AS-IS here (not
    //     `.filename()` like run_case's <img> probe) -- and works directly because this
    //     particular chain never strips the leading '/' the way JoinPath does for images.
    // PT: Diferente de <img src=...> (resolvido document-relative via JoinPath -- ver o próprio
    //     comentário de run_case), "@font-face { src: ...; }" é passado pro Rml::LoadFontFace()
    //     COMPLETAMENTE CRU: o StyleSheetParser::ParseFontFaceBlock do RmlUi (Source/Core/
    //     StyleSheetParser.cpp) lê `src` via FontFacePropertyParser (que só faz
    //     StringUtilities::ExpandString() do valor separado por vírgula -- nenhum join de path)
    //     e entrega direto pro Rml::LoadFontFace(src, ...), que por si só só encaminha pro
    //     LoadFontFace(file_path, ...) do motor de fonte -- confirmado lendo essa cadeia de
    //     chamada ponta-a-ponta durante o desenvolvimento deste teste (um nome de arquivo puro
    //     aqui resolveria relativo ao cwd do PROCESSO, não este scratch dir, perdendo a fixture
    //     silenciosamente e produzindo um falso-positivo com a guarda nunca sequer exercitada).
    //     O `font_path` ABSOLUTO portanto precisa ser usado COMO-ESTÁ aqui (não `.filename()`
    //     como o probe <img> de run_case) -- e funciona direto porque esta cadeia específica
    //     nunca remove a '/' inicial do jeito que o JoinPath faz para imagens.
    if (!write_font_probe_rml(rml_path, font_path)) {
      std::printf("asset_decode_hostile_sanity FAIL [%s]: could not write font probe .rml\n", name);
      ++failures;
      return;
    }

    const bool ok_load = ui.load(rml_path.c_str());
    if (!ok_load) {
      std::printf("asset_decode_hostile_sanity FAIL [%s]: document load itself failed -- a "
                  "hostile @font-face asset must degrade the FONT, not the whole document\n", name);
      ++failures;
    } else {
      ui.update();
      ui.render();
      ui.update();
      ui.render();
      if (!ui.ok()) {
        std::printf("asset_decode_hostile_sanity FAIL [%s]: ui.ok() went false after rendering a "
                    "hostile @font-face asset -- the engine did not survive intact\n", name);
        ++failures;
      } else {
        std::printf("asset_decode_hostile_sanity [%s]: loaded+rendered, no crash, ui.ok()=true\n",
                    name);
      }
    }

    std::error_code ec;
    fs::remove(font_path, ec);
    fs::remove(rml_path, ec);
  };

  {
    const auto bytes = build_png_truncated();
    run_case("png_truncated", &bytes, 0);
  }
  {
    const auto bytes = build_garbage();
    run_case("garbage", &bytes, 0);
  }
  {
    const auto bytes = build_huge_dims();
    run_case("huge_dims", &bytes, 0);
  }
  {
    const std::vector<unsigned char> bytes;  // 0-byte file
    run_case("empty", &bytes, 0);
  }
  {
    run_case("oversized", nullptr, kAssetCapBytes + 1);
  }
  {
    // EN: IMPORTANTE #2 -- the real discriminating regression guard (see kHugeFileBytes's own
    //     comment for why 256 MiB+1 above cannot serve this purpose).
    // PT: IMPORTANTE #2 -- a guarda de regressão que de fato discrimina (ver o próprio
    //     comentário de kHugeFileBytes pro porquê do 256 MiB+1 acima não servir pra isso).
    run_case("img_huge_1tib", nullptr, kHugeFileBytes);
  }
  {
    // EN: IMPORTANTE #1 + #2 combined -- the font path's ONLY guard (central Open()-level,
    //     base_url_file_interface.hpp), exercised at a size that actually discriminates.
    // PT: IMPORTANTE #1 + #2 combinados -- a ÚNICA guarda do caminho de fonte (nível Open()
    //     central, base_url_file_interface.hpp), exercitada num tamanho que de fato discrimina.
    run_font_case("font_huge_1tib", kHugeFileBytes);
  }

  std::error_code ec;
  fs::remove(tmp_dir, ec);

  // ===========================================================================================
  // EN: "the lib continues usable" oracle -- after five hostile documents, a REAL PNG must still
  //     decode correctly, PIXEL-PROVEN (not just "render() did not crash" -- a texture pipeline
  //     that silently wedged itself into permanent fallback after the hostile corpus would still
  //     pass a load()+ok() check with zero decoded pixels). Reuses circle_rgba.png -- already
  //     copied FLAT into CMAKE_CURRENT_BINARY_DIR by the echo_sanity block in
  //     tests/CMakeLists.txt (same fixture texture_png_alpha.cpp's own Check A proves decodes to
  //     a warm-red circle, R=220 G=80 B=80) -- via a fresh probe.rml at 64dp (1:1 with the
  //     fixture's own 64x64 pixel size at the default dp_ratio=1.0, matching
  //     texture_png_alpha.cpp's own sizing exactly). WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR
  //     (repo-wide embed-test convention) resolves "circle_rgba.png" by bare relative name.
  // PT: oráculo "a lib continua usável" -- depois de cinco documentos hostis, um PNG REAL ainda
  //     precisa decodificar corretamente, PROVADO POR PIXEL (não só "render() não crashou" -- um
  //     pipeline de textura que silenciosamente travou em fallback permanente após o corpus
  //     hostil ainda passaria um check de load()+ok() com zero pixels decodificados). Reusa
  //     circle_rgba.png -- já copiado PLANO em CMAKE_CURRENT_BINARY_DIR pelo bloco echo_sanity em
  //     tests/CMakeLists.txt (mesma fixture cujo próprio Check A de texture_png_alpha.cpp prova
  //     decodificar pra um círculo vermelho quente, R=220 G=80 B=80) -- via um probe.rml fresco
  //     em 64dp (1:1 com o tamanho de pixel próprio 64x64 da fixture no dp_ratio=1.0 default,
  //     casando com o próprio dimensionamento de texture_png_alpha.cpp exatamente).
  //     WORKING_DIRECTORY = CMAKE_CURRENT_BINARY_DIR (convenção repo-wide de teste embed) resolve
  //     "circle_rgba.png" por nome relativo simples.
  // ===========================================================================================
  if (!write_probe_rml("post_hostile_probe.rml", fs::path("circle_rgba.png"), /*dp_size=*/64)) {
    std::puts("asset_decode_hostile_sanity FAIL: could not write post_hostile_probe.rml");
    return 4;
  }
  if (!ui.load("post_hostile_probe.rml")) {
    std::puts("asset_decode_hostile_sanity FAIL: post-hostile real-asset load failed -- the "
              "engine's document pipeline did not survive the hostile corpus");
    return 5;
  }
  capture(ui, W, H);  // warm-up frame, same idiom as input_hardening_sanity.cpp/texture_png_alpha.cpp.
  const auto px = capture(ui, W, H);
  if (!ui.ok()) {
    std::puts("asset_decode_hostile_sanity FAIL: ui.ok() false after the post-hostile real-asset "
              "render -- the engine did not survive intact");
    return 6;
  }
  const size_t red = count_red(px, W, H);
  std::printf("asset_decode_hostile_sanity: post-hostile real-asset red_count=%zu (expect > 500, "
              "same threshold/fixture texture_png_alpha.cpp's own Check A uses)\n", red);
  if (red < 500) {
    std::printf("asset_decode_hostile_sanity FAIL: post-hostile real-asset PNG did not decode "
                "(red_count=%zu < 500) -- the texture pipeline did not survive the hostile "
                "corpus intact, even though no crash occurred\n", red);
    return 7;
  }
  std::puts("asset_decode_hostile_sanity: post-hostile real-asset decoded correctly -- engine "
            "remains fully usable");

  if (failures == 0) {
    std::puts("asset_decode_hostile_sanity: PASS");
    return 0;
  }
  std::printf("asset_decode_hostile_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
