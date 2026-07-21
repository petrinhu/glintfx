// SPDX-License-Identifier: MIT-0
// EN: Single translation unit that compiles miniaudio's implementation (A3-AUDIO, module
//     GLINTFX_MODULE_AUDIO). Same pattern as src/stb_image_impl.cpp: the header is included
//     once here with MINIAUDIO_IMPLEMENTATION defined; every other translation unit
//     (src/audio.cpp) includes the header without that define (declarations only).
//
//     Feature-macro configuration (deliberately minimal, see glintfx/docs/audio.md for the
//     full rationale):
//       MA_NO_ENCODING   -- this slice never WRITES audio out (no recording-to-file feature),
//                           only plays back already-authored assets.
//       MA_NO_GENERATION -- no waveform/noise-generator API needed by this slice.
//     NOT defined (left enabled, on purpose): the built-in WAV/FLAC/MP3 decoders
//     (dr_wav/dr_flac/dr_mp3) -- they come for free with no extra vendoring, so
//     glintfx::Audio::load_sound() supports all three out of the box. OGG is the one common
//     format miniaudio does NOT decode without an external stb_vorbis dependency we do not
//     vendor in this slice -- deferred, see glintfx/docs/audio.md.
//
//     Compiled with -w (this exact file only, via set_source_files_properties() in
//     glintfx/CMakeLists.txt, mirroring the -w treatment RmlUi_Renderer_GL3.cpp gets) --
//     third-party code, not ours to fix warnings in; glintfx's OWN glintfx_audio sources
//     (src/audio.cpp) compile with -Wall -Wextra like every other module in this library.
// PT: Única unidade de tradução que compila a implementação do miniaudio (A3-AUDIO, módulo
//     GLINTFX_MODULE_AUDIO). Mesmo padrão do src/stb_image_impl.cpp: o header é incluído uma
//     vez aqui com MINIAUDIO_IMPLEMENTATION definido; toda outra unidade de tradução
//     (src/audio.cpp) inclui o header sem esse define (só declarações).
//
//     Configuração de feature-macro (deliberadamente mínima, ver glintfx/docs/audio.md pra
//     racional completa):
//       MA_NO_ENCODING   -- esta fatia nunca ESCREVE áudio em arquivo (sem feature de
//                           gravação), só reproduz assets já autorados.
//       MA_NO_GENERATION -- nenhuma API de gerador de waveform/ruído é necessária nesta fatia.
//     NÃO definido (deixado habilitado, de propósito): os decoders embutidos WAV/FLAC/MP3
//     (dr_wav/dr_flac/dr_mp3) -- vêm de graça sem vendoring extra, então
//     glintfx::Audio::load_sound() suporta os três de fábrica. OGG é o único formato comum que
//     o miniaudio NÃO decodifica sem uma dependência externa stb_vorbis que não vendorizamos
//     nesta fatia -- adiado, ver glintfx/docs/audio.md.
//
//     Compilado com -w (só este arquivo, via set_source_files_properties() no
//     glintfx/CMakeLists.txt, espelhando o tratamento -w que o RmlUi_Renderer_GL3.cpp recebe)
//     -- código de terceiro, não é nosso trabalho consertar warning nele; os sources PRÓPRIOS
//     do glintfx_audio (src/audio.cpp) compilam com -Wall -Wextra como todo outro módulo desta
//     biblioteca.
// Copyright (c) 2026 David Reid (miniaudio, MIT-0/public domain -- see third_party/miniaudio/README.md)
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#include "miniaudio.h"
