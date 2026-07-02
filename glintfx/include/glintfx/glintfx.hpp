// SPDX-License-Identifier: MPL-2.0
// EN: Umbrella public header — include this one instead of individual headers.
//     version.hpp is always included (L1.9-VERSEMBED: glintfx::version() must be available
//     regardless of backend); app.hpp is only included when GLINTFX_BACKEND_GLFW is enabled
//     (standalone build); ui_layer.hpp is always included (embed mode is always available).
// PT: Header público guarda-chuva — inclua este em vez dos headers individuais.
//     version.hpp sempre incluído (L1.9-VERSEMBED: glintfx::version() deve estar disponível
//     independente do backend); app.hpp só é incluído quando GLINTFX_BACKEND_GLFW está
//     habilitado (build standalone); ui_layer.hpp sempre incluído (embed mode sempre disponível).
#pragma once
#include <glintfx/config.hpp>
#include <glintfx/version.hpp>
#if GLINTFX_BACKEND_GLFW
#include <glintfx/app.hpp>
#endif
#include <glintfx/ui_layer.hpp>
