// SPDX-License-Identifier: MPL-2.0
// EN: fonteng_own_sanity -- L1.19-FONTENG (FT-F3) minimal visual oracle.
//
//     Renders tests/fonteng_scene.rml (3 static text lines: ascender/descender-heavy ASCII,
//     lowercase ASCII, and the pt-br-accented Latin-1 Supplement set) and verifies, via
//     coarse pixel statistics on the captured frame (the same statistical-oracle discipline
//     render_sanity.cpp/data_model_scalar.cpp already use -- tolerant of llvmpipe's
//     non-deterministic tile-rendering order, exact for plain un-blurred text):
//
//       (1) Some "ink" (bright, near-white-on-black) pixels exist at all -- proves SOMETHING
//           rendered (catches the "renders nothing" regression class: e.g. a face that never
//           resolves, an atlas that stays all-zero, a GenerateString that returns an empty
//           mesh_list).
//       (2) Ink coverage stays well under a full-block threshold -- proves the atlas is
//           actually shaped GLYPHS, not a solid rectangle (catches the "renders a filled box
//           instead of text" regression class -- e.g. a coverage-atlas blit bug that always
//           writes 255 instead of the real per-pixel coverage byte).
//       (3) EACH of the 3 text lines' own horizontal band has SOME ink -- proves all three
//           lines rendered, not just the first (line 3 is the pt-br-accented one -- this
//           specifically exercises BuildBakeSet()'s accented codepoints, not just plain
//           ASCII, per this ticket's own scope).
//
//     This test is NOT gated behind GLINTFX_OWN_FONT_ENGINE -- it runs identically whichever
//     font engine is active (RmlUi's FreeType default, OFF/default; or glintfx's own
//     SOV-SFNT/SOV-RAST engine, ON) precisely BECAUSE the oracle is engine-agnostic (any
//     correct font engine must satisfy checks 1-3 for this document). That symmetry is the
//     point of the A/B design (see glintfx/CMakeLists.txt's GLINTFX_OWN_FONT_ENGINE option):
//     the same test proves both sides render legible text, without needing two different
//     assertions per side.
// PT: fonteng_own_sanity -- oráculo visual mínimo do L1.19-FONTENG (FT-F3).
//
//     Renderiza tests/fonteng_scene.rml (3 linhas de texto estático: ASCII carregado de
//     ascendente/descendente, ASCII minúsculo, e o conjunto Latin-1 Supplement acentuado
//     pt-br) e verifica, via estatísticas grosseiras de pixel no frame capturado (a mesma
//     disciplina de oráculo estatístico que render_sanity.cpp/data_model_scalar.cpp já usam
//     -- tolerante à ordem não-determinística de rendering de tiles do llvmpipe, exata para
//     texto simples sem blur):
//
//       (1) Existem alguns pixels de "tinta" (claros, quase-branco-sobre-preto) -- prova que
//           ALGO renderizou (pega a classe de regressão "não renderiza nada": ex.: uma face
//           que nunca resolve, um atlas que fica todo-zero, um GenerateString que retorna um
//           mesh_list vazio).
//       (2) A cobertura de tinta fica bem abaixo de um limiar de bloco cheio -- prova que o
//           atlas de fato tem GLYPHS com forma, não um retângulo sólido (pega a classe de
//           regressão "renderiza uma caixa preenchida em vez de texto" -- ex.: um bug de
//           blit do atlas de cobertura que sempre escreve 255 em vez do byte de cobertura
//           real por-pixel).
//       (3) CADA uma das 3 linhas de texto tem SUA PRÓPRIA banda horizontal com ALGUMA tinta
//           -- prova que as três linhas renderizaram, não só a 1ª (a linha 3 é a acentuada
//           pt-br -- isto exercita especificamente os codepoints acentuados do
//           BuildBakeSet(), não só ASCII puro, conforme o próprio escopo desta tarefa).
//
//     Este teste NÃO é bloqueado atrás de GLINTFX_OWN_FONT_ENGINE -- roda identicamente com
//     qualquer motor de fonte ativo (default FreeType do RmlUi, OFF/padrão; ou o motor
//     próprio SOV-SFNT/SOV-RAST do glintfx, ON) precisamente PORQUE o oráculo é
//     agnóstico-de-motor (qualquer motor de fonte correto precisa satisfazer os checks 1-3
//     pra este documento). Essa simetria é o ponto do design A/B (ver a option
//     GLINTFX_OWN_FONT_ENGINE do glintfx/CMakeLists.txt): o mesmo teste prova que os dois
//     lados renderizam texto legível, sem precisar de duas asserções diferentes por lado.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <vector>

static bool read_ppm(const char* path, std::vector<unsigned char>& data, int& w, int& h) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    char magic[3] = {0};
    if (fscanf(f, "%2s %d %d %*d%*c", magic, &w, &h) != 3) { fclose(f); return false; }
    data.resize((size_t)w * h * 3);
    fread(data.data(), 1, data.size(), f);
    fclose(f);
    return true;
}

// EN: Counts "ink" pixels (max channel > threshold) within row range [row_begin, row_end).
// PT: Conta pixels de "tinta" (canal máximo > threshold) na faixa de linha [row_begin, row_end).
static size_t count_bright_rows(const std::vector<unsigned char>& px, int w, int row_begin, int row_end,
                                 int threshold = 100) {
    size_t count = 0;
    for (int y = row_begin; y < row_end; ++y) {
        for (int x = 0; x < w; ++x) {
            const size_t i = (size_t)(y * w + x) * 3;
            const int r = px[i + 0], g = px[i + 1], b = px[i + 2];
            if (r > threshold || g > threshold || b > threshold) ++count;
        }
    }
    return count;
}

