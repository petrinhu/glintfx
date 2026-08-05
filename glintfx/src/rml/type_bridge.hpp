// SPDX-License-Identifier: Apache-2.0
// EN: Internal header (src/rml/-only, NOT under glintfx/include/glintfx/ -- the include-based
//     encapsulation gate (tools/check_encapsulation.sh) does not cover this file) -- the ONLY
//     place in glintfx that converts between the library's own public value types
//     (glintfx::Vec2F/ColorF, glintfx/draw2d.hpp) and RmlUi's equivalent types
//     (Rml::Vector2f/Colourb). RMLX-0 (TODO.md): this is the anticorruption seam the whole
//     RMLX-1..11 arc (eliminating RmlUi) narrows against -- every glintfx/-vs-RmlUi type
//     boundary should eventually route through here, so a future RmlUi swap-out touches ONE
//     file's worth of conversion logic, not N call sites scattered across src/rml/.
//
//     The static_asserts below are the actual point of this header, more than the two tiny
//     conversion functions: they are a LAYOUT TRIPWIRE against a future RmlUi version bump. If
//     upstream ever changes Vector2f's or Colourb's field count/type/order, these asserts fail
//     the BUILD at the exact place that assumed the old layout, instead of silently
//     miscompiling a coordinate or a channel swap that only shows up as a visual bug at
//     runtime (the failure mode AUD-L1-QUALITY and the auditoria-domino discipline both exist
//     to catch BEFORE it reaches a consumer -- see the house's feedback_auditoria_domino memory
//     for the rule this generalises: "validate float before cast" and "does this class of bug
//     have a sibling nobody checked").
//
//     Two type pairs, two directions each (four functions total) -- deliberately NOT a
//     templated/generic bridge: RmlUi's own `Rml::String` is confirmed (Config.h:108) to be a
//     bare `using String = std::string`, so no alias is created for it (would be pure noise);
//     `Rml::Variant` is a non-trivial class with small-string-optimisation storage and stays
//     entirely confined to src/rml/ callers that already include RmlUi headers directly -- it
//     is NOT replicated or wrapped here, on purpose (a value-type bridge for a non-trivial
//     class would just be a second implementation of Variant to keep in sync forever).
// PT: Header interno (só-src/rml/, NÃO sob glintfx/include/glintfx/ -- o gate include-based de
//     encapsulamento (tools/check_encapsulation.sh) não cobre este arquivo) -- o ÚNICO lugar na
//     glintfx que converte entre os tipos-valor públicos da própria lib (glintfx::Vec2F/ColorF,
//     glintfx/draw2d.hpp) e os tipos equivalentes do RmlUi (Rml::Vector2f/Colourb). RMLX-0
//     (TODO.md): esta é a costura anticorrupção contra a qual o arco inteiro RMLX-1..11
//     (eliminar o RmlUi) se estreita -- toda fronteira de tipo glintfx/-vs-RmlUi deveria
//     eventualmente passar por aqui, para que uma futura troca do RmlUi toque UM arquivo de
//     lógica de conversão, não N sítios de chamada espalhados por src/rml/.
//
//     Os static_asserts abaixo são o ponto de fato deste header, mais até que as duas funções
//     de conversão minúsculas: são um FIO DE TROPEÇO DE LAYOUT contra um futuro bump de versão
//     do RmlUi. Se o upstream algum dia mudar a contagem/tipo/ordem de campo de Vector2f ou
//     Colourb, estes asserts derrubam o BUILD exatamente no lugar que assumia o layout antigo,
//     em vez de miscompilar silenciosamente uma coordenada ou uma troca de canal que só aparece
//     como bug visual em runtime (o modo de falha que a disciplina AUD-L1-QUALITY e
//     auditoria-dominó existem pra pegar ANTES de chegar num consumidor -- ver a memória da casa
//     feedback_auditoria_domino pra regra que isto generaliza: "validar float antes do cast" e
//     "essa classe de bug tem um irmão que ninguém checou").
//
//     Dois pares de tipo, duas direções cada (quatro funções no total) -- deliberadamente NÃO
//     uma ponte genérica/templada: o próprio `Rml::String` do RmlUi é confirmado (Config.h:108)
//     como um `using String = std::string` cru, então nenhum alias é criado para ele (seria
//     ruído puro); `Rml::Variant` é uma classe não-trivial com armazenamento small-string-
//     optimisation e fica inteiramente confinada aos chamadores de src/rml/ que já incluem os
//     headers do RmlUi diretamente -- NÃO é replicada nem envolvida aqui, de propósito (uma
//     ponte de tipo-valor para uma classe não-trivial seria só uma segunda implementação de
//     Variant pra manter sincronizada para sempre).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include <glintfx/draw2d.hpp> // EN: glintfx::Vec2F / glintfx::ColorF. PT: glintfx::Vec2F / glintfx::ColorF.

#include <RmlUi/Core/Types.h> // EN: Rml::Vector2f / Rml::Colourb. PT: Rml::Vector2f / Rml::Colourb.

