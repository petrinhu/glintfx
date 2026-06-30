// SPDX-License-Identifier: MPL-2.0
// EN: Texture PNG/alpha test — texture_png_alpha (T4, v0.2.3).
//
//     Verifies three properties of the PNG LoadTexture override + premultiply:
//
//       (A) PNG decoded (not white fallback):
//             red_count (R>150, G<100, B<100) > 500
//             The circle pixels are red (R=220, G=80, B=80), proving PNG was decoded.
//             Fallback (white texture) would give 0 red pixels.
//
//       (B) Not fallback white:
//             white_count (R>220, G>220, B>220) < 2 000
//             Fallback makes the entire 64x64 img box white (4 096 white pixels > 2 000).
//
//       (C) No halo — premultiplied alpha is correct:
//             reddish_count (R>100, G<100) < 3 500
//             circle_rgba.png stores transparent pixels as (R=220, G=80, B=80, A=0).
//             Without premultiply: GL blend (GL_ONE, GL_ONE_MINUS_SRC_ALPHA) adds the
//               circle color to the background even at A=0, making transparent areas
//               bright reddish — reddish_count grows to ~4 096 (all img box pixels).
//             With premultiply: R=R*A/255=0, G=G*A/255=0 for A=0 → no contribution →
//               transparent areas stay dark → reddish_count ≈ 2 472 (circle only).
//             Threshold 3 500 sits between 2 472 (pass) and 4 096 (fail).
//
//     Window: 256×256. Document: tests/ui/tex.rml (dark body, 64dp×64dp img).
//     PNG: tests/ui/circle_rgba.png (64×64 RGBA; warm-red circle; transparent corners).
//     WORKING_DIRECTORY = CMAKE_BINARY_DIR so "tests/ui/tex.rml" resolves to
//     build/tests/ui/tex.rml (copied by CMake at configure time).
//
// PT: Teste de textura PNG/alpha — texture_png_alpha (T4, v0.2.3).
//
//     Verifica três propriedades do override de LoadTexture PNG + premultiply:
//
//       (A) PNG decodificado (não fallback branco):
//             red_count (R>150, G<100, B<100) > 500
//             Os pixels do círculo são vermelhos (R=220, G=80, B=80), provando que o PNG
//             foi decodificado. Fallback (textura branca) daria 0 pixels vermelhos.
//
//       (B) Não é fallback branco:
//             white_count (R>220, G>220, B>220) < 2 000
//             Fallback torna toda a img box 64x64 branca (4 096 pixels brancos > 2 000).
//
//       (C) Sem halo — alpha premultiplicado está correto:
//             reddish_count (R>100, G<100) < 3 500
//             circle_rgba.png armazena pixels transparentes como (R=220, G=80, B=80, A=0).
//             Sem premultiply: blend GL (GL_ONE, GL_ONE_MINUS_SRC_ALPHA) adiciona a cor do
//               círculo ao fundo mesmo com A=0, tornando as áreas transparentes avermelhadas
//               — reddish_count cresce para ~4 096 (todos os pixels da img box).
//             Com premultiply: R=R*A/255=0, G=G*A/255=0 para A=0 → sem contribuição →
//               áreas transparentes ficam escuras → reddish_count ≈ 2 472 (só círculo).
//             Limiar 3 500 fica entre 2 472 (pass) e 4 096 (fail).
//
//     Janela: 256×256. Documento: tests/ui/tex.rml (body escuro, img 64dp×64dp).
//     PNG: tests/ui/circle_rgba.png (64×64 RGBA; círculo vermelho quente; cantos transparentes).
//     WORKING_DIRECTORY = CMAKE_BINARY_DIR para que "tests/ui/tex.rml" resolva para
//     build/tests/ui/tex.rml (copiado pelo CMake no configure).
//
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>

