// SPDX-License-Identifier: Apache-2.0
// EN: Public descriptor for programmatic font-face registration (UI-FONTFACE, v0.24.0 --
//     consumes the "glintfx never exposes Rml::LoadFontFace" gap: the 3 core overloads existed
//     in the vendored RmlUi since day one and glintfx's own font engine implements all 3
//     (src/font_engine_own.cpp), but nothing in glintfx's public surface ever called them --
//     a host could only register a font via an RCSS `@font-face` block inside a `.rml`
//     document, never at runtime from C++ (e.g. a font shipped as a game asset resolved only
//     after some other subsystem decides which localisation/DLC pack is active).
//
//     A STRUCT, not N positional bool/enum/int parameters (líder's decision, one-way-door in a
//     distributed framework): follows the house precedent of AppConfig/UiLayerConfig/
//     AudioConfig/GamepadsConfig -- a struct's field set can grow across releases without
//     breaking every call site's positional argument order, which a `bool load_font_face(const
//     char*, const char*, FontStyle, FontWeight, bool)` signature could not do without an ABI/
//     source break the day a 6th knob (e.g. a face_index for a font-collection file) is needed.
//
//     ENGINE ASYMMETRY (READ BEFORE RELYING ON `family = nullptr`): when `family` is left null,
//     load_font_face() forwards to RmlUi's family-LESS `Rml::LoadFontFace(path, fallback_face,
//     weight)` overload, and the two font engines glintfx can install (glintfx/include/glintfx/
//     font_engine.hpp's `FontEngine`) derive the registered family DIFFERENTLY:
//       - FontEngine::Own (glintfx's own clean-room engine, src/font_engine_own.cpp
//         `FontEngineOwn::LoadFontFace` family-less overload): derives the family from the
//         font FILE'S OWN STEM (the path's basename, extension stripped) -- it does not parse
//         the SFNT 'name' table at all (see that method's own doc-comment for why: no
//         confirmed RmlUi call site needs it, SOV-SFNT does not implement 'name'-table
//         parsing). `assets/OpenSans-Regular.ttf` registers as family `"opensans-regular"`
//         (lower-cased at write time -- see FontEngineOwn::RegisterFace's own doc-comment).
//       - FontEngine::FreeType (RmlUi's built-in default engine): reads the font file's own
//         SFNT 'name' table and registers whatever family the FONT DESIGNER embedded there --
//         e.g. the SAME `OpenSans-Regular.ttf` file typically registers as `"Open Sans"` (a
//         space-separated, differently-cased string that has NO fixed relationship to the
//         file's own name on disk).
//     A caller that depends on a SPECIFIC, PREDICTABLE family string across a build that might
//     ship with either engine (FontEngine::FreeType is the documented one-line rollback --
//     glintfx/include/glintfx/font_engine.hpp) MUST pass an explicit non-null `family` here --
//     doing so drives BOTH engines through their respective family-WITH-parameters overloads
//     (`Rml::LoadFontFace(path, family, style, weight, fallback_face)`), which register the
//     EXACT string this struct carries, identically, regardless of which engine is active. Only
//     the family-less (`family = nullptr`) path is engine-dependent; every other field
//     (`style`/`weight`/`fallback_face`) behaves identically on both engines (both interfaces
//     take the SAME RmlUi-level `Rml::Style::FontStyle`/`Rml::Style::FontWeight` parameters --
//     see FontEngineInterface.h in the vendored RmlUi source).
//
// PT: Descritor público para registro programático de face de fonte (UI-FONTFACE, v0.24.0 --
//     consome a lacuna "a glintfx nunca expõe Rml::LoadFontFace": as 3 sobrecargas do core
//     existiam no RmlUi vendorizado desde o dia zero e o motor de fonte próprio da glintfx as
//     implementa as 3 (src/font_engine_own.cpp), mas nada na superfície pública da glintfx algum
//     dia as chamou -- um host só conseguia registrar uma fonte via um bloco RCSS `@font-face`
//     dentro de um documento `.rml`, nunca em runtime a partir de C++ (ex.: uma fonte
//     distribuída como asset de jogo, resolvida só depois de algum outro subsistema decidir
//     qual pacote de localização/DLC está ativo).
//
//     UMA STRUCT, não N parâmetros posicionais bool/enum/int (decisão do líder, one-way-door
//     num framework distribuído): segue o precedente da casa de AppConfig/UiLayerConfig/
//     AudioConfig/GamepadsConfig -- o conjunto de campos de uma struct cresce entre releases
//     sem quebrar a ordem de argumento posicional de todo call site, o que uma assinatura
//     `bool load_font_face(const char*, const char*, FontStyle, FontWeight, bool)` não
//     conseguiria fazer sem uma quebra de ABI/fonte no dia em que um 6º botão (ex.: um
//     face_index pra um arquivo de coleção de fontes) for necessário.
//
//     ASSIMETRIA DE MOTOR (LEIA ANTES DE DEPENDER DE `family = nullptr`): quando `family` fica
//     nulo, load_font_face() encaminha para a sobrecarga sem-família `Rml::LoadFontFace(path,
//     fallback_face, weight)` do RmlUi, e os dois motores de fonte que a glintfx pode instalar
//     (`FontEngine` de glintfx/include/glintfx/font_engine.hpp) derivam a família registrada de
//     jeitos DIFERENTES:
//       - FontEngine::Own (motor próprio clean-room da glintfx, sobrecarga sem-família de
//         `FontEngineOwn::LoadFontFace` em src/font_engine_own.cpp): deriva a família do
//         PRÓPRIO STEM do arquivo de fonte (o basename do caminho, sem extensão) -- não parseia
//         a tabela 'name' do SFNT de jeito nenhum (ver o próprio doc-comment daquele método pro
//         motivo: nenhum ponto de chamada confirmado do RmlUi precisa disso, o SOV-SFNT não
//         implementa parse de tabela 'name'). `assets/OpenSans-Regular.ttf` registra como
//         família `"opensans-regular"` (minusculizado em tempo de escrita -- ver o próprio
//         doc-comment de FontEngineOwn::RegisterFace).
//       - FontEngine::FreeType (motor default embutido do RmlUi): lê a tabela 'name' SFNT do
//         próprio arquivo de fonte e registra a família que o DESIGNER DA FONTE embutiu lá --
//         ex.: o MESMO arquivo `OpenSans-Regular.ttf` tipicamente registra como `"Open Sans"`
//         (uma string separada por espaço, com capitalização diferente, sem relação fixa
//         nenhuma com o próprio nome do arquivo em disco).
//     Um chamador que depende de uma string de família ESPECÍFICA e PREVISÍVEL num build que
//     pode sair com qualquer um dos dois motores (FontEngine::FreeType é o rollback documentado
//     de uma linha -- glintfx/include/glintfx/font_engine.hpp) PRECISA passar um `family`
//     explícito e não-nulo aqui -- fazer isso dirige OS DOIS motores pelas respectivas
//     sobrecargas com-família (`Rml::LoadFontFace(path, family, style, weight, fallback_face)`),
//     que registram a string EXATA que esta struct carrega, identicamente, independente de qual
//     motor está ativo. Só o caminho sem-família (`family = nullptr`) é motor-dependente; todo
//     outro campo (`style`/`weight`/`fallback_face`) se comporta identicamente nos dois motores
//     (as duas interfaces recebem os MESMOS parâmetros `Rml::Style::FontStyle`/
//     `Rml::Style::FontWeight` em nível RmlUi -- ver FontEngineInterface.h no source do RmlUi
//     vendorizado).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

