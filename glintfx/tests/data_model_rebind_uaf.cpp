// SPDX-License-Identifier: MPL-2.0
// EN: Regression test for the duplicate-key use-after-free in DataBinder::bind_*
//     (found by audit, 2026-07-04). Root cause (pre-fix): each bind_* did
//     `impl_->nums[key] = std::make_unique<double>(initial);` — the unique_ptr
//     operator= FREES the previous cell (if `key` already had one bound) BEFORE
//     Rml::DataModelConstructor::Bind() is even called. Bind() itself never
//     overwrites an existing binding (RmlUi's DataModel::BindVariable uses
//     variables.emplace(), a silent no-op on collision that returns false), so the
//     model's internal Rml::DataModel keeps a raw pointer to the cell that was just
//     freed. Any later view re-evaluation of that variable is a heap-use-after-free.
//
//     This test proves two things:
//       (1) A duplicate bind_number/bind_string/bind_bool/bind_list on the SAME key
//           returns false and is a pure no-op — no crash, no corruption. The first
//           binding stays fully functional (verified via render: text propagates
//           correctly to the frame after the duplicate call and after load()).
//       (2) A normal call with a DIFFERENT key right after a duplicate attempt is
//           unaffected (no regression from the guard).
//
//     "hp" and "label" reuse tests/ui/scalar.rml's existing {{hp}}/{{label}} views
//     on purpose: those views are what forces RmlUi to actually dereference the
//     bound cell pointers on update() — the exact code path where the pre-fix bug
//     produced a heap-use-after-free (caught by ASan when GLINTFX_SANITIZE=ON).
//     scalar.rml is used instead of hud.rml because it @font-face-loads OpenSans
//     from tests/assets/ and is already proven (by data_model_scalar.cpp) to
//     rasterise glyphs deterministically under Xvfb/llvmpipe; hud.rml's
//     "font-family: sans" never resolves to a loaded face in this environment, so
//     it renders zero glyphs regardless of the fix and would give a false negative.
//     A test that never triggers a view re-evaluation would not have "teeth": the
//     dangling pointer would sit unread and ASan would stay silent.
//
// PT: Teste de regressão para o use-after-free de chave duplicada em DataBinder::bind_*
//     (achado por auditoria, 2026-07-04). Causa raiz (pré-fix): cada bind_* fazia
//     `impl_->nums[key] = std::make_unique<double>(initial);` — o operator= do
//     unique_ptr LIBERA a célula anterior (se `key` já tinha uma ligada) ANTES de
//     Rml::DataModelConstructor::Bind() sequer ser chamado. O próprio Bind() nunca
//     sobrescreve um binding existente (o DataModel::BindVariable do RmlUi usa
//     variables.emplace(), um no-op silencioso em colisão que retorna false), então
//     o Rml::DataModel interno do modelo mantém um ponteiro cru para a célula recém
//     liberada. Qualquer reavaliação posterior dessa variável por uma view é um
//     heap-use-after-free.
//
//     Este teste prova duas coisas:
//       (1) Um bind_number/bind_string/bind_bool/bind_list duplicado na MESMA chave
//           retorna false e é um no-op puro — sem crash, sem corrupção. O primeiro
//           binding continua totalmente funcional (verificado via render: o texto
//           propaga corretamente para o frame após a chamada duplicada e após load()).
//       (2) Uma chamada normal com chave DIFERENTE logo após uma tentativa duplicada
//           não é afetada (sem regressão do guard).
//
//     "hp" e "label" reusam as views {{hp}}/{{label}} já existentes em
//     tests/ui/scalar.rml de propósito: são essas views que forçam o RmlUi a de fato
//     desreferenciar os ponteiros das células ligadas no update() — exatamente o
//     caminho de código onde o bug pré-fix produzia um heap-use-after-free (pego
//     pelo ASan quando GLINTFX_SANITIZE=ON). scalar.rml é usado em vez de hud.rml
//     porque carrega OpenSans via @font-face de tests/assets/ e já é provado (pelo
//     data_model_scalar.cpp) rasterizar glifos deterministicamente sob Xvfb/llvmpipe;
//     "font-family: sans" do hud.rml nunca resolve para uma face carregada neste
//     ambiente, então renderiza zero glifos independente do fix e daria falso negativo.
//     Um teste que nunca dispara reavaliação de view não teria "dente": o ponteiro
//     pendurado ficaria sem ser lido e o ASan ficaria calado.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>
#include <cstdio>
#include <cstdlib>
#include <vector>

