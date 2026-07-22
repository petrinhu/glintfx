# SPDX-License-Identifier: MPL-2.0
# EN: Standalone CMake project -- pragmatic Windows CI harness for glintfx's PURE atoms only
#     (A3-AUDIO/FX-CARVE-1 CI integration, S3). Compiles and tests ONLY what has zero GL/window/
#     RmlUi/Freetype dependency: i18n (glintfx::I18n, ADR-0015 module "i18n") and audio
#     (glintfx::Audio, ADR-0015 module "audio", vendored miniaudio) -- both explicitly documented
#     in glintfx/CMakeLists.txt as depending on `core` ONLY -- plus a compile-only smoke of
#     src/stb_image_impl.cpp (image decode atom). This is NOT the main glintfx build: no
#     FetchContent, no RmlUi, no Freetype, no GLFW, no GL, no Xvfb -- porting the full build to
#     Windows (RmlUi+Freetype via vcpkg, ~10+ min/job plus ongoing maintenance) is deliberately
#     OUT of scope here, deferred (see the plan this slice implements,
#     docs/superpowers/plans/2026-07-20-framework2d-A3-audio-e-carving-uifx.md section 5); the
#     framework stays Linux-first. This project is invoked ONLY by
#     .github/workflows/windows-atoms.yml (runs-on: windows-latest, MSVC) -- it is NOT part of
#     the main glintfx/CMakeLists.txt tree and is never built by the Linux legs of CI.
#     Sources compiled: ../../src/i18n.cpp, ../../src/audio.cpp, ../../src/miniaudio_impl.c (the
#     miniaudio implementation TU, MIT-0/public-domain, compiled with warnings OFF like the main
#     build does via set_source_files_properties), ../../src/stb_image_impl.cpp (compile-only,
#     no test exercises it here -- decode correctness is covered by the main Linux suite's
#     texture_png_alpha/asset_decode_hostile_sanity). Tests: the 3 i18n_*.cpp + 5 audio_*.cpp
#     (AUDIO-F2 added audio_polyphony_sanity/audio_loop_fade_sanity to the original 3)
#     pure-unit sanities from ../ (tests/), linked against a tiny static lib built from the
#     sources above -- same test SOURCE FILES the main tests/CMakeLists.txt registers under
#     `if(GLINTFX_MODULE_I18N)`/`if(GLINTFX_MODULE_AUDIO)` (byte-identical, not a fork: this
#     harness references them by relative path, it does not copy them).
#     C++ standard: 20, not the main build's 23 -- a deliberately conservative choice for a
#     pragmatic MSVC harness (the project's declared floor is C++17, target C++23; neither
#     audio.cpp nor i18n.cpp use any C++23-specific construct today, confirmed by grep before
#     writing this file -- if that ever changes, bump this to 23 and re-validate under MSVC).
#     Threads: find_package(Threads) only (matches glintfx_audio's own link set in the main
#     CMakeLists.txt, minus CMAKE_DL_LIBS/m -- miniaudio needs neither on Windows: it resolves
#     every backend it needs -- WASAPI/DirectSound/WinMM -- via LoadLibraryA/GetProcAddress at
#     runtime, confirmed by grep in third_party/miniaudio/miniaudio.h, so this harness has ZERO
#     hard link dependency on a Windows audio library, same "no hard link dep, discover at
#     runtime" spirit the main build documents for glloader/GLINTFX_MODULE_AUDIO on Linux).
# PT: Projeto CMake standalone -- harness pragmático de CI Windows SÓ para os átomos PUROS do
#     glintfx (integração de CI do A3-AUDIO/FX-CARVE-1, S3). Compila e testa APENAS o que tem
#     zero dependência de GL/janela/RmlUi/Freetype: i18n (glintfx::I18n, módulo "i18n" da
#     ADR-0015) e audio (glintfx::Audio, módulo "audio" da ADR-0015, miniaudio vendorizado) --
#     ambos documentados explicitamente no glintfx/CMakeLists.txt como dependendo SÓ de `core` --
#     mais um smoke compile-only de src/stb_image_impl.cpp (átomo de decode de imagem). NÃO é o
#     build principal do glintfx: sem FetchContent, sem RmlUi, sem Freetype, sem GLFW, sem GL,
#     sem Xvfb -- portar o build completo pro Windows (RmlUi+Freetype via vcpkg, ~10+ min/job
#     mais manutenção contínua) fica deliberadamente FORA de escopo aqui, adiado (ver o plano que
#     esta fatia implementa,
#     docs/superpowers/plans/2026-07-20-framework2d-A3-audio-e-carving-uifx.md seção 5); o
#     framework continua Linux-first. Este projeto é invocado SÓ pelo
#     .github/workflows/windows-atoms.yml (runs-on: windows-latest, MSVC) -- NÃO faz parte da
#     árvore principal glintfx/CMakeLists.txt e nunca é buildado pelas legs Linux do CI.
#     Fontes compiladas: ../../src/i18n.cpp, ../../src/audio.cpp, ../../src/miniaudio_impl.c (a
#     TU de implementação do miniaudio, MIT-0/domínio público, compilada com warnings
#     DESLIGADOS como o build principal faz via set_source_files_properties()),
#     ../../src/stb_image_impl.cpp (compile-only, nenhum teste o exercita aqui -- corretude de
#     decode é coberta pela suíte Linux principal via texture_png_alpha/
#     asset_decode_hostile_sanity). Testes: as 3 sanities unit puras i18n_*.cpp + as 3
#     audio_*.cpp de ../ (tests/), linkadas contra uma libzinha estática buildada dos fontes
#     acima -- os MESMOS arquivos-fonte de teste que o tests/CMakeLists.txt principal registra
#     sob `if(GLINTFX_MODULE_I18N)`/`if(GLINTFX_MODULE_AUDIO)` (byte-idênticos, não um fork:
#     este harness os referencia por caminho relativo, não os copia).
#     Padrão C++: 20, não o 23 do build principal -- escolha deliberadamente conservadora para
#     um harness MSVC pragmático (o piso declarado do projeto é C++17, alvo C++23; nem
#     audio.cpp nem i18n.cpp usam nenhuma construção específica de C++23 hoje, confirmado via
#     grep antes de escrever este arquivo -- se isso mudar algum dia, subir para 23 e
#     revalidar sob MSVC).
#     Threads: só find_package(Threads) (espelha o conjunto de link do próprio glintfx_audio no
#     CMakeLists.txt principal, menos CMAKE_DL_LIBS/m -- o miniaudio não precisa de nenhum dos
#     dois no Windows: ele resolve todo backend que precisa -- WASAPI/DirectSound/WinMM -- via
#     LoadLibraryA/GetProcAddress em runtime, confirmado via grep em
#     third_party/miniaudio/miniaudio.h, então este harness tem ZERO dependência de link rígida
#     com uma lib de áudio do Windows, mesmo espírito "sem dep de link rígida, descoberta em
#     runtime" que o build principal documenta para o glloader/GLINTFX_MODULE_AUDIO no Linux).
cmake_minimum_required(VERSION 3.16)
project(glintfx_win_atoms LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GLINTFX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

find_package(Threads REQUIRED)

# EN: glintfx_win_atoms -- static lib bundling the two pure atoms (i18n + audio) plus the
#     miniaudio implementation TU. PUBLIC include of include/ (public headers, same layout the
#     main build exposes); PRIVATE include of third_party/miniaudio (audio.cpp's own #include
#     "miniaudio.h", mirrors the main CMakeLists.txt's glintfx_audio target).
# PT: glintfx_win_atoms -- lib estática empacotando os dois átomos puros (i18n + audio) mais a
#     TU de implementação do miniaudio. Include PUBLIC de include/ (headers públicos, mesmo
#     layout que o build principal expõe); include PRIVATE de third_party/miniaudio (o próprio
#     #include "miniaudio.h" do audio.cpp, espelha o alvo glintfx_audio do CMakeLists.txt
#     principal).
add_library(glintfx_win_atoms STATIC
  "${GLINTFX_ROOT}/src/i18n.cpp"
  "${GLINTFX_ROOT}/src/audio.cpp"
  "${GLINTFX_ROOT}/src/miniaudio_impl.c"
)
target_include_directories(glintfx_win_atoms
  PUBLIC
    "${GLINTFX_ROOT}/include"
  PRIVATE
    "${GLINTFX_ROOT}/third_party/miniaudio"
)
target_link_libraries(glintfx_win_atoms PUBLIC Threads::Threads)

# EN: miniaudio_impl.c is third-party code (MIT-0) -- disable warnings on it exactly like the
#     main CMakeLists.txt does for the same file (set_source_files_properties(... "-w")); MSVC's
#     equivalent flag is /w. audio.cpp/i18n.cpp (ours) get the compiler's normal warning level.
# PT: miniaudio_impl.c é código de terceiro (MIT-0) -- desliga warnings nele exatamente como o
#     CMakeLists.txt principal faz para o mesmo arquivo (set_source_files_properties(... "-w"));
#     o equivalente no MSVC é /w. audio.cpp/i18n.cpp (nossos) recebem o nível de warning normal
#     do compilador.
set_source_files_properties("${GLINTFX_ROOT}/src/miniaudio_impl.c" PROPERTIES
  COMPILE_OPTIONS "$<IF:$<C_COMPILER_ID:MSVC>,/w,-w>"
)

# EN: Compile-only smoke of the image atom (stb_image_impl.cpp) -- proves it builds under MSVC;
#     no test executable exercises it here (decode correctness stays a Linux-suite concern, see
#     texture_png_alpha/asset_decode_hostile_sanity in the main tests/CMakeLists.txt). OBJECT
#     library so there is nothing to link/run, just compile.
# PT: Smoke compile-only do átomo de imagem (stb_image_impl.cpp) -- prova que builda sob MSVC;
#     nenhum teste o exercita aqui (corretude de decode continua sendo responsabilidade da suíte
#     Linux, ver texture_png_alpha/asset_decode_hostile_sanity no tests/CMakeLists.txt
#     principal). Lib OBJECT porque não há nada pra linkar/rodar, só compilar.
add_library(glintfx_win_image_compile_only OBJECT
  "${GLINTFX_ROOT}/src/stb_image_impl.cpp"
)
target_include_directories(glintfx_win_image_compile_only PRIVATE
  "${GLINTFX_ROOT}/include"
  "${GLINTFX_ROOT}/third_party/stb"
)

enable_testing()

# EN: The 3 i18n_*.cpp + 5 audio_*.cpp pure-unit sanities, referenced from ../ (tests/) by
#     relative path -- byte-identical to what tests/CMakeLists.txt registers under
#     if(GLINTFX_MODULE_I18N)/if(GLINTFX_MODULE_AUDIO); this harness does not fork/copy them.
#     All 8 use AudioConfig::null_backend=true (audio_*) or touch no GL/window at all (i18n_*),
#     so none need a display or a real sound device -- true headless CI, on Windows too.
# PT: As 3 sanities unit puras i18n_*.cpp + as 5 audio_*.cpp, referenciadas de ../ (tests/) por
#     caminho relativo -- byte-idênticas ao que o tests/CMakeLists.txt registra sob
#     if(GLINTFX_MODULE_I18N)/if(GLINTFX_MODULE_AUDIO); este harness não as forka/copia. As 8
#     usam AudioConfig::null_backend=true (audio_*) ou não tocam em GL/janela nenhuma (i18n_*),
#     então nenhuma precisa de display nem de dispositivo de som real -- CI headless de verdade,
#     também no Windows.
set(GLINTFX_WIN_ATOM_TESTS
  i18n_catalog_sanity
  i18n_lookup_fallback_sanity
  i18n_plural_sanity
  audio_lifecycle_sanity
  audio_playback_sanity
  audio_hostile_sanity
  audio_polyphony_sanity
  audio_loop_fade_sanity
)
foreach(_glintfx_win_test IN LISTS GLINTFX_WIN_ATOM_TESTS)
  add_executable(${_glintfx_win_test} "${GLINTFX_ROOT}/tests/${_glintfx_win_test}.cpp")
  target_link_libraries(${_glintfx_win_test} PRIVATE glintfx_win_atoms)
  add_test(NAME ${_glintfx_win_test} COMMAND ${_glintfx_win_test})
endforeach()
