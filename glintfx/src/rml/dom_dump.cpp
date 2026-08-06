// SPDX-License-Identifier: Apache-2.0
// EN: RMLX-1/S6a implementation. See dom_dump.hpp's own header comment for the confinement
//     rationale and the two spec-faithfulness decisions (class split from the RAW attribute,
//     not `GetClassNames()`; `<head>` extraction is a plain text scan, not a tree walk) this
//     file implements.
// PT: Implementação da RMLX-1/S6a. Ver o próprio comentário de cabeçalho de dom_dump.hpp pro
//     racional de confinamento e as duas decisões de fidelidade-à-spec (split de classe a
//     partir do atributo CRU, não `GetClassNames()`; extração de `<head>` é um scan de texto
//     puro, não uma travessia de árvore) que este arquivo implementa.
// Copyright (c) 2026 Petrus Silva Costa
#include "dom_dump.hpp"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>
#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/Variant.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace glintfx {

namespace {

// EN: The 4-character whitespace set docs/uix-dom.md sections 2/6/7 all anchor on -- the SAME
//     set `StringUtilities::IsWhitespace` uses upstream
//     (`examples/RmlUi/Include/RmlUi/Core/StringUtilities.h:61-64`), not a coincidence (section
//     2's own prose says so). Named once here so every caller below (escape, class-token split,
//     self-closing-tag trailing-space skip) shares the exact same definition -- a SECOND,
//     silently-different "what counts as whitespace" in this file would be the precise kind of
//     private assumption docs/uix-dom.md's own "Why this document exists" section warns about.
// PT: O conjunto de 4 caracteres de whitespace em que as seções 2/6/7 do docs/uix-dom.md se
//     ancoram -- o MESMO conjunto que `StringUtilities::IsWhitespace` usa upstream
//     (`examples/RmlUi/Include/RmlUi/Core/StringUtilities.h:61-64`), não é coincidência (a
//     própria prosa da seção 2 diz isso). Nomeado uma vez aqui pra todo chamador abaixo (escape,
//     split de token de classe, pulo de espaço-final de tag auto-fechada) compartilhar
//     exatamente a mesma definição -- uma SEGUNDA definição de "o que conta como whitespace",
//     silenciosamente diferente, neste arquivo seria exatamente o tipo de suposição privada que
//     a própria seção "Por que este documento existe" do docs/uix-dom.md avisa.
bool is_ws(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

char to_lower_ascii(char c) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

// EN: Case-insensitive literal substring search, ASCII-fold only (both needles this file
//     searches for -- "<head", "</head" -- are pure ASCII, so folding non-ASCII bytes would be
//     pointless work, not a correctness gap).
// PT: Busca de substring literal insensível a caixa, dobra só ASCII (as duas agulhas que este
//     arquivo busca -- "<head", "</head" -- são ASCII puro, então dobrar byte não-ASCII seria
//     trabalho inútil, não uma lacuna de correção).
std::size_t ifind(const std::string& hay, const char* needle, std::size_t from) {
  const std::size_t needle_len = std::string(needle).size();
  if (needle_len == 0 || hay.size() < needle_len) return std::string::npos;
  for (std::size_t i = from; i + needle_len <= hay.size(); ++i) {
    bool match = true;
    for (std::size_t j = 0; j < needle_len; ++j) {
      if (to_lower_ascii(hay[i + j]) != to_lower_ascii(needle[j])) {
        match = false;
        break;
      }
    }
    if (match) return i;
  }
  return std::string::npos;
}

// EN: `UIX-HEAD-COMENTARIO` fix. Tag-name character classes, mirrored from
//     `glintfx/src/uix/dom/lexer.cpp`'s own `is_name_start`/`is_name_char` (letters/`_` to
//     start, plus digits/`-` to continue) -- same character set, independent copy, because this
//     TU is confined to `glintfx/src/rml/` and may not `#include` across the `S6a`/`S6b` wall
//     (dom_dump.hpp's own header comment). Kept loose (permits any RML-legal custom tag name,
//     not just the fixed set {rml, head, body, style, script} this file cares about) so a tag
//     name comparison against "head"/"style"/"script" below is a real name-equality check, not
//     an accidental prefix match.
// PT: Conserto da `UIX-HEAD-COMENTARIO`. Classes de caractere de nome de tag, espelhadas de
//     `is_name_start`/`is_name_char` do `glintfx/src/uix/dom/lexer.cpp` (letras/`_` pra
//     começar, dígitos/`-` pra continuar) -- mesmo conjunto de caracteres, cópia independente,
//     porque esta TU é confinada a `glintfx/src/rml/` e não pode `#include` através da parede
//     `S6a`/`S6b` (o próprio comentário de cabeçalho de dom_dump.hpp). Mantido frouxo (permite
//     qualquer nome de tag customizada legal em RML, não só o conjunto fixo {rml, head, body,
//     style, script} que este arquivo se importa) pra uma comparação de nome contra
//     "head"/"style"/"script" abaixo ser uma checagem de igualdade de nome real, não um
//     casamento de prefixo por acidente.
bool is_tag_name_start(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}
bool is_tag_name_char(char c) {
  return is_tag_name_start(c) || (c >= '0' && c <= '9') || c == '-';
}

// EN: Quote-aware scan for the `>` that closes a tag (open OR close), starting at `from` (any
//     position at/after the tag's own `<`). A `>` inside a single- or double-quoted attribute
//     value does not end the tag -- same rule `dom_dump_scan_head`'s own opening-tag scan
//     already applied ONLY to `<head ...>` itself before this fix; now shared by every tag this
//     file's structural scanner walks past, which is exactly what closes the false-positive hole
//     a `<head` substring sitting inside SOME OTHER tag's quoted attribute value would otherwise
//     open (docs/uix-dom.md section 4 is about the real `<head>` element; a quoted-attribute
//     substring is not a tag at all, real or otherwise).
// PT: Scan consciente de aspas pelo `>` que fecha uma tag (abertura OU fechamento), começando em
//     `from` (qualquer posição em/depois do próprio `<` da tag). Um `>` dentro de um valor de
//     atributo entre aspas simples ou duplas não fecha a tag -- mesma regra que o próprio scan de
//     tag-de-abertura do `dom_dump_scan_head` já aplicava SÓ ao `<head ...>` antes deste conserto;
//     agora compartilhada por toda tag que o scanner estrutural deste arquivo passa por cima, que é
//     exatamente o que fecha o furo de falso-positivo que um substring `<head` sentado dentro do
//     valor de atributo entre aspas de QUALQUER OUTRA tag abriria (a seção 4 do docs/uix-dom.md é
//     sobre o elemento `<head>` real; um substring dentro de atributo entre aspas não é tag
//     nenhuma, real ou não).
std::size_t find_tag_close_gt(const std::string& source, std::size_t from) {
  bool in_squote = false;
  bool in_dquote = false;
  for (std::size_t i = from; i < source.size(); ++i) {
    const char c = source[i];
    if (in_squote) {
      if (c == '\'') in_squote = false;
      continue;
    }
    if (in_dquote) {
      if (c == '"') in_dquote = false;
      continue;
    }
    if (c == '\'') {
      in_squote = true;
      continue;
    }
    if (c == '"') {
      in_dquote = true;
      continue;
    }
    if (c == '>') return i;
  }
  return std::string::npos;
}

// EN: Result of the structural scan below: where the REAL `<head>` element's own open/close tags
//     sit, if any.
// PT: Resultado do scan estrutural abaixo: onde os próprios tags de abertura/fechamento do
//     elemento `<head>` REAL estão, se existir algum.
struct HeadTagSpan {
  bool found = false;
  bool self_closing = false;
  std::size_t open_close_gt = std::string::npos; // '>' closing the OPEN <head ...> tag
  std::size_t close_open_lt = std::string::npos; // '<' opening the CLOSE </head> tag
};

// EN: 🔴 `UIX-HEAD-COMENTARIO` -- the structural fix. Replaces the old bare `ifind(source,
//     "<head", ...)` (a comment-unaware literal substring search, the measured defect: in 15 of
//     16 corpus fixtures it matched prose inside the leading provenance comment, ~819-1069 bytes
//     before the real element -- docs/uix-dom.md section 9's ledger, class (a)) with a single
//     forward pass that walks the document AS TAGS, not as an undifferentiated byte soup, and
//     recognizes a `<head`/`</head` occurrence as the real element only when it is actually
//     reached as a tag token at the top level of the scan -- never while inside:
//       - an XML comment (`<!-- ... -->`) -- the measured bug;
//       - a quoted attribute value of ANY tag (not just `<head>`'s own -- the pre-existing
//         quote-aware scan only protected `<head>`'s OWN attributes; a `<head` substring sitting
//         inside e.g. `<meta title="looks like <head> here">`'s attribute value was NOT
//         protected before this fix, because the old code never looked at any tag OTHER than the
//         one substring match it had already committed to);
//       - the opaque CDATA-like content of a `<style>` or `<script>` tag -- confirmed against
//         upstream RmlUi source, not assumed: `Factory.cpp:255-257` calls
//         `XMLParser::RegisterPersistentCDATATag("script")` and `("style")`, so RmlUi's OWN
//         parser (`BaseXMLParser::ReadCDATA`, `BaseXMLParser.cpp:349-390`) treats both tags'
//         content as raw, un-tokenized data -- no nested `<...>` inside a `<style>`/`<script>`
//         block is ever "a tag" to upstream, so it must not be one to this scanner either. This
//         is exactly the risk this repo's own fixtures document as real (see the provenance
//         comment `dom_dump.hpp` and every corpus `.rml` file under
//         `glintfx/src/uix/dom/test_fixtures/` carries: *"Real RCSS comment prose inside several
//         of these 15 files' <head>/<style> blocks contains a literal, un-escaped '<'..."*),
//         even though no fixture in today's corpus happens to spell that literal `<` as `<head`
//         or `</head` specifically (measured: exactly one `<head`-shaped run per fixture, the
//         real element, plus the 3 comment-prose false matches this fix removes).
//
//     🔴 Declared, deliberate scope teto -- what this scanner does NOT recognize as opaque, on
//     purpose:
//       - `<![CDATA[ ... ]]>` sections -- upstream RmlUi supports the literal `<![CDATA[` XML
//         construct too (`BaseXMLParser.cpp:145-152`, a SEPARATE code path from the persistent
//         CDATA-tag mechanism style/script use), but no fixture in this wave's corpus
//         (`glintfx/tests/*.rml`, `glintfx/demos/showcase/*.rml`,
//         `glintfx/src/uix/dom/test_fixtures/*.rml`) uses it, and `docs/rmlx-subset.md`'s own
//         frozen subset does not name it either -- adding a third opaque-content mechanism here
//         with zero corpus pressure is exactly the kind of unasked-for scope this repo's "if the
//         corpus doesn't exercise it, don't invent handling for it" posture (docs/uix-dom.md
//         section 4's own `<head>`-attribute-non-capture precedent) argues against. A future
//         fixture that needs it is a new, separate finding.
//       - a `<head`/`</head` byte sequence sitting inside the opaque content of a THIRD-PARTY
//         opaque construct this file has no concept of (a nested CDATA-tag registered only by an
//         embedding application, for instance) -- out of this format's knowledge, same "the
//         corpus decides" limit as every other edge case this file documents.
//       - **the CLOSING `</style`/`</script` search itself is a plain, literal, non-tag-name-
//         verified substring search** (mirroring how this file's own `</head` search already
//         worked, and matching the PRE-existing, documented simplification of this format's
//         closing-tag boundary, docs/uix-dom.md section 4's literal wording) -- NOT the
//         name-verified re-scan upstream's own `ReadCDATA` performs (`BaseXMLParser.cpp:365-388`:
//         a `<...>` sequence found inside CDATA content is only accepted as the terminator if its
//         OWN tag name, after stripping a leading `/`, equals the registered CDATA tag name;
//         anything else -- including an unrelated closing tag like a coincidental `</head>` --
//         is folded back into the accumulated data and scanning continues). Replicating that
//         full re-scan is more machinery than this corpus currently justifies; declared here so a
//         future maintainer does not assume byte-for-byte fidelity with upstream's CDATA-close
//         detection that this file does not actually implement.
// PT: 🔴 `UIX-HEAD-COMENTARIO` -- o conserto estrutural. Substitui o antigo `ifind(source,
//     "<head", ...)` cru (uma busca de substring literal sem consciência de comentário, o
//     defeito medido: em 15 das 16 fixtures do corpus ele casava prosa dentro do comentário de
//     proveniência inicial, ~819-1069 bytes antes do elemento real -- ledger da seção 9 do
//     docs/uix-dom.md, classe (a)) por uma única passada pra frente que percorre o documento COMO
//     TAGS, não como uma sopa de bytes indiferenciada, e só reconhece uma ocorrência de
//     `<head`/`</head` como o elemento real quando ela de fato é alcançada como um token de tag
//     no nível de topo do scan -- nunca enquanto estiver dentro de:
//       - um comentário XML (`<!-- ... -->`) -- o bug medido;
//       - um valor de atributo entre aspas de QUALQUER tag (não só a própria `<head>` -- o scan
//         consciente de aspas pré-existente só protegia os PRÓPRIOS atributos da `<head>`; um
//         substring `<head` sentado dentro do valor de atributo de, ex.,
//         `<meta title="parece <head> aqui">` NÃO era protegido antes deste conserto, porque o
//         código antigo nunca olhava pra tag nenhuma além do único casamento de substring ao qual
//         já tinha se comprometido);
//       - o conteúdo opaco, tipo-CDATA, de uma tag `<style>` ou `<script>` -- confirmado contra o
//         fonte do RmlUi upstream, não suposto: `Factory.cpp:255-257` chama
//         `XMLParser::RegisterPersistentCDATATag("script")` e `("style")`, então o próprio parser
//         do RmlUi (`BaseXMLParser::ReadCDATA`, `BaseXMLParser.cpp:349-390`) trata o conteúdo das
//         duas tags como dado cru, não-tokenizado -- nenhum `<...>` aninhado dentro de um bloco
//         `<style>`/`<script>` é "uma tag" nenhuma pro upstream, então não pode ser uma pra este
//         scanner também. Este é exatamente o risco que as PRÓPRIAS fixtures deste repo
//         documentam como real (ver o comentário de proveniência que o `dom_dump.hpp` e todo
//         `.rml` do corpus sob `glintfx/src/uix/dom/test_fixtures/` carrega: *"Real RCSS comment
//         prose inside several of these 15 files' <head>/<style> blocks contains a literal,
//         un-escaped '<'..."*), mesmo que nenhuma fixture do corpus de hoje por acaso soletre
//         esse `<` literal como `<head` ou `</head` especificamente (medido: exatamente uma
//         ocorrência com forma `<head` por fixture, o elemento real, mais os 3 falsos casamentos
//         de prosa de comentário que este conserto remove).
//
//     🔴 Teto de escopo declarado, deliberado -- o que este scanner NÃO reconhece como opaco, de
//     propósito:
//       - seções `<![CDATA[ ... ]]>` -- o RmlUi upstream também suporta o construto XML literal
//         `<![CDATA[` (`BaseXMLParser.cpp:145-152`, um caminho de código SEPARADO do mecanismo de
//         tag-CDATA-persistente que style/script usam), mas nenhuma fixture do corpus desta onda
//         (`glintfx/tests/*.rml`, `glintfx/demos/showcase/*.rml`,
//         `glintfx/src/uix/dom/test_fixtures/*.rml`) usa isso, e o subconjunto congelado do
//         `docs/rmlx-subset.md` também não nomeia -- somar um terceiro mecanismo de
//         conteúdo-opaco aqui com pressão zero de corpus é exatamente o tipo de escopo não-pedido
//         que a postura "se o corpus não exercita, não inventa tratamento" deste repo (o próprio
//         precedente de não-captura de atributo de `<head>` da seção 4 do docs/uix-dom.md)
//         argumenta contra. Uma fixture futura que precise disso é um achado novo, separado.
//       - uma sequência de bytes `<head`/`</head` sentada dentro do conteúdo opaco de um TERCEIRO
//         construto opaco que este arquivo não tem conceito nenhum (uma tag-CDATA aninhada
//         registrada só por uma aplicação hospedeira, por exemplo) -- fora do conhecimento deste
//         formato, mesmo limite "o corpus decide" que todo outro caso de borda deste arquivo
//         documenta.
//       - **a própria busca de FECHAMENTO `</style`/`</script` é uma busca de substring literal,
//         simples, sem verificação de nome de tag** (espelhando como a própria busca `</head`
//         deste arquivo já funcionava, e batendo com a simplificação PRÉ-existente, documentada,
//         da fronteira de tag-de-fechamento deste formato, redação literal da seção 4 do
//         docs/uix-dom.md) -- NÃO o re-scan com verificação de nome que o próprio `ReadCDATA` do
//         upstream faz (`BaseXMLParser.cpp:365-388`: uma sequência `<...>` achada dentro do
//         conteúdo CDATA só é aceita como terminador se o PRÓPRIO nome de tag dela, depois de
//         tirar uma `/` inicial, for igual ao nome de tag CDATA registrado; qualquer outra coisa
//         -- incluindo uma tag de fechamento não-relacionada como um `</head>` coincidente -- é
//         dobrada de volta pro dado acumulado e o scan continua). Replicar aquele re-scan
//         completo é mais maquinaria do que este corpus justifica hoje; declarado aqui pra um
//         mantenedor futuro não supor fidelidade byte-a-byte com a detecção de fechamento de
//         CDATA do upstream que este arquivo de fato não implementa.
HeadTagSpan locate_head_tag_span(const std::string& source) {
  HeadTagSpan span;
  const std::size_t n = source.size();
  std::size_t pos = 0;

  while (pos < n) {
    // EN: XML comment -- skip the WHOLE thing, wholesale, before even looking at what is inside.
    //     An unterminated comment (no "-->" anywhere in the rest of the source) consumes the
    //     rest of the document -- malformed input, same "fail toward not-found rather than
    //     guessing" posture the rest of this file already applies.
    // PT: Comentário XML -- pula o TODO, por inteiro, antes até de olhar o que tem dentro. Um
    //     comentário não-terminado (nenhum "-->" no resto da fonte) consome o resto do
    //     documento -- input malformado, mesma postura "falha pro lado de não-achado em vez de
    //     chutar" que o resto deste arquivo já aplica.
    if (pos + 4 <= n && source.compare(pos, 4, "<!--") == 0) {
      const std::size_t end = source.find("-->", pos + 4);
      pos = (end == std::string::npos) ? n : end + 3;
      continue;
    }

    if (source[pos] != '<') {
      ++pos;
      continue;
    }

    // EN: `pos` is a '<'. Determine whether it opens a recognizable tag token (name starting
    //     right after '<', or after '</' for a closing tag) -- a '<' that does not, per this
    //     file's own tag-name character classes above, is not treated as any kind of tag (e.g. a
    //     stray '<' in malformed input, or a numeric-comparison-looking `a<b` outside any
    //     attribute -- extremely unlikely in real RML body text since XML forbids bare '<' in
    //     text, but this scanner fails toward "just a byte", not toward a guess). Advances by
    //     exactly one byte in that case so the very next position is still examined (never skips
    //     past a real tag that happens to immediately follow an unrecognized '<').
    // PT: `pos` é um '<'. Determina se abre um token de tag reconhecível (nome começando logo
    //     após '<', ou após '</' pra uma tag de fechamento) -- um '<' que não abre, pelas
    //     próprias classes de caractere de nome-de-tag deste arquivo acima, não é tratado como
    //     tag nenhuma (ex.: um '<' solto em input malformado, ou algo com cara de comparação
    //     numérica `a<b` fora de qualquer atributo -- extremamente improvável em texto de body
    //     RML real já que XML proíbe '<' solto em texto, mas este scanner falha pro lado de "só
    //     um byte", não pro lado de um chute). Avança exatamente um byte nesse caso pra que a
    //     posição seguinte ainda seja examinada (nunca pula por cima de uma tag real que por
    //     acaso vem logo depois de um '<' não-reconhecido).
    const bool closing = (pos + 1 < n && source[pos + 1] == '/');
    const std::size_t name_start = pos + (closing ? 2 : 1);
    std::size_t name_end = name_start;
    if (name_end < n && is_tag_name_start(source[name_end])) {
      ++name_end;
      while (name_end < n && is_tag_name_char(source[name_end])) ++name_end;
    }
    const bool boundary_ok =
        name_end > name_start && name_end < n &&
        (is_ws(source[name_end]) || source[name_end] == '>' || source[name_end] == '/');
    if (!boundary_ok) {
      ++pos;
      continue;
    }

    std::string tag_name = source.substr(name_start, name_end - name_start);
    for (char& c : tag_name) c = to_lower_ascii(c);

    const std::size_t close_gt = find_tag_close_gt(source, name_end);
    if (close_gt == std::string::npos) {
      // EN: An unterminated tag -- malformed input, consume the rest (same "fail toward
      //     not-found" posture as the comment branch above).
      // PT: Uma tag não-terminada -- input malformado, consome o resto (mesma postura "falha pro
      //     lado de não-achado" do ramo de comentário acima).
      pos = n;
      continue;
    }

    bool self_closing = false;
    {
      std::size_t j = close_gt;
      while (j > pos && is_ws(source[j - 1])) --j;
      if (j > pos && source[j - 1] == '/') self_closing = true;
    }

    if (!span.found && !closing && tag_name == "head") {
      span.found = true;
      span.self_closing = self_closing;
      span.open_close_gt = close_gt;
      if (self_closing) return span; // no interior, nothing left to scan for
      pos = close_gt + 1;
      continue;
    }

    if (span.found && !span.self_closing && closing && tag_name == "head") {
      // EN: The real closing tag -- only reachable here because every comment, every quoted
      //     attribute value (of this or any preceding tag) and every <style>/<script> opaque
      //     block between the open tag and here was already skipped wholesale above, never
      //     re-examined byte by byte at this top level.
      // PT: A tag de fechamento real -- só alcançável aqui porque todo comentário, todo valor de
      //     atributo entre aspas (desta ou de qualquer tag anterior) e todo bloco opaco
      //     <style>/<script> entre a tag de abertura e aqui já foi pulado por inteiro acima,
      //     nunca re-examinado byte a byte neste nível de topo.
      span.close_open_lt = pos;
      return span;
    }

    if (!closing && (tag_name == "style" || tag_name == "script")) {
      // EN: Opaque CDATA-like content (see this function's own header comment for the upstream
      //     citation) -- jump straight to the literal closing-tag text and never look at a
      //     single byte of the content in between at this top level.
      // PT: Conteúdo opaco tipo-CDATA (ver o próprio comentário de cabeçalho desta função pra
      //     citação do upstream) -- pula direto pro texto literal da tag de fechamento e nunca
      //     olha um byte sequer do conteúdo no meio, neste nível de topo.
      const std::string closer = std::string("</") + tag_name;
      const std::size_t style_close = ifind(source, closer.c_str(), close_gt + 1);
      pos = (style_close == std::string::npos) ? n : style_close;
      continue;
    }

    // EN: Any other tag (open or close, not <head>, not opaque) -- already consumed up to its
    //     own '>' via the quote-aware scan above; nothing more to do but move past it.
    // PT: Qualquer outra tag (abertura ou fechamento, não <head>, não opaca) -- já consumida até
    //     o próprio '>' via o scan consciente-de-aspas acima; nada mais a fazer além de passar
    //     por cima dela.
    pos = close_gt + 1;
  }

  return span;
}

// EN: Splits `raw` on runs of the 4-char whitespace set, drops empty tokens, deduplicates and
//     sorts ascending byte-wise -- all three in one step via `std::set<std::string>`, whose
//     default `operator<` on `std::string` compares via `char_traits<char>::lt`, DEFINED by the
//     standard as an UNSIGNED-byte comparison (`static_cast<unsigned char>(c1) <
//     static_cast<unsigned char>(c2)`) -- i.e. exactly the "strcmp-style, no locale" byte-wise
//     order docs/uix-dom.md section 7 asks for, with zero extra code needed to get there.
// PT: Divide `raw` em runs do conjunto de 4 caracteres de whitespace, descarta tokens vazios,
//     deduplica e ordena ascendente byte-a-byte -- os três num passo só via
//     `std::set<std::string>`, cujo `operator<` default sobre `std::string` compara via
//     `char_traits<char>::lt`, DEFINIDO pela norma como comparação de byte SEM SINAL
//     (`static_cast<unsigned char>(c1) < static_cast<unsigned char>(c2)`) -- ou seja, exatamente
//     a ordem byte-a-byte "estilo strcmp, sem locale" que a seção 7 do docs/uix-dom.md pede, sem
//     código extra nenhum pra chegar lá.
// EN: docs/uix-dom.md section 7's class-split algorithm, revised 2026-08-05
//     (UIX-CLASS-SPLIT-SPEC): cut `raw` on LITERAL SPACE (`' '`, 0x20) only -- not `is_ws()`
//     above, which stays the 4-char set for section 6/escaping/self-closing-tag purposes and is
//     deliberately NOT reused here, this is the one place in this file where that would be
//     wrong. Within each space-delimited segment, trim a leading/trailing run of the OTHER 3
//     whitespace bytes (`\t`, `\n`, `\r`); an embedded one (surrounded by real content on both
//     sides) survives verbatim. Matches `StringUtilities::ExpandString(raw, ' ')`
//     (`examples/RmlUi/Source/Core/ElementStyle.cpp:580-583`) for this case; see section 7's own
//     text for the one documented, deliberate non-replication (repeated/leading/trailing space
//     producing an empty-string class entry in live RmlUi -- not reproduced here, ledger row
//     dated 2026-08-05).
// PT: Algoritmo de split de classe da seção 7 do docs/uix-dom.md, revisado em 2026-08-05
//     (UIX-CLASS-SPLIT-SPEC): corta `raw` só por ESPAÇO LITERAL (`' '`, 0x20) -- não pelo
//     `is_ws()` acima, que continua o conjunto de 4 chars pros propósitos de seção 6/escape/
//     espaço-final de tag auto-fechada e de propósito NÃO é reaproveitado aqui, este é o único
//     lugar deste arquivo onde isso seria errado. Dentro de cada segmento delimitado por
//     espaço, apara um run no início/fim dos outros 3 bytes de whitespace (`\t`, `\n`, `\r`); um
//     embutido (cercado de conteúdo real dos dois lados) sobrevive verbatim. Bate com
//     `StringUtilities::ExpandString(raw, ' ')`
//     (`examples/RmlUi/Source/Core/ElementStyle.cpp:580-583`) pra este caso; ver o próprio texto
//     da seção 7 pra única não-replicação documentada e deliberada (espaço repetido/no
//     início/fim produzindo uma entrada de classe string-vazia no RmlUi ao vivo -- não
//     reproduzida aqui, linha do ledger datada de 2026-08-05).
std::set<std::string> split_dedup_sorted(const std::string& raw) {
  auto is_other_ws = [](char c) { return c == '\t' || c == '\n' || c == '\r'; };
  std::set<std::string> out;
  const std::size_t n = raw.size();
  std::size_t pos = 0;
  while (pos <= n) {
    const std::size_t next_space = raw.find(' ', pos);
    const std::size_t end = (next_space == std::string::npos) ? n : next_space;
    std::size_t start = pos;
    while (start < end && is_other_ws(raw[start])) ++start;
    std::size_t trimmed_end = end;
    while (trimmed_end > start && is_other_ws(raw[trimmed_end - 1])) --trimmed_end;
    if (trimmed_end > start) out.insert(raw.substr(start, trimmed_end - start));
    if (next_space == std::string::npos) break;
    pos = next_space + 1;
  }
  return out;
}

} // namespace

std::string dom_dump_escape(const std::string& raw) {
  std::string out;
  out.reserve(raw.size());
  for (unsigned char c : raw) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out += static_cast<char>(c);
        break;
    }
  }
  return out;
}

