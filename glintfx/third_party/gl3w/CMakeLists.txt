# SPDX-License-Identifier: MPL-2.0
# EN: Vendored gl3w — pre-generated GL loader (public domain / UNLICENSE).
#     Files were generated with gl3w_gen.py against GL 4.6 spec and committed
#     to avoid the network dependency at configure time.
# PT: gl3w vendorizado — loader de GL pré-gerado (domínio público / UNLICENSE).
#     Arquivos gerados com gl3w_gen.py contra a spec GL 4.6 e commitados
#     para evitar dependência de rede em tempo de configure.
cmake_minimum_required(VERSION 3.16)
project(gl3w LANGUAGES C)

find_package(OpenGL REQUIRED)

# EN: INTERFACE library: consumers get include paths and gl3w.c compiled in-place.
# PT: Biblioteca INTERFACE: consumidores recebem include paths e gl3w.c compilado inline.
add_library(gl3w INTERFACE)
target_sources(gl3w INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/gl3w.c>)
target_include_directories(gl3w INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(gl3w INTERFACE OpenGL::GL ${CMAKE_DL_LIBS})
