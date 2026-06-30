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

private:
  struct Impl;
  Impl* impl_;
};

} // namespace glintfx
