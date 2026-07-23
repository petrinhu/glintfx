// SPDX-License-Identifier: MPL-2.0
// EN: draw2d_hostile_sanity (D2D-1B) -- the fail-high surface of `glintfx::Draw2d` (D7/D10/D11).
//     Every case below must degrade gracefully (a false/invalid/no-op result) and MUST NOT
//     crash -- ASan-clean is the actual gate (see AGENTS.md's CI policy: this suite runs under
//     the `claudio` sanitize leg). A small, FIXED corpus (re-derived here, a DIFFERENT test
//     binary from image_decode_sanity.cpp/asset_decode_hostile_sanity.cpp -- same "not a fuzz
//     campaign" reasoning those files' own top comments give: stb_image is already one of the
//     world's most fuzzed OSS targets, this module's own code around it is a handful of lines).
//     Covers: load_texture on a hostile corpus (missing/null/empty/garbage/truncated/oversized),
//     load_texture before init(), a destroyed/tampered/foreign texture handle passed to
//     draw_sprite/destroy_texture, begin(0,0), a nested begin(), draw_sprite outside a bracket,
//     double shutdown() (explicit + via the destructor), and destroying a `Draw2d` while a
//     texture is still alive (D11's implicit teardown).
// PT: draw2d_hostile_sanity (D2D-1B) -- a superfície fail-high do `glintfx::Draw2d` (D7/D10/D11).
//     Todo caso abaixo precisa degradar graciosamente (um resultado false/inválido/no-op) e NÃO
//     PODE crashar -- ASan-limpo é o gate de fato (ver a política de CI do AGENTS.md: esta suíte
//     roda sob a perna sanitize do `claudio`). Um corpus pequeno e FIXO (re-derivado aqui, um
//     binário de teste DIFERENTE de image_decode_sanity.cpp/asset_decode_hostile_sanity.cpp --
//     mesmo racional de "não é campanha de fuzz" que os próprios comentários de topo daqueles
//     arquivos dão: o stb_image já é um dos alvos OSS mais fuzzados do mundo, o código próprio
//     deste módulo ao redor dele é um punhado de linhas). Cobre: load_texture num corpus hostil
//     (ausente/nulo/vazio/lixo/truncado/oversized), load_texture antes de init(), um handle de
//     textura destruído/adulterado/estrangeiro entregue a draw_sprite/destroy_texture,
//     begin(0,0), um begin() aninhado, draw_sprite fora de um bracket, shutdown() duplo
//     (explícito + via destrutor), e destruir um `Draw2d` com uma textura ainda viva (o teardown
//     implícito da D11).
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "gl_loader.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;
using glintfx::ColorF;
using glintfx::Draw2d;
using glintfx::RectF;
using glintfx::Texture2d;

namespace {

int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::printf("FAIL: %s\n", what);
    ++g_failures;
  }
}

bool write_file(const fs::path& path, const std::vector<unsigned char>& bytes) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) return false;
  if (!bytes.empty())
    f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return f.good();
}

// EN: A valid, tiny TGA (re-derived, same idiom as draw2d_render_sanity.cpp's own builder) --
//     used for the "still usable" leg and for the destroyed/tampered-handle cases, which need a
//     REAL texture to destroy/tamper in the first place.
// PT: Um TGA válido e minúsculo (re-derivado, mesmo idioma do próprio builder de
//     draw2d_render_sanity.cpp) -- usado na perna "ainda usável" e nos casos de handle
//     destruído/adulterado, que precisam de uma textura REAL para destruir/adulterar antes de
//     mais nada.
std::vector<unsigned char> build_tga_solid(int w, int h, unsigned char r, unsigned char g,
                                           unsigned char b) {
  std::vector<unsigned char> f;
  auto push16 = [&](int v) {
    f.push_back(static_cast<unsigned char>(v & 0xFF));
    f.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
  };
  f.push_back(0);
  f.push_back(0);
  f.push_back(2);
  push16(0);
  push16(0);
  f.push_back(0);
  push16(0);
  push16(0);
  push16(w);
  push16(h);
  f.push_back(24);
  f.push_back(0x20);
  for (int i = 0; i < w * h; ++i) {
    f.push_back(b);
    f.push_back(g);
    f.push_back(r);
  }
  return f;
}

