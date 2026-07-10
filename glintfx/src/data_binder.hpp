// SPDX-License-Identifier: MPL-2.0
// EN: DataBinder — owns stable cells and a DataModelConstructor/Handle for a single
//     RmlUi data model. Internal to glintfx; never exposed in public headers.
//     Lifecycle: create() -> bind_*() -> (load happens externally) -> set_*().
//     Cells are stored in std::map to guarantee pointer stability on insert.
// PT: DataBinder — possui células estáveis e DataModelConstructor/Handle para um
//     único data model do RmlUi. Interno ao glintfx; nunca exposto nos headers públicos.
//     Ciclo de vida: create() -> bind_*() -> (load acontece externamente) -> set_*().
//     Células guardadas em std::map para garantir estabilidade de ponteiro no insert.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
namespace Rml { class Context; }
#include <cstddef>
#include <string>  // EN: std::string out-param for get_string (L1.16-DOMRW).
                   // PT: out-param std::string de get_string (L1.16-DOMRW).

namespace glintfx {

class DataBinder {
public:
  DataBinder();
  ~DataBinder();
  DataBinder(const DataBinder&)            = delete;
  DataBinder& operator=(const DataBinder&) = delete;

  // EN: Create a data model named 'name' on ctx.
  //     Must be called before any bind_* and before LoadDocument.
  //     allow_missing_variables=true so views that reference not-yet-bound vars
  //     produce default values silently (no log errors during load).
  //     Returns false when ctx is null, name is null, or creation failed.
  // PT: Cria um data model chamado 'name' em ctx.
  //     Deve ser chamado antes de qualquer bind_* e antes de LoadDocument.
  //     allow_missing_variables=true para que views que referenciam vars não-ligadas
  //     produzam valores padrão silenciosamente (sem erros de log no load).
  //     Retorna false se ctx for nulo, name for nulo ou a criação falhar.
  bool create(Rml::Context* ctx, const char* name);

  // EN: Bind typed cells. Must be called after create() and before LoadDocument.
  //     Returns false when the model was not created yet.
  // PT: Liga células tipadas. Deve ser chamado após create() e antes de LoadDocument.
  //     Retorna false quando o modelo ainda não foi criado.
  bool bind_number(const char* key, double initial);
  bool bind_string(const char* key, const char* initial);
  bool bind_bool  (const char* key, bool initial);
  bool bind_list  (const char* key);        // EN: string list for data-for.
                                            // PT: lista de strings para data-for.

  // EN: Update setters — mutate the stable cell and dirty the variable.
  //     No-op when the key was never bound (safe to call at any time after load).
  // PT: Setters de atualização — mutam a célula estável e marcam a variável suja.
  //     No-op quando a chave nunca foi ligada (seguro chamar a qualquer hora após load).
  void set_number(const char* key, double v);
  void set_string(const char* key, const char* v);
  void set_bool  (const char* key, bool v);
  void set_list  (const char* key, const char* const* items, std::size_t n);

  // EN: Read-back getters (L1.16-DOMRW, consumes AUD-PUB-6(e) -- the data-model was write-only
  //     before this: a host could push values in via set_*, but had no way to read back a value
  //     the UI itself (or the user, through a bound control -- e.g. an <input data-value>) wrote
  //     into the model. Correct: DataModelConstructor::Bind(name, ptr) (see
  //     Include/RmlUi/Core/DataModelHandle.h in the pinned RmlUi source) registers a
  //     DataVariable wrapping the SAME pointer this class stores the cell at
  //     (`static_cast<void*>(ptr)`) -- any bidirectional RmlUi-driven write (a data-view setter
  //     firing from user interaction) therefore lands directly in *our* cell, not in some
  //     RmlUi-internal shadow copy. Reading the cell here IS reading the model's live, current
  //     value, with no extra RmlUi call needed.
  //     Guard: false (out left untouched) when the key was never bound (mirrors set_*'s
  //     "no-op when key unknown" convention, but observable here since there is an out-param).
  //     No document/load-order guard of its own -- Engine::get_* (the caller) enforces "must be
  //     ok()"; a key that IS bound is readable at any point after bind_* succeeds, load() or not.
  // PT: Getters de leitura de volta (L1.16-DOMRW, consome AUD-PUB-6(e) -- o data-model era
  //     write-only antes disto: um host podia empurrar valores via set_*, mas não tinha como ler
  //     de volta um valor que a própria UI (ou o usuário, através de um controle ligado -- ex.:
  //     um <input data-value>) escreveu no modelo. Correto: DataModelConstructor::Bind(name, ptr)
  //     (ver Include/RmlUi/Core/DataModelHandle.h no source pinado do RmlUi) registra uma
  //     DataVariable envolvendo o MESMO ponteiro onde esta classe guarda a célula
  //     (`static_cast<void*>(ptr)`) -- qualquer escrita bidirecional dirigida pelo RmlUi (um
  //     setter de data-view disparando por interação do usuário) portanto cai direto na NOSSA
  //     célula, não numa cópia-sombra interna do RmlUi. Ler a célula aqui É ler o valor vivo e
  //     corrente do modelo, sem chamada extra ao RmlUi.
  //     Guard: false (out intocado) quando a chave nunca foi ligada (espelha a convenção
  //     "no-op quando chave desconhecida" do set_*, mas observável aqui por haver out-param).
  //     Sem guard próprio de documento/ordem-de-load -- Engine::get_* (o chamador) enforça
  //     "precisa ser ok()"; uma chave que ESTÁ ligada é legível a qualquer momento após bind_*
  //     ter sucesso, com ou sem load().
  bool get_number(const char* key, double& out) const;
  bool get_string(const char* key, std::string& out) const;
  bool get_bool  (const char* key, bool& out) const;

private:
  struct Impl;
  Impl* impl_;
};

} // namespace glintfx
