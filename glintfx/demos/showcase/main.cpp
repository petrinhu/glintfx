// SPDX-License-Identifier: MPL-2.0
// EN: glintfx showcase — visual demo exercising filters, gradients, box-shadow,
//     backdrop-filter, and mask via the public glintfx::App facade only.
// PT: Showcase do glintfx — demo visual exercitando filtros, degradês, box-shadow,
//     backdrop-filter e máscara usando apenas a fachada pública glintfx::App.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/glintfx.hpp>

int main()
{
    glintfx::App app({ .title = "glintfx showcase", .width = 900, .height = 600 });
    app.load("showcase.rml");
    app.run();
    return 0;
}