#include <algorithm> // EN: std::clamp (to_rml's straight-to-byte quantisation). PT: std::clamp (quantizacao straight->byte do to_rml).
#include <cmath>     // EN: std::lround (nearest-byte rounding, not truncation). PT: std::lround (arredondamento pro byte mais proximo, nao truncamento).
#include <type_traits>

namespace glintfx {

// EN: Layout tripwire (see file header). sizeof + is_standard_layout together confirm both
//     types are still "2 floats" / "4 bytes" with no vtable/padding surprise -- if either
//     fails, a future RmlUi bump changed something this file's conversions assume.
// PT: Fio de tropeço de layout (ver cabeçalho do arquivo). sizeof + is_standard_layout juntos
//     confirmam que os dois tipos ainda são "2 floats" / "4 bytes" sem surpresa de
//     vtable/padding -- se algum falhar, um futuro bump do RmlUi mudou algo que as conversões
//     deste arquivo assumem.
static_assert(sizeof(Rml::Vector2f) == 8,
              "RmlUi Vector2f layout changed -- glintfx type_bridge.hpp assumes 2 floats (8 bytes)");
static_assert(std::is_standard_layout_v<Rml::Vector2f>,
              "RmlUi Vector2f is no longer standard-layout -- glintfx type_bridge.hpp's field-copy "
              "conversion needs re-checking");
static_assert(sizeof(Rml::Colourb) == 4,
              "RmlUi Colourb layout changed -- glintfx type_bridge.hpp assumes 4 bytes (r,g,b,a)");
static_assert(std::is_standard_layout_v<Rml::Colourb>,
              "RmlUi Colourb is no longer standard-layout -- glintfx type_bridge.hpp's field-copy "
              "conversion needs re-checking");

// EN: glintfx::Vec2F -> Rml::Vector2f. Field-by-field copy (not reinterpret_cast/memcpy) --
//     cheap enough (2 floats) that a defensive, self-documenting copy costs nothing measurable,
//     and it does not rely on field ORDER matching (the static_asserts above only guarantee
//     size/layout class, not that `x` is Rml::Vector2f's first member).
// PT: glintfx::Vec2F -> Rml::Vector2f. Cópia campo a campo (não reinterpret_cast/memcpy) --
//     barato o bastante (2 floats) que uma cópia defensiva e autoexplicativa não custa nada
//     mensurável, e não depende da ORDEM de campo bater (os static_asserts acima só garantem
//     tamanho/classe de layout, não que `x` seja o primeiro membro de Rml::Vector2f).
inline Rml::Vector2f to_rml(Vec2F v) { return Rml::Vector2f{v.x, v.y}; }

// EN: Rml::Vector2f -> glintfx::Vec2F.
// PT: Rml::Vector2f -> glintfx::Vec2F.
inline Vec2F from_rml(Rml::Vector2f v) { return Vec2F{v.x, v.y}; }

// EN: glintfx::ColorF (straight, non-premultiplied, [0,1] per channel -- see draw2d.hpp's own
//     doc-comment) -> Rml::Colourb (straight, [0,255] byte per channel). Explicit *255.f +
//     std::lround (nearest, not truncated) + std::clamp: ColorF's own intake already clamps to
//     [0,1] (draw2d.hpp), but this conversion clamps AGAIN defensively at the byte boundary --
//     cheap, and it means this function has no UB/wraparound path even if a future caller
//     constructs a ColorF by aggregate-init bypassing that intake clamp.
// PT: glintfx::ColorF (straight, não-premultiplicado, [0,1] por canal -- ver o doc-comment do
//     próprio draw2d.hpp) -> Rml::Colourb (straight, byte [0,255] por canal). *255.f explícito +
//     std::lround (mais próximo, não truncado) + std::clamp: a própria entrada do ColorF já
//     clampa em [0,1] (draw2d.hpp), mas esta conversão clampa DE NOVO defensivamente na
//     fronteira do byte -- barato, e significa que esta função não tem caminho de UB/wraparound
//     mesmo que um chamador futuro construa um ColorF por aggregate-init pulando aquele clamp de
//     entrada.
inline Rml::Colourb to_rml(ColorF c) {
  auto channel = [](float x) -> unsigned char {
    return static_cast<unsigned char>(std::lround(std::clamp(x, 0.f, 1.f) * 255.f));
  };
  return Rml::Colourb(channel(c.r), channel(c.g), channel(c.b), channel(c.a));
}

// EN: Rml::Colourb -> glintfx::ColorF. Explicit /255.f per channel (per RMLX-0's own brief --
//     no implicit int/float division, no shared "255.f" constant reused by mistake for a
//     different scale elsewhere).
// PT: Rml::Colourb -> glintfx::ColorF. /255.f explícito por canal (conforme o próprio brief da
//     RMLX-0 -- sem divisão implícita int/float, sem constante "255.f" compartilhada reusada
//     por engano pra outra escala em outro lugar).
inline ColorF from_rml(Rml::Colourb c) {
  return ColorF{c.red / 255.f, c.green / 255.f, c.blue / 255.f, c.alpha / 255.f};
}

} // namespace glintfx
