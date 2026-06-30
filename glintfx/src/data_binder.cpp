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

bool DataBinder::bind_number(const char* key, double initial) {
  if (!impl_->created || !key) return false;
  auto& cell = impl_->nums[key] = std::make_unique<double>(initial);
  return impl_->ctor.Bind(Rml::String(key), cell.get());
}

bool DataBinder::bind_string(const char* key, const char* initial) {
  if (!impl_->created || !key) return false;
  auto& cell = impl_->strs[key] =
      std::make_unique<Rml::String>(initial ? initial : "");
  return impl_->ctor.Bind(Rml::String(key), cell.get());
}

bool DataBinder::bind_bool(const char* key, bool initial) {
  if (!impl_->created || !key) return false;
  auto& cell = impl_->bools[key] = std::make_unique<bool>(initial);
  return impl_->ctor.Bind(Rml::String(key), cell.get());
}

bool DataBinder::bind_list(const char* key) {
  if (!impl_->created || !key) return false;
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

void DataBinder::set_list(const char* key, const char* const* items, std::size_t n) {
  if (!key) return;
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