namespace glintfx {

// EN: Mirrors Rml::Style::FontStyle's two keyword values 1:1 (own enum -- no Rml:: type crosses
//     this header, the encapsulation invariant every glintfx public header upholds; mapped to
//     the RmlUi type internally, src/bootstrap.cpp). Italic ~= CSS `font-style: italic`.
// PT: Espelha 1:1 os dois valores de palavra-chave de Rml::Style::FontStyle (enum próprio --
//     nenhum tipo Rml:: cruza este header, o invariante de encapsulamento que todo header
//     público da glintfx sustenta; mapeado ao tipo do RmlUi internamente, src/bootstrap.cpp).
//     Italic ~= CSS `font-style: italic`.
enum class FontStyle {
  Normal,
  Italic,
};

// EN: A deliberately SMALL enum, not RmlUi's raw [1,1000] numeric weight range (Rml::
//     Style::FontWeight is a uint16_t where Auto=0, Normal=400, Bold=700, and any value in
//     [1,1000] is technically valid) -- v1 exposes the 3 values every confirmed call site
//     (RCSS `@font-face`'s own `font-weight: normal|bold|<number>` keywords collapse to, in
//     practice) needs; Auto (0) means "load/register whichever weight(s) the font file itself
//     declares, do not force one" (RmlUi's own default -- see FontFaceDesc::weight's
//     doc-comment). A numeric-weight overload is a natural, additive extension if a real
//     variable-font consumer ever needs e.g. 550 -- not added here, anti-over-engineering
//     (no confirmed caller needs it yet).
// PT: Um enum deliberadamente PEQUENO, não a faixa numérica crua [1,1000] do RmlUi (Rml::
//     Style::FontWeight é um uint16_t onde Auto=0, Normal=400, Bold=700, e qualquer valor em
//     [1,1000] é tecnicamente válido) -- a v1 expõe os 3 valores que todo ponto de chamada
//     confirmado (as próprias palavras-chave `font-weight: normal|bold|<número>` do RCSS
//     `@font-face` colapsam pra isso, na prática) precisa; Auto (0) significa "carregue/
//     registre qualquer peso que o próprio arquivo de fonte declarar, não force um" (o default
//     do próprio RmlUi -- ver o doc-comment de FontFaceDesc::weight). Uma sobrecarga de peso
//     numérico é uma extensão natural e aditiva se algum consumidor real de variable-font
//     precisar, ex., de 550 -- não adicionada aqui, anti-over-engineering (nenhum chamador
//     confirmado precisa disso ainda).
enum class FontWeight {
  Auto,
  Normal,
  Bold,
};

// EN: Descriptor for glintfx::UiLayer::load_font_face() / glintfx::App::load_font_face() (both
//     forward to the SAME Engine/Bootstrap implementation -- see bootstrap.hpp's doc-comment
//     for the exact Rml::LoadFontFace overload each field combination selects). See this file's
//     own header comment above for the family=nullptr engine-asymmetry contract.
// PT: Descritor para glintfx::UiLayer::load_font_face() / glintfx::App::load_font_face() (ambos
//     encaminham à MESMA implementação Engine/Bootstrap -- ver o doc-comment de bootstrap.hpp
//     pra sobrecarga exata de Rml::LoadFontFace que cada combinação de campo seleciona). Ver o
//     comentário de cabeçalho deste arquivo acima pro contrato de assimetria-de-motor do
//     family=nullptr.
struct FontFaceDesc {
  // EN: REQUIRED. Path to the font file (TTF/OTF/font-collection), resolved through the SAME
  //     Rml::FileInterface every other glintfx asset load goes through (glintfx's own
  //     BaseUrlFileInterface -- asset_base_url prefixing, src/base_url_file_interface.hpp --
  //     applies here exactly like it does to an RCSS `@font-face src` or an `<img>`). `nullptr`
  //     or `""` -> load_font_face() returns `false` immediately, no crash, no RmlUi call made
  //     (AUD-TEC-5 fail-high, same discipline as every id-keyed Bootstrap method).
  // PT: OBRIGATÓRIO. Caminho do arquivo de fonte (TTF/OTF/coleção-de-fontes), resolvido pela
  //     MESMA Rml::FileInterface que todo outro carregamento de asset da glintfx atravessa (o
  //     BaseUrlFileInterface próprio da glintfx -- prefixo de asset_base_url,
  //     src/base_url_file_interface.hpp -- aplica aqui exatamente como aplica a um `@font-face
  //     src` do RCSS ou a uma `<img>`). `nullptr` ou `""` -> load_font_face() retorna `false`
  //     imediatamente, sem crash, sem chamada ao RmlUi (fail-high AUD-TEC-5, mesma disciplina de
  //     todo método do Bootstrap indexado por id).
  const char* path = nullptr;

