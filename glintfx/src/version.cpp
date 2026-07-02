// SPDX-License-Identifier: MPL-2.0
// EN: version() implementation — always compiled regardless of GLINTFX_BACKEND_GLFW so
//     embed-only builds (Engine + UiLayer only, no App) still expose glintfx::version().
//     Moved out of app.cpp (L1.9-VERSEMBED): app.cpp is only compiled when the GLFW
//     backend is enabled, which left embed-only consumers (e.g. GusWorld) without access
//     to the version string.
// PT: Implementação de version() — sempre compilada independente de GLINTFX_BACKEND_GLFW
//     para que builds embed-only (só Engine + UiLayer, sem App) ainda exponham
//     glintfx::version(). Movida para fora de app.cpp (L1.9-VERSEMBED): app.cpp só é
//     compilado com o backend GLFW habilitado, o que deixava consumidores embed-only
//     (ex.: GusWorld) sem acesso à string de versão.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/version.hpp>
#include <glintfx/config.hpp>

namespace glintfx {

const char* version() { return GLINTFX_VERSION; }

} // namespace glintfx
