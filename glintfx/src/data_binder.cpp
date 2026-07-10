// SPDX-License-Identifier: MPL-2.0
// EN: DataBinder implementation — wraps RmlUi DataModelConstructor + DataModelHandle,
//     owns stable cell storage keyed by variable name.
//     The Impl contains the actual RmlUi types; data_binder.hpp exposes only a pImpl
//     pointer so no RmlUi header is required by callers of data_binder.hpp.
// PT: Implementação do DataBinder — envolve DataModelConstructor + DataModelHandle do RmlUi,
//     possui armazenamento estável de células indexado por nome de variável.
//     O Impl contém os tipos RmlUi reais; data_binder.hpp expõe apenas um ponteiro pImpl
//     para que nenhum header RmlUi seja exigido pelos callers de data_binder.hpp.
// Copyright (c) 2026 Petrus Silva Costa

#include "data_binder.hpp"
#include <RmlUi/Core.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace glintfx {

struct DataBinder::Impl {
  Rml::DataModelConstructor ctor;    // EN: valid only after create(). PT: válido só após create().
  Rml::DataModelHandle      handle;  // EN: used to dirty variables. PT: usado para marcar variáveis sujas.
  bool created = false;

  // EN: Stable cell storage — std::map guarantees pointer stability on insert/emplace;
  //     addresses passed to Bind() remain valid for the lifetime of this Impl.
  // PT: Armazenamento estável de células — std::map garante estabilidade de ponteiro
  //     no insert/emplace; endereços passados ao Bind() permanecem válidos pelo lifetime deste Impl.
  std::map<std::string, std::unique_ptr<double>>                   nums;
  std::map<std::string, std::unique_ptr<Rml::String>>              strs;
  std::map<std::string, std::unique_ptr<bool>>                     bools;
  std::map<std::string, std::unique_ptr<std::vector<Rml::String>>> lists;
};

DataBinder::DataBinder()  : impl_(new Impl()) {}
DataBinder::~DataBinder() { delete impl_; }

bool DataBinder::create(Rml::Context* ctx, const char* name) {
  if (!ctx || !name || impl_->created) return false;
  // EN: CreateDataModel with allow_missing_variables=true: views that reference
  //     not-yet-bound vars yield default values silently instead of logging errors.
  // PT: CreateDataModel com allow_missing_variables=true: views que referenciam variáveis
  //     ainda não-ligadas produzem valores padrão silenciosamente em vez de logar erros.
  impl_->ctor = ctx->CreateDataModel(Rml::String(name), nullptr, true);
  if (!impl_->ctor) return false;
  // EN: RegisterArray<std::vector<Rml::String>>: makes string lists usable with data-for.
  //     Rml::String is a builtin scalar; the container still needs this registration.
  // PT: RegisterArray<std::vector<Rml::String>>: torna listas de strings usáveis com data-for.
  //     Rml::String é escalar builtin; o container ainda precisa desta registração.
  impl_->ctor.RegisterArray<std::vector<Rml::String>>();
  impl_->handle  = impl_->ctor.GetModelHandle();
  impl_->created = true;
  return true;
}

// EN: Guard against duplicate keys BEFORE touching any storage — same principle as
//     create()'s "if (... || impl_->created) return false;" check-before-mutate.
//     Rml::DataModelConstructor::Bind() does NOT overwrite an existing binding
//     (RmlUi's DataModel::BindVariable uses variables.emplace(), a no-op on collision
//     that just returns false); if we had already replaced the map entry via
//     operator=(unique_ptr), the previous cell would already be freed by the time
//     Bind() reports failure, and RmlUi's internal Rml::DataModel would be left holding
//     a dangling pointer to it — a use-after-free the moment any view re-evaluates that
//     variable. Checking `.find(key) != end()` first means a duplicate call is a pure
//     no-op: nothing is allocated, nothing is freed, nothing is (re)bound.
// PT: Guard contra chaves duplicadas ANTES de tocar em qualquer storage — mesmo princípio
//     do check-before-mutate de create() ("if (... || impl_->created) return false;").
//     Rml::DataModelConstructor::Bind() NÃO sobrescreve um binding existente (o
//     DataModel::BindVariable do RmlUi usa variables.emplace(), no-op em colisão que só
//     retorna false); se já tivéssemos substituído a entrada do mapa via
//     operator=(unique_ptr), a célula anterior já estaria liberada quando Bind() reporta
//     falha, e o Rml::DataModel interno do RmlUi ficaria com um ponteiro pendurado pra
//     ela — um use-after-free no instante em que qualquer view reavaliar essa variável.
//     Checar `.find(key) != end()` antes faz da chamada duplicada um no-op puro: nada é
//     alocado, nada é liberado, nada é (re)ligado.
bool DataBinder::bind_number(const char* key, double initial) {
  if (!impl_->created || !key) return false;
  if (impl_->nums.find(key) != impl_->nums.end()) return false;
  auto& cell = impl_->nums[key] = std::make_unique<double>(initial);
  return impl_->ctor.Bind(Rml::String(key), cell.get());
}

