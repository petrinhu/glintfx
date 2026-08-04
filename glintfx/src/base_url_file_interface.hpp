// SPDX-License-Identifier: Apache-2.0
// EN: BaseUrlFileInterface — Rml::FileInterface that prepends a configurable base URL to
//     relative asset paths, enabling assets to be resolved from an absolute root independent
//     of the process working directory or the document's own location.
//     Behaviour when base_url is set:
//       - Path is RELATIVE (does not start with '/') → resolve base_url + '/' + path and
//         REFUSE to open it if the resolved path escapes base_url (ASSET-PATH, below).
//       - Path is ABSOLUTE (starts with '/') → open path as-is (absolute wins). Unchanged by
//         ASSET-PATH: this is the documented escape hatch for already-resolved RmlUi-internal
//         paths, not the traversal surface that was reported (see ASSET-PATH note).
//     Behaviour when base_url is empty (default): identical to RmlUi's built-in
//     FileInterfaceDefault — just fopen(path).
//     The raw-path fallback is tried when the base_url-prefixed open fails to find an existing
//     file (NOT when it escapes the base — see ASSET-PATH note below) so that absolute
//     RmlUi-internal paths (already resolved by RmlUi itself) still work.
//
//     ⚠ ASSET-PATH (2026-07-30, directory-traversal hardening, reciprocity request from a real
//     consumer that depends on this class's validation): before this, Open() did PURE STRING
//     CONCATENATION (`base_url_ + '/' + path`) with no traversal check at all -- a `path` of
//     `../../etc/passwd` (RELATIVE: does not start with '/', so it took the base-prefixed
//     branch, not the absolute-wins one) escaped base_url_ freely. Fixed via
//     `std::filesystem::weakly_canonical()` on both base_url_ and the candidate
//     (base_url_ / path), then a component-wise PREFIX comparison (candidate must have
//     base_canon as an ancestor, base_canon's own path.begin()..end() component sequence must
//     be a strict prefix of candidate's) -- resolve_within_base() below. A path that resolves
//     outside the base is REFUSED OUTRIGHT (nullptr, matching fopen's own failure convention):
//     deliberately NOT falling through to the raw-path fallback for an escaping path, because
//     that fallback opens against the process CWD with the SAME unmodified (hostile) relative
//     string -- falling through would just move the exact same traversal to a different anchor
//     and defeat the guard for the one input it exists to stop. The fallback still fires, as
//     before, for a resolved-WITHIN-base path that simply does not exist there (a normal 404,
//     not an escape attempt).
//     DECIDED POLICY ON SYMLINKS (the líder's/CTO's own open question, answered here): resolve,
//     don't refuse outright -- weakly_canonical() follows symlinks for the portion of the path
//     that exists on disk (documented std::filesystem behaviour: canonical()-equivalent
//     resolution for the existing prefix, lexical-only normalisation for the non-existing
//     tail), so a symlink PLANTED INSIDE base_url_ that points outside it IS caught by the same
//     prefix comparison -- its canonicalised target, not its lexical path, is what gets
//     compared. ⚠ RESIDUAL RISK, DECLARED NOT ELIMINATED: this is a check-then-open pattern
//     (TOCTOU/CWE-367) -- a symlink swapped between the weakly_canonical() call and the
//     fopen_capped() call a few lines later (or a FUSE mount whose resolution target changes
//     live) is NOT caught by construction; RmlUi's Rml::FileInterface::Open() contract returns
//     a stdio FILE*, which offers no fd-relative/O_NOFOLLOW primitive to close that gap without
//     a much larger interface change out of this fatia's scope. Accepted for the CURRENT threat
//     model this class documents (AUD-L1-PARSE's own top comment: asset identifiers come from
//     compile-time constants in every consumer today, not live untrusted input) -- re-evaluate
//     if that changes.
// PT: BaseUrlFileInterface — Rml::FileInterface que prefixa um base URL configurável a
//     caminhos de asset relativos, permitindo que assets sejam resolvidos a partir de uma
//     raiz absoluta independente do diretório de trabalho do processo ou da localização do
//     documento.
//     Comportamento quando base_url está definido:
//       - Path é RELATIVO (não começa com '/') → resolve base_url + '/' + path e RECUSA abrir
//         se o path resolvido escapar de base_url (ASSET-PATH, abaixo).
//       - Path é ABSOLUTO (começa com '/') → abre o path como-está (absoluto prevalece). Não
//         alterado pelo ASSET-PATH: é a válvula de escape documentada para paths internos do
//         RmlUi já resolvidos, não a superfície de travessia que foi reportada (ver nota
//         ASSET-PATH).
//     Comportamento quando base_url está vazio (padrão): idêntico ao FileInterfaceDefault
//     embutido do RmlUi — apenas fopen(path).
//     O fallback de path bruto é tentado quando o open prefixado com base_url falha por o
//     arquivo NÃO EXISTIR (NÃO quando ele escapa da base — ver nota ASSET-PATH abaixo), para
//     que paths absolutos internos do RmlUi (já resolvidos por ele) ainda funcionem.
//
//     ⚠ ASSET-PATH (2026-07-30, hardening de travessia de diretório, pedido de reciprocidade de
//     um consumidor real que depende da validação desta classe): antes disto, Open() fazia
//     CONCATENAÇÃO PURA DE STRING (`base_url_ + '/' + path`) sem checagem de travessia nenhuma
//     -- um `path` de `../../etc/passwd` (RELATIVO: não começa com '/', então entrava no ramo
//     prefixado com base, não no de absoluto-prevalece) escapava de base_url_ livremente.
//     Consertado via `std::filesystem::weakly_canonical()` em base_url_ e no candidato
//     (base_url_ / path), depois uma comparação de PREFIXO componente-a-componente (o candidato
//     tem de ter base_canon como ancestral, a sequência de componentes de base_canon tem de ser
//     prefixo estrito da do candidato) -- resolve_within_base() abaixo. Um path que resolve pra
//     fora da base é RECUSADO DIRETO (nullptr, casando com a própria convenção de falha do
//     fopen): deliberadamente NÃO cai no fallback de path bruto pra um path que escapa, porque
//     aquele fallback abre contra o CWD do processo com a MESMA string (hostil) sem modificar
//     -- cair nele só moveria a mesma travessia exata pra outra âncora e derrotaria a guarda
//     justamente pro input que ela existe pra barrar. O fallback continua disparando, como
//     antes, pra um path resolvido DENTRO da base que simplesmente não existe lá (um 404
//     normal, não uma tentativa de escape).
//     POLÍTICA DECIDIDA SOBRE SYMLINK (a própria pergunta em aberto do líder/CTO, respondida
//     aqui): resolver, não recusar de cara -- weakly_canonical() segue symlinks pra porção do
//     path que existe em disco (comportamento documentado do std::filesystem: resolução
//     equivalente a canonical() pro prefixo existente, normalização só-lexical pro final
//     não-existente), então um symlink PLANTADO DENTRO de base_url_ que aponta pra fora DELE É
//     pego pela mesma comparação de prefixo -- o alvo canonicalizado dele, não o path lexical,
//     é o que é comparado. ⚠ RISCO RESIDUAL, DECLARADO NÃO ELIMINADO: este é um padrão
//     checa-depois-abre (TOCTOU/CWE-367) -- um symlink trocado entre a chamada de
//     weakly_canonical() e a chamada de fopen_capped() algumas linhas depois (ou um mount FUSE
//     cujo alvo de resolução muda ao vivo) NÃO é pego por construção; o contrato de
//     Rml::FileInterface::Open() do RmlUi retorna um FILE* de stdio, que não oferece primitiva
//     fd-relativa/O_NOFOLLOW pra fechar essa brecha sem uma mudança de interface bem maior que
//     o escopo desta fatia. Aceito pro modelo de ameaça ATUAL que esta classe documenta (o
//     próprio comentário de topo do AUD-L1-PARSE: identificadores de asset vêm de constante de
//     compilação em todo consumidor hoje, não de input hostil ao vivo) -- reavaliar se isso
//     mudar.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include "byte_ceiling.hpp"

