// SPDX-License-Identifier: MPL-2.0
// consumer-example — drop-in proof: only glintfx public API used.
// Prova de drop-in: apenas a API pública da glintfx é usada.
// Copyright (c) 2026 Petrus Silva Costa

#include <glintfx/glintfx.hpp>

// EN: Minimal consumer: create an App, load a local RML document, run one frame.
//     No GL, GLFW, or RmlUi headers needed — glintfx encapsulates everything.
// PT: Consumidor mínimo: cria um App, carrega documento RML local, executa um frame.
//     Sem headers de GL, GLFW ou RmlUi — glintfx encapsula tudo.
int main()
{
    glintfx::App app;
    app.load("showcase.rml");
    app.poll_events();
    app.update();
    app.render();
    return 0;
}