HeadScanResult dom_dump_scan_head(const std::string& source) {
  // EN: `UIX-HEAD-COMENTARIO` fix -- delegates the actual location work to
  //     `locate_head_tag_span()` (this TU's own anonymous namespace above), which walks the
  //     document AS TAGS (comment/quote/opaque-content aware) instead of doing an
  //     undifferentiated literal-substring search. See that function's own header comment for
  //     the full rationale, the upstream citation for the <style>/<script> opaque treatment, and
  //     the declared scope teto (CDATA sections, third-party opaque constructs, and the
  //     literal -- not tag-name-verified -- </style>//</script> closing search are explicitly
  //     NOT handled).
  // PT: Conserto da `UIX-HEAD-COMENTARIO` -- delega o trabalho de localização de fato pro
  //     `locate_head_tag_span()` (namespace anônimo desta própria TU, acima), que percorre o
  //     documento COMO TAGS (consciente de comentário/aspas/conteúdo-opaco) em vez de fazer uma
  //     busca de substring literal indiferenciada. Ver o próprio comentário de cabeçalho daquela
  //     função pro racional completo, a citação do upstream pro tratamento opaco de
  //     <style>/<script>, e o teto de escopo declarado (seções CDATA, construtos opacos de
  //     terceiros, e a busca de fechamento </style>/</script> literal -- sem verificação de nome
  //     de tag -- explicitamente NÃO tratados).
  const HeadTagSpan span = locate_head_tag_span(source);
  if (!span.found) return HeadScanResult{false, ""};
  if (span.self_closing) return HeadScanResult{true, ""};

  const std::size_t content_start = span.open_close_gt + 1;
  if (span.close_open_lt == std::string::npos) {
    // EN: Opening tag found but no matching close anywhere in the rest of the source --
    //     malformed. Return everything to end of source rather than silently dropping content;
    //     still marked PRESENT since a real `<head>` open tag genuinely was found (same "fail
    //     toward not-found rather than guessing" posture the rest of this file applies).
    // PT: Tag de abertura achada mas sem fechamento correspondente em lugar nenhum do resto da
    //     fonte -- malformado. Devolve tudo até o fim da fonte em vez de derrubar conteúdo em
    //     silêncio; ainda marcado PRESENT já que uma tag de abertura `<head>` real de fato foi
    //     achada (mesma postura "falha pro lado de não-achado em vez de chutar" que o resto
    //     deste arquivo aplica).
    return HeadScanResult{true, source.substr(content_start)};
  }
  return HeadScanResult{true, source.substr(content_start, span.close_open_lt - content_start)};
}