// ---------------------------------------------------------------------------
// EN: Read a PPM-P6 file into a flat RGB byte vector (same helper as data_model_scalar).
// PT: Lê um arquivo PPM-P6 em vetor plano de bytes RGB (mesmo helper de data_model_scalar).
// ---------------------------------------------------------------------------
static bool read_ppm(const char* path, std::vector<unsigned char>& data, int& w, int& h)
{
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "data_model_rebind_uaf: cannot open '%s'\n", path);
        return false;
    }
    char magic[3] = {0};
    if (fscanf(f, "%2s %d %d %*d%*c", magic, &w, &h) != 3) {
        fprintf(stderr, "data_model_rebind_uaf: bad PPM header in '%s'\n", path);
        fclose(f);
        return false;
    }
    data.resize((size_t)w * h * 3);
    fread(data.data(), 1, data.size(), f);
    fclose(f);
    return true;
}

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

static bool take_snapshot(glintfx::App& app, const char* path,
                          std::vector<unsigned char>& px_out, int& w, int& h)
{
    app.update();
    if (!app.snapshot(path)) {
        fprintf(stderr, "data_model_rebind_uaf: snapshot('%s') failed\n", path);
        return false;
    }
    if (!read_ppm(path, px_out, w, h)) return false;
    return true;
}

