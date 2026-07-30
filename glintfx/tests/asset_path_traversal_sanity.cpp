// SPDX-License-Identifier: MPL-2.0
// EN: asset_path_traversal_sanity (ASSET-PATH, 2026-07-30) -- contract test for
//     glintfx::BaseUrlFileInterface::Open()'s directory-traversal guard
//     (src/base_url_file_interface.hpp, resolve_within_base()). PURE unit test: constructs the
//     interface directly, no glintfx::App/UiLayer, no GL context, no RmlUi::Initialise() --
//     Open()/Close()/Read() only ever touch stdio + std::filesystem, and Rml::Log::Message()
//     falls back to LogDefault when no SystemInterface is installed (verified against
//     RmlUi's own Log.cpp), so this binary needs nothing beyond linking RmlUi::Core for
//     FileInterface's vtable/out-of-line destructor.
//
//     Sandbox layout (created at CONFIGURE time by tests/CMakeLists.txt, deterministic --
//     avoids a runtime mkdtemp race and keeps the fixture inspectable after a failed run):
//       asset_path_sandbox/
//         base/            <- base_url_ points here
//           safe.txt              "safe-inside-base\n"
//           sub/                  (empty subdirectory, for the sub/../safe.txt legitimate-
//                                   navigation check)
//           link_to_outside       symlink -> ../outside/secret.txt (planted INSIDE base,
//                                  resolves OUTSIDE it -- the symlink case ASSET-PATH's own
//                                  top comment discusses)
//         outside/
//           secret.txt            "SECRET-OUTSIDE-BASE\n" -- must NEVER be reachable via base
//
//     WHAT THIS PROVES: [1] a legitimate relative path resolves and opens with the expected
//     content; [2] a `../` escape to a sibling directory is refused (nullptr); [3] a `../../`
//     escape toward a real absolute system path (/etc/passwd, near-guaranteed present on any
//     Linux CI runner) is refused -- the same shape the reporting consumer named explicitly;
//     [4] a symlink PLANTED inside the base that points outside it is ALSO refused (proves
//     resolve_within_base()'s weakly_canonical()-based comparison follows symlinks, not just
//     lexical `..` segments -- skipped with a printed note, not a hard failure, if the sandbox
//     symlink could not be created at configure time, e.g. no symlink permission on the CI
//     filesystem); [5] `sub/../safe.txt` -- a path that LEXICALLY contains `..` but stays
//     within the base after resolution -- still opens successfully, proving the guard does not
//     over-block ordinary relative navigation.
//
//     WHAT THIS DELIBERATELY DOES NOT PROVE: the TOCTOU residual risk named in
//     base_url_file_interface.hpp's own ASSET-PATH note (a symlink swapped between the
//     weakly_canonical() check and the fopen_capped() call a few lines later) -- exercising a
//     genuine race would require a second thread/process winning a timing window against this
//     test, which would be flaky by construction, not a deterministic contract proof. Declared,
//     not silently skipped, same discipline as gl_proc_address_sanity.cpp's own header comment.
// PT: asset_path_traversal_sanity (ASSET-PATH, 2026-07-30) -- teste de contrato para a guarda
//     de travessia de diretório de glintfx::BaseUrlFileInterface::Open()
//     (src/base_url_file_interface.hpp, resolve_within_base()). Teste unitário PURO: constrói
//     a interface direto, sem glintfx::App/UiLayer, sem contexto GL, sem RmlUi::Initialise()
//     -- Open()/Close()/Read() só tocam stdio + std::filesystem, e Rml::Log::Message() cai pro
//     LogDefault quando nenhum SystemInterface está instalado (verificado contra o próprio
//     Log.cpp do RmlUi), então este binário não precisa de nada além de linkar RmlUi::Core pra
//     vtable/destrutor fora-de-linha do FileInterface.
//
//     Layout do sandbox (criado em tempo de CONFIGURE por tests/CMakeLists.txt, determinístico
//     -- evita corrida de mkdtemp em runtime e mantém o fixture inspecionável após um run que
//     falhou):
//       asset_path_sandbox/
//         base/            <- base_url_ aponta aqui
//           safe.txt              "safe-inside-base\n"
//           sub/                  (subdiretório vazio, pro cheque de navegação legítima
//                                   sub/../safe.txt)
//           link_to_outside       symlink -> ../outside/secret.txt (plantado DENTRO da base,
//                                  resolve pra FORA dela -- o caso de symlink que o próprio
//                                  comentário de topo do ASSET-PATH discute)
//         outside/
//           secret.txt            "SECRET-OUTSIDE-BASE\n" -- nunca deve ser alcançável via base
//
//     O QUE ISTO PROVA: [1] um path relativo legítimo resolve e abre com o conteúdo esperado;
//     [2] um escape `../` pra um diretório irmão é recusado (nullptr); [3] um escape `../../`
//     em direção a um path absoluto real de sistema (/etc/passwd, quase garantidamente
//     presente em qualquer runner de CI Linux) é recusado -- a mesma forma que o consumidor que
//     reportou nomeou explicitamente; [4] um symlink PLANTADO dentro da base que aponta pra
//     fora dela TAMBÉM é recusado (prova que a comparação baseada em weakly_canonical() de
//     resolve_within_base() segue symlink, não só segmentos `..` lexicais -- pulado com nota
//     impressa, não falha dura, se o symlink do sandbox não pôde ser criado em tempo de
//     configure, ex.: sem permissão de symlink no filesystem do CI); [5] `sub/../safe.txt` --
//     um path que LEXICALMENTE contém `..` mas permanece dentro da base após a resolução --
//     ainda abre com sucesso, provando que a guarda não bloqueia em excesso navegação relativa
//     comum.
//
//     O QUE ISTO DELIBERADAMENTE NÃO PROVA: o risco residual de TOCTOU nomeado na própria nota
//     ASSET-PATH de base_url_file_interface.hpp (um symlink trocado entre o cheque de
//     weakly_canonical() e a chamada de fopen_capped() algumas linhas depois) -- exercitar uma
//     corrida genuína exigiria uma segunda thread/processo vencendo uma janela de timing contra
//     este teste, o que seria flaky por construção, não uma prova determinística de contrato.
//     Declarado, não silenciosamente pulado, mesma disciplina do próprio comentário de
//     cabeçalho de gl_proc_address_sanity.cpp.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/base_url_file_interface.hpp"
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace {

// EN: Reads the full contents of an already-open handle back into a string, then Close()s it.
// PT: Lê o conteúdo completo de um handle já aberto de volta pra uma string, depois dá Close().
std::string slurp_and_close(glintfx::BaseUrlFileInterface& fi, Rml::FileHandle h) {
  std::string out;
  char buf[256];
  size_t n;
  while ((n = fi.Read(buf, sizeof(buf), h)) > 0)
    out.append(buf, n);
  fi.Close(h);
  return out;
}

} // namespace