namespace {

// EN: Sorted (name, decoded-string-value) pairs for `el`'s generic ATTR block -- "id"/"class"
//     excluded (section 7: they have dedicated ID/CLASS lines), sorted by NAME ascending
//     byte-wise (same `std::string operator<` reasoning as split_dedup_sorted() above).
//     `Rml::ElementAttributes` (`Dictionary` = `SmallUnorderedMap<String, Variant>`) is NOT
//     contractually ordered -- this sort is the reason dom_dump_determinism_sanity.cpp's F6
//     case (same attributes, different SOURCE order) proves byte-identical output.
// PT: Pares (nome, valor-string-decodificado) ordenados pro bloco ATTR genérico de `el` --
//     "id"/"class" excluídos (seção 7: têm linhas ID/CLASS dedicadas), ordenados por NOME
//     ascendente byte-a-byte (mesmo raciocínio de `std::string operator<` do
//     split_dedup_sorted() acima). `Rml::ElementAttributes` (`Dictionary` =
//     `SmallUnorderedMap<String, Variant>`) NÃO é ordenado por contrato -- este sort é o motivo
//     do caso F6 do dom_dump_determinism_sanity.cpp (mesmos atributos, ordem-fonte diferente)
//     provar saída byte-idêntica.
std::vector<std::pair<std::string, std::string>> sorted_generic_attrs(const Rml::Element* el) {
  std::vector<std::pair<std::string, std::string>> attrs;
  for (const auto& kv : el->GetAttributes()) {
    const std::string& name = kv.first;
    if (name == "id" || name == "class") continue;
    attrs.emplace_back(std::string(name), kv.second.Get<Rml::String>());
  }
  std::sort(attrs.begin(), attrs.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
  return attrs;
}

// EN: `path` is the CALLER's own path (already includes this element's own segment) -- pure
//     text-emission function, no traversal decision made here (traversal/indexing lives in
//     dump_node below).
// PT: `path` é o próprio caminho do CHAMADOR (já inclui o segmento deste elemento) -- função
//     puramente de emissão de texto, nenhuma decisão de travessia é tomada aqui
//     (travessia/indexação mora no dump_node abaixo).
void emit_element_header(const Rml::Element* el, const std::string& path, std::string& out) {
  out += path;
  out += " ELEM ";
  out += std::string(el->GetTagName());
  out += "\n";

  const std::string id(el->GetId());
  if (!id.empty()) {
    out += path;
    out += " ID ";
    out += dom_dump_escape(id);
    out += "\n";
  }

  // EN: Raw `class` attribute value -- see dom_dump.hpp's "TECHNICAL NOTE" (and docs/uix-dom.md
  //     section 7) for why this is GetAttribute(), never GetClassNames().
  // PT: Valor cru do atributo `class` -- ver a "NOTA TÉCNICA" de dom_dump.hpp (e a seção 7 do
  //     docs/uix-dom.md) pro motivo disto ser GetAttribute(), nunca GetClassNames().
  const std::string raw_class = el->GetAttribute<Rml::String>("class", Rml::String());
  const std::set<std::string> classes = split_dedup_sorted(raw_class);
  if (!classes.empty()) {
    out += path;
    out += " CLASS";
    for (const std::string& c : classes) {
      out += " ";
      out += dom_dump_escape(c);
    }
    out += "\n";
  }

  for (const auto& [name, value] : sorted_generic_attrs(el)) {
    out += path;
    out += " ATTR ";
    out += dom_dump_escape(name);
    out += "=";
    out += dom_dump_escape(value);
    out += "\n";
  }
}

// EN: Pre-order depth-first, one node (element OR text) per call -- see docs/uix-dom.md
//     section 5 for the two record shapes this dispatches between. `include_non_dom_elements =
//     false` (GetNumChildren/GetChild's own default) on purpose: excludes internally-generated
//     pseudo-elements (e.g. scrollbar widgets) RmlUi may attach to certain elements, which are
//     NOT part of the authored source tree this format describes -- see dom_dump.hpp's header
//     comment for why this is a documented, deliberate reading of section 5 ("the body
//     subtree"), not a silent omission.
// PT: Pré-ordem profundidade-primeiro, um nó (elemento OU texto) por chamada -- ver a seção 5
//     do docs/uix-dom.md pras duas formas de registro entre as quais isto despacha.
//     `include_non_dom_elements = false` (o próprio default de GetNumChildren/GetChild) de
//     propósito: exclui pseudo-elementos gerados internamente (ex.: widgets de scrollbar) que o
//     RmlUi pode anexar a certos elementos, que NÃO fazem parte da árvore-fonte autorada que
//     este formato descreve -- ver o comentário de cabeçalho de dom_dump.hpp pro motivo disto
//     ser uma leitura documentada e deliberada da seção 5 ("a subárvore body"), não uma omissão
//     silenciosa.
void dump_node(const Rml::Element* el, const std::string& path, std::string& out) {
  if (el->GetTagName() == "#text") {
    const auto* text_el = dynamic_cast<const Rml::ElementText*>(el);
    // EN: Guarded, not asserted -- "#text" is Factory's own tag+instancer name for
    //     Rml::ElementText (Factory.cpp:330-341/394), so this should never fail for a document
    //     RmlUi itself produced; a defensive empty TEXT line (rather than a null-deref) is the
    //     fail-safe choice if that invariant is ever violated by a future RmlUi version.
    // PT: Guardado, não assertado -- "#text" é o próprio nome de tag+instancer do Factory pro
    //     Rml::ElementText (Factory.cpp:330-341/394), então isto nunca deveria falhar pra um
    //     documento que o próprio RmlUi produziu; uma linha TEXT vazia defensiva (em vez de um
    //     null-deref) é a escolha à prova de falha se essa invariante for violada por uma
    //     futura versão do RmlUi.
    out += path;
    out += " TEXT ";
    out += dom_dump_escape(text_el ? std::string(text_el->GetText()) : std::string());
    out += "\n";
    return;
  }

  emit_element_header(el, path, out);
  const int n = el->GetNumChildren(false);
  out += path;
  out += " CHILDREN ";
  out += std::to_string(n);
  out += "\n";
  for (int i = 0; i < n; ++i) {
    dump_node(el->GetChild(i), path + "/" + std::to_string(i), out);
  }
}

} // namespace

std::string dom_dump_document(Rml::ElementDocument* doc, const std::string& source) {
  assert(doc != nullptr &&
         "dom_dump_document: doc must not be null (docs/uix-dom.md section 3: a document "
         "missing <body> is out of this format's scope; a null doc is a strictly worse caller "
         "error, checked by the call site, not this function)");

  std::string out;
  const HeadScanResult head = dom_dump_scan_head(source);
  if (head.present) {
    out += "HEAD PRESENT ";
    out += dom_dump_escape(head.raw);
    out += "\n";
  } else {
    out += "HEAD ABSENT\n";
  }

  // EN: `doc` (an Rml::ElementDocument*) IS the `<body>` element itself upstream -- docs/uix-
  //     dom.md section 3, evidenced by XMLNodeHandlerBody::ElementStart applying <body>'s own
  //     attributes directly to the ElementDocument when `document == element`
  //     (examples/RmlUi/Source/Core/XMLNodeHandlerBody.cpp:14-20) -- so dumping it through the
  //     SAME dump_node() every other element goes through (no root-specific branch) is exactly
  //     section 5's own "uniform record shape for every node, root included" design, not a
  //     coincidence that happens to produce the right tag name.
  // PT: `doc` (um Rml::ElementDocument*) É o próprio elemento `<body>` upstream -- seção 3 do
  //     docs/uix-dom.md, evidenciado por XMLNodeHandlerBody::ElementStart aplicar os próprios
  //     atributos de `<body>` direto no ElementDocument quando `document == element`
  //     (examples/RmlUi/Source/Core/XMLNodeHandlerBody.cpp:14-20) -- então dumpar ele pelo MESMO
  //     dump_node() por que todo outro elemento passa (sem ramo específico-de-raiz) é
  //     exatamente o próprio design "forma de registro uniforme pra todo nó, raiz incluída" da
  //     seção 5, não uma coincidência que por acaso produz o nome de tag certo.
  dump_node(doc, "body", out);
  return out;
}

} // namespace glintfx
