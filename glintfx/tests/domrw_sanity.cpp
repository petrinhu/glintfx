// SPDX-License-Identifier: MPL-2.0
// EN: domrw_sanity -- verifies Engine::set_text/add_class/remove_class/set_property (DOM
//     write by id) and Engine::get_number/get_string/get_bool (data-model read-back)
//     (L1.16-DOMRW, consumes AUD-PUB-6(d)+(e) -- imperative setters + write-only-to-two-way
//     data-model), the exact code UiLayer/App both delegate to with zero extra logic (straight
//     passthrough, see ui_layer.cpp/app.cpp) -- so this single test covers both public
//     consumption paths, same precedent as focus_sanity.cpp/document_reload_leak.cpp ("drive
//     Engine directly for the internal oracle").
//
//     Drives Engine (not UiLayer) directly because the ORACLEs needed here --
//     Rml::Element::GetInnerRML()/IsClassSet()/GetProperty<float>() -- are only reachable from
//     a test that includes RmlUi/Core/Element.h + RmlUi/Core/ElementDocument.h directly;
//     UiLayer's public header deliberately exposes no Rml:: types (encapsulation gate) and has
//     no context()/document accessor for tests to hook into.
//
//     set_text's markup-escaping "dente" (the case a stub that skipped
//     Rml::StringUtilities::EncodeRml would NOT pass): a malicious/untrusted text payload
//     containing `& < > "` is fed to set_text, then Rml::Element::GetInnerRML() is used as the
//     discriminating oracle. ElementText::GetRML() (Source/Core/ElementText.cpp, confirmed by
//     reading the pinned RmlUi 6.3 source) re-serialises a text node's content via the SAME
//     Rml::StringUtilities::EncodeRml the glintfx implementation uses -- so a correctly-escaped
//     set_text round-trips to the EXACT escaped string. An implementation that forwarded the
//     raw text straight into SetInnerRML() would instead have the RML parser interpret `<bye>`
//     as an ELEMENT (not literal text), producing a structurally different document that does
//     NOT round-trip back to the expected escaped string -- this is the test's bite.
//
//     Data-model get_* order note (per the task's "chave inexistente/ordem errada -> false"):
//     a key that was never bound via a matching bind_* is "ordem errada" and returns false
//     exactly like an unknown key would -- the two cases collapse to the same map-lookup-miss
//     in DataBinder. Once a key IS bound, it is deliberately readable BOTH before and after
//     load() (verified below) -- see DataBinder::get_number's doc-comment in data_binder.hpp
//     for why this is correct-by-construction (Rml::DataModelConstructor::Bind wraps the SAME
//     pointer glintfx's cell lives at, confirmed against the pinned RmlUi source), not merely
//     permissive.
//
// PT: domrw_sanity -- verifica Engine::set_text/add_class/remove_class/set_property (escrita de
//     DOM por id) e Engine::get_number/get_string/get_bool (leitura de volta do data-model)
//     (L1.16-DOMRW, consome AUD-PUB-6(d)+(e) -- setters imperativos + data-model
//     write-only-vira-two-way), o código EXATO para o qual UiLayer/App encaminham sem lógica
//     extra (repasse direto, ver ui_layer.cpp/app.cpp) -- então este único teste cobre os dois
//     caminhos públicos de consumo, mesmo precedente de focus_sanity.cpp/
//     document_reload_leak.cpp ("dirige o Engine diretamente pelo oráculo interno").
//
//     Dirige o Engine (não o UiLayer) diretamente porque os ORÁCULOS necessários aqui --
//     Rml::Element::GetInnerRML()/IsClassSet()/GetProperty<float>() -- só são alcançáveis de um
//     teste que inclui RmlUi/Core/Element.h + RmlUi/Core/ElementDocument.h diretamente; o
//     header público do UiLayer deliberadamente não expõe tipos Rml:: (gate de encapsulamento)
//     e não tem accessor de context()/documento pra testes se conectarem.
//
//     O "dente" de escape de markup do set_text (o caso que um stub que pulasse
//     Rml::StringUtilities::EncodeRml NÃO passaria): um payload de texto malicioso/não
//     confiável contendo `& < > "` é passado ao set_text, então Rml::Element::GetInnerRML() é
//     usado como oráculo discriminante. ElementText::GetRML() (Source/Core/ElementText.cpp,
//     confirmado lendo o source pinado do RmlUi 6.3) re-serializa o conteúdo de um nó de texto
//     via o MESMO Rml::StringUtilities::EncodeRml que a implementação da glintfx usa -- então
//     um set_text corretamente escapado faz round-trip pra EXATAMENTE a string escapada. Uma
//     implementação que encaminhasse o texto cru direto pro SetInnerRML() faria o parser RML
//     interpretar `<bye>` como um ELEMENTO (não texto literal), produzindo um documento
//     estruturalmente diferente que NÃO faz round-trip de volta pra string escapada esperada --
//     esta é a mordida do teste.
//
//     Nota de ordem do get_* do data-model (conforme "chave inexistente/ordem errada -> false"
//     da tarefa): uma chave que nunca foi ligada por um bind_* correspondente é "ordem errada"
//     e retorna false exatamente como uma chave desconhecida retornaria -- os dois casos
//     colapsam pro mesmo miss de busca no mapa dentro do DataBinder. Uma vez que uma chave ESTÁ
//     ligada, ela é deliberadamente legível TANTO antes QUANTO depois de load() (verificado
//     abaixo) -- ver o doc-comment de DataBinder::get_number em data_binder.hpp pro motivo
//     disto ser correto-por-construção (Rml::DataModelConstructor::Bind envolve o MESMO
//     ponteiro onde a célula da glintfx vive, confirmado contra o source pinado do RmlUi), não
//     meramente permissivo.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/window_glfw.hpp"
#include "../src/engine.hpp"
#include "../src/system_clock.hpp"
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <cstdio>
#include <string>

