// SPDX-License-Identifier: MPL-2.0
// EN: Single translation unit that compiles stb_image (PNG/JPG/TGA decode).
//     Only these three formats are compiled in; everything else is stripped.
//     The header is included once here with STB_IMAGE_IMPLEMENTATION defined;
//     all other translation units include the header without that define (declarations only).
// PT: Única unidade de tradução que compila o stb_image (decode PNG/JPG/TGA).
//     Apenas esses três formatos são compilados; todo o resto é descartado.
//     O header é incluído uma vez aqui com STB_IMAGE_IMPLEMENTATION definido;
//     todas as outras unidades de tradução incluem o header sem esse define (só declarações).
// Copyright (c) 2026 Petrus Silva Costa
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_TGA
#include "stb_image.h"
