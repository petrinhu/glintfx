// SPDX-License-Identifier: MPL-2.0
// EN: Live-scalar propagation test — data_model_scalar (T2, v0.2.3).
//
//     Proof: set_string() / set_number() followed by update() reflects the new value in
//     the rendered frame. Two snapshot pairs:
//
//       (1) String path:
//             snapshot A — initial state: label="" (empty), hp=0 (→ "0")
//             set_string("label","HELLO HP 99") → update() → snapshot B
//             assert: bright_B > bright_A  (11 chars appeared vs near-blank)
//
//       (2) Number path:
//             snapshot B from above (label="HELLO HP 99", hp="0")
//             set_number("hp", 99.0) → update() → snapshot C
//             assert: bright_C > bright_B  ("99" replaced "0", one extra glyph)
//
//     Pixel statistic: count of pixels where max(R,G,B) > 100. Deterministic under
//     Xvfb / Mesa llvmpipe for plain text rendering (no blur/glow convolution).
//     Window: 320×160.
//     Document: tests/ui/scalar.rml — binds to data model "hud"; loads OpenSans.
//
// PT: Teste de propagação de escalares vivos — data_model_scalar (T2, v0.2.3).
//
//     Prova: set_string() / set_number() seguido de update() reflete o novo valor no
//     frame renderizado. Dois pares de snapshots:
//
//       (1) Caminho string:
//             snapshot A — estado inicial: label="" (vazio), hp=0 (→ "0")
//             set_string("label","HELLO HP 99") → update() → snapshot B
//             asserção: bright_B > bright_A  (11 chars apareceram vs quase em branco)
//
//       (2) Caminho numérico:
//             snapshot B acima (label="HELLO HP 99", hp="0")
//             set_number("hp", 99.0) → update() → snapshot C
//             asserção: bright_C > bright_B  ("99" substituiu "0", um glifo extra)
//
//     Estatística de pixel: contagem de pixels com max(R,G,B) > 100. Determinístico sob
//     Xvfb / Mesa llvmpipe para renderização de texto simples (sem convolução blur/glow).
//     Janela: 320×160.
//     Documento: tests/ui/scalar.rml — liga ao data model "hud"; carrega OpenSans.
//
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <cstdlib>
#include <vector>

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
        fprintf(stderr, "data_model_scalar: cannot open '%s'\n", path);
        return false;
    }
    char magic[3] = {0};
    if (fscanf(f, "%2s %d %d %*d%*c", magic, &w, &h) != 3) {
        fprintf(stderr, "data_model_scalar: bad PPM header in '%s'\n", path);
        fclose(f);
        return false;
    }
    data.resize((size_t)w * h * 3);
    fread(data.data(), 1, data.size(), f);
    fclose(f);
    return true;
}

// ---------------------------------------------------------------------------
// EN: Count pixels whose maximum channel value exceeds `threshold`.
//     Aggregate statistic; tolerant of per-pixel llvmpipe noise because text
//     rendering (no blur/glow) is deterministic on the software rasteriser.
// PT: Conta pixels cujo valor máximo de canal supera `threshold`.
//     Estatística agregada; tolerante ao ruído per-pixel do llvmpipe porque
//     a renderização de texto (sem blur/glow) é determinística no rasterizador SW.
// ---------------------------------------------------------------------------
static size_t count_bright(const std::vector<unsigned char>& px, int threshold = 100)
{
    size_t n = px.size() / 3;
    size_t count = 0;
    for (size_t i = 0; i < n; ++i) {
        int r = px[i*3+0], g = px[i*3+1], b = px[i*3+2];
        if (r > threshold || g > threshold || b > threshold) ++count;
    }
    return count;
}

// ---------------------------------------------------------------------------
// EN: Helpers: render one extra update+frame and save a snapshot.
// PT: Auxiliares: renderiza um update+frame extra e salva um snapshot.
// ---------------------------------------------------------------------------
static bool take_snapshot(glintfx::App& app, const char* path,
                          std::vector<unsigned char>& px_out, int& w, int& h)
{
    // EN: update() propagates any pending DirtyVariable calls to the DOM;
    //     snapshot() then renders the updated frame and writes the PPM.
    // PT: update() propaga chamadas DirtyVariable pendentes ao DOM;
    //     snapshot() renderiza o frame atualizado e grava o PPM.
    app.update();
    if (!app.snapshot(path)) {
        fprintf(stderr, "data_model_scalar: snapshot('%s') failed\n", path);
        return false;
    }
    if (!read_ppm(path, px_out, w, h)) return false;
    return true;
}