// EN: A malformed-but-recognised-format TGA -- a well-formed 18-byte header, EXCEPT the
//     image_type byte is 99, not one of the standard values (0/1/2/3/9/10/11/32/33) stb_image's
//     TGA parser accepts. Deterministic, immediate rejection INSIDE the header parse itself,
//     unlike a merely-short pixel-data buffer: stb_image's TGA reader does not treat a short
//     pixel-data read as a hard failure (confirmed empirically -- an early draft of this corpus
//     used that shape and it decoded "successfully" from a 5-byte pixel buffer instead of
//     failing), so this is the reliable "recognised container, invalid content" case instead.
// PT: Um TGA de formato-reconhecido-mas-malformado -- um header de 18 bytes bem-formado, EXCETO
//     que o byte image_type é 99, não um dos valores padrão (0/1/2/3/9/10/11/32/33) que o parser
//     TGA do stb_image aceita. Rejeição determinística e imediata já dentro do próprio parse do
//     header, diferente de um buffer de dado de pixel meramente curto: o leitor TGA do stb_image
//     não trata uma leitura curta de dado de pixel como falha dura (confirmado empiricamente --
//     um rascunho inicial deste corpus usava essa forma e decodificou "com sucesso" a partir de
//     um buffer de pixel de 5 bytes em vez de falhar), então este é o caso confiável de
//     "container reconhecido, conteúdo inválido" no lugar.
std::vector<unsigned char> build_tga_bad_type() {
  std::vector<unsigned char> f;
  auto push16 = [&](int v) {
    f.push_back(static_cast<unsigned char>(v & 0xFF));
    f.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
  };
  f.push_back(0);
  f.push_back(0);
  f.push_back(99); // 99: not a valid TGA image_type.
  push16(0);
  push16(0);
  f.push_back(0);
  push16(0);
  push16(0);
  push16(8);
  push16(8);
  f.push_back(24);
  f.push_back(0x20);
  for (int i = 0; i < 8 * 8 * 3; ++i) f.push_back(static_cast<unsigned char>(0xAA ^ i));
  return f;
}

std::vector<unsigned char> build_garbage() {
  std::vector<unsigned char> f;
  f.reserve(1024);
  for (int i = 0; i < 1024; ++i) f.push_back(static_cast<unsigned char>((i * 131 + 17) & 0xFF));
  return f;
}

bool write_sparse_file(const fs::path& path, std::uint64_t size) {
  {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
  }
  std::error_code ec;
  fs::resize_file(path, size, ec);
  return !ec;
}

} // namespace

