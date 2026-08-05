// SPDX-License-Identifier: Apache-2.0
// EN: REVIEW-ONLY scratch test (RMLX-0/F1 adversarial review, qa-engineer, 2026-08-05) --
//     EXERCISES glintfx/src/rml/type_bridge.hpp, which had ZERO consumer/test as of e7506a2
//     (confirmed: `grep -rl type_bridge glintfx/` only matched doc-comments in
//     input_map.hpp/bootstrap.hpp, never an #include). No RmlUi document, no GL context, no
//     window -- pure value-type round-trip + field-order + rounding checks. NOT part of the
//     accepted F1 deliverable; written to PROVE the conversions are correct (or find the
//     silent-coordinate/channel-swap bug class the brief warned about), not to leave permanent
//     test debt on a header the implementer explicitly deferred consumers for ("fio F2 em
//     diante"). Left in the tree for the orchestrator to keep, adapt, or drop.
// PT: Teste-rascunho SÓ-DE-REVIEW (review adversarial RMLX-0/F1, qa-engineer, 2026-08-05) --
//     EXERCITA glintfx/src/rml/type_bridge.hpp, que tinha ZERO consumidor/teste no e7506a2
//     (confirmado: `grep -rl type_bridge glintfx/` só casou doc-comments em
//     input_map.hpp/bootstrap.hpp, nunca um #include). Sem documento RmlUi, sem contexto GL,
//     sem janela -- só checagens de ida-e-volta de tipo-valor + ordem de campo + arredondamento.
//     NÃO faz parte do entregável aceito da F1; escrito pra PROVAR que as conversões estão
//     corretas (ou achar a classe de bug silencioso de troca de coordenada/canal que o brief
//     avisou), não pra deixar dívida de teste permanente num header que o implementer
//     deliberadamente adiou consumidores ("fio F2 em diante"). Deixado na árvore para o
//     orquestrador manter, adaptar ou descartar.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/rml/type_bridge.hpp"

#include <cstdio>
#include <cstdlib>

