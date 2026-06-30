// SPDX-License-Identifier: MPL-2.0
// EN: BaseUrlFileInterface — Rml::FileInterface that prepends a configurable base URL to
//     relative asset paths, enabling assets to be resolved from an absolute root independent
//     of the process working directory or the document's own location.
//     Behaviour when base_url is set:
//       - Path is RELATIVE (does not start with '/') → open base_url + '/' + path.
//       - Path is ABSOLUTE (starts with '/') → open path as-is (absolute wins).
//     Behaviour when base_url is empty (default): identical to RmlUi's built-in
//     FileInterfaceDefault — just fopen(path).
//     The fallback path (raw path) is tried when the base_url-prefixed open fails so that
//     absolute RmlUi-internal paths (already resolved by RmlUi itself) still work.
// PT: BaseUrlFileInterface — Rml::FileInterface que prefixa um base URL configurável a
//     caminhos de asset relativos, permitindo que assets sejam resolvidos a partir de uma
//     raiz absoluta independente do diretório de trabalho do processo ou da localização do
//     documento.
//     Comportamento quando base_url está definido:
//       - Path é RELATIVO (não começa com '/') → abre base_url + '/' + path.
//       - Path é ABSOLUTO (começa com '/') → abre o path como-está (absoluto prevalece).
//     Comportamento quando base_url está vazio (padrão): idêntico ao FileInterfaceDefault
//     embutido do RmlUi — apenas fopen(path).
//     O fallback (path bruto) é tentado quando o open prefixado com base_url falhar, para que
//     paths absolutos internos do RmlUi (já resolvidos por ele) ainda funcionem.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Types.h>
#include <cstdio>
#include <string>

namespace glintfx {

class BaseUrlFileInterface : public Rml::FileInterface {
public:
  // EN: Set the base URL prepended to relative paths in Open().
  //     Pass nullptr or "" to clear (reverts to default fopen behaviour).
  //     Trailing slashes are stripped; an internal '/' is inserted in Open().
  // PT: Define o base URL prefixado a caminhos relativos em Open().
  //     Passe nullptr ou "" para limpar (volta ao comportamento padrão fopen).
  //     Barras finais são removidas; um '/' interno é inserido em Open().
  void set_base_url(const char* url) noexcept {
    base_url_ = (url && url[0] != '\0') ? url : std::string{};
    while (!base_url_.empty() && base_url_.back() == '/')
      base_url_.pop_back();
  }

  // EN: Returns the current base URL (empty string when not set).
  // PT: Retorna o base URL atual (string vazia quando não definido).
  const std::string& base_url() const noexcept { return base_url_; }

  // EN: Open a file. When base_url is non-empty and path is relative, the prefixed path
  //     is tried first; if it fails, the raw path is tried as fallback.
  // PT: Abre um arquivo. Quando base_url não está vazio e o path é relativo, o path
  //     prefixado é tentado primeiro; se falhar, tenta o path bruto como fallback.
  Rml::FileHandle Open(const Rml::String& path) override {
    if (!base_url_.empty() && !is_absolute(path)) {
      std::string full = base_url_;
      full += '/';
      full += path;
      if (FILE* f = fopen(full.c_str(), "rb"))
        return reinterpret_cast<Rml::FileHandle>(f);
    }
    // EN: Fallback: raw path (default RmlUi behaviour; also handles absolute paths).
    // PT: Fallback: path bruto (comportamento padrão do RmlUi; lida com paths absolutos).
    return reinterpret_cast<Rml::FileHandle>(fopen(path.c_str(), "rb"));
  }

  void Close(Rml::FileHandle file) override {
    fclose(reinterpret_cast<FILE*>(file));
  }

  size_t Read(void* buffer, size_t size, Rml::FileHandle file) override {
    return fread(buffer, 1, size, reinterpret_cast<FILE*>(file));
  }

  bool Seek(Rml::FileHandle file, long offset, int origin) override {
    return fseek(reinterpret_cast<FILE*>(file), offset, origin) == 0;
  }

  size_t Tell(Rml::FileHandle file) override {
    return ftell(reinterpret_cast<FILE*>(file));
  }

private:
  std::string base_url_;

  // EN: Linux/POSIX: a path is absolute when its first char is '/'.
  //     This project targets Linux x86-64 only.
  // PT: Linux/POSIX: um path é absoluto quando o primeiro caractere é '/'.
  //     Este projeto mira Linux x86-64 somente.
  static bool is_absolute(const Rml::String& path) noexcept {
    return !path.empty() && path[0] == '/';
  }
};

} // namespace glintfx
