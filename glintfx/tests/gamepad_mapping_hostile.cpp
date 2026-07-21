// SPDX-License-Identifier: MPL-2.0
// EN: Pure unit test for MappingDb::parse_text()'s hostile-input robustness (A2-GAMEPAD,
//     framework-2D) -- no GL, no window, no RmlUi, no Xvfb. Same discipline as
//     i18n_catalog_sanity.cpp/audio_hostile_sanity.cpp/gamepad_decode_hostile.cpp (auditoria-
//     dominó: gamecontrollerdb.txt is exactly the kind of externally-sourced/editable text file
//     this house's "reject the bad input, never abort the host" convention exists for). Every
//     case below must return 0 entries added (or be silently skipped as ONE malformed field
//     inside an otherwise-good line) and MUST NOT crash -- if this binary segfaults/aborts, ctest
//     reports it as a failure on its own, no assertion needed to detect that class of bug.
// PT: Teste unit puro pra robustez a input hostil do MappingDb::parse_text() (A2-GAMEPAD,
//     framework-2D) -- sem GL, sem janela, sem RmlUi, sem Xvfb. Mesma disciplina do
//     i18n_catalog_sanity.cpp/audio_hostile_sanity.cpp/gamepad_decode_hostile.cpp
//     (auditoria-dominó: o gamecontrollerdb.txt é exatamente o tipo de arquivo texto de origem/
//     edição externa pro qual a convenção "rejeita o input ruim, nunca aborta o host" desta casa
//     existe). Todo caso abaixo precisa retornar 0 entradas adicionadas (ou ser silenciosamente
//     pulado como UM campo malformado dentro de uma linha boa no resto) e NÃO PODE crashar -- se
//     este binário segfaultar/abortar, o ctest já reporta isso como falha por conta própria,
//     nenhuma asserção extra é necessária pra detectar essa classe de bug.
// Copyright (c) 2026 Petrus Silva Costa
#include "gamepad_mapping.hpp"