#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Types.h>
#include <climits>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>

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
  // EN: TETO-DEDUP (W26) -- VALUE now sourced from the shared policy constant
  //     (byte_ceiling.hpp's `glintfx::kMaxUntrustedFileBytes`, an unsigned `std::size_t`)
  //     instead of a locally re-typed, `long`-typed 256 MiB literal. The CHECK stays exactly
  //     where it was (fopen_capped() below), still this class's own independent guard (see this
  //     member's own top comment above for the belt-and-suspenders relationship with
  //     render_gl3.cpp's `kMaxAssetFileBytes`, unchanged by this dedup).
  //     THE TRAP THIS CAST AVOIDS: `fopen_capped()` compares this constant against `ftell()`'s
  //     own `long` (SIGNED) return, AFTER a `size < 0` guard has already run. Converting HERE, at
  //     declaration time, once -- rather than comparing the shared `std::size_t` (UNSIGNED)
  //     constant directly against that `long size` at the comparison site -- keeps that
  //     downstream comparison signed-vs-signed. A signed/unsigned comparison right there would be
  //     exactly the silent-bug class this project hunts: if the `size < 0` guard a few lines
  //     below fopen_capped() were ever reordered or removed, comparing an unsigned constant
  //     against a negative `long` would sign-extend it into a huge unsigned value and let it
  //     silently PASS the cap check instead of failing it. The `static_assert` below proves the
  //     shared 256 MiB value fits in `long` on any platform this project targets (the C++
  //     standard guarantees `LONG_MAX >= 2^31 - 1 = 2147483647`; 256 MiB = 268435456 fits
  //     comfortably even at that 32-bit floor), so the cast below never truncates.
  // PT: TETO-DEDUP (W26) -- VALOR agora vindo da constante de política compartilhada
  //     (`glintfx::kMaxUntrustedFileBytes` de byte_ceiling.hpp, um `std::size_t` UNSIGNED) em vez
  //     de um literal de 256 MiB tipado `long` re-tipado localmente. A CHECAGEM permanece
  //     exatamente onde estava (fopen_capped() abaixo), ainda a própria guarda independente desta
  //     classe (ver o próprio comentário de topo deste membro acima pra relação
  //     cinto-e-suspensório com o `kMaxAssetFileBytes` de render_gl3.cpp, inalterada por este
  //     dedup).
  //     A ARMADILHA QUE ESTE CAST EVITA: fopen_capped() compara esta constante contra o próprio
  //     retorno `long` (SIGNED) do ftell(), DEPOIS que uma guarda `size < 0` já rodou. Converter
  //     AQUI, no ponto de declaração, uma única vez -- em vez de comparar a constante `std::size_t`
  //     (UNSIGNED) compartilhada direto contra aquele `long size` no ponto de comparação -- mantém
  //     essa comparação a jusante signed-vs-signed. Uma comparação signed/unsigned bem ali seria
  //     exatamente a classe de bug silencioso que este projeto caça: se a guarda `size < 0` algumas
  //     linhas abaixo de fopen_capped() fosse um dia reordenada ou removida, comparar uma constante
  //     unsigned contra um `long` negativo faria sign-extend dele pra um valor unsigned enorme e o
  //     deixaria passar SILENCIOSAMENTE pela checagem de teto em vez de falhar nela. O
  //     `static_assert` abaixo prova que o valor compartilhado de 256 MiB cabe em `long` em
  //     qualquer plataforma que este projeto mira (o padrão C++ garante `LONG_MAX >= 2^31 - 1 =
  //     2147483647`; 256 MiB = 268435456 cabe confortavelmente mesmo nesse piso de 32 bits), então
  //     o cast abaixo nunca trunca.
  static_assert(glintfx::kMaxUntrustedFileBytes <= static_cast<std::size_t>(LONG_MAX),
                "kMaxUntrustedFileBytes must fit in `long` for BaseUrlFileInterface's "
                "ftell()-based size check (TETO-DEDUP)");
  static constexpr long kMaxFileBytes =
      static_cast<long>(glintfx::kMaxUntrustedFileBytes); // 256 MiB.

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

  // EN: Open a file. When base_url is non-empty and path is relative, the prefixed path is
  //     resolved and validated to stay within base_url (ASSET-PATH, see this class's own top
  //     comment) before being tried; a path that ESCAPES the base is refused outright, no
  //     fallback. A path that resolves within the base but does not exist there falls through
  //     to the raw-path fallback below (existing behaviour, unchanged). Every successful open
  //     attempt goes through fopen_capped() below (AUD-L1-PARSE) so the size ceiling applies
  //     uniformly regardless of which path was actually opened.
  // PT: Abre um arquivo. Quando base_url não está vazio e o path é relativo, o path prefixado
  //     é resolvido e validado pra permanecer dentro de base_url (ASSET-PATH, ver o próprio
  //     comentário de topo desta classe) antes de ser tentado; um path que ESCAPA da base é
  //     recusado direto, sem fallback. Um path que resolve dentro da base mas não existe lá
  //     cai no fallback de path bruto abaixo (comportamento existente, inalterado). Toda
  //     tentativa de abertura bem-sucedida passa por fopen_capped() abaixo (AUD-L1-PARSE) pra
  //     que o teto de tamanho se aplique uniformemente independente de qual path foi de fato
  //     aberto.
  Rml::FileHandle Open(const Rml::String& path) override {
    if (!base_url_.empty() && !is_absolute(path)) {
      std::filesystem::path resolved;
      if (!resolve_within_base(base_url_, path, resolved)) {
        Rml::Log::Message(Rml::Log::LT_WARNING,
                          "BaseUrlFileInterface::Open: '%s' resolves outside base '%s' -- refusing "
                          "(ASSET-PATH directory-traversal guard).",
                          path.c_str(), base_url_.c_str());
        return reinterpret_cast<Rml::FileHandle>(static_cast<FILE*>(nullptr));
      }
      if (FILE* f = fopen_capped(resolved.string().c_str()))
        return reinterpret_cast<Rml::FileHandle>(f);
      // EN: Resolved WITHIN the base but not found there -- fall through to the raw-path
      //     fallback below (pre-existing behaviour: a plain 404, not an escape attempt).
      // PT: Resolveu DENTRO da base mas não foi encontrado lá -- cai no fallback de path bruto
      //     abaixo (comportamento pré-existente: um 404 comum, não uma tentativa de escape).
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

  // EN: ASSET-PATH — resolves `base / rel` and proves the result stays within `base` before
  //     letting Open() try to fopen() it. Returns false (out `resolved` left untouched) when:
  //     the base itself cannot be resolved (should not happen for a base_url_ the host already
  //     set successfully, but fail-closed rather than assume); OR the candidate path cannot be
  //     resolved (weakly_canonical() itself does not throw for a nonexistent tail, so this is
  //     effectively unreachable in practice, but the error_code overload is used anyway so a
  //     failure here fails closed instead of throwing an exception across a virtual-call
  //     boundary an RmlUi-embedding host does not expect); OR — the actual traversal check —
  //     the candidate's component sequence does not have the base's component sequence as a
  //     strict prefix. Component-wise comparison (std::filesystem::path::begin()/end(), NOT a
  //     raw string prefix compare) is deliberate: it will not be fooled by a base of
  //     "/srv/assets" matching a candidate of "/srv/assets-evil/x" the way a naive
  //     `candidate.rfind(base, 0) == 0` string check would (that string IS a prefix of this
  //     one, but "assets-evil" is a SIBLING directory, not a descendant of "assets"). See this
  //     class's own top comment (ASSET-PATH) for the symlink-resolution and TOCTOU discussion.
  // PT: ASSET-PATH — resolve `base / rel` e prova que o resultado permanece dentro de `base`
  //     antes de deixar o Open() tentar dar fopen() nele. Retorna false (o `resolved` de saída
  //     fica intocado) quando: a própria base não pode ser resolvida (não deveria acontecer pra
  //     um base_url_ que o host já definiu com sucesso, mas falha fechada em vez de presumir);
  //     OU o path candidato não pode ser resolvido (weakly_canonical() em si não lança pra um
  //     final não-existente, então isto é efetivamente inalcançável na prática, mas a
  //     sobrecarga com error_code é usada mesmo assim pra que uma falha aqui feche em vez de
  //     lançar uma exceção através de uma fronteira de virtual-call que um host embarcando o
  //     RmlUi não espera); OU — a checagem de travessia de fato — a sequência de componentes do
  //     candidato não tem a sequência de componentes da base como prefixo estrito. Comparação
  //     componente-a-componente (std::filesystem::path::begin()/end(), NÃO uma comparação de
  //     prefixo de string crua) é deliberada: não se deixa enganar por uma base "/srv/assets"
  //     casando com um candidato "/srv/assets-evil/x" do jeito que uma checagem ingênua
  //     `candidate.rfind(base, 0) == 0` faria (essa string É prefixo daquela, mas
  //     "assets-evil" é diretório IRMÃO, não descendente de "assets"). Ver o próprio comentário
  //     de topo desta classe (ASSET-PATH) pra discussão de resolução de symlink e TOCTOU.
  static bool resolve_within_base(const std::string& base, const Rml::String& rel,
                                  std::filesystem::path& resolved) {
    namespace fs = std::filesystem;
    std::error_code ec;

    const fs::path base_canon = fs::weakly_canonical(fs::path(base), ec);
    if (ec) return false;

    const fs::path candidate_canon =
        fs::weakly_canonical(fs::path(base) / fs::path(rel.c_str()), ec);
    if (ec) return false;

    auto base_it = base_canon.begin();
    auto cand_it = candidate_canon.begin();
    for (; base_it != base_canon.end(); ++base_it, ++cand_it) {
      if (cand_it == candidate_canon.end() || *cand_it != *base_it)
        return false;
    }

    resolved = candidate_canon;
    return true;
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
