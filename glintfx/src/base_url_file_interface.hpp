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
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Types.h>
#include <cstdio>
#include <string>

namespace glintfx {

class BaseUrlFileInterface : public Rml::FileInterface {
public:
  // EN: AUD-L1-PARSE — hard ceiling (bytes) on any file this interface will hand an Open()
  //     handle for, enforced HERE (Open(), not per-caller) because Open() is the ONE place
  //     every consumer of Rml::GetFileInterface() converges on before reading a single byte:
  //       - Gl3RenderInterface::LoadTexture (render_gl3.cpp) — image assets.
  //       - FontEngineOwn::LoadFontFace (font_engine_own.cpp) — via the base
  //         Rml::FileInterface::LoadFile()'s own internal Open() call (LoadFile is virtual on
  //         Rml::FileInterface but NOT overridden here, so it runs RmlUi's default body:
  //         Open() -> Length() -> `out_data = String(length, 0)` -> Read() -> Close(); the size
  //         cap only has to live in Open() to still gate that unconditional String(length, 0)).
  //       - RmlUi's OWN default FreeType FontProvider::LoadFontFace (Source/Core/
  //         FontEngineDefault/FontProvider.cpp) — calls Open()+Length()+`new byte[length]`
  //         DIRECTLY, does NOT go through LoadFile() at all. A guard placed only on an
  //         overridden LoadFile() (instead of Open()) would therefore miss the FreeType path
  //         entirely — this is why the guard lives here, not in a LoadFile() override.
  //       - RmlUi's own .rml/.rcss document/stylesheet loading (also Open()-based).
  //     A hostile/corrupt file at ANY of these call sites (or a symlink/FUSE mount reporting a
  //     bogus size via SEEK_END) would otherwise drive an unbounded allocation — std::vector<
  //     unsigned char> buf(len) in LoadTexture, Rml::String(length,0) in the base LoadFile(),
  //     `new byte[length]` in FreeType's FontProvider — straight to bad_alloc/OOM, propagating
  //     across the RmlUi virtual-call boundary into the host application (DoS by asset, not a
  //     memory-safety bug, but just as real for an embedding host).
  //     256 MiB is generous for any single image/font/document asset a UI realistically needs.
  //     Returning a NULL handle (matching fopen's own failure convention) makes every existing
  //     caller's pre-existing `if (!handle)`/`if (!fh)` fail-secure path fire for free — no
  //     caller-side code needs to change.
  //     render_gl3.cpp's Gl3RenderInterface::LoadTexture ALSO carries its OWN explicit
  //     file-size + INT_MAX guards (added in an earlier AUD-L1-PARSE commit on this same
  //     branch). This is DELIBERATE layered defense (belt-and-suspenders), not redundant dead
  //     code kept by oversight: this Open()-level guard is the one that actually protects the
  //     FONT paths (which had no guard of their own before this commit); keeping the
  //     image-specific guard in render_gl3.cpp means the image path stays protected even if a
  //     host embedding glintfx ever calls `Rml::SetFileInterface()` itself after Bootstrap::
  //     load() runs, replacing this capped interface with one that does not share this ceiling
  //     (a real possibility in embed mode — UiLayer does not own the whole process the way App
  //     does). Verified NOT to be dead code today via mutation testing (removing the guard
  //     right here in a `GLINTFX_SANITIZE=ON` build makes tests/asset_decode_hostile_sanity's
  //     new "font_huge_1tib" case abort under ASan's own allocation-size-too-big trap — that
  //     case has NO other guard protecting it — while the render_gl3.cpp-guarded image cases
  //     stay green; see that test file's own top comment for the full writeup).
  // PT: AUD-L1-PARSE — teto rígido (bytes) sobre qualquer arquivo para o qual esta interface
  //     vai entregar um handle de Open(), aplicado AQUI (Open(), não por-chamador) porque
  //     Open() é o ÚNICO ponto onde TODO consumidor de Rml::GetFileInterface() converge antes
  //     de ler um único byte:
  //       - Gl3RenderInterface::LoadTexture (render_gl3.cpp) — assets de imagem.
  //       - FontEngineOwn::LoadFontFace (font_engine_own.cpp) — via a própria chamada Open()
  //         interna do LoadFile() base de Rml::FileInterface (LoadFile é virtual em
  //         Rml::FileInterface mas NÃO sobrescrito aqui, então roda o corpo default do RmlUi:
  //         Open() -> Length() -> `out_data = String(length, 0)` -> Read() -> Close(); o teto
  //         só precisa viver em Open() pra ainda barrar esse String(length, 0) incondicional).
  //       - O PRÓPRIO FontProvider::LoadFontFace default do FreeType do RmlUi (Source/Core/
  //         FontEngineDefault/FontProvider.cpp) — chama Open()+Length()+`new byte[length]`
  //         DIRETO, NÃO passa por LoadFile() nenhum. Uma guarda só num LoadFile() sobrescrito
  //         (em vez de Open()) perderia o caminho do FreeType inteiro — por isso a guarda vive
  //         aqui, não num override de LoadFile().
  //       - O próprio carregamento de documento/stylesheet .rml/.rcss do RmlUi (também baseado
  //         em Open()).
  //     Um arquivo hostil/corrompido em QUALQUER um desses pontos de chamada (ou um symlink/
  //     mount FUSE reportando um tamanho falso via SEEK_END) senão levaria a uma alocação sem
  //     teto — std::vector<unsigned char> buf(len) em LoadTexture, Rml::String(length,0) no
  //     LoadFile() da base, `new byte[length]` no FontProvider do FreeType — direto a
  //     bad_alloc/OOM, propagando pela fronteira de virtual-call do RmlUi até a aplicação host
  //     (DoS via asset, não um bug de memory-safety, mas igualmente real pra um host
  //     embarcador).
  //     256 MiB é generoso pra qualquer asset único de imagem/fonte/documento que uma UI
  //     realisticamente precisa. Retornar um handle NULL (casando com a própria convenção de
  //     falha do fopen) faz o caminho fail-secure `if (!handle)`/`if (!fh)` já existente de todo
  //     chamador disparar de graça — nenhum código do lado chamador precisa mudar.
  //     O Gl3RenderInterface::LoadTexture de render_gl3.cpp TAMBÉM carrega suas PRÓPRIAS
  //     guardas explícitas de tamanho de arquivo + INT_MAX (adicionadas num commit AUD-L1-PARSE
  //     anterior nesta mesma branch). Isto é defesa em camadas DELIBERADA (cinto-e-suspensório),
  //     não código morto redundante mantido por descuido: esta guarda a nível de Open() é a que
  //     de fato protege os caminhos de FONTE (que não tinham guarda própria antes deste
  //     commit); manter a guarda específica de imagem em render_gl3.cpp significa que o
  //     caminho de imagem continua protegido mesmo se um host que embarca a glintfx algum dia
  //     chamar `Rml::SetFileInterface()` por conta própria depois de Bootstrap::load() rodar,
  //     substituindo esta interface com teto por uma que não compartilhe este teto (uma
  //     possibilidade real em embed mode — UiLayer não é dona do processo inteiro como o App
  //     é). Verificado NÃO ser código morto hoje via mutation testing (remover a guarda bem
  //     aqui num build `GLINTFX_SANITIZE=ON` faz o novo caso "font_huge_1tib" de
  //     tests/asset_decode_hostile_sanity abortar sob a própria trap de
  //     allocation-size-too-big do ASan -- esse caso não tem NENHUMA outra guarda o
  //     protegendo -- enquanto os casos de imagem guardados por render_gl3.cpp permanecem
  //     verdes; ver o próprio comentário do topo daquele arquivo de teste pro relato completo).
  static constexpr long kMaxFileBytes = 256L * 1024L * 1024L;  // 256 MiB.

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
  //     is tried first; if it fails, the raw path is tried as fallback. Both attempts go
  //     through fopen_capped() below (AUD-L1-PARSE) so the size ceiling applies uniformly
  //     regardless of which of the two open attempts succeeds.
  // PT: Abre um arquivo. Quando base_url não está vazio e o path é relativo, o path
  //     prefixado é tentado primeiro; se falhar, tenta o path bruto como fallback. As duas
  //     tentativas passam por fopen_capped() abaixo (AUD-L1-PARSE) pra que o teto de tamanho
  //     se aplique uniformemente independente de qual das duas tentativas de abertura vinga.
  Rml::FileHandle Open(const Rml::String& path) override {
    if (!base_url_.empty() && !is_absolute(path)) {
      std::string full = base_url_;
      full += '/';
      full += path;
      if (FILE* f = fopen_capped(full.c_str()))
        return reinterpret_cast<Rml::FileHandle>(f);
    }
    // EN: Fallback: raw path (default RmlUi behaviour; also handles absolute paths).
    // PT: Fallback: path bruto (comportamento padrão do RmlUi; lida com paths absolutos).
    return reinterpret_cast<Rml::FileHandle>(fopen_capped(path.c_str()));
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

  // EN: AUD-L1-PARSE — fopen() + size-cap check, shared by BOTH open attempts in Open() above
  //     so kMaxFileBytes applies uniformly. Returns nullptr (fopen's own failure convention,
  //     already handled fail-secure by every existing caller of Open()) on open failure, a
  //     non-seekable handle (fail closed rather than proceeding with an unknown size), OR a
  //     size over the cap (logged once here, at the single point every caller converges on —
  //     see this class's own top comment for why individual callers do not each need their own
  //     copy of this check).
  // PT: AUD-L1-PARSE — fopen() + checagem de teto de tamanho, compartilhada pelas DUAS
  //     tentativas de abertura do Open() acima pra que kMaxFileBytes se aplique uniformemente.
  //     Retorna nullptr (a própria convenção de falha do fopen, já tratada fail-secure por todo
  //     chamador existente de Open()) em falha de abertura, um handle não-seekável (falha
  //     fechada em vez de seguir com tamanho desconhecido), OU um tamanho acima do teto
  //     (logado uma única vez aqui, no ponto único onde todo chamador converge — ver o próprio
  //     comentário do topo desta classe pro porquê de cada chamador individual não precisar da
  //     própria cópia desta checagem).
  static FILE* fopen_capped(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f)
      return nullptr;
    if (fseek(f, 0, SEEK_END) != 0) {
      fclose(f);
      return nullptr;
    }
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 0) {
      fclose(f);
      return nullptr;
    }
    if (size > kMaxFileBytes) {
      Rml::Log::Message(Rml::Log::LT_WARNING,
          "BaseUrlFileInterface::Open: '%s' is %ld bytes, exceeding the %ld byte cap -- "
          "refusing to open.",
          path, size, kMaxFileBytes);
      fclose(f);
      return nullptr;
    }
    return f;
  }
};

} // namespace glintfx
