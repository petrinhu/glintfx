# SPDX-License-Identifier: MPL-2.0
# consumer-example — drop-in proof: consumes glintfx via FetchContent,
# no direct references to GL / GLFW / RmlUi.
# Prova de drop-in: consome glintfx via FetchContent,
# sem referências diretas a GL / GLFW / RmlUi.
# Copyright (c) 2026 Petrus Silva Costa
cmake_minimum_required(VERSION 3.16)
project(consumer LANGUAGES CXX)

include(FetchContent)

# EN: Bring glintfx in from the sibling directory (no network required).
#     Consumers in the wild would use GIT_REPOSITORY + GIT_TAG instead.
# PT: Traz glintfx do diretório irmão (sem rede necessária).
#     Consumidores externos usariam GIT_REPOSITORY + GIT_TAG.
FetchContent_Declare(glintfx
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../glintfx)
FetchContent_MakeAvailable(glintfx)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE glintfx::glintfx)   # 1 linha — sem GL/GLFW/RmlUi