bool DataBinder::bind_string(const char* key, const char* initial) {
  if (!impl_->created || !key) return false;
  if (impl_->strs.find(key) != impl_->strs.end()) return false;
  auto& cell = impl_->strs[key] =
      std::make_unique<Rml::String>(initial ? initial : "");
  return impl_->ctor.Bind(Rml::String(key), cell.get());
}

bool DataBinder::bind_bool(const char* key, bool initial) {
  if (!impl_->created || !key) return false;
  if (impl_->bools.find(key) != impl_->bools.end()) return false;
  auto& cell = impl_->bools[key] = std::make_unique<bool>(initial);
  return impl_->ctor.Bind(Rml::String(key), cell.get());
}

bool DataBinder::bind_list(const char* key) {
  if (!impl_->created || !key) return false;
  if (impl_->lists.find(key) != impl_->lists.end()) return false;
  auto& cell = impl_->lists[key] =
      std::make_unique<std::vector<Rml::String>>();
  return impl_->ctor.Bind(Rml::String(key), cell.get());
}

void DataBinder::set_number(const char* key, double v) {
  if (!key) return;
  auto it = impl_->nums.find(key);
  if (it == impl_->nums.end()) return;
  *it->second = v;
  impl_->handle.DirtyVariable(Rml::String(key));
}

void DataBinder::set_string(const char* key, const char* v) {
  if (!key) return;
  auto it = impl_->strs.find(key);
  if (it == impl_->strs.end()) return;
  *it->second = v ? v : "";
  impl_->handle.DirtyVariable(Rml::String(key));
}

void DataBinder::set_bool(const char* key, bool v) {
  if (!key) return;
  auto it = impl_->bools.find(key);
  if (it == impl_->bools.end()) return;
  *it->second = v;
  impl_->handle.DirtyVariable(Rml::String(key));
}

bool DataBinder::get_number(const char* key, double& out) const {
  if (!key) return false;
  auto it = impl_->nums.find(key);
  if (it == impl_->nums.end()) return false;
  out = *it->second;
  return true;
}

bool DataBinder::get_string(const char* key, std::string& out) const {
  if (!key) return false;
  auto it = impl_->strs.find(key);
  if (it == impl_->strs.end()) return false;
  // EN: Rml::String is std::string under the pinned RmlUi build config (no custom allocator
  //     override in this project) -- a direct copy-assign, no per-character translation needed.
  // PT: Rml::String é std::string na config de build pinada do RmlUi (sem override de alocador
  //     customizado neste projeto) -- copy-assign direto, sem tradução por caractere necessária.
  out = *it->second;
  return true;
}

bool DataBinder::get_bool(const char* key, bool& out) const {
  if (!key) return false;
  auto it = impl_->bools.find(key);
  if (it == impl_->bools.end()) return false;
  out = *it->second;
  return true;
}

void DataBinder::set_list(const char* key, const char* const* items, std::size_t n) {
  if (!key) return;
  // EN: Guard against null items pointer when count is non-zero — would be UB in the loop.
  // PT: Guard contra ponteiro items nulo quando o count é não-zero — seria UB no loop.
  if (n > 0 && !items) return;
  auto it = impl_->lists.find(key);
  if (it == impl_->lists.end()) return;
  auto& vec = *it->second;
  vec.clear();
  vec.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    vec.emplace_back(items[i] ? items[i] : "");
  }
  impl_->handle.DirtyVariable(Rml::String(key));
}

} // namespace glintfx
