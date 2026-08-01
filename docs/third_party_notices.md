# Third-party notices (full detail) / Avisos de terceiros (detalhe completo)

> **EN:** This file carries the FULL disclosure prose for every third-party dependency glintfx links, vendors, or consumes as data/input — provenance, exact file paths, what each piece is used for, and the acknowledgment/patch-disclosure narrative. It exists because `NOTICE` changed nature under Apache-2.0: §4(d) of that license makes `NOTICE`'s content **mandatory propagation** for anyone who redistributes the software, so `NOTICE` itself was cut down to the legally required minimum (name, copyright, license grant, and the attributions each dependency's own license actually requires). Nothing was deleted — every paragraph that used to live in `NOTICE` moved here, verbatim in substance. See [ADR-0019](adr/0019-license-rotation-apache-2.0.md) for the decision that split the two files, and `NOTICE` itself for the short, normative version.
>
> **PT:** Este arquivo carrega a prosa de divulgação COMPLETA de toda dependência de terceiro que o glintfx linka, vendoriza ou consome como dado/input — proveniência, caminhos exatos de arquivo, para que cada peça é usada, e a narrativa de agradecimento/divulgação de patch. Ele existe porque o `NOTICE` mudou de natureza sob a Apache-2.0: o §4(d) dessa licença torna o conteúdo do `NOTICE` **propagação obrigatória** para quem redistribui o software, então o `NOTICE` em si foi reduzido ao mínimo legalmente exigido (nome, copyright, concessão de licença, e as atribuições que a própria licença de cada dependência de fato exige). Nada foi apagado — todo parágrafo que vivia no `NOTICE` se mudou pra cá, em substância verbatim. Ver [ADR-0019](adr/0019-license-rotation-apache-2.0.md) para a decisão que separou os dois arquivos, e o próprio `NOTICE` para a versão curta e normativa.

---

## Linked / fetched libraries (not vendored in this repository) / Bibliotecas linkadas / baixadas (não vendorizadas neste repositório)

**EN:** These libraries are downloaded at build time (via `FetchContent`) or provided by the system. They are NOT part of this repository's source tree (the clones under `examples/` are gitignored and exist only for study / reverse-engineering — never copied from, per this project's clean-room policy).

**PT:** Estas bibliotecas são baixadas em tempo de build (via `FetchContent`) ou fornecidas pelo sistema. NÃO fazem parte da árvore de fonte deste repositório (os clones em `examples/` são gitignored e existem só para estudo / engenharia reversa — nunca copiados, pela política clean-room deste projeto).

