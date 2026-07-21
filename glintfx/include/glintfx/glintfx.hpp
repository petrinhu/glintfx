// SPDX-License-Identifier: MPL-2.0
// EN: Umbrella public header — include this one instead of individual headers.
//     version.hpp is always included (L1.9-VERSEMBED: glintfx::version() must be available
//     regardless of backend); app.hpp is only included when GLINTFX_BACKEND_GLFW is enabled
//     (standalone build); ui_layer.hpp is always included (embed mode is always available);
//     i18n.hpp is included when GLINTFX_MODULE_I18N is enabled (default ON, ADR-0015 module
//     "i18n" -- depends on core only, independent of GLINTFX_BACKEND_GLFW either way); audio.hpp
//     is included when GLINTFX_MODULE_AUDIO is enabled (default ON, ADR-0015 module "audio",
//     framework-2D slice A3-AUDIO -- same independence class as i18n).
// PT: Header público guarda-chuva — inclua este em vez dos headers individuais.
//     version.hpp sempre incluído (L1.9-VERSEMBED: glintfx::version() deve estar disponível
//     independente do backend); app.hpp só é incluído quando GLINTFX_BACKEND_GLFW está
//     habilitado (build standalone); ui_layer.hpp sempre incluído (embed mode sempre disponível);
//     i18n.hpp é incluído quando GLINTFX_MODULE_I18N está habilitado (padrão ON, módulo "i18n"
//     da ADR-0015 -- depende só de core, independente de GLINTFX_BACKEND_GLFW de qualquer jeito);
//     audio.hpp é incluído quando GLINTFX_MODULE_AUDIO está habilitado (padrão ON, módulo
//     "audio" da ADR-0015, fatia framework-2D A3-AUDIO -- mesma classe de independência do i18n).
#pragma once
#include <glintfx/config.hpp>
#include <glintfx/version.hpp>
#if GLINTFX_BACKEND_GLFW
#include <glintfx/app.hpp>
#endif
#include <glintfx/ui_layer.hpp>
#if GLINTFX_MODULE_I18N
#include <glintfx/i18n.hpp>
#endif
#if GLINTFX_MODULE_AUDIO
#include <glintfx/audio.hpp>
#endif
