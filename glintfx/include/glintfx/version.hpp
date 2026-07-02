// SPDX-License-Identifier: MPL-2.0
// EN: Library version accessor — always available regardless of GLINTFX_BACKEND_GLFW.
//     Kept in its own header (not app.hpp) so embed-only consumers (GLINTFX_BACKEND_GLFW=OFF,
//     e.g. GusWorld) that never include app.hpp still have access to glintfx::version().
//     Implementation lives in src/version.cpp, which is compiled unconditionally (see
//     CMakeLists.txt _glintfx_sources) — unlike src/app.cpp, which is GLFW-backend-only.
// PT: Acessor de versão da lib — sempre disponível independente de GLINTFX_BACKEND_GLFW.
//     Mantido em header próprio (não em app.hpp) para que consumidores embed-only
//     (GLINTFX_BACKEND_GLFW=OFF, ex.: GusWorld), que nunca incluem app.hpp, ainda tenham
//     acesso a glintfx::version(). Implementação vive em src/version.cpp, compilado
//     incondicionalmente (ver CMakeLists.txt _glintfx_sources) — diferente de src/app.cpp,
//     que é exclusivo do backend GLFW.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

namespace glintfx {

// EN: Returns the library version string (e.g. "0.2.4"), sourced from config.hpp
//     (GLINTFX_VERSION, generated from the CMake project() VERSION at configure time).
// PT: Retorna a string de versão da lib (ex.: "0.2.4"), obtida de config.hpp
//     (GLINTFX_VERSION, gerado a partir do VERSION do project() do CMake em configure).
const char* version();

} // namespace glintfx