int main() {
  int failures = 0;
  const std::filesystem::path sandbox = "asset_path_sandbox";
  const std::filesystem::path base = sandbox / "base";

  glintfx::BaseUrlFileInterface fi;
  fi.set_base_url(base.string().c_str());

  // ---------------------------------------------------------------------------
  // [1] Legitimate relative path inside the base opens and reads back correctly.
  // ---------------------------------------------------------------------------
  {
    Rml::FileHandle h = fi.Open("safe.txt");
    if (h == 0) {
      std::fprintf(stderr,
                   "asset_path_traversal_sanity FAIL [1]: Open(\"safe.txt\") refused -- a legitimate "
                   "in-base path must succeed\n");
      ++failures;
    } else {
      std::string content = slurp_and_close(fi, h);
      if (content != "safe-inside-base\n") {
        std::fprintf(stderr,
                     "asset_path_traversal_sanity FAIL [1]: safe.txt content mismatch, got \"%s\"\n",
                     content.c_str());
        ++failures;
      } else {
        std::puts("asset_path_traversal_sanity [1] PASS: in-base path opens with correct content");
      }
    }
  }

  // ---------------------------------------------------------------------------
  // [2] "../outside/secret.txt" escapes to a SIBLING directory of base -- must be refused.
  // ---------------------------------------------------------------------------
  {
    Rml::FileHandle h = fi.Open("../outside/secret.txt");
    if (h != 0) {
      std::fprintf(stderr,
                   "asset_path_traversal_sanity FAIL [2]: Open(\"../outside/secret.txt\") SUCCEEDED -- "
                   "directory-traversal guard did not stop a sibling-directory escape\n");
      fi.Close(h);
      ++failures;
    } else {
      std::puts("asset_path_traversal_sanity [2] PASS: sibling-directory escape refused");
    }
  }

  // ---------------------------------------------------------------------------
  // [3] "../../etc/passwd" -- the exact shape reported by the consumer, aimed at a real
  //     absolute system file. Must be refused regardless of whether the target exists.
  // ---------------------------------------------------------------------------
  {
    Rml::FileHandle h = fi.Open("../../etc/passwd");
    if (h != 0) {
      std::fprintf(stderr,
                   "asset_path_traversal_sanity FAIL [3]: Open(\"../../etc/passwd\") SUCCEEDED -- the "
                   "exact traversal shape reported by the consumer was not stopped\n");
      fi.Close(h);
      ++failures;
    } else {
      std::puts("asset_path_traversal_sanity [3] PASS: ../../etc/passwd refused");
    }
  }

  // ---------------------------------------------------------------------------
  // [4] Symlink PLANTED inside base pointing OUTSIDE it -- proves resolve_within_base()
  //     resolves symlinks (weakly_canonical()), not just lexical ".." segments. Skipped, not
  //     failed, if the sandbox symlink is absent (CMake could not create it at configure time).
  // ---------------------------------------------------------------------------
  {
    std::error_code ec;
    const bool link_exists =
        std::filesystem::is_symlink(base / "link_to_outside", ec) && !ec;
    if (!link_exists) {
      std::puts(
          "asset_path_traversal_sanity [4] SKIP: sandbox symlink 'link_to_outside' not present "
          "(could not be created at configure time on this filesystem)");
    } else {
      Rml::FileHandle h = fi.Open("link_to_outside");
      if (h != 0) {
        std::fprintf(stderr,
                     "asset_path_traversal_sanity FAIL [4]: Open(\"link_to_outside\") SUCCEEDED -- a "
                     "symlink planted inside the base that points outside it was not stopped\n");
        fi.Close(h);
        ++failures;
      } else {
        std::puts("asset_path_traversal_sanity [4] PASS: outside-pointing symlink refused");
      }
    }
  }

  // ---------------------------------------------------------------------------
  // [5] "sub/../safe.txt" -- lexically contains "..", but resolves back inside the base.
  //     Must succeed: the guard must not over-block ordinary relative navigation.
  // ---------------------------------------------------------------------------
  {
    Rml::FileHandle h = fi.Open("sub/../safe.txt");
    if (h == 0) {
      std::fprintf(stderr,
                   "asset_path_traversal_sanity FAIL [5]: Open(\"sub/../safe.txt\") refused -- this "
                   "path resolves back INSIDE the base and must be allowed (false positive)\n");
      ++failures;
    } else {
      std::string content = slurp_and_close(fi, h);
      if (content != "safe-inside-base\n") {
        std::fprintf(stderr,
                     "asset_path_traversal_sanity FAIL [5]: sub/../safe.txt content mismatch, got \"%s\"\n",
                     content.c_str());
        ++failures;
      } else {
        std::puts(
            "asset_path_traversal_sanity [5] PASS: in-base '..' navigation not over-blocked");
      }
    }
  }

  if (failures == 0) {
    std::puts("asset_path_traversal_sanity: PASS");
    return 0;
  }
  std::fprintf(stderr, "asset_path_traversal_sanity: %d check(s) FAILED\n", failures);
  return failures;
}