int main() {
  glintfx::WindowGlfw host;
  if (!host.create("domrw_host", 200, 150)) { std::puts("FAIL: host create"); return 1; }

  glintfx::SystemClock clock;
  glintfx::Engine engine;
  if (!engine.attach(&clock, 200, 150)) { std::puts("FAIL: engine attach"); return 2; }

  // ---------------------------------------------------------------------------
  // (C0) Hardening -- no document loaded yet: all 4 DOM-write methods must fail soft.
  // ---------------------------------------------------------------------------
  if (engine.set_text("text-target", "x")) { std::puts("FAIL: set_text before load()"); return 3; }
  if (engine.add_class("class-target", "highlight")) {
    std::puts("FAIL: add_class before load()"); return 4;
  }
  if (engine.remove_class("class-target", "base")) {
    std::puts("FAIL: remove_class before load()"); return 5;
  }
  if (engine.set_property("prop-target", "opacity", "0.5")) {
    std::puts("FAIL: set_property before load()"); return 6;
  }

  // ---------------------------------------------------------------------------
  // Data-model: create + bind BEFORE load() (mandatory order, enforced by Engine).
  // get_* on a bound-but-not-yet-loaded key must already work -- see the file header
  // comment for why this is correct-by-construction, not merely permissive.
  // ---------------------------------------------------------------------------
  double num_out = -1.0;
  std::string str_out = "unset";
  bool bool_out = false;

  if (engine.get_number("n", num_out)) {
    std::puts("FAIL: get_number(\"n\") returned true before bind_number"); return 7;
  }
  if (!engine.create_data_model("m")) { std::puts("FAIL: create_data_model"); return 8; }
  if (!engine.bind_number("n", 3.5)) { std::puts("FAIL: bind_number"); return 9; }
  if (!engine.bind_string("s", "hi")) { std::puts("FAIL: bind_string"); return 10; }
  if (!engine.bind_bool("b", true)) { std::puts("FAIL: bind_bool"); return 11; }

  // Readable immediately after bind_*, BEFORE load() -- the "dente" that a naive
  // load()-gated implementation would not pass.
  if (!engine.get_number("n", num_out) || num_out != 3.5) {
    std::puts("FAIL: get_number(\"n\") before load()"); return 12;
  }
  if (!engine.get_string("s", str_out) || str_out != "hi") {
    std::puts("FAIL: get_string(\"s\") before load()"); return 13;
  }
  if (!engine.get_bool("b", bool_out) || bool_out != true) {
    std::puts("FAIL: get_bool(\"b\") before load()"); return 14;
  }

  if (!engine.load("domrw_scene.rml")) { std::puts("FAIL: load(domrw_scene.rml)"); return 15; }
  engine.update();

  Rml::Context* ctx = engine.context();
  if (!ctx) { std::puts("FAIL: null context after successful load"); return 16; }
  Rml::ElementDocument* doc = ctx->GetDocument(0);
  if (!doc) { std::puts("FAIL: null document"); return 17; }

  // ---------------------------------------------------------------------------
  // (C1) Hardening -- null/empty id, null/empty cls/prop, unknown id -- document now loaded.
  // ---------------------------------------------------------------------------
  if (engine.set_text(nullptr, "x")) { std::puts("FAIL: set_text(nullptr, ...)"); return 18; }
  if (engine.set_text("", "x")) { std::puts("FAIL: set_text(\"\", ...)"); return 19; }
  if (engine.set_text("does-not-exist", "x")) {
    std::puts("FAIL: set_text(unknown id, ...)"); return 20;
  }

  if (engine.add_class(nullptr, "highlight")) { std::puts("FAIL: add_class(nullptr, cls)"); return 21; }
  if (engine.add_class("", "highlight")) { std::puts("FAIL: add_class(\"\", cls)"); return 22; }
  if (engine.add_class("class-target", nullptr)) {
    std::puts("FAIL: add_class(id, nullptr)"); return 23;
  }
  if (engine.add_class("class-target", "")) { std::puts("FAIL: add_class(id, \"\")"); return 24; }
  if (engine.add_class("does-not-exist", "highlight")) {
    std::puts("FAIL: add_class(unknown id, cls)"); return 25;
  }

  if (engine.remove_class(nullptr, "base")) { std::puts("FAIL: remove_class(nullptr, cls)"); return 26; }
  if (engine.remove_class("", "base")) { std::puts("FAIL: remove_class(\"\", cls)"); return 27; }
  if (engine.remove_class("class-target", nullptr)) {
    std::puts("FAIL: remove_class(id, nullptr)"); return 28;
  }
  if (engine.remove_class("class-target", "")) {
    std::puts("FAIL: remove_class(id, \"\")"); return 29;
  }
  if (engine.remove_class("does-not-exist", "base")) {
    std::puts("FAIL: remove_class(unknown id, cls)"); return 30;
  }

  if (engine.set_property(nullptr, "opacity", "0.5")) {
    std::puts("FAIL: set_property(nullptr, prop, value)"); return 31;
  }
  if (engine.set_property("", "opacity", "0.5")) {
    std::puts("FAIL: set_property(\"\", prop, value)"); return 32;
  }
  if (engine.set_property("prop-target", nullptr, "0.5")) {
    std::puts("FAIL: set_property(id, nullptr, value)"); return 33;
  }
  if (engine.set_property("prop-target", "", "0.5")) {
    std::puts("FAIL: set_property(id, \"\", value)"); return 34;
  }
  if (engine.set_property("does-not-exist", "opacity", "0.5")) {
    std::puts("FAIL: set_property(unknown id, prop, value)"); return 35;
  }

  // ---------------------------------------------------------------------------
  // (A) set_text happy path + the RML-injection "dente" (see file header comment).
  // ---------------------------------------------------------------------------
  Rml::Element* text_el = doc->GetElementById("text-target");
  if (!text_el) { std::puts("FAIL: #text-target not found"); return 36; }

  if (!engine.set_text("text-target", "hello")) { std::puts("FAIL: set_text happy path"); return 37; }
  if (text_el->GetInnerRML() != "hello") {
    std::puts("FAIL: GetInnerRML() != \"hello\" after plain set_text"); return 38;
  }

  // Null text normalises to "" -- never rejected.
  if (!engine.set_text("text-target", nullptr)) { std::puts("FAIL: set_text(id, nullptr)"); return 39; }
  if (text_el->GetInnerRML() != "") {
    std::puts("FAIL: GetInnerRML() != \"\" after set_text(id, nullptr)"); return 40;
  }

  // The escaping "dente": `& < > "` must round-trip through GetInnerRML() as the ESCAPED
  // form, proving no <bye> element was created and no attribute-breaking quote leaked.
  const char* malicious = "He said \"hi\" & <bye>";
  const std::string expected_escaped = "He said &quot;hi&quot; &amp; &lt;bye&gt;";
  if (!engine.set_text("text-target", malicious)) {
    std::puts("FAIL: set_text(id, malicious)"); return 41;
  }
  if (text_el->GetInnerRML() != expected_escaped) {
    std::printf("FAIL: GetInnerRML() = \"%s\", expected escaped \"%s\"\n",
                text_el->GetInnerRML().c_str(), expected_escaped.c_str());
    return 42;
  }
  // The malicious payload must NOT have created a real "bye" element.
  if (text_el->GetNumChildren() != 1 || text_el->GetChild(0)->GetTagName() != "#text") {
    std::puts("FAIL: set_text(malicious) created non-text child(ren) -- markup was NOT escaped");
    return 43;
  }

  // ---------------------------------------------------------------------------
  // (B) add_class/remove_class happy path.
  // ---------------------------------------------------------------------------
  Rml::Element* class_el = doc->GetElementById("class-target");
  if (!class_el) { std::puts("FAIL: #class-target not found"); return 44; }
  if (!class_el->IsClassSet("base")) {
    std::puts("FAIL: #class-target does not start with class \"base\" (scene setup)"); return 45;
  }

  if (!engine.add_class("class-target", "highlight")) { std::puts("FAIL: add_class happy path"); return 46; }
  if (!class_el->IsClassSet("highlight")) { std::puts("FAIL: \"highlight\" not set after add_class"); return 47; }
  if (!class_el->IsClassSet("base")) { std::puts("FAIL: add_class removed unrelated \"base\""); return 48; }

  if (!engine.remove_class("class-target", "base")) { std::puts("FAIL: remove_class happy path"); return 49; }
  if (class_el->IsClassSet("base")) { std::puts("FAIL: \"base\" still set after remove_class"); return 50; }
  if (!class_el->IsClassSet("highlight")) {
    std::puts("FAIL: remove_class(\"base\") removed unrelated \"highlight\""); return 51;
  }

  // Removing a class the element never had is a documented safe no-op (still true).
  if (!engine.remove_class("class-target", "never-had")) {
    std::puts("FAIL: remove_class(never-had class) did not return true (documented no-op)");
    return 52;
  }

  // ---------------------------------------------------------------------------
  // (C) set_property happy path + propagated parse-failure (invalid value -> false,
  //     no side effect -- "propague o false de parse inválido" per the task brief).
  // ---------------------------------------------------------------------------
  Rml::Element* prop_el = doc->GetElementById("prop-target");
  if (!prop_el) { std::puts("FAIL: #prop-target not found"); return 53; }

  if (!engine.set_property("prop-target", "opacity", "0.5")) {
    std::puts("FAIL: set_property happy path"); return 54;
  }
  if (prop_el->GetProperty<float>("opacity") != 0.5f) {
    std::puts("FAIL: opacity != 0.5 after set_property happy path"); return 55;
  }

  if (engine.set_property("prop-target", "opacity", "not-a-number")) {
    std::puts("FAIL: set_property with unparseable value returned true"); return 56;
  }
  if (prop_el->GetProperty<float>("opacity") != 0.5f) {
    std::puts("FAIL: opacity changed after a REJECTED set_property call"); return 57;
  }

  // ---------------------------------------------------------------------------
  // Data-model get_* AFTER load() + AFTER set_* -- reflects the current value; unknown key
  // still false.
  // ---------------------------------------------------------------------------
  engine.set_number("n", 42.0);
  engine.set_string("s", "world");
  engine.set_bool("b", false);
  engine.update();

  if (!engine.get_number("n", num_out) || num_out != 42.0) {
    std::puts("FAIL: get_number(\"n\") after set_number"); return 58;
  }
  if (!engine.get_string("s", str_out) || str_out != "world") {
    std::puts("FAIL: get_string(\"s\") after set_string"); return 59;
  }
  if (!engine.get_bool("b", bool_out) || bool_out != false) {
    std::puts("FAIL: get_bool(\"b\") after set_bool"); return 60;
  }
  if (engine.get_number("does-not-exist", num_out)) {
    std::puts("FAIL: get_number(unknown key) returned true"); return 61;
  }
  if (engine.get_number(nullptr, num_out)) { std::puts("FAIL: get_number(nullptr)"); return 62; }
  if (engine.get_string(nullptr, str_out)) { std::puts("FAIL: get_string(nullptr)"); return 63; }
  if (engine.get_bool(nullptr, bool_out)) { std::puts("FAIL: get_bool(nullptr)"); return 64; }

  std::puts("domrw_sanity: PASS");
  return 0;
}