namespace {

int g_failures = 0;

#define CHECK(cond, msg)                                                   \
  do {                                                                     \
    if (!(cond)) {                                                         \
      std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
      ++g_failures;                                                        \
    }                                                                      \
  } while (0)

// EN: Vec2F -> Rml::Vector2f field ORDER (not just size/type) -- static_assert in type_bridge.hpp
//     only proves size==8 + standard-layout, NOT that x maps to x and y maps to y. Distinct
//     values on x/y so a swap bug flips the check, not slips through by coincidence.
// PT: ORDEM de campo Vec2F -> Rml::Vector2f (não só tamanho/tipo) -- o static_assert em
//     type_bridge.hpp só prova sizeof==8 + standard-layout, NÃO que x mapeia para x e y para y.
//     Valores distintos em x/y para que um bug de troca derrube a checagem, não escape por
//     coincidência.
void check_vec2_order_and_roundtrip() {
  glintfx::Vec2F v{3.0f, 7.0f};
  Rml::Vector2f r = glintfx::to_rml(v);
  CHECK(r.x == 3.0f, "to_rml(Vec2F): x did not map to Rml::Vector2f.x");
  CHECK(r.y == 7.0f, "to_rml(Vec2F): y did not map to Rml::Vector2f.y");

  glintfx::Vec2F back = glintfx::from_rml(r);
  CHECK(back.x == 3.0f && back.y == 7.0f, "Vec2F round-trip (to_rml . from_rml) is not identity");

  Rml::Vector2f r2{11.0f, -4.5f};
  glintfx::Vec2F v2 = glintfx::from_rml(r2);
  CHECK(v2.x == 11.0f, "from_rml(Vector2f): x did not map to Vec2F.x");
  CHECK(v2.y == -4.5f, "from_rml(Vector2f): y did not map to Vec2F.y");
}

// EN: ColorF -> Rml::Colourb CHANNEL order -- distinct r/g/b/a so a swap (e.g. to_rml writing
//     (g,r,b,a) into the constructor's (red,green,blue,alpha) params) fails loudly instead of
//     passing on a grey/symmetric test color.
// PT: ORDEM de canal ColorF -> Rml::Colourb -- r/g/b/a distintos para que uma troca (ex.: to_rml
//     escrevendo (g,r,b,a) nos parâmetros (red,green,blue,alpha) do construtor) falhe alto em vez
//     de passar numa cor de teste cinza/simétrica.
void check_color_channel_order() {
  glintfx::ColorF c{40.f / 255.f, 80.f / 255.f, 120.f / 255.f, 200.f / 255.f};
  Rml::Colourb rc = glintfx::to_rml(c);
  CHECK(rc.red == 40, "to_rml(ColorF): red channel wrong (order swap?)");
  CHECK(rc.green == 80, "to_rml(ColorF): green channel wrong (order swap?)");
  CHECK(rc.blue == 120, "to_rml(ColorF): blue channel wrong (order swap?)");
  CHECK(rc.alpha == 200, "to_rml(ColorF): alpha channel wrong (order swap?)");
}

// EN: Exhaustive byte round-trip: for every possible Rml::Colourb channel value (0..255),
//     from_rml then to_rml must reproduce the EXACT same byte. This is the property that would
//     catch an off-by-one in the *255.f + lround + clamp pipeline (truncation vs rounding,
//     wrong clamp bound, etc) that a handful of hand-picked values could miss.
// PT: Ida-e-volta exaustiva de byte: para todo valor possível de canal de Rml::Colourb
//     (0..255), from_rml seguido de to_rml tem que reproduzir o MESMO byte exato. Esta é a
//     propriedade que pegaria um off-by-one no pipeline *255.f + lround + clamp (truncamento vs
//     arredondamento, limite de clamp errado etc) que uns poucos valores escolhidos a dedo
//     poderiam deixar passar.
void check_color_byte_roundtrip_exhaustive() {
  for (int byte = 0; byte <= 255; ++byte) {
    Rml::Colourb original(static_cast<unsigned char>(byte), 0, 0, 255);
    glintfx::ColorF as_float = glintfx::from_rml(original);
    Rml::Colourb back = glintfx::to_rml(as_float);
    if (back.red != byte) {
      std::fprintf(stderr, "FAIL: byte round-trip broken at channel value %d -> got %d (%s:%d)\n",
                   byte, static_cast<int>(back.red), __FILE__, __LINE__);
      ++g_failures;
    }
  }
}

// EN: from_rml division is explicit /255.f per channel (per the header's own doc-comment) --
//     confirm 255 (not premultiplied/clamped weirdly) maps to exactly 1.0f, and 0 maps to
//     exactly 0.0f (the two values with no floating-point rounding ambiguity at all).
// PT: A divisão de from_rml é /255.f explícito por canal (conforme o próprio doc-comment do
//     header) -- confirma que 255 (não premultiplicado/clampado de forma estranha) mapeia para
//     exatamente 1.0f, e 0 mapeia para exatamente 0.0f (os dois valores sem ambiguidade nenhuma
//     de arredondamento de ponto flutuante).
void check_color_endpoints() {
  glintfx::ColorF white_channel = glintfx::from_rml(Rml::Colourb(255, 255, 255, 255));
  CHECK(white_channel.r == 1.0f && white_channel.g == 1.0f && white_channel.b == 1.0f &&
            white_channel.a == 1.0f,
        "from_rml(255,255,255,255) did not map to ColorF{1,1,1,1} exactly");

  glintfx::ColorF black_channel = glintfx::from_rml(Rml::Colourb(0, 0, 0, 0));
  CHECK(black_channel.r == 0.0f && black_channel.g == 0.0f && black_channel.b == 0.0f &&
            black_channel.a == 0.0f,
        "from_rml(0,0,0,0) did not map to ColorF{0,0,0,0} exactly");
}

// EN: MUTATION-TESTING-DRIVEN ADDITION (RMLX-0/F1 review) -- the exhaustive byte round-trip
//     above (check_color_byte_roundtrip_exhaustive) only feeds to_rml() inputs of the exact
//     form `byte/255.f` (i.e. values that already came out of from_rml()), and a planted mutant
//     that replaces `std::lround(x*255.f)` with plain truncation `(unsigned char)(x*255.f)`
//     SURVIVED it -- every `k/255.f*255.f` in float32 happened to round back to `k` under
//     truncation too, for all 256 values, so that test cannot observe the rounding-vs-
//     truncation difference the header's own doc-comment claims to defend against ("nearest,
//     not truncated"). A ColorF that is NOT derived from from_rml() -- e.g. produced by
//     blending/interpolation -- can land exactly on a half-integer boundary, where the two
//     algorithms diverge by definition. x=0.5f is the sharpest such case: 0.5f*255.f == 127.5f
//     EXACTLY in float32 (127.5 = 255/2, exactly representable), so std::lround must yield 128
//     (round-half-AWAY-FROM-ZERO) while plain truncation yields 127. This is the test that
//     should have been there from the start; written here as a direct consequence of a mutant
//     surviving, not from re-reading the source.
// PT: ADIÇÃO DIRIGIDA POR MUTATION TESTING (review RMLX-0/F1) -- o round-trip exaustivo de byte
//     acima (check_color_byte_roundtrip_exhaustive) só alimenta to_rml() com entradas na forma
//     exata `byte/255.f` (ou seja, valores que já saíram de from_rml()), e um mutante plantado
//     que troca `std::lround(x*255.f)` por truncamento puro `(unsigned char)(x*255.f)`
//     SOBREVIVEU a ele -- todo `k/255.f*255.f` em float32 por acaso arredondou de volta pra `k`
//     também sob truncamento, nos 256 valores, então aquele teste não consegue observar a
//     diferença arredondamento-vs-truncamento que o próprio doc-comment do header afirma
//     defender ("nearest, not truncated"). Um ColorF que NÃO vem de from_rml() -- ex.: produzido
//     por blend/interpolação -- pode cair exatamente numa fronteira de meio-inteiro, onde os
//     dois algoritmos divergem por definição. x=0.5f é o caso mais afiado: 0.5f*255.f ==
//     127.5f EXATAMENTE em float32 (127.5 = 255/2, exatamente representável), então
//     std::lround tem que dar 128 (arredonda PRA-LONGE-DE-ZERO no meio) enquanto truncamento
//     puro dá 127. Este é o teste que deveria ter existido desde o início; escrito aqui como
//     consequência direta de um mutante sobrevivente, não de reler o source.
void check_color_rounding_not_truncation() {
  glintfx::ColorF half{0.5f, 0.5f, 0.5f, 0.5f};
  Rml::Colourb rc = glintfx::to_rml(half);
  CHECK(rc.red == 128,
        "to_rml(0.5f): red channel is 127 (truncated) instead of 128 (rounded) -- "
        "lround regressed to truncation?");
  CHECK(rc.green == 128,
        "to_rml(0.5f): green channel is 127 (truncated) instead of 128 (rounded)");
  CHECK(rc.blue == 128, "to_rml(0.5f): blue channel is 127 (truncated) instead of 128 (rounded)");
  CHECK(rc.alpha == 128,
        "to_rml(0.5f): alpha channel is 127 (truncated) instead of 128 (rounded)");
}

} // namespace

int main() {
  check_vec2_order_and_roundtrip();
  check_color_channel_order();
  check_color_byte_roundtrip_exhaustive();
  check_color_endpoints();
  check_color_rounding_not_truncation();

  if (g_failures != 0) {
    std::fprintf(stderr, "type_bridge_review_sanity: %d check(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("PASS");
  return 0;
}