  // EN: OPTIONAL. `nullptr` (the default) derives the registered family -- see this file's own
  //     header comment above for the full, ENGINE-DEPENDENT contract of that path. A non-null,
  //     non-empty family drives the family-WITH-parameters `Rml::LoadFontFace` overload, whose
  //     registered family is this EXACT string on both engines (engine-independent). An empty
  //     string `""` is treated the SAME as `nullptr` (derive), not as "register under the empty
  //     family" -- an empty family string is not a meaningful RCSS `font-family` match target.
  // PT: OPCIONAL. `nullptr` (o default) deriva a família registrada -- ver o comentário de
  //     cabeçalho deste arquivo acima pro contrato completo, DEPENDENTE-DE-MOTOR, desse
  //     caminho. Uma família não-nula e não-vazia dirige a sobrecarga com-parâmetros do
  //     `Rml::LoadFontFace`, cuja família registrada é esta string EXATA nos dois motores
  //     (independente de motor). Uma string vazia `""` é tratada IGUAL a `nullptr` (deriva),
  //     não como "registre sob a família vazia" -- uma string de família vazia não é um alvo de
  //     casamento de `font-family` RCSS significativo.
  const char* family = nullptr;

  // EN: RCSS `font-style` to register the face under -- IGNORED when `family` is `nullptr`
  //     (the family-less Rml::LoadFontFace overload has no style/family parameter at all; style
  //     is then whatever the font file's own metadata implies, same as RCSS `@font-face` with
  //     no `font-style` declared).
  // PT: `font-style` RCSS sob o qual registrar a face -- IGNORADO quando `family` é `nullptr`
  //     (a sobrecarga sem-família de Rml::LoadFontFace não tem parâmetro de style/family
  //     nenhum; o style então é o que os próprios metadados do arquivo de fonte implicarem,
  //     igual a um `@font-face` RCSS sem `font-style` declarado).
  FontStyle style = FontStyle::Normal;

