// SPDX-License-Identifier: MPL-2.0
// EN: Embedded User-Agent stylesheet -- 'display: block' defaults for structural elements.
//     Applied as a low-specificity base to every document by Bootstrap::load(); see that
//     file for the merge mechanism (StyleSheetContainer::CombineStyleSheetContainer).
//     Author rules of equal or higher specificity always win (e.g. showcase.rcss's own
//     explicit 'display: block' keeps working unchanged).
// PT: Stylesheet User-Agent embutida -- defaults de 'display: block' para elementos
//     estruturais. Aplicada como base de baixa especificidade a todo documento por
//     Bootstrap::load(); ver esse arquivo para o mecanismo de merge
//     (StyleSheetContainer::CombineStyleSheetContainer). Regras do autor com
//     especificidade igual ou maior sempre vencem (ex.: o 'display: block' explícito do
//     showcase.rcss segue funcionando sem mudança).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

namespace glintfx {

// EN: Raw RCSS text, parsed once per Bootstrap lifetime (see Bootstrap::init()).
// PT: Texto RCSS bruto, parseado uma vez por lifetime do Bootstrap (ver Bootstrap::init()).
inline constexpr const char* kUaStylesheetRcss = R"rcss(
div, p,
h1, h2, h3, h4, h5, h6,
ul, ol, li,
section, article, header, footer, nav, main
{
  display: block;
}
)rcss";

} // namespace glintfx