- **RmlUi** — MIT License. Copyright (c) 2008-2014 CodePoint Ltd, Shift Technology Ltd, and contributors.
- **FreeType** — used under the FreeType License (FTL). Portions of this software are copyright (c) The FreeType Project (www.freetype.org). All rights reserved. A hard `find_package(Freetype REQUIRED)` system dependency, always linked: RmlUi's own default font engine requires it unconditionally, independently of glintfx's own optional `GLINTFX_OWN_FONT_ENGINE` selector ([ADR-0011](adr/0011-soft-font-flip.md) — the "soft flip" made glintfx's own engine the runtime default, it did not drop the FreeType dependency).
- **OpenGL (libGL / Mesa)** — MIT/SGI-style licenses; copyright their respective authors. System library, not distributed by this project.
- **GLFW** — https://www.glfw.org — zlib/libpng License. Copyright (c) 2002-2006 Marcus Geelnard, (c) 2006-2019 Camilla Löwy. Linked only when built with `GLINTFX_BACKEND_GLFW=ON` (the default; see [ADR-0008](adr/0008-embed-guest-mode.md)).

## Bundled (vendored) third-party source files / Arquivos-fonte de terceiro vendorizados (incluídos no repositório)

**EN:** These files are committed directly into the repository source tree.

**PT:** Estes arquivos são commitados diretamente na árvore de fonte do repositório.

- **stb_image v2.30** — Public domain / MIT (dual-licensed; used here as public domain). No copyright notice required; included here for disclosure. Original author: Sean Barrett (nothings.org). File: `glintfx/third_party/stb/stb_image.h`. Used for: PNG, JPG, and TGA texture decode in the GL3 render backend (`glintfx/src/render_gl3.cpp` + `glintfx/src/stb_image_impl.cpp`), and the public `IMG-DECODE` pair (`glintfx/include/glintfx/image.hpp` + `glintfx/src/image.cpp`).

- **stb_image_write v1.16** — Public domain / MIT (dual-licensed; used here as public domain). No copyright notice required; included here for disclosure. Original author: Sean Barrett (nothings.org). File: `glintfx/third_party/stb/stb_image_write.h`. Used for: PNG/JPG/BMP/TGA/HDR image encode in the public `IMG-ENCODE` pair (`glintfx/include/glintfx/image.hpp` + `glintfx/src/image_encode.cpp` + `glintfx/src/stb_image_write_impl.cpp`). Five of this module's seven `ImageFormat` values go through this vendored encoder; PPM is hand-written and QOI is glintfx's own clean-room encoder (`glintfx/src/qoi_encode.hpp`, see that file's own header for provenance) — neither has an upstream counterpart here.

- **miniaudio v0.11.25** (commit `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d`, 2026-03-04) — MIT No Attribution (MIT-0) / public domain (dual-licensed; used here as MIT-0). Original author: David Reid (mackron@gmail.com, https://miniaud.io). File: `glintfx/third_party/miniaudio/miniaudio.h`. Used for: audio decode/playback (WAV/FLAC/MP3) in the `GLINTFX_MODULE_AUDIO` atom (`glintfx/src/audio.cpp` + `glintfx/src/miniaudio_impl.c`). **Note:** carries one disclosed, reviewed one-line patch fixing a genuine upstream heap-use-after-free (`ma_resource_manager_data_buffer_node_acquire`, epilogue dereferenced a just-freed node on the synchronous-decode-failure path) — found via this module's own ASan test pass. Full writeup: `glintfx/third_party/miniaudio/README.md`. Same disclosure discipline as the RmlUi patch below (`glintfx/patches/README.md`). (This patch was not filed upstream as a new ticket: upstream `dev` had already fixed the same bug independently by the time it was found here — see this project's operating memory on upstream reporting for the reasoning.)

- **SDL_GameControllerDB** (Linux subset, vendored as DATA, not code) — zlib License. Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>. Source: https://github.com/mdqinc/SDL_GameControllerDB (formerly gabomdq/SDL_GameControllerDB). Pinned commit: `8d9fefd7b810f2541f78cc7a8ccbd185bc84c7a5` (2026-07-15). File: `glintfx/third_party/gamecontrollerdb/gamecontrollerdb_linux.txt` (filtered to `platform:Linux` entries only, 728 of the upstream's 2244 platform-tagged lines) + `gamecontrollerdb_linux.inc` (generated from the `.txt` by `tools/gen_gamepad_db.py`). Used for: gamepad button/axis mapping resolution in the `GLINTFX_MODULE_GAMEPAD` atom (`glintfx/src/gamepad_mapping.cpp`). Consumed purely as TEXT DATA by glintfx's own clean-room parser — no SDL header or source file is read, compiled, or linked anywhere in this repository. See `glintfx/third_party/gamecontrollerdb/README.md` for full provenance, license text, and the re-sync recipe.

- **glcorearb.h** — MIT License. Copyright 2013-2026 The Khronos Group Inc. SPDX-License-Identifier: MIT. File: `glintfx/third_party/khronos/GL/glcorearb.h`.

- **khrplatform.h** — MIT License. Copyright (c) 2008-2018 The Khronos Group Inc. File: `glintfx/third_party/khronos/KHR/khrplatform.h`.

- **gl.xml** (OpenGL / OpenGL-ES XML API Registry) — Apache License 2.0. Copyright 2013-2026 The Khronos Group Inc. SPDX-License-Identifier: Apache-2.0. Source: https://github.com/KhronosGroup/OpenGL-Registry. File: `glintfx/third_party/khronos/gl.xml`. Used for: input data to `tools/gen_glloader.py` ([L1.14-GLLOADER]), which generates `glintfx/src/gl_loader.h` / `gl_loader.c` — glintfx's own GL 3.3 core-profile function-pointer loader, replacing the previously vendored gl3w third-party loader. `gl_loader.{h,c}` are glintfx's own source, carrying this project's own SPDX header (Apache-2.0 as of the rotation this file documents, formerly MPL-2.0), not third-party code; they are listed here only to disclose the Apache-2.0 registry data they are mechanically derived from.

## Bundled assets / Assets embarcados

- **Open Sans** — Apache License 2.0. Copyright The Open Sans Project Authors (https://github.com/googlefonts/opensans). File: `glintfx/demos/showcase/assets/OpenSans-Regular.ttf`.

- **Pixel Operator Mono** — Creative Commons Zero (CC0 1.0, public domain). By Jayvee Enaguas (HarvettFox96), https://notabug.org/HarvettFox96/ttf-pixeloperator. File: `glintfx/demos/showcase/assets/PixelOperatorMono.ttf`. Used as a Latin-only test fixture (font-fallback coverage tests).

---

**EN:** Full license texts of these dependencies are available in their respective upstream repositories. Any future in-project reimplementation of their functionality is done clean-room (from understanding, without copying source).

**PT:** Os textos completos de licença destas dependências estão disponíveis nos respectivos repositórios upstream. Qualquer reimplementação futura, dentro do projeto, da funcionalidade delas é feita clean-room (a partir do entendimento, sem copiar fonte).

---

## Acknowledgment / Agradecimento

**EN:** glintfx is built on top of RmlUi (https://github.com/mikke89/RmlUi, MIT License) — the HTML/CSS-style UI engine and layout core this library wraps and drives from `.rcss`. Our thanks to mikke89 and the RmlUi contributors for a mature, well-documented project to build on. Where we find and fix a genuine bug in RmlUi itself, we track the fix as an explicit, reviewed source patch (see `glintfx/patches/README.md`) and aim to report/submit it back upstream — e.g. `rmlui-2cd28864-teardown-ub.patch`, which fixes two undefined-behavior findings in RmlUi's own document/element teardown order (see `docs/embed-integration.md` section 18 for the consumer-facing writeup, and `glintfx/patches/README.md` for the technical detail). Any such patch is meant to be temporary: retired from this repository once the equivalent fix lands upstream.

**PT:** o glintfx é construído sobre o RmlUi (https://github.com/mikke89/RmlUi, licença MIT) — o motor de UI estilo HTML/CSS e o núcleo de layout que esta biblioteca embrulha e comanda a partir de `.rcss`. Nosso agradecimento ao mikke89 e aos contribuidores do RmlUi por um projeto maduro e bem documentado sobre o qual construir. Quando encontramos e corrigimos um bug genuíno no próprio RmlUi, rastreamos a correção como um patch de fonte explícito e revisado (ver `glintfx/patches/README.md`) e buscamos reportá-lo/submetê-lo de volta ao upstream — ex.: o `rmlui-2cd28864-teardown-ub.patch`, que corrige dois achados de undefined behavior na própria ordem de teardown de documento/elemento do RmlUi (ver `docs/embed-integration.md` seção 18 para o relato voltado ao consumidor, e `glintfx/patches/README.md` para o detalhe técnico). Todo patch assim é pensado para ser temporário: aposentado deste repositório assim que a correção equivalente chegar no upstream.