int main() {
  constexpr int W = 64, H = 64;
  constexpr std::uint64_t kCapBytes = 256ull * 1024ull * 1024ull;

  glintfx::WindowGlfw host;
  if (!host.create("draw2d_hostile_host", W, H)) {
    std::puts("draw2d_hostile_sanity FAIL: host window create failed");
    return 1;
  }

  char tmpl[] = "/var/tmp/glintfx_draw2d_hostile_XXXXXX";
  char* dir_c = mkdtemp(tmpl);
  if (!dir_c) {
    std::puts("draw2d_hostile_sanity FAIL: mkdtemp failed");
    return 2;
  }
  const fs::path dir(dir_c);

  // -------------------------------------------------------------------------------------------
  // Case: load_texture() BEFORE init() -- must fail-high, no crash.
  // -------------------------------------------------------------------------------------------
  {
    Draw2d d2d;
    const fs::path p = dir / "pre_init.tga";
    write_file(p, build_tga_solid(2, 2, 1, 2, 3));
    Texture2d t = d2d.load_texture(p.c_str());
    check(!t.ok(), "load_texture before init(): returns ok()==false");
    check(!d2d.ok(), "load_texture before init(): d2d itself is not ok()");
    // destroy_texture()/draw_sprite()/begin()/end() before init() must also be safe no-ops.
    d2d.destroy_texture(t);
    d2d.begin(W, H);
    d2d.draw_sprite(t, RectF{0, 0, 1, 1});
    d2d.end();
    check(true, "pre-init operations did not crash"); // reaching this line IS the proof.
  }

  Draw2d d2d;
  if (!d2d.init()) {
    std::puts("draw2d_hostile_sanity FAIL: Draw2d::init() failed");
    return 3;
  }

  // -------------------------------------------------------------------------------------------
  // Hostile corpus (D7): missing file, nullptr, empty, garbage, truncated, oversized-cap.
  // -------------------------------------------------------------------------------------------
  {
    Texture2d t = d2d.load_texture((dir / "does_not_exist.tga").c_str());
    check(!t.ok(), "hostile corpus: missing file rejected");
  }
  {
    Texture2d t = d2d.load_texture(nullptr);
    check(!t.ok(), "hostile corpus: nullptr path rejected");
  }
  {
    const fs::path p = dir / "empty.tga";
    write_file(p, {});
    Texture2d t = d2d.load_texture(p.c_str());
    check(!t.ok(), "hostile corpus: 0-byte file rejected");
  }
  {
    const fs::path p = dir / "garbage.tga";
    write_file(p, build_garbage());
    Texture2d t = d2d.load_texture(p.c_str());
    check(!t.ok(), "hostile corpus: garbage bytes rejected");
  }
  {
    const fs::path p = dir / "bad_type.tga";
    write_file(p, build_tga_bad_type());
    Texture2d t = d2d.load_texture(p.c_str());
    check(!t.ok(), "hostile corpus: TGA with an invalid image_type rejected");
  }
  {
    // Sparse file one byte past the 256 MiB cap -- must be rejected INSTANTLY (guard 1/2, D7),
    // never attempt to Seek/Read the nominal size (the sparse hole makes real disk cost ~0
    // regardless, same idiom as asset_decode_hostile_sanity.cpp's own "oversized" case).
    const fs::path p = dir / "oversized.tga";
    write_sparse_file(p, kCapBytes + 1);
    Texture2d t = d2d.load_texture(p.c_str());
    check(!t.ok(), "hostile corpus: file one byte past the 256 MiB cap rejected");
  }
  check(d2d.ok(), "hostile corpus: d2d survives the whole corpus intact");

  // -------------------------------------------------------------------------------------------
  // "Still usable" oracle -- a REAL texture must still load correctly after the hostile corpus.
  // -------------------------------------------------------------------------------------------
  const fs::path p_real = dir / "real.tga";
  write_file(p_real, build_tga_solid(4, 4, 10, 20, 30));
  Texture2d tex_real = d2d.load_texture(p_real.c_str());
  check(tex_real.ok() && tex_real.width() == 4 && tex_real.height() == 4,
        "still usable: a real texture decodes correctly after the hostile corpus");

  // -------------------------------------------------------------------------------------------
  // Destroyed/tampered/foreign handle -> draw_sprite()/destroy_texture() must fail-high, no
  // crash, no double-free.
  // -------------------------------------------------------------------------------------------
  {
    Texture2d victim = d2d.load_texture(p_real.c_str());
    check(victim.ok(), "destroyed-handle setup: victim texture loaded");
    d2d.destroy_texture(victim);
    check(!victim.ok(), "destroy_texture(): zeroes the caller's handle");

    Texture2d victim2 = d2d.load_texture(p_real.c_str());
    Texture2d stale_copy = victim2; // copy BEFORE destroying the original -- the classic UAF shape.
    d2d.destroy_texture(victim2);
    d2d.destroy_texture(stale_copy); // double-destroy via a stale copy -- must be a safe no-op.
    check(true, "double-destroy via a stale copy did not crash");
    d2d.draw_sprite(stale_copy, RectF{0, 0, 1, 1}); // draw with an already-destroyed handle.
    check(true, "draw_sprite() with a destroyed handle did not crash");

    Texture2d tampered = d2d.load_texture(p_real.c_str());
    Texture2d tampered_copy = tampered;                 // a legitimate copy, deliberately mutated below.
    d2d.destroy_texture(tampered);                      // the ORIGINAL id/generation now belong to a dead slot...
    Texture2d fresh = d2d.load_texture(p_real.c_str()); // ...reused by this new load (same slot).
    check(fresh.ok(), "tampered-handle setup: a fresh load reused the just-freed slot");
    d2d.draw_sprite(tampered_copy, RectF{0, 0, 1, 1}); // stale generation vs. the reused slot.
    check(fresh.ok(), "draw_sprite() with a stale-generation handle left the reused slot intact");
    d2d.destroy_texture(fresh);
  }

  // -------------------------------------------------------------------------------------------
  // Foreign handle -- a texture loaded by a DIFFERENT Draw2d instance, sharing the same GL
  // context, must be rejected (D7's owner-tag guarantee, not a numeric coincidence).
  // -------------------------------------------------------------------------------------------
  {
    Draw2d other;
    check(other.init(), "foreign-handle setup: second Draw2d instance init'd");
    Texture2d foreign = other.load_texture(p_real.c_str());
    check(foreign.ok(), "foreign-handle setup: texture loaded on the second instance");

    d2d.draw_sprite(foreign, RectF{0, 0, 1, 1}); // d2d does not own this handle -- must reject.
    check(d2d.ok(), "draw_sprite() with a foreign-instance handle rejected, d2d still ok()");

    // D7: "safe on ... foreign, zeroes the handle" -- d2d.destroy_texture() zeroes ITS OWN
    // out-param regardless (least-surprise: a handle you tried to destroy is never left looking
    // valid), but the ACTUAL safety property under test is that `other`'s own registry entry was
    // NEVER touched by d2d's (rejected) attempt -- proven by drawing with a copy of `foreign`
    // taken BEFORE the zeroing, still against `other`, still successfully.
    Texture2d foreign_copy_before_zero = foreign;
    d2d.destroy_texture(foreign);
    check(!foreign.ok(), "destroy_texture() with a foreign-instance handle still zeroes the out-param (D7)");
    other.begin(W, H);
    other.draw_sprite(foreign_copy_before_zero, RectF{0, 0, 1, 1});
    other.end();
    check(other.ok(), "the ORIGIN instance's own texture is untouched by d2d's rejected attempt");
    other.shutdown();
  }

  // -------------------------------------------------------------------------------------------
  // begin(0,0) -- degenerate bracket, draws must no-op until end(), no crash.
  // -------------------------------------------------------------------------------------------
  {
    d2d.begin(0, 0);
    d2d.draw_sprite(tex_real, RectF{0, 0, 10, 10});
    d2d.end();
    check(d2d.ok(), "begin(0,0): degenerate bracket survived intact");
  }

  // -------------------------------------------------------------------------------------------
  // draw_sprite() outside any bracket -- must no-op, no crash.
  // -------------------------------------------------------------------------------------------
  {
    d2d.draw_sprite(tex_real, RectF{0, 0, 10, 10});
    check(d2d.ok(), "draw_sprite() outside begin()/end() did not crash");
  }

  // -------------------------------------------------------------------------------------------
  // Nested begin() -- implicit close of the first bracket, no crash.
  // -------------------------------------------------------------------------------------------
  {
    d2d.begin(W, H);
    d2d.draw_sprite(tex_real, RectF{0, 0, 10, 10});
    d2d.begin(W, H); // nested -- implicitly flushes+closes the first bracket.
    d2d.draw_sprite(tex_real, RectF{0, 0, 10, 10});
    d2d.end();
    check(d2d.ok(), "nested begin() survived intact");
  }

  // -------------------------------------------------------------------------------------------
  // Double shutdown() (explicit) + a THIRD implicit call via the destructor -- idempotent,
  // no crash (D11).
  // -------------------------------------------------------------------------------------------
  d2d.shutdown();
  check(!d2d.ok(), "shutdown(): d2d.ok() is now false");
  d2d.shutdown(); // 2nd explicit call -- must be a no-op, not a double-free.
  check(!d2d.ok(), "double shutdown(): still false, no crash");
  // ~Draw2d() below runs the 3rd (implicit) call.

  // -------------------------------------------------------------------------------------------
  // Destroying a Draw2d while a texture is still alive -- shutdown()/the destructor must release
  // it without crashing (D11 -- no explicit destroy_texture() call before this scope ends).
  // -------------------------------------------------------------------------------------------
  {
    Draw2d d2d_live;
    check(d2d_live.init(), "live-texture-teardown setup: instance init'd");
    Texture2d t = d2d_live.load_texture(p_real.c_str());
    check(t.ok(), "live-texture-teardown setup: texture loaded, never destroyed explicitly");
    // d2d_live goes out of scope here with `t` still alive -- ~Draw2d() must clean it up.
  }
  check(true, "destroying a Draw2d with a live texture did not crash");

  std::error_code ec;
  fs::remove_all(dir, ec);

  if (g_failures == 0) {
    std::puts("draw2d_hostile_sanity: PASS");
    return 0;
  }
  std::printf("draw2d_hostile_sanity: %d check(s) FAILED\n", g_failures);
  return g_failures;
}
