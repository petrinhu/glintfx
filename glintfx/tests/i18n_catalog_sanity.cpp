// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for glintfx::I18n's catalog parser (i18n-1, framework-2D) -- no GL, no
//     window, no RmlUi, no Xvfb (see the "why outside GLFW/Xvfb" comment in
//     tests/CMakeLists.txt). Exercises: a well-formed multi-locale/multi-line catalog load;
//     robustness against malformed input (comments, blank lines, a key=value line with no '='
//     at all, a key=value line seen before any [locale] header, an empty key, an unterminated
//     "[" section header) -- every one of these must be SKIPPED and COUNTED, never a crash or an
//     aborted load; last-write-wins on a key repeated within the same locale; escape handling
//     (`\\`, `\n`, `\t`); and the nullptr/missing-path hardening on both load_catalog_*
//     overloads (mirrors the project's "input validation em borda" convention, e.g.
//     input_hardening_sanity.cpp for the UI-layer equivalent).
// PT: Teste unit puro para o parser de catálogo do glintfx::I18n (i18n-1, framework-2D) -- sem
//     GL, sem janela, sem RmlUi, sem Xvfb (ver o comentário "por que fora do GLFW/Xvfb" em
//     tests/CMakeLists.txt). Exercita: carga de catálogo multi-locale/multi-linha bem-formado;
//     robustez contra input malformado (comentários, linhas em branco, linha key=value sem '='
//     nenhum, linha key=value vista antes de qualquer cabeçalho [locale], chave vazia,
//     cabeçalho de seção "[" não-terminado) -- cada um DEVE ser pulado e CONTADO, nunca um
//     crash ou uma carga abortada; último-escreve-vence numa chave repetida dentro do mesmo
//     locale; tratamento de escape (`\\`, `\n`, `\t`); e o hardening de nullptr/caminho ausente
//     nas duas sobrecargas de load_catalog_* (espelha a convenção "input validation em borda"
//     do projeto, ex.: input_hardening_sanity.cpp pro equivalente da camada UI).
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/i18n.hpp>

#include <cstdio>
#include <filesystem>

namespace {
int g_failures = 0;

void check(bool cond, const char* what) {
  if (!cond) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    ++g_failures;
  }
}

} // namespace

using namespace glintfx;