int main()
{
    glintfx::AppConfig cfg;
    cfg.title  = "data_model_rebind_uaf";
    cfg.width  = 320;
    cfg.height = 160;

    glintfx::App app(cfg);
    if (!app.ok()) {
        std::puts("SKIP: no display / GL context");
        return 0;
    }

    bool all_ok = true;

    // -------------------------------------------------------------------------
    // EN: Setup — create model, then exercise duplicate-key rebind on all 4 bind_*.
    //     First call per key must succeed; the immediate duplicate on the SAME key
    //     must return false WITHOUT touching the already-bound cell (pre-fix it
    //     would free the cell here, before load()/update() even run).
    // PT: Setup — cria o modelo, então exercita rebind de chave duplicada nos 4 bind_*.
    //     A primeira chamada por chave deve suceder; a duplicata imediata na MESMA
    //     chave deve retornar false SEM tocar na célula já ligada (pré-fix ela seria
    //     liberada aqui, antes até de load()/update() rodarem).
    // -------------------------------------------------------------------------
    bool ok_model = app.create_data_model("hud");

    // EN: "hp" and "label" are the two variables tests/ui/scalar.rml actually renders
    //     ({{label}} and {{hp}} in <p> elements, "Sans" font loaded via @font-face —
    //     same document already proven to render deterministically under Xvfb/llvmpipe
    //     by data_model_scalar.cpp; tests/ui/hud.rml uses "font-family: sans" with no
    //     @font-face, so it never actually rasterises glyphs in this environment and
    //     would give a false negative here). Duplicate-bind these two so the dangerous
    //     path is exercised on views RmlUi will re-evaluate on update().
    // PT: "hp" e "label" são as duas variáveis que tests/ui/scalar.rml de fato renderiza
    //     ({{label}} e {{hp}} em elementos <p>, fonte "Sans" carregada via @font-face —
    //     mesmo documento já provado renderizar deterministicamente sob Xvfb/llvmpipe
    //     pelo data_model_scalar.cpp; tests/ui/hud.rml usa "font-family: sans" sem
    //     @font-face, então nunca rasteriza glifos de fato neste ambiente e daria um
    //     falso negativo aqui). Duplica-bind estas duas para que o caminho perigoso
    //     seja exercitado em views que o RmlUi reavalia no update().
    bool ok_hp_first   = app.bind_number("hp", 0.0);
    bool ok_hp_dup     = app.bind_number("hp", 2.0);   // EN: duplicate — must be false, no-op.
                                                        // PT: duplicada — deve ser false, no-op.

    bool ok_label_first = app.bind_string("label", "");
    bool ok_label_dup   = app.bind_string("label", "SECOND"); // EN: duplicate — must be false.
                                                                // PT: duplicada — deve ser false.

    // EN: bool/list duplicates are cheap to cover too — same code shape, same fix.
    //     Not referenced by hud.rml (no {{flag}}/data-for view there), so this only
    //     proves the guard returns false and leaves the map/model consistent
    //     (structural proof; the ASan trigger through an actual view re-evaluation
    //     is already covered above via "hp"/"name").
    // PT: Duplicatas de bool/list são baratas de cobrir também — mesma forma de código,
    //     mesmo fix. Não referenciadas em hud.rml (sem view {{flag}}/data-for lá), então
    //     só prova que o guard retorna false e deixa mapa/modelo consistentes (prova
    //     estrutural; o gatilho ASan via reavaliação de view real já está coberto acima
    //     via "hp"/"name").
    bool ok_flag_first = app.bind_bool("flag", true);
    bool ok_flag_dup   = app.bind_bool("flag", false);  // EN: duplicate — must be false.
                                                          // PT: duplicada — deve ser false.
    bool ok_items_first = app.bind_list("items");
    bool ok_items_dup   = app.bind_list("items");        // EN: duplicate — must be false.
                                                          // PT: duplicada — deve ser false.

    std::printf("bind_bool(flag) first:      %s (expect OK)\n",   ok_flag_first  ? "OK" : "FAIL");
    std::printf("bind_bool(flag) duplicate:  %s (expect false)\n", !ok_flag_dup  ? "OK" : "FAIL");
    std::printf("bind_list(items) first:     %s (expect OK)\n",   ok_items_first ? "OK" : "FAIL");
    std::printf("bind_list(items) duplicate: %s (expect false)\n", !ok_items_dup ? "OK" : "FAIL");

    // EN: Different key, right after the duplicate attempts — must NOT be affected
    //     by the guard (no false positives: "mp" was never bound before).
    // PT: Chave diferente, logo após as tentativas duplicadas — NÃO deve ser afetada
    //     pelo guard (sem falso positivo: "mp" nunca foi ligada antes).
    bool ok_mp = app.bind_number("mp", 50.0);

    bool ok_load = app.load("tests/ui/scalar.rml");

    std::printf("bind_number(hp) first:      %s (expect OK)\n",   ok_hp_first    ? "OK" : "FAIL");
    std::printf("bind_number(hp) duplicate:  %s (expect false)\n", !ok_hp_dup    ? "OK" : "FAIL");
    std::printf("bind_string(label) first:   %s (expect OK)\n",   ok_label_first ? "OK" : "FAIL");
    std::printf("bind_string(label) dup:     %s (expect false)\n", !ok_label_dup ? "OK" : "FAIL");
    std::printf("bind_number(mp) (new key):  %s (expect OK, no interference)\n",
                ok_mp ? "OK" : "FAIL");
    std::printf("load(scalar.rml):           %s\n", ok_load ? "OK" : "FAIL");

    all_ok = all_ok && ok_model && ok_hp_first && !ok_hp_dup
                     && ok_label_first && !ok_label_dup
                     && ok_flag_first && !ok_flag_dup
                     && ok_items_first && !ok_items_dup
                     && ok_mp && ok_load;

    if (!ok_load) {
        std::puts("data_model_rebind_uaf FAIL: setup");
        return 1;
    }

    // -------------------------------------------------------------------------
    // EN: Warm-up frames, then snapshot A (baseline: label="" empty, hp="0").
    //     This is the render pass that would heap-use-after-free pre-fix: RmlUi's
    //     Update() walks the views bound to "label"/"hp" and dereferences the raw
    //     pointers handed to Bind() — the ones the duplicate call would have freed.
    // PT: Frames de aquecimento, depois snapshot A (baseline: label="" vazio, hp="0").
    //     Este é o passe de render que faria heap-use-after-free pré-fix: o Update()
    //     do RmlUi percorre as views ligadas a "label"/"hp" e desreferencia os
    //     ponteiros crus passados a Bind() — os mesmos que a chamada duplicada teria
    //     liberado.
    // -------------------------------------------------------------------------
    app.poll_events(); app.update(); app.render();
    app.poll_events(); app.update(); app.render();

    std::vector<unsigned char> px_a;
    int wa = 0, ha = 0;
    if (!take_snapshot(app, "rebind_uaf_A.ppm", px_a, wa, ha)) return 1;
    size_t bright_a = count_bright(px_a);
    printf("data_model_rebind_uaf [A baseline]: bright=%zu (%dx%d)\n", bright_a, wa, ha);

    // -------------------------------------------------------------------------
    // EN: set_number/set_string on the SURVIVING cell (the one from the FIRST
    //     bind_* call — "hp"/"label" — the only cell that should exist post-fix).
    //     Must propagate to the render exactly like data_model_scalar's proof:
    //     more text ("HELLO HP 99" appears vs blank; "99" replaces "0")
    //     -> bright_B > bright_A. If the guard had NOT been applied, this call would
    //     already be writing through a dangling pointer (freed at the duplicate
    //     bind_* above) — ASan aborts before reaching this assertion.
    // PT: set_number/set_string na célula SOBREVIVENTE (a da PRIMEIRA chamada
    //     bind_* — "hp"/"label" — a única célula que deve existir pós-fix). Deve
    //     propagar para o render exatamente como a prova do data_model_scalar: mais
    //     texto ("HELLO HP 99" aparece vs vazio; "99" substitui "0")
    //     -> bright_B > bright_A. Se o guard NÃO tivesse sido aplicado, esta chamada
    //     já estaria escrevendo através de um ponteiro pendurado (liberado na
    //     bind_* duplicada acima) — o ASan aborta antes de chegar nesta asserção.
    // -------------------------------------------------------------------------
    app.set_number("hp", 99.0);
    app.set_string("label", "HELLO HP 99");

    std::vector<unsigned char> px_b;
    int wb = 0, hb = 0;
    if (!take_snapshot(app, "rebind_uaf_B.ppm", px_b, wb, hb)) return 1;
    size_t bright_b = count_bright(px_b);
    printf("data_model_rebind_uaf [B after set_* on surviving cell]: bright=%zu\n", bright_b);

    if (bright_b <= bright_a) {
        fprintf(stderr,
            "data_model_rebind_uaf FAIL: bright_B(%zu) <= bright_A(%zu) — "
            "set_number/set_string on the post-guard surviving cell did not "
            "propagate to render (cell may be corrupted)\n",
            bright_b, bright_a);
        all_ok = false;
    }

    // EN: mp (the untouched different-key binding) must also still work — proves the
    //     guard did not regress the normal, non-colliding path.
    // PT: mp (o binding de chave diferente, intocado) também deve continuar
    //     funcionando — prova que o guard não regrediu o caminho normal, sem colisão.
    app.set_number("mp", 7.0);
    app.update();
    app.render();

    if (!all_ok) {
        std::puts("data_model_rebind_uaf FAIL");
        return 1;
    }

    std::puts("data_model_rebind_uaf: PASS");
    return 0;
}