#include <cstdio>
#include <string>

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
  // EN: parse_text(nullptr) -- must never crash, must return 0.
  // PT: parse_text(nullptr) -- nunca pode crashar, precisa retornar 0.
  // ---------------------------------------------------------------------------
  {
    MappingDb db;
    check(db.parse_text(nullptr) == 0, "parse_text(nullptr): returns 0");
  }

  // ---------------------------------------------------------------------------
  // EN: A line with no commas at all -- not the guid,name,... shape.
  // PT: Uma linha sem vírgula nenhuma -- não é a forma guid,nome,...
  // ---------------------------------------------------------------------------
  {
    MappingDb db;
    check(db.parse_text("this-line-has-no-commas-at-all") == 0, "line with no commas: 0 entries added");
  }

  // ---------------------------------------------------------------------------
  // EN: GUID short, odd-length, non-hex -- each rejects the whole line.
  // PT: GUID curto, tamanho ímpar, não-hex -- cada um rejeita a linha inteira.
  // ---------------------------------------------------------------------------
  {
    MappingDb db;
    check(db.parse_text("deadbeef,Too Short,a:b0,platform:Linux,") == 0, "short guid: 0 entries added");
    check(db.parse_text("030000005e0400008e0200001401000,Odd Length,a:b0,platform:Linux,") == 0,
          "odd-length (31 char) guid: 0 entries added");
    check(db.parse_text("0300000zzz0400008e02000014010000,Non Hex,a:b0,platform:Linux,") == 0,
          "non-hex guid: 0 entries added");
  }

  // ---------------------------------------------------------------------------
  // EN: Malformed value specs -- b99999 (huge but syntactically valid index, must not crash
  //     resolve_button() later even though it never will index anything with it here), a-1
  //     (negative index -- rejected, malformed), h9.99 (a hat number this module does not
  //     support, plus a large mask -- both syntactically parseable, so accepted; safety at
  //     resolve time is gamepad_mapping_sanity's/gamepad_decode's job, not the parser's). Each
  //     bad FIELD is skipped; the line itself still adds 1 entry (platform:Linux is present and
  //     the guid is well-formed).
  // PT: Value specs malformados -- b99999 (índice enorme mas sintaticamente válido, não pode
  //     crashar o resolve_button() depois mesmo que nunca vá indexar nada com ele aqui), a-1
  //     (índice negativo -- rejeitado, malformado), h9.99 (um número de hat que este módulo não
  //     suporta, mais uma máscara grande -- ambos sintaticamente parseáveis, então aceitos;
  //     segurança em tempo de resolução é trabalho do gamepad_mapping_sanity/gamepad_decode, não
  //     do parser). Cada CAMPO ruim é pulado; a linha em si ainda adiciona 1 entrada
  //     (platform:Linux está presente e o guid é bem-formado).
  // ---------------------------------------------------------------------------
  {
    MappingDb db;
    const std::string guid = "0123456789abcdef0123456789abcdef";
    const std::string line = guid.substr(0, 32) +
                              ",Weird Pad,a:b99999,b:a-1,x:h9.99,platform:Linux,";
    const int added = db.parse_text(line.c_str());
    check(added == 1, "malformed-value-spec fields inside an otherwise-good line: line still accepted (1 entry)");
    // EN: Reaching here without a crash is the primary assertion for this case.
    // PT: Chegar aqui sem crashar é a asserção primária deste caso.
  }

  // ---------------------------------------------------------------------------
  // EN: A single ~1 MB line (no newline anywhere in the input) -- must not crash, must not hang.
  // PT: Uma única linha de ~1 MB (sem newline em nenhum lugar do input) -- não pode crashar, não
  //     pode travar.
  // ---------------------------------------------------------------------------
  {
    MappingDb   db;
    std::string huge_line = "0123456789abcdef0123456789abcdef,Huge Pad,";
    while (huge_line.size() < 1024 * 1024) {
      huge_line += "unknownfield:garbage,";
    }
    huge_line += "platform:Linux";
    const int added = db.parse_text(huge_line.c_str());
    check(added == 1, "~1 MB single line: still parses (unrecognised fields skipped), 1 entry added");
  }

  // ---------------------------------------------------------------------------
  // EN: Text with NO trailing '\n' at all -- the final line must still be processed.
  // PT: Texto SEM '\n' final nenhum -- a última linha ainda precisa ser processada.
  // ---------------------------------------------------------------------------
  {
    MappingDb         db;
    const std::string no_trailing_newline = "0123456789abcdef0123456789abcdef,No Newline,a:b0,platform:Linux";
    check(db.parse_text(no_trailing_newline.c_str()) == 1, "no trailing newline: last line still processed");
  }

  // ---------------------------------------------------------------------------
  // EN: Multiple lines, some malformed, some not -- malformed lines contribute 0, well-formed
  //     ones still add, and the malformed ones never crash or corrupt the ones around them.
  // PT: Múltiplas linhas, algumas malformadas, outras não -- linhas malformadas contribuem 0, as
  //     bem-formadas ainda adicionam, e as malformadas nunca crasham/corrompem as ao redor.
  // ---------------------------------------------------------------------------
  {
    MappingDb         db;
    const std::string multi =
        "# a comment line, must be ignored\n"
        "not,enough,fields,but,still,no,platform,field\n"
        "\n" // EN: blank line. PT: linha em branco.
        "0123456789abcdef0123456789abcdef,Good Pad 1,a:b0,platform:Linux\n"
        "short,guid,platform:Linux\n"
        "fedcba9876543210fedcba9876543210,Good Pad 2,a:b0,platform:Windows\n" // EN: wrong platform. PT: plataforma errada.
        "0123456789ABCDEF0123456789ABCDEF,Good Pad 1 Uppercase,a:b0,platform:Linux\n";
    const int added = db.parse_text(multi.c_str());
    // EN: "Good Pad 1" (line 4) and its uppercase-GUID re-statement (line 7, same GUID
    //     case-insensitively) both count as ADDED by parse_text()'s own per-line counting (even
    //     though the second is a last-write-wins overwrite of the same key) -- comment/blank/
    //     under-field/short-guid/wrong-platform lines contribute 0 each.
    // PT: "Good Pad 1" (linha 4) e a re-declaração com GUID maiúsculo dela (linha 7, mesmo GUID
    //     sem diferenciar maiúsculas) contam ambas como ADICIONADAS pela própria contagem
    //     por-linha do parse_text() (mesmo a segunda sendo uma sobrescrita
    //     último-escreve-vence da mesma chave) -- linhas de comentário/branco/poucos-campos/
    //     guid-curto/plataforma-errada contribuem 0 cada.
    check(added == 2, "mixed well-formed/malformed lines: exactly the 2 well-formed platform:Linux lines counted");
  }

  if (g_failures > 0) {
    std::fprintf(stderr, "gamepad_mapping_hostile: %d assertion(s) FAILED\n", g_failures);
    return 1;
  }
  std::puts("gamepad_mapping_hostile: PASS");
  return 0;
}