// ---------------------------------------------------------------------------
int main()
{
    // EN: 320×160: small enough to be fast, tall enough to fit two 40 px text lines.
    // PT: 320×160: pequeno o suficiente para ser rápido, alto para duas linhas de 40 px.
    glintfx::AppConfig cfg;
    cfg.title  = "data_model_scalar";
    cfg.width  = 320;
    cfg.height = 160;

    glintfx::App app(cfg);
    if (!app.ok()) {
        // EN: Non-fatal under CI without display — report skip, exit 0.
        // PT: Não-fatal em CI sem display — reporta skip, sai 0.
        std::puts("SKIP: no display / GL context");
        return 0;
    }

    // EN: Setup: create_data_model → bind_string/bind_number → load.
    //     Both scalars must be bound BEFORE load so their data-views are compiled
    //     at document parse time (RmlUi constraint).
    // PT: Setup: create_data_model → bind_string/bind_number → load.
    //     Ambos os escalares devem ser ligados ANTES do load para que as data-views
    //     sejam compiladas no parse do documento (restrição do RmlUi).
    bool ok_model = app.create_data_model("hud");
    bool ok_label = app.bind_string("label", "");       // EN: empty → no visible text.
    bool ok_hp    = app.bind_number("hp",    0.0);      // EN: 0.0 → renders "0".
    bool ok_load  = app.load("tests/ui/scalar.rml");

    if (!ok_model || !ok_label || !ok_hp || !ok_load) {
        fprintf(stderr,
            "data_model_scalar FAIL: setup — "
            "model=%d label=%d hp=%d load=%d\n",
            ok_model, ok_label, ok_hp, ok_load);
        return 1;
    }

    // EN: Two warm-up frames to let RmlUi layout + font atlas settle before capture.
    // PT: Dois frames de aquecimento para o layout e atlas de fontes do RmlUi estabilizarem.
    app.poll_events(); app.update(); app.render();
    app.poll_events(); app.update(); app.render();

    // -------------------------------------------------------------------------
    // EN: Snapshot A — baseline: label="" (empty), hp=0 → "0".
    //     Expected: minimal bright pixels (only the single "0" glyph).
    // PT: Snapshot A — baseline: label="" (vazio), hp=0 → "0".
    //     Esperado: pixels claros mínimos (apenas o glifo "0").
    // -------------------------------------------------------------------------
    std::vector<unsigned char> px_a;
    int wa = 0, ha = 0;
    if (!take_snapshot(app, "scalar_A.ppm", px_a, wa, ha)) return 1;
    size_t bright_a = count_bright(px_a);
    printf("data_model_scalar [A baseline]: bright=%zu  (%dx%d)\n", bright_a, wa, ha);

    // -------------------------------------------------------------------------
    // EN: Snapshot B — after set_string("label","HELLO HP 99"):
    //     "HELLO HP 99" (11 chars at 40 px) appears in the label paragraph.
    //     Expected: bright_B >> bright_A (large text vs near-blank).
    // PT: Snapshot B — após set_string("label","HELLO HP 99"):
    //     "HELLO HP 99" (11 chars a 40 px) aparece no parágrafo label.
    //     Esperado: bright_B >> bright_A (muito texto vs quase vazio).
    // -------------------------------------------------------------------------
    app.set_string("label", "HELLO HP 99");
    std::vector<unsigned char> px_b;
    int wb = 0, hb = 0;
    if (!take_snapshot(app, "scalar_B_str.ppm", px_b, wb, hb)) return 1;
    size_t bright_b = count_bright(px_b);
    printf("data_model_scalar [B after set_string]: bright=%zu\n", bright_b);

    if (bright_b <= bright_a) {
        fprintf(stderr,
            "data_model_scalar FAIL [string path]: "
            "bright_B(%zu) <= bright_A(%zu) — "
            "set_string did not propagate to render\n",
            bright_b, bright_a);
        return 1;
    }

    // -------------------------------------------------------------------------
    // EN: Snapshot C — after set_number("hp", 99.0):
    //     hp changes from "0" (1 char) to "99" (2 chars) in the hp paragraph.
    //     Expected: bright_C > bright_B (one extra numeral glyph at 40 px).
    //     RmlUi formats double via "%.3f" + TrimTrailingDotZeros:
    //       0.0 → "0"   (1 char)
    //       99.0 → "99" (2 chars)
    // PT: Snapshot C — após set_number("hp", 99.0):
    //     hp muda de "0" (1 char) para "99" (2 chars) no parágrafo hp.
    //     Esperado: bright_C > bright_B (um glifo numérico extra a 40 px).
    //     RmlUi formata double via "%.3f" + TrimTrailingDotZeros:
    //       0.0 → "0"   (1 char)
    //       99.0 → "99" (2 chars)
    // -------------------------------------------------------------------------
    app.set_number("hp", 99.0);
    std::vector<unsigned char> px_c;
    int wc = 0, hc = 0;
    if (!take_snapshot(app, "scalar_C_num.ppm", px_c, wc, hc)) return 1;
    size_t bright_c = count_bright(px_c);
    printf("data_model_scalar [C after set_number]: bright=%zu\n", bright_c);

    if (bright_c <= bright_b) {
        fprintf(stderr,
            "data_model_scalar FAIL [number path]: "
            "bright_C(%zu) <= bright_B(%zu) — "
            "set_number did not propagate to render\n",
            bright_c, bright_b);
        return 1;
    }

    std::puts("data_model_scalar: PASS");
    return 0;
}