// ---------------------------------------------------------------------------
// EN: Read a PPM-P6 file into a flat RGB byte vector.
//     Returns false on any parse or I/O error.
// PT: Lê um arquivo PPM-P6 em vetor plano de bytes RGB.
//     Retorna false em qualquer erro de parse ou I/O.
// ---------------------------------------------------------------------------
static bool read_ppm(const char* path, std::vector<unsigned char>& data, int& w, int& h)
{
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "texture_png_alpha: cannot open '%s'\n", path);
        return false;
    }
    char magic[3] = {0};
    if (fscanf(f, "%2s %d %d %*d%*c", magic, &w, &h) != 3) {
        fprintf(stderr, "texture_png_alpha: bad PPM header in '%s'\n", path);
        fclose(f);
        return false;
    }
    if (magic[0] != 'P' || magic[1] != '6') {
        fprintf(stderr, "texture_png_alpha: not a P6 PPM in '%s'\n", path);
        fclose(f);
        return false;
    }
    data.resize((size_t)w * h * 3);
    if (fread(data.data(), 1, data.size(), f) != data.size()) {
        fprintf(stderr, "texture_png_alpha: short read in '%s'\n", path);
        fclose(f);
        return false;
    }
    fclose(f);
    return true;
}

// ---------------------------------------------------------------------------
int main()
{
    // EN: 256×256: fits the 64×64 img element with body background around it.
    // PT: 256×256: comporta a img de 64×64 com fundo do body em volta.
    glintfx::AppConfig cfg;
    cfg.title  = "texture_png_alpha";
    cfg.width  = 256;
    cfg.height = 256;

    glintfx::App app(cfg);
    if (!app.ok()) {
        // EN: Non-fatal under CI without display — report skip, exit 0.
        // PT: Não-fatal em CI sem display — reporta skip, sai 0.
        std::puts("SKIP: no display / GL context");
        return 0;
    }

    if (!app.load("tests/ui/tex.rml")) {
        fputs("texture_png_alpha FAIL: load('tests/ui/tex.rml') returned false\n", stderr);
        return 1;
    }

    // EN: Two warm-up frames: let RmlUi layout + texture upload settle.
    // PT: Dois frames de aquecimento: deixa layout + upload de textura estabilizarem.
    app.poll_events(); app.update(); app.render();
    app.poll_events(); app.update(); app.render();

    // EN: Render one more frame, capture PPM.
    // PT: Renderiza mais um frame, captura PPM.
    app.update();
    if (!app.snapshot("tex_out.ppm")) {
        fputs("texture_png_alpha FAIL: snapshot() returned false\n", stderr);
        return 1;
    }

    std::vector<unsigned char> px;
    int w = 0, h = 0;
    if (!read_ppm("tex_out.ppm", px, w, h)) return 1;

    size_t total = (size_t)w * h;
    if (total == 0) {
        fputs("texture_png_alpha FAIL: captured image is empty\n", stderr);
        return 1;
    }

    // EN: Aggregate pixel statistics.
    // PT: Estatísticas agregadas de pixels.
    size_t red_count     = 0;  // EN: R>150 AND G<100 AND B<100 (circle colour)
                                // PT: R>150 E G<100 E B<100 (cor do círculo)
    size_t white_count   = 0;  // EN: R>220 AND G>220 AND B>220 (fallback white)
                                // PT: R>220 E G>220 E B>220 (branco do fallback)
    size_t reddish_count = 0;  // EN: R>100 AND G<100 (halo probe: reddish pixels)
                                // PT: R>100 E G<100 (sonda de halo: pixels avermelhados)

    for (size_t i = 0; i < total; ++i) {
        int r = px[i*3+0], g = px[i*3+1], b = px[i*3+2];
        if (r > 150 && g < 100 && b < 100) ++red_count;
        if (r > 220 && g > 220 && b > 220) ++white_count;
        if (r > 100 && g < 100)            ++reddish_count;
    }

    printf("texture_png_alpha: %dx%d  red=%zu  white=%zu  reddish=%zu\n",
           w, h, red_count, white_count, reddish_count);

    int failures = 0;

    // -------------------------------------------------------------------------
    // EN: Check A — PNG decoded (not white fallback): red circle pixels present.
    //     circle_rgba.png circle has R=220, G=80, B=80 → satisfies R>150, G<100, B<100.
    //     Expected: ~2 472 red pixels (π × 28²). Threshold: 500 (very conservative).
    //     Fallback (white texture): 0 red pixels → FAIL.
    // PT: Cheque A — PNG decodificado (não fallback branco): pixels do círculo vermelho presentes.
    //     Círculo em circle_rgba.png tem R=220, G=80, B=80 → satisfaz R>150, G<100, B<100.
    //     Esperado: ~2 472 pixels vermelhos (π × 28²). Limiar: 500 (muito conservador).
    //     Fallback (textura branca): 0 pixels vermelhos → FAIL.
    // -------------------------------------------------------------------------
    if (red_count < 500) {
        fprintf(stderr,
            "texture_png_alpha FAIL [A] red_count=%zu < 500 "
            "— PNG not decoded (white fallback?)\n",
            red_count);
        ++failures;
    }

    // -------------------------------------------------------------------------
    // EN: Check B — not fallback white: few white pixels in the frame.
    //     Fallback renders the entire 64×64 img box white = 4 096 white pixels > 2 000.
    //     Correctly decoded PNG: circle is red, transparent area is dark body → ~0 white pixels.
    // PT: Cheque B — não é fallback branco: poucos pixels brancos no frame.
    //     Fallback renderiza toda a img box 64×64 branca = 4 096 pixels brancos > 2 000.
    //     PNG corretamente decodificado: círculo é vermelho, área transparente é body escuro → ~0 brancos.
    // -------------------------------------------------------------------------
    if (white_count > 2000) {
        fprintf(stderr,
            "texture_png_alpha FAIL [B] white_count=%zu > 2 000 "
            "— image appears to be fallback white\n",
            white_count);
        ++failures;
    }

    // -------------------------------------------------------------------------
    // EN: Check C — no halo (premultiplied alpha correct): reddish pixels only from circle.
    //     circle_rgba.png transparent pixels carry RGB=(220,80,80) with A=0 (alpha-straight storage).
    //     Without premultiply: GL blend GL_ONE+GL_ONE_MINUS_SRC_ALPHA uses the full RGB even at
    //       A=0 → adds (220,80,80) to every transparent pixel → all 1 624 transparent img pixels
    //       become reddish → reddish_count ≈ 2 472 + 1 624 = 4 096.
    //     With premultiply (R=R*A/255): A=0 → RGB=(0,0,0) → no colour contribution →
    //       transparent pixels stay dark (body #111111) → reddish_count ≈ 2 472 (circle only).
    //     Threshold 3 500 is between 2 472 (pass) and 4 096 (fail).
    // PT: Cheque C — sem halo (alpha premultiplicado correto): pixels avermelhados apenas do círculo.
    //     Pixels transparentes de circle_rgba.png têm RGB=(220,80,80) com A=0 (armazenamento straight).
    //     Sem premultiply: blend GL GL_ONE+GL_ONE_MINUS_SRC_ALPHA usa RGB completo mesmo com A=0
    //       → adiciona (220,80,80) a cada pixel transparente → todos os 1 624 pixels transparentes
    //       da img ficam avermelhados → reddish_count ≈ 2 472 + 1 624 = 4 096.
    //     Com premultiply (R=R*A/255): A=0 → RGB=(0,0,0) → sem contribuição de cor →
    //       pixels transparentes ficam escuros (body #111111) → reddish_count ≈ 2 472 (só círculo).
    //     Limiar 3 500 fica entre 2 472 (pass) e 4 096 (fail).
    // -------------------------------------------------------------------------
    if (reddish_count > 3500) {
        fprintf(stderr,
            "texture_png_alpha FAIL [C] reddish_count=%zu > 3 500 "
            "— halo detected: premultiply may be missing\n",
            reddish_count);
        ++failures;
    }

    if (failures == 0) {
        std::puts("texture_png_alpha: PASS");
        return 0;
    }
    fprintf(stderr, "texture_png_alpha: %d check(s) FAILED\n", failures);
    return failures;
}
