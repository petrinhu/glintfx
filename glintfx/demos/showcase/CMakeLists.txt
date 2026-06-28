# SPDX-License-Identifier: MPL-2.0
# EN: glintfx visual showcase — demo target consuming only the public glintfx facade.
# PT: Showcase visual do glintfx — alvo demo que consome apenas a fachada pública.
add_executable(glintfx_showcase main.cpp)
target_link_libraries(glintfx_showcase PRIVATE glintfx)

# EN: Copy assets (RML, RCSS, font, license) to the binary directory so the demo
#     can run from there without an install step.
# PT: Copia assets (RML, RCSS, fonte, licença) para o diretório do binário para
#     que o demo rode de lá sem necessidade de instalação.
file(COPY showcase.rml showcase.rcss assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