  // EN: RCSS `font-weight` to load/register -- Auto (RmlUi's own `Style::FontWeight::Auto`, 0)
  //     means "load whichever weight(s) the font file declares, do not force a single one" (the
  //     upstream doc-comment's own wording, Include/RmlUi/Core/Core.h in the vendored RmlUi
  //     source). Unlike `style`, `weight` IS honoured even when `family` is `nullptr` -- BOTH
  //     Rml::LoadFontFace overloads (family-less and family-with-parameters) take a `weight`
  //     argument.
  // PT: `font-weight` RCSS a carregar/registrar -- Auto (o próprio `Style::FontWeight::Auto` do
  //     RmlUi, 0) significa "carregue qualquer peso que o arquivo de fonte declarar, não force
  //     um único" (a própria redação do doc-comment upstream, Include/RmlUi/Core/Core.h no
  //     source do RmlUi vendorizado). Diferente do `style`, `weight` É honrado mesmo quando
  //     `family` é `nullptr` -- AS DUAS sobrecargas de Rml::LoadFontFace (sem-família e
  //     com-parâmetros) recebem um argumento `weight`.
  FontWeight weight = FontWeight::Auto;

  // EN: `true` registers this face as a FALLBACK candidate -- RmlUi resolves an unknown glyph
  //     missing from a span's PRIMARY font by walking every face registered with
  //     `fallback_face=true`, in REGISTRATION ORDER, across the WHOLE process (not scoped to
  //     one document/context) -- the SAME mechanism RCSS's `-rmlui-fallback-face: true;`
  //     `@font-face` property drives, now reachable programmatically without authoring a
  //     document just to register a fallback face. `false` (the default) registers a normal,
  //     PRIMARY-only face -- never consulted as a fallback for a DIFFERENT family's missing
  //     glyph.
  // PT: `true` registra esta face como candidata de FALLBACK -- o RmlUi resolve um glyph
  //     desconhecido, ausente da fonte PRIMÁRIA de um span, percorrendo toda face registrada
  //     com `fallback_face=true`, em ORDEM DE REGISTRO, através do processo INTEIRO (não
  //     restrito a um documento/contexto) -- o MESMO mecanismo que a propriedade RCSS
  //     `-rmlui-fallback-face: true;` de `@font-face` dirige, agora alcançável
  //     programaticamente sem autorar um documento só pra registrar uma face de fallback.
  //     `false` (o default) registra uma face normal, SÓ-PRIMÁRIA -- nunca consultada como
  //     fallback pro glyph ausente de uma família DIFERENTE.
  bool fallback_face = false;
};

} // namespace glintfx
