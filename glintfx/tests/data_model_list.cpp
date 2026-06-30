// SPDX-License-Identifier: MPL-2.0
// EN: Live-list propagation test — data_model_list (T3, v0.2.3).
//
//     Proves that bind_list() + set_list() wires a std::vector<Rml::String> to a
//     data-for loop in the RML document and that mutations propagate to the render.
//
//     Snapshot pairs:
//
//       Snapshot A — initial state: log list is empty.
//                    Expected: near-zero bright pixels (only the dark background).
//
//       Snapshot B — after set_list("log", items, 3):
//                    "linha 1", "linha 2", "linha 3" are rendered as three <p> blocks.
//                    Expected: bright_B > bright_A (three text lines appeared).
//
//     Pixel statistic: count of pixels where max(R,G,B) > 100.
//     Deterministic under Xvfb / Mesa llvmpipe for plain text (no blur/glow convolution).
//     Window: 400x300.
//     Document: tests/ui/log.rml — binds to data model "hud"; iterates list "log".
//
// PT: Teste de propagação de lista viva — data_model_list (T3, v0.2.3).
//
//     Prova que bind_list() + set_list() liga um std::vector<Rml::String> a um
//     laço data-for no documento RML e que mutações propagam para o render.
//
//     Pares de snapshots:
//
//       Snapshot A — estado inicial: lista log vazia.
//                    Esperado: pixels claros quase nulos (apenas fundo escuro).
//
//       Snapshot B — após set_list("log", items, 3):
//                    "linha 1", "linha 2", "linha 3" são renderizados como três <p>.
//                    Esperado: bright_B > bright_A (três linhas de texto aparecem).
//
//     Estatística de pixel: contagem de pixels com max(R,G,B) > 100.
//     Determinístico sob Xvfb / Mesa llvmpipe para texto simples (sem blur/glow).
//     Janela: 400x300.
//     Documento: tests/ui/log.rml — liga ao data model "hud"; itera lista "log".
//
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// ---------------------------------------------------------------------------
// EN: Read a PPM-P6 file into a flat RGB byte vector.
//     Validates the "P6" magic and checks the fread return value.
//     Returns false on any parse, I/O, or format error.
// PT: Lê um arquivo PPM-P6 em vetor plano de bytes RGB.
//     Valida o magic "P6" e verifica o retorno do fread.
//     Retorna false em qualquer erro de parse, I/O ou formato.
// ---------------------------------------------------------------------------
static bool read_ppm(const char* path,
                     std::vector<unsigned char>& data, int& w, int& h)
{
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "data_model_list: cannot open '%s'\n", path);
        return false;
    }

    char magic[3] = {0};
    int  maxval   = 0;
    // EN: Parse header: magic, width, height, maxval, then one whitespace byte.
    // PT: Parse do header: magic, largura, altura, maxval, depois um byte de espaço.
    if (fscanf(f, "%2s %d %d %d", magic, &w, &h, &maxval) != 4) {
        fprintf(stderr, "data_model_list: bad PPM header in '%s'\n", path);
        fclose(f);
        return false;
    }
    if (strcmp(magic, "P6") != 0) {
        fprintf(stderr,
            "data_model_list: expected P6 magic, got '%s' in '%s'\n",
            magic, path);
        fclose(f);
        return false;
    }
    // EN: Reject non-8-bit PPM: maxval != 255 means 16-bit or non-standard encoding;
    //     the pixel analysis assumes 1-byte-per-channel (3 bytes per pixel).
    // PT: Rejeita PPM não-8-bit: maxval != 255 indica codificação 16-bit ou não-padrão;
    //     a análise de pixels assume 1 byte por canal (3 bytes por pixel).
    if (maxval != 255) {
        fprintf(stderr,
            "data_model_list: unsupported PPM maxval %d in '%s' (expected 255)\n",
            maxval, path);
        fclose(f);
        return false;
    }
    // EN: Consume the single whitespace byte that follows the header.
    // PT: Consome o único byte de espaço que segue o header.
    fgetc(f);

    const size_t nbytes = (size_t)w * h * 3;
    data.resize(nbytes);
    const size_t nread = fread(data.data(), 1, nbytes, f);
    fclose(f);

    if (nread != nbytes) {
        fprintf(stderr,
            "data_model_list: short read from '%s': got %zu, expected %zu\n",
            path, nread, nbytes);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// EN: Count pixels whose maximum channel value exceeds `threshold`.
//     Aggregate statistic; tolerant of per-pixel llvmpipe variation because
//     plain text rendering (no blur/glow) is deterministic on the software rasteriser.
// PT: Conta pixels cujo valor máximo de canal supera `threshold`.
//     Estatística agregada; tolerante ao ruído per-pixel do llvmpipe porque
//     a renderização de texto simples (sem blur/glow) é determinística no rasterizador SW.
// ---------------------------------------------------------------------------
static size_t count_bright(const std::vector<unsigned char>& px,
                           int threshold = 100)
{
    const size_t n = px.size() / 3;
    size_t count = 0;
    for (size_t i = 0; i < n; ++i) {
        const int r = px[i*3+0], g = px[i*3+1], b = px[i*3+2];
        if (r > threshold || g > threshold || b > threshold) ++count;
    }
    return count;
}

// ---------------------------------------------------------------------------
// EN: Render one update+frame and save a snapshot to `path`.
//     App::snapshot() renders before the swap so the captured pixels reflect
//     the DirtyVariable changes propagated by update().
// PT: Renderiza um update+frame e salva um snapshot em `path`.
//     App::snapshot() renderiza antes do swap para que os pixels capturados
//     reflitam as mudanças de DirtyVariable propagadas por update().
// ---------------------------------------------------------------------------
static bool take_snapshot(glintfx::App& app, const char* path,
                          std::vector<unsigned char>& px_out, int& w, int& h)
{
    app.update();
    if (!app.snapshot(path)) {
        fprintf(stderr, "data_model_list: snapshot('%s') failed\n", path);
        return false;
    }
    if (!read_ppm(path, px_out, w, h)) return false;
    return true;
}

// ---------------------------------------------------------------------------
int main()
{
    // EN: 400x300: wide enough to show three text lines without wrapping.
    // PT: 400x300: largo o suficiente para mostrar três linhas de texto sem quebra.
    glintfx::AppConfig cfg;
    cfg.title  = "data_model_list";
    cfg.width  = 400;
    cfg.height = 300;

    glintfx::App app(cfg);
    if (!app.ok()) {
        // EN: Non-fatal under CI without display — report skip, exit 0.
        // PT: Não-fatal em CI sem display — reporta skip, sai 0.
        std::puts("SKIP: no display / GL context");
        return 0;
    }

    // EN: Setup: create_data_model -> bind_list -> load.
    //     bind_list must be called BEFORE load so the data-for view is compiled
    //     at document parse time (RmlUi constraint).
    // PT: Setup: create_data_model -> bind_list -> load.
    //     bind_list deve ser chamado ANTES do load para que a view data-for seja
    //     compilada no parse do documento (restrição do RmlUi).
    const bool ok_model = app.create_data_model("hud");
    const bool ok_list  = app.bind_list("log");
    const bool ok_load  = app.load("tests/ui/log.rml");

    if (!ok_model || !ok_list || !ok_load) {
        fprintf(stderr,
            "data_model_list FAIL: setup — "
            "model=%d list=%d load=%d\n",
            ok_model, ok_list, ok_load);
        return 1;
    }

    // EN: Two warm-up frames to let RmlUi layout + font atlas settle before capture.
    // PT: Dois frames de aquecimento para o layout e atlas de fontes do RmlUi estabilizarem.
    app.poll_events(); app.update(); app.render();
    app.poll_events(); app.update(); app.render();

    // -------------------------------------------------------------------------
    // EN: Snapshot A — baseline: log list is empty.
    //     Expected: near-zero bright pixels (only dark background visible).
    // PT: Snapshot A — baseline: lista log vazia.
    //     Esperado: pixels claros quase nulos (apenas fundo escuro visível).
    // -------------------------------------------------------------------------
    std::vector<unsigned char> px_a;
    int wa = 0, ha = 0;
    if (!take_snapshot(app, "list_A.ppm", px_a, wa, ha)) return 1;
    const size_t bright_a = count_bright(px_a);
    printf("data_model_list [A baseline, empty list]: bright=%zu  (%dx%d)\n",
           bright_a, wa, ha);

    // -------------------------------------------------------------------------
    // EN: Snapshot B — after set_list("log", 3 items):
    //     Three <p> lines are rendered: "linha 1", "linha 2", "linha 3".
    //     Expected: bright_B > bright_A (text pixels appeared).
    // PT: Snapshot B — após set_list("log", 3 itens):
    //     Três linhas <p> são renderizadas: "linha 1", "linha 2", "linha 3".
    //     Esperado: bright_B > bright_A (pixels de texto aparecem).
    // -------------------------------------------------------------------------
    const char* items[] = {"linha 1", "linha 2", "linha 3"};
    app.set_list("log", items, 3);

    std::vector<unsigned char> px_b;
    int wb = 0, hb = 0;
    if (!take_snapshot(app, "list_B_3items.ppm", px_b, wb, hb)) return 1;
    const size_t bright_b = count_bright(px_b);
    printf("data_model_list [B after set_list x3]: bright=%zu\n", bright_b);

    if (bright_b <= bright_a) {
        fprintf(stderr,
            "data_model_list FAIL [list path]: "
            "bright_B(%zu) <= bright_A(%zu) — "
            "set_list did not propagate to render\n",
            bright_b, bright_a);
        return 1;
    }

    std::puts("data_model_list: PASS");
    return 0;
}
