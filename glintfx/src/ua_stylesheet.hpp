// SPDX-License-Identifier: MPL-2.0
// EN: Embedded User-Agent stylesheet -- 'display: block' defaults for structural elements, plus
//     minimal usable-out-of-the-box scrollbar defaults (v0.6.0). Applied as a low-specificity
//     base to every document by Bootstrap::load(); see that file for the merge mechanism
//     (StyleSheetContainer::CombineStyleSheetContainer). Author rules of EQUAL OR HIGHER
//     specificity always win (e.g. showcase.rcss's own explicit 'display: block' keeps working
//     unchanged) -- AND the merge order itself (Bootstrap::load() merges the document's OWN
//     sheet on TOP of this one, see the detailed comment there) means an author rule of EQUAL
//     specificity to any rule below still wins, not just a strictly-higher one. CAVEAT (review
//     v0.6.0, Minor #1): "equal specificity" means equal to the SHAPE of the rule below, not to
//     the tag being styled in isolation -- the 'display: block' rules above are single-tag
//     selectors (RmlUi specificity ~10000 each), so a single-tag author rule ties and wins as
//     expected, but the scrollbar rules further down are TWO-TAG COMPOUND selectors (e.g.
//     `scrollbarvertical sliderbar`, ~20000 -- RmlUi accumulates ~10000 per tag in the selector
//     chain, StyleSheetSelector.cpp). A single-tag author override on those elements alone (e.g.
//     `sliderbar { background-color: red; }`, ~10000) is LOWER specificity and does NOT win. To
//     override a scrollbar default an author must either match the same compound-tag shape
//     (`scrollbarvertical sliderbar { ... }`) or add a class/id ancestor to unambiguously outrank
//     it (`.scrollable scrollbarvertical sliderbar { ... }` or `#my-list scrollbarvertical
//     sliderbar { ... }`) -- see docs/effects.md's scrollbar how-to for worked examples.
//
//     Scrollbar defaults (v0.6.0, GLINTFX-SCROLL-1 follow-up): investigated whether RmlUi ships
//     any built-in default sizing/styling for the scrollbarvertical/horizontal/slidertrack/
//     sliderbar/sliderarrowdec/sliderarrowinc element types it synthesizes for an
//     overflow-y:auto (etc.) container (Source/Core/ElementScroll.cpp -- Factory::InstanceElement
//     with those tag names, plain DOM elements, no attached style of their own) -- found NONE:
//     Source/Core/WidgetScroll.cpp::FormatElements explicitly zero-sizes the track/arrows when no
//     RCSS-authored height/width resolves ("If no height has been explicitly specified for the
//     track, it'll be initialised to -1 as per normal block... arrow_size.x < 0 ->
//     arrow_box.SetContent(Vector2f(0, 0))"), confirmed against RmlUi's own sample stylesheet
//     (Samples/assets/invader.rcss) which authors explicit width/height/decorators for every one
//     of these tags -- i.e. WITHOUT author (or UA) RCSS the scrollbar is a zero-size, invisible,
//     unclickable no-op. The rules below are the MINIMUM to make it visible and usable: a bar
//     width/height (scrollbarvertical/horizontal), a translucent thumb with a minimum drag size
//     (sliderbar), a faint track (slidertrack, so the scrollable extent reads visually), and
//     sized+coloured arrows (sliderarrowdec/inc) -- plain background-color (a native RmlUi
//     property, RegisterProperty(PropertyId::BackgroundColor, ...) in
//     Source/Core/StyleSheetSpecification.cpp), no decorator/image assets required, so this stays
//     a pure-RCSS-string default with no shipped texture. slidertrack/sliderbar/sliderarrowdec/
//     sliderarrowinc deliberately do NOT set their own width/height on the CROSS axis (e.g. no
//     `width` on a vertical sliderbar) -- RmlUi's WidgetScroll lays them out as normal block
//     children of scrollbarvertical/horizontal, so width:auto already fills the parent's content
//     width, matching Samples/assets/invader.rcss's own unstyled-cross-axis convention for the
//     no-margin case.
// PT: Stylesheet User-Agent embutida -- defaults de 'display: block' para elementos
//     estruturais, mais defaults mínimos de scrollbar usável-de-fábrica (v0.6.0). Aplicada como
//     base de baixa especificidade a todo documento por Bootstrap::load(); ver esse arquivo para
//     o mecanismo de merge (StyleSheetContainer::CombineStyleSheetContainer). Regras do autor
//     com especificidade IGUAL OU MAIOR sempre vencem (ex.: o 'display: block' explícito do
//     showcase.rcss segue funcionando sem mudança) -- E a própria ordem do merge (Bootstrap::
//     load() mescla a sheet PRÓPRIA do documento POR CIMA desta aqui, ver o comentário detalhado
//     lá) significa que uma regra do autor de especificidade IGUAL a qualquer regra abaixo ainda
//     vence, não só uma estritamente maior. RESSALVA (review v0.6.0, Minor #1): "especificidade
//     igual" quer dizer igual ao FORMATO da regra abaixo, não à tag estilizada isoladamente -- as
//     regras de 'display: block' acima são seletores de UMA tag (especificidade ~10000 cada no
//     RmlUi), então uma regra de autor de uma tag empata e vence como esperado, mas as regras de
//     scrollbar mais abaixo são seletores COMPOSTOS DE DUAS TAGS (ex.: `scrollbarvertical
//     sliderbar`, ~20000 -- o RmlUi acumula ~10000 por tag na cadeia do seletor,
//     StyleSheetSelector.cpp). Uma sobreposição de autor de uma tag só nesses elementos (ex.:
//     `sliderbar { background-color: red; }`, ~10000) tem especificidade MENOR e NÃO vence. Para
//     sobrepor um default de scrollbar o autor precisa ou igualar o mesmo formato composto de tags
//     (`scrollbarvertical sliderbar { ... }`) ou adicionar um ancestral classe/id para vencer sem
//     ambiguidade (`.scrollable scrollbarvertical sliderbar { ... }` ou `#minha-lista
//     scrollbarvertical sliderbar { ... }`) -- ver o how-to de scrollbar em docs/effects.md para
//     exemplos resolvidos.
//
//     Defaults de scrollbar (v0.6.0, desdobramento do GLINTFX-SCROLL-1): investigado se o RmlUi
//     embute algum default de tamanho/estilo para os tipos de elemento scrollbarvertical/
//     horizontal/slidertrack/sliderbar/sliderarrowdec/sliderarrowinc que ele sintetiza para um
//     container overflow-y:auto (etc.) (Source/Core/ElementScroll.cpp -- Factory::InstanceElement
//     com esses nomes de tag, elementos DOM simples, sem estilo próprio anexado) -- não achei
//     NENHUM: Source/Core/WidgetScroll.cpp::FormatElements zera explicitamente o tamanho de
//     track/setas quando nenhuma altura/largura autorada em RCSS resolve ("If no height has been
//     explicitly specified for the track, it'll be initialised to -1 as per normal block...
//     arrow_size.x < 0 -> arrow_box.SetContent(Vector2f(0, 0))"), confirmado contra a própria
//     stylesheet de exemplo do RmlUi (Samples/assets/invader.rcss), que autora width/height/
//     decorators explícitos para cada uma dessas tags -- ou seja, SEM RCSS de autor (ou UA) a
//     scrollbar é um no-op de tamanho zero, invisível, inclicável. As regras abaixo são o MÍNIMO
//     para torná-la visível e usável: uma largura/altura de barra (scrollbarvertical/horizontal),
//     um thumb translúcido com tamanho mínimo de arraste (sliderbar), uma trilha discreta
//     (slidertrack, para que a extensão rolável se leia visualmente), e setas dimensionadas+
//     coloridas (sliderarrowdec/inc) -- background-color simples (propriedade nativa do RmlUi,
//     RegisterProperty(PropertyId::BackgroundColor, ...) em Source/Core/StyleSheetSpecification.
//     cpp), sem decorator/asset de imagem necessário, então isto permanece um default puro-string-
//     RCSS sem textura embarcada. slidertrack/sliderbar/sliderarrowdec/sliderarrowinc
//     deliberadamente NÃO definem a própria largura/altura no eixo TRANSVERSAL (ex.: sem `width`
//     num sliderbar vertical) -- o WidgetScroll do RmlUi os posiciona como filhos block normais
//     de scrollbarvertical/horizontal, então width:auto já preenche a largura de conteúdo do pai,
//     igualando a convenção de eixo-transversal-sem-estilo do próprio Samples/assets/
//     invader.rcss para o caso sem margem.
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

scrollbarvertical
{
  width: 10dp;
}
scrollbarvertical slidertrack
{
  background-color: #ffffff20;
}
scrollbarvertical sliderbar
{
  background-color: #ffffff80;
  min-height: 16dp;
}
scrollbarvertical sliderarrowdec,
scrollbarvertical sliderarrowinc
{
  height: 10dp;
  background-color: #ffffff40;
}

scrollbarhorizontal
{
  height: 10dp;
}
scrollbarhorizontal slidertrack
{
  background-color: #ffffff20;
}
scrollbarhorizontal sliderbar
{
  background-color: #ffffff80;
  min-width: 16dp;
}
scrollbarhorizontal sliderarrowdec,
scrollbarhorizontal sliderarrowinc
{
  width: 10dp;
  background-color: #ffffff40;
}
)rcss";

} // namespace glintfx
