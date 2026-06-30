// SPDX-License-Identifier: MPL-2.0
// EN: Structural render sanity test — renders the CI showcase (showcase_test.rml), captures a
//     frame via App::snapshot, and verifies coarse pixel statistics: not black, has dark
//     background, has colourful/bright content.
//     Unlike the pixel-golden test (GLINTFX_GOLDEN_TEST), this check is tolerant of per-pixel
//     variation caused by llvmpipe's threaded tile rendering: blur/glow convolution accumulation
//     order varies by thread scheduling → individual pixels differ across runs (MSE ~3 000),
//     but aggregate statistics (mean brightness, dark/bright pixel counts) remain stable.
//     This makes render_sanity a deterministic CI gate for the glintfx visual pipeline.
// PT: Teste de sanidade de renderização estrutural — renderiza o showcase CI (showcase_test.rml),
//     captura um frame via App::snapshot e verifica estatísticas grosseiras de pixels: não é
//     preto, tem fundo escuro, tem conteúdo colorido/brilhante.
//     Ao contrário do pixel-golden (GLINTFX_GOLDEN_TEST), esta verificação tolera variação
//     per-pixel causada pelo rendering de tiles em threads do llvmpipe: a ordem de acumulação
//     de convolução de blur/glow varia pelo agendamento de threads → pixels individuais diferem
//     entre execuções (MSE ~3 000), mas estatísticas agregadas (brilho médio, contagem de
//     pixels escuros/brilhantes) permanecem estáveis.
//     Isso torna render_sanity um gate determinístico de CI para o pipeline visual do glintfx.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <vector>
#include <cstdlib>

// EN: Read a PPM-P6 file into a flat RGB byte vector; returns false on any error.
// PT: Lê um arquivo PPM-P6 em vetor plano de bytes RGB; retorna false em qualquer erro.
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

int main() {
    // EN: 900×600 matches the golden_test window so assets/RCSS resolve the same path.
    // PT: 900×600 coincide com a janela do golden_test para que assets/RCSS resolvam o mesmo path.
    glintfx::App app({ .title = "render_sanity", .width = 900, .height = 600 });
    app.load("showcase_test.rml");

    // EN: Two warm-up frames: let RmlUi layout/font-engine settle before capture.
    // PT: Dois frames de aquecimento: deixa o layout/font-engine do RmlUi estabilizar.
    app.poll_events(); app.update(); app.render();
    app.poll_events(); app.update(); app.render();

    // EN: App::snapshot renders one extra frame, reads FBO 0 before swap, saves PPM, then swaps.
    // PT: App::snapshot renderiza um frame extra, lê FBO 0 antes do swap, salva PPM, depois troca.
    bool cap_ok = app.snapshot("sanity_out.ppm");
    if (!cap_ok) {
        fputs("render_sanity FAIL: snapshot() returned false\n", stderr);
        return 1;
    }

    std::vector<unsigned char> px;
    int w = 0, h = 0;
    if (!read_ppm("sanity_out.ppm", px, w, h)) {
        fputs("render_sanity FAIL: could not read sanity_out.ppm\n", stderr);
        return 1;
    }
    size_t total = (size_t)w * h;
    if (total == 0) {
        fputs("render_sanity FAIL: captured image is empty\n", stderr);
        return 1;
    }

    // EN: Aggregate statistics over all pixels.
    // PT: Estatísticas agregadas sobre todos os pixels.
    long long brightness_sum = 0;
    size_t dark_count   = 0; // EN: pixels with R+G+B < 120   PT: pixels com R+G+B < 120
    size_t bright_count = 0; // EN: pixels with max(R,G,B)>150 PT: pixels com max(R,G,B)>150

    for (size_t i = 0; i < total; ++i) {
        int r = px[i*3+0], g = px[i*3+1], b = px[i*3+2];
        brightness_sum += r + g + b;
        if (r + g + b < 120)          ++dark_count;
        if (r > 150 || g > 150 || b > 150) ++bright_count;
    }

    double mean_brightness = (double)brightness_sum / ((double)total * 3.0);
    double dark_pct   = 100.0 * (double)dark_count   / (double)total;
    double bright_pct = 100.0 * (double)bright_count / (double)total;

    printf("render_sanity: %dx%d  mean=%.1f  dark=%.1f%%  bright=%.1f%%\n",
           w, h, mean_brightness, dark_pct, bright_pct);

    int failures = 0;

    // EN: Check 1 — not all black: scene rendered something visible.
    //     The dark body background (#0d1020: R=13,G=16,B=32) contributes ~20 per channel,
    //     so a correctly rendered frame has mean >> 5 even if mostly dark.
    // PT: Cheque 1 — não tudo preto: a cena renderizou algo visível.
    //     O fundo escuro (#0d1020: R=13,G=16,B=32) contribui ~20 por canal, então um frame
    //     corretamente renderizado tem média >> 5 mesmo que majoritariamente escuro.
    if (mean_brightness < 5.0) {
        fprintf(stderr,
            "render_sanity FAIL [1] mean_brightness=%.1f < 5 — image appears all-black\n",
            mean_brightness);
        ++failures;
    }

    // EN: Check 2 — dark background pixels present: body/.section-dark rendered correctly.
    //     Body background #0d1020 has R+G+B=61, safely below the 120 threshold.
    //     Requiring at least 10% of the 900×600 frame (= 54 000 px) to qualify as dark.
    // PT: Cheque 2 — pixels de fundo escuro presentes: body/.section-dark renderizado.
    //     Fundo #0d1020 tem R+G+B=61, seguramente abaixo do limiar 120.
    //     Requerendo ao menos 10% do frame 900×600 (= 54 000 px) classificados como escuros.
    if (dark_count < total / 10) {
        fprintf(stderr,
            "render_sanity FAIL [2] dark_count=%zu < %zu (10%%) — background may not have rendered\n",
            dark_count, total / 10);
        ++failures;
    }

    // EN: Check 3 — bright/colourful pixels present: gradient section + glow effect rendered.
    //     .section-gradient uses linear-gradient(135deg, #6a0ddc[B=220], #e02828[R=224],
    //     #0f8a4a[G=138]). The glow card box-shadow is cyan #5fd0ff (B=255). All exceed 150.
    //     Requiring at least 1% of pixels (= 5 400 px) to have any channel > 150.
    // PT: Cheque 3 — pixels brilhantes/coloridos presentes: seção gradiente + glow renderizados.
    //     .section-gradient usa linear-gradient(135deg, #6a0ddc[B=220], #e02828[R=224],
    //     #0f8a4a[G=138]). O box-shadow do card glow é ciano #5fd0ff (B=255). Todos > 150.
    //     Requerendo ao menos 1% dos pixels (= 5 400 px) com algum canal > 150.
    if (bright_count < total / 100) {
        fprintf(stderr,
            "render_sanity FAIL [3] bright_count=%zu < %zu (1%%) — gradient/glow effects may be missing\n",
            bright_count, total / 100);
        ++failures;
    }

    if (failures == 0) {
        fputs("render_sanity: PASS\n", stdout);
        return 0;
    }
    fprintf(stderr, "render_sanity: %d check(s) FAILED\n", failures);
    return failures;
}
