// SPDX-License-Identifier: MPL-2.0
// EN: Public font-engine selector (L1.20-FONTFLIP Phase 2). A plain, engine-agnostic enum --
//     no Rml:: or FreeType type appears here, or anywhere else in glintfx's public headers
//     (the encapsulation invariant every header under glintfx/include/glintfx/ upholds).
// PT: Seletor público de motor de fonte (L1.20-FONTFLIP Fase 2). Um enum simples, agnóstico de
//     engine -- nenhum tipo Rml:: ou FreeType aparece aqui, nem em nenhum outro header público
//     do glintfx (o invariante de encapsulamento que todo header sob glintfx/include/glintfx/
//     sustenta).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

namespace glintfx {

// EN: Selects which glyph rasteriser/shaper glintfx installs into RmlUi at construction time
//     (App/UiLayer ctor -> Bootstrap::init(), which runs Rml::Initialise() once and never
//     revisits the choice for that instance's lifetime -- RmlUi resolves the registered
//     FontEngineInterface once, so there is no runtime setter to flip this after construction;
//     rebuild-a-config-and-reconstruct is the supported way to change it).
//
//     Own (the default, since L1.20-FONTFLIP Phase 1 -- the "soft flip"): glintfx's own
//     clean-room SOV-SFNT/SOV-RAST/SOV-HINT font engine (L1.19-FONTENG), vendored under
//     glintfx/vendor/core/. Requires the library to have been built with
//     GLINTFX_OWN_FONT_ENGINE=ON (the CMake default -- see glintfx/CMakeLists.txt). If the
//     library was instead built with GLINTFX_OWN_FONT_ENGINE=OFF (a consumer opt-out at BUILD
//     time), requesting Own here falls back to FreeType with a warning logged -- never a crash
//     or a hard failure (see Bootstrap::init()'s doc-comment, src/bootstrap.cpp, for the exact
//     fallback mechanics).
//
//     FreeType: RmlUi's own built-in, FreeType-backed default font engine -- unchanged,
//     production-proven. Always available regardless of how glintfx was built (FreeType stays
//     linked either way; GLINTFX_OWN_FONT_ENGINE only controls whether the OWN engine is ALSO
//     compiled in). Selecting this is the one-line, no-rebuild rollback for a consumer that
//     hits a rendering regression with the own engine: `font_engine = FontEngine::FreeType`.
//
// PT: Seleciona qual rasterizador/shaper de glifo o glintfx instala no RmlUi na construção
//     (ctor de App/UiLayer -> Bootstrap::init(), que roda Rml::Initialise() uma vez e nunca
//     revisita a escolha durante o lifetime daquela instância -- o RmlUi resolve a
//     FontEngineInterface registrada uma única vez, então não há setter de runtime para trocar
//     isto após a construção; reconfigurar-e-reconstruir é a forma suportada de mudar).
//
//     Own (o padrão, desde o L1.20-FONTFLIP Fase 1 -- o "flip suave"): o motor de fonte próprio
//     clean-room do glintfx (SOV-SFNT/SOV-RAST/SOV-HINT, L1.19-FONTENG), vendorizado em
//     glintfx/vendor/core/. Exige que a lib tenha sido buildada com GLINTFX_OWN_FONT_ENGINE=ON
//     (o padrão do CMake -- ver glintfx/CMakeLists.txt). Se a lib foi buildada com
//     GLINTFX_OWN_FONT_ENGINE=OFF (um opt-out do consumidor em tempo de BUILD), pedir Own aqui
//     cai no FreeType com um aviso logado -- nunca um crash ou falha dura (ver o doc-comment de
//     Bootstrap::init(), src/bootstrap.cpp, para a mecânica exata do fallback).
//
//     FreeType: o motor de fonte default do próprio RmlUi, apoiado em FreeType -- inalterado,
//     comprovado em produção. Sempre disponível independente de como o glintfx foi buildado (o
//     FreeType continua linkado de qualquer forma; GLINTFX_OWN_FONT_ENGINE só controla se o
//     motor PRÓPRIO também é compilado). Selecionar este é o rollback de uma linha, sem
//     rebuild, para um consumidor que encontrar uma regressão de renderização com o motor
//     próprio: `font_engine = FontEngine::FreeType`.
enum class FontEngine {
  Own,
  FreeType,
};

} // namespace glintfx