int main() {
    // EN: 640x260: wide enough for "acao ceu izq oru nao" at 48px, tall enough for 3 lines
    //     (~72px each incl. line-height/margin) with headroom.
    // PT: 640x260: largo o bastante para "acao ceu izq oru nao" a 48px, alto o bastante para
    //     3 linhas (~72px cada incl. altura-de-linha/margem) com folga.
    glintfx::App app({ .title = "fonteng_own_sanity", .width = 640, .height = 260 });
    if (!app.ok()) {
        std::puts("SKIP: no display / GL context");
        return 0;
    }
    if (!app.load("tests/fonteng_scene.rml")) {
        fputs("fonteng_own_sanity FAIL: load() returned false\n", stderr);
        return 1;
    }

    app.poll_events(); app.update(); app.render();
    app.poll_events(); app.update(); app.render();

    if (!app.snapshot("fonteng_own_sanity_out.ppm")) {
        fputs("fonteng_own_sanity FAIL: snapshot() returned false\n", stderr);
        return 1;
    }

    std::vector<unsigned char> px;
    int w = 0, h = 0;
    if (!read_ppm("fonteng_own_sanity_out.ppm", px, w, h)) {
        fputs("fonteng_own_sanity FAIL: could not read fonteng_own_sanity_out.ppm\n", stderr);
        return 1;
    }
    const size_t total = (size_t)w * h;
    if (total == 0) {
        fputs("fonteng_own_sanity FAIL: captured image is empty\n", stderr);
        return 1;
    }

    const size_t bright_total = count_bright_rows(px, w, 0, h);
    const double bright_pct = 100.0 * (double)bright_total / (double)total;

    // EN: Each of the 3 <p> lines occupies roughly one third of the (margin-adjusted) body
    //     height -- split the frame into 3 equal horizontal bands as an approximation (no
    //     exact per-line layout query needed; the oracle only needs "some ink somewhere in
    //     this third", not pixel-exact bounds).
    // PT: Cada uma das 3 linhas <p> ocupa aproximadamente um terço da altura do body (ajustado
    //     por margem) -- divide o frame em 3 bandas horizontais iguais como aproximação (sem
    //     necessidade de query exata de layout por-linha; o oráculo só precisa de "alguma
    //     tinta em algum lugar deste terço", não limites pixel-exatos).
    const int band_h = h / 3;
    const size_t bright_line1 = count_bright_rows(px, w, 0, band_h);
    const size_t bright_line2 = count_bright_rows(px, w, band_h, 2 * band_h);
    const size_t bright_line3 = count_bright_rows(px, w, 2 * band_h, h);

    printf("fonteng_own_sanity: %dx%d  bright_total=%zu (%.2f%%)  line1=%zu line2=%zu line3=%zu\n", w,
           h, bright_total, bright_pct, bright_line1, bright_line2, bright_line3);

    int failures = 0;

    // EN: Check 1 -- some ink exists (0.3% of a 640x260 frame ~ 500 px -- generous floor for
    //     3 lines of 48px text, comfortably above capture/compression noise).
    // PT: Cheque 1 -- existe alguma tinta (0.3% de um frame 640x260 ~ 500 px -- piso generoso
    //     pra 3 linhas de texto a 48px, confortavelmente acima de ruído de captura).
    if (bright_pct < 0.3) {
        fprintf(stderr, "fonteng_own_sanity FAIL [1] bright_pct=%.2f%% < 0.3%% -- no text rendered\n",
                bright_pct);
        ++failures;
    }

    // EN: Check 2 -- ink stays well under a filled-block threshold (glyphs are shapes, not a
    //     solid rectangle -- 40% is generous headroom above what dense 48px text realistically
    //     covers, while still catching a "coverage always 255" atlas-blit bug).
    // PT: Cheque 2 -- a tinta fica bem abaixo de um limiar de bloco preenchido (glyphs são
    //     formas, não um retângulo sólido -- 40% é folga generosa acima do que texto denso a
    //     48px realisticamente cobre, mas ainda pega um bug de blit "cobertura sempre 255").
    if (bright_pct > 40.0) {
        fprintf(stderr,
            "fonteng_own_sanity FAIL [2] bright_pct=%.2f%% > 40%% -- looks like a filled block, not glyphs\n",
            bright_pct);
        ++failures;
    }

    // EN: Check 3 -- each of the 3 lines' own band has some ink (line 3 = pt-br accented set).
    // PT: Cheque 3 -- cada uma das 3 linhas tem alguma tinta na própria banda (linha 3 =
    //     conjunto acentuado pt-br).
    if (bright_line1 == 0) {
        fputs("fonteng_own_sanity FAIL [3a] line1 (ASCII ascender/descender) band is fully blank\n", stderr);
        ++failures;
    }
    if (bright_line2 == 0) {
        fputs("fonteng_own_sanity FAIL [3b] line2 (lowercase ASCII) band is fully blank\n", stderr);
        ++failures;
    }
    if (bright_line3 == 0) {
        fputs("fonteng_own_sanity FAIL [3c] line3 (pt-br accented) band is fully blank\n", stderr);
        ++failures;
    }

    if (failures == 0) {
        fputs("fonteng_own_sanity: PASS\n", stdout);
        return 0;
    }
    fprintf(stderr, "fonteng_own_sanity: %d check(s) FAILED\n", failures);
    return failures;
}