int main() {
  // ---------------------------------------------------------------------------
  // EN: Well-formed load -- two locales, plain keys, a pluralized pair, comments and blank
  //     lines interspersed. entries_loaded must count every accepted key=value pair (6 total:
  //     3 per locale), lines_skipped must be exactly 0.
  // PT: Carga bem-formada -- dois locales, chaves simples, um par pluralizado, comentários e
  //     linhas em branco intercaladas. entries_loaded precisa contar todo par key=value aceito
  //     (6 no total: 3 por locale), lines_skipped precisa ser exatamente 0.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    const char* catalog =
        "# HUD strings.\n"
        "[en-US]\n"
        "\n"
        "hud.title = My Game\n"
        "hud.lives.one = # life\n"
        "hud.lives.other = # lives\n"
        "\n"
        "# pt-BR section below.\n"
        "[pt-BR]\n"
        "hud.title = Meu Jogo\n"
        "hud.lives.one = # vida\n"
        "hud.lives.other = # vidas\n";
    I18nLoadResult r = i18n.load_catalog_string(catalog);
    check(r.ok, "well-formed catalog: ok == true");
    check(r.entries_loaded == 6, "well-formed catalog: entries_loaded == 6");
    check(r.lines_skipped == 0, "well-formed catalog: lines_skipped == 0");

    check(i18n.set_locale("en-US"), "set_locale(\"en-US\") returns true");
    check(i18n.tr("hud.title") == "My Game", "tr(\"hud.title\") resolves en-US value");
    check(i18n.set_locale("pt-BR"), "set_locale(\"pt-BR\") returns true");
    check(i18n.tr("hud.title") == "Meu Jogo", "tr(\"hud.title\") resolves pt-BR value");
  }

  // ---------------------------------------------------------------------------
  // EN: Malformed-line robustness -- every line below is individually broken in a different
  //     way; the load must complete (ok == true), skip every broken line, and accept only the
  //     two well-formed key=value lines.
  // PT: Robustez a linha malformada -- toda linha abaixo é quebrada de um jeito diferente; a
  //     carga precisa completar (ok == true), pular toda linha quebrada, e aceitar só as duas
  //     linhas key=value bem-formadas.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    const char* hostile =
        "before.any.section = orphaned\n"  // EN: no [locale] open yet -- skip. PT: nenhum [locale] aberto -- pula.
        "[\n"                              // EN: unterminated section header -- skip. PT: cabeçalho de seção não-terminado -- pula.
        "[en-US]\n"
        "no equals sign here\n"            // EN: no '=' -- skip. PT: sem '=' -- pula.
        " = value with empty key\n"        // EN: empty key after trim -- skip. PT: chave vazia após trim -- pula.
        "greeting = Hello\n"               // EN: well-formed. PT: bem-formada.
        "[]\n"                             // EN: empty locale name -- skip, section stays "en-US". PT: nome de locale vazio -- pula, seção continua "en-US".
        "farewell = Bye\n";                // EN: well-formed, still under en-US (the [] above must not have switched sections).
                                            // PT: bem-formada, ainda sob en-US ([] acima não pode ter trocado de seção).
    I18nLoadResult r = i18n.load_catalog_string(hostile);
    check(r.ok, "hostile catalog: load completes (ok == true), never aborts");
    check(r.entries_loaded == 2, "hostile catalog: entries_loaded == 2 (greeting + farewell)");
    // EN: 5 skipped lines: "before.any.section = orphaned" (no [locale] open), "[" (unterminated
    //     header), "no equals sign here" (no '='), " = value with empty key" (empty key after
    //     trim), and "[]" (empty locale name).
    // PT: 5 linhas puladas: "before.any.section = orphaned" (sem [locale] aberto), "[" (cabeçalho
    //     não-terminado), "no equals sign here" (sem '='), " = value with empty key" (chave
    //     vazia após trim), e "[]" (nome de locale vazio).
    check(r.lines_skipped == 5, "hostile catalog: lines_skipped == 5");

    i18n.set_locale("en-US");
    check(i18n.tr("greeting") == "Hello", "hostile catalog: greeting still resolves");
    check(i18n.tr("farewell") == "Bye", "hostile catalog: farewell resolves under the SAME section (empty [] header did not switch it)");
  }

  // ---------------------------------------------------------------------------
  // EN: Last-write-wins on a repeated key within the same locale, in the SAME load call.
  // PT: Último-escreve-vence numa chave repetida dentro do mesmo locale, na MESMA chamada de load.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    I18nLoadResult r = i18n.load_catalog_string(
        "[en]\n"
        "k = first\n"
        "k = second\n");
    check(r.entries_loaded == 2, "repeated key: both writes counted (entries_loaded == 2)");
    i18n.set_locale("en");
    check(i18n.tr("k") == "second", "repeated key: last write wins");
  }

  // ---------------------------------------------------------------------------
  // EN: A second load_catalog_string() call merges into the existing catalog (base-then-
  //     override pattern documented in i18n.hpp) rather than replacing it.
  // PT: Uma segunda chamada a load_catalog_string() se funde no catálogo existente (padrão
  //     base-depois-override documentado em i18n.hpp) em vez de substituí-lo.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    i18n.load_catalog_string("[en]\na = A\nb = B\n");
    i18n.load_catalog_string("[en]\nb = B2\n"); // EN: overrides "b", leaves "a" untouched.
    i18n.set_locale("en");
    check(i18n.tr("a") == "A", "merge: earlier key 'a' survives a later, unrelated load");
    check(i18n.tr("b") == "B2", "merge: later load overrides 'b'");
  }

  // ---------------------------------------------------------------------------
  // EN: Escape handling -- `\\`, `\n`, `\t` are the only recognised escapes; an unknown escape
  //     (`\z`) is copied through literally (both characters), never a parse error.
  // PT: Tratamento de escape -- `\\`, `\n`, `\t` são os únicos escapes reconhecidos; um escape
  //     desconhecido (`\z`) é copiado literalmente (os dois caracteres), nunca erro de parse.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    i18n.load_catalog_string("[en]\nk = line1\\nline2\\ttabbed\\\\backslash and \\z unknown\n");
    i18n.set_locale("en");
    check(i18n.tr("k") == "line1\nline2\ttabbed\\backslash and \\z unknown", "escape sequences decode correctly, unknown escape passed through literally");
  }

  // ---------------------------------------------------------------------------
  // EN: nullptr/missing-path hardening -- neither overload may crash, and both must report
  //     ok == false without mutating any previously loaded catalog state.
  // PT: Hardening de nullptr/caminho ausente -- nenhuma das duas sobrecargas pode crashar, e as
  //     duas precisam reportar ok == false sem mutar nenhum estado de catálogo já carregado.
  // ---------------------------------------------------------------------------
  {
    I18n i18n;
    i18n.load_catalog_string("[en]\nk = kept\n");

    I18nLoadResult r1 = i18n.load_catalog_string(nullptr);
    check(!r1.ok, "load_catalog_string(nullptr): ok == false");
    check(r1.entries_loaded == 0, "load_catalog_string(nullptr): entries_loaded == 0");

    I18nLoadResult r2 = i18n.load_catalog_file(nullptr);
    check(!r2.ok, "load_catalog_file(nullptr): ok == false");

    I18nLoadResult r3 = i18n.load_catalog_file("/definitely/does/not/exist/glintfx-i18n-test.cat");
    check(!r3.ok, "load_catalog_file(missing path): ok == false");

    i18n.set_locale("en");
    check(i18n.tr("k") == "kept", "prior catalog state untouched by the three failed loads above");
  }

  // ---------------------------------------------------------------------------
  // EN: Directory-as-path hardening (review adversarial finding, i18n-1) -- opening a directory
  //     with std::ifstream "succeeds" at is_open() on Linux (the open() syscall accepts a
  //     directory fd for reading), so is_open() alone cannot tell a directory apart from a real
  //     file; without the peek() probe in load_catalog_file(), this used to return {.ok = true,
  //     .entries_loaded = 0}, indistinguishable from a legitimately empty catalog file. Uses a
  //     freshly created temp directory (not a hardcoded system path like "/tmp", which may not
  //     exist/be readable on every CI runner) so this test is self-contained; cleaned up
  //     regardless of pass/fail via the RAII guard below.
  // PT: Hardening de diretório-como-caminho (achado do review adversarial, i18n-1) -- abrir um
  //     diretório com std::ifstream "funciona" no is_open() no Linux (a syscall open() aceita um
  //     fd de diretório pra leitura), então is_open() sozinho não distingue um diretório de um
  //     arquivo de verdade; sem a sondagem peek() em load_catalog_file(), isso retornava
  //     {.ok = true, .entries_loaded = 0}, indistinguível de um catálogo legitimamente vazio.
  //     Usa um diretório temporário recém-criado (não um caminho de sistema fixo tipo "/tmp", que
  //     pode não existir/ser legível em todo runner de CI) pra este teste ser autocontido;
  //     limpo independente de pass/fail via o guard RAII abaixo.
  // ---------------------------------------------------------------------------
  {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "glintfx-i18n-catalog-sanity-dir-test";
    std::error_code ec;
    fs::remove_all(dir, ec); // EN: clean slate from a possible prior crashed run. PT: estado limpo de uma execução anterior que possa ter crashado.
    fs::create_directory(dir, ec);
    check(!ec, "test setup: temp directory created");

    struct DirGuard {
      fs::path path;
      ~DirGuard() {
        std::error_code cleanup_ec;
        fs::remove_all(path, cleanup_ec);
      }
    } dir_guard{dir};

    I18n i18n;
    i18n.load_catalog_string("[en]\nk = kept\n");
    // EN: dir.string().c_str() (not dir.c_str()) -- std::filesystem::path::value_type is
    //     platform-dependent: `char` on POSIX (Linux), `wchar_t` on Windows, so dir.c_str()
    //     returns a `const wchar_t*` on MSVC, which does not implicitly convert to the
    //     `const char*` load_catalog_file() expects (MSVC error C2664, caught live by the
    //     win-atoms CI job -- .string() is the portable narrow-string conversion on every
    //     platform).
    // PT: dir.string().c_str() (não dir.c_str()) -- std::filesystem::path::value_type depende
    //     de plataforma: `char` no POSIX (Linux), `wchar_t` no Windows, então dir.c_str()
    //     devolve um `const wchar_t*` no MSVC, que não converte implicitamente pro
    //     `const char*` que load_catalog_file() espera (erro MSVC C2664, capturado ao vivo
    //     pelo job de CI win-atoms -- .string() é a conversão portável pra string estreita em
    //     toda plataforma).
    I18nLoadResult r = i18n.load_catalog_file(dir.string().c_str());
    check(!r.ok, "load_catalog_file(directory): ok == false, not confused with an empty file");
    check(r.entries_loaded == 0, "load_catalog_file(directory): entries_loaded == 0");
    i18n.set_locale("en");
    check(i18n.tr("k") == "kept", "prior catalog state untouched by the failed directory load");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "i18n_catalog_sanity: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("i18n_catalog_sanity: PASS");
  return 0;
}
