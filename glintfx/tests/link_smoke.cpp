// SPDX-License-Identifier: MPL-2.0
// EN: glintfx_link_smoke -- the executable QW-MSVCLINK/PROG-1 adds specifically to close a CI
//     gap: glintfx/CMakeLists.txt's `windows msvc build` job (.github/workflows/ci.yml) only
//     ever built the `glintfx` STATIC target directly (`cmake --build ... --target glintfx`).
//     A static-library archiver (lib.exe/ar) only packs object files -- it never resolves
//     symbols or PRIVATE usage requirements (like a bare `m` library name) against anything,
//     so a Unix-only dependency mistakenly linked into one of glintfx's OBJECT-library modules
//     can sail through that job green while still breaking every REAL consumer the moment they
//     link an executable (this is exactly how the `m`/`m.lib` bug shipped unnoticed until the
//     GusWorld consumer's own Windows CI caught it in practice -- see the platform-guard
//     comment next to glintfx_audio's target_link_libraries() call in ../CMakeLists.txt).
//
//     This file is deliberately NOT part of the normal GLINTFX_BUILD_TESTS suite (tests/
//     CMakeLists.txt, gated by GLINTFX_BUILD_TESTS + enable_testing()) -- that suite assumes a
//     GLFW/Xvfb-capable environment for most of its targets, exactly what the `windows msvc
//     build` job deliberately does NOT install (GLINTFX_BACKEND_GLFW=OFF there, see that job's
//     own header comment in ci.yml). Instead it is gated by its own option,
//     GLINTFX_BUILD_LINK_SMOKE (glintfx/CMakeLists.txt, declared right after the
//     glintfx::glintfx alias), so it can be turned on in that job WITHOUT dragging in GLFW,
//     Xvfb, or the rest of the ~50-target test suite -- keeping the job fast, which is the
//     whole reason a full `add_subdirectory(tests)` was not chosen here instead.
//
//     Deliberately exercises the public API of the two modules most exposed to this exact
//     class of bug (`core`-only, no GL/window/RmlUi dependency, so they build under this job's
//     degraded GLFW=OFF configuration): glintfx::Audio (the module the m.lib regression was
//     actually found in) and glintfx::I18n (the same "core ONLY" independence class per
//     ADR-0015 (b), broadening the linked-symbol surface for near-to-zero extra cost). Uses
//     AudioConfig::null_backend=true -- exactly like the audio_*_sanity tests -- so this needs
//     no real sound device (headless CI, on Windows too). Every call here is expected to
//     SUCCEED or fail SAFELY (per audio.hpp/i18n.hpp's own no-crash contracts); this is a link
//     smoke, not a behavioural test, so no assertions are made on return values -- exit code 0
//     is the only signal this executable's CI step checks (compiling AND linking successfully
//     is the entire point).
// PT: glintfx_link_smoke -- o executável que a QW-MSVCLINK/PROG-1 acrescenta especificamente
//     para fechar uma lacuna de CI: o job `windows msvc build` do glintfx/CMakeLists.txt
//     (.github/workflows/ci.yml) sempre buildou só o alvo STATIC `glintfx` diretamente
//     (`cmake --build ... --target glintfx`). Um arquivador de biblioteca estática (lib.exe/ar)
//     só empacota arquivos-objeto -- nunca resolve símbolo nem usage requirement PRIVATE (como
//     um nome de biblioteca cru `m`) contra nada, então uma dependência Unix-only linkada por
//     engano num dos módulos OBJECT-library da glintfx pode passar por esse job verde e ainda
//     assim quebrar todo consumidor DE VERDADE no momento em que ele linka um executável -- foi
//     exatamente assim que o bug do `m`/`m.lib` foi lançado sem ser notado até o próprio CI
//     Windows do consumidor GusWorld pegá-lo na prática (ver o comentário de guard de
//     plataforma ao lado da chamada target_link_libraries() do glintfx_audio em
//     ../CMakeLists.txt).
//
//     Este arquivo deliberadamente NÃO faz parte da suíte normal GLINTFX_BUILD_TESTS
//     (tests/CMakeLists.txt, gateada por GLINTFX_BUILD_TESTS + enable_testing()) -- aquela
//     suíte assume um ambiente capaz de GLFW/Xvfb pra maioria dos seus alvos, exatamente o que
//     o job `windows msvc build` deliberadamente NÃO instala (GLINTFX_BACKEND_GLFW=OFF lá, ver
//     o próprio comentário de cabeçalho daquele job no ci.yml). Em vez disso é gateado pela
//     própria option, GLINTFX_BUILD_LINK_SMOKE (glintfx/CMakeLists.txt, declarada logo após o
//     alias glintfx::glintfx), então pode ser ligado naquele job SEM arrastar GLFW, Xvfb, nem o
//     resto da suíte de ~50 alvos -- mantendo o job rápido, que é o motivo inteiro de um
//     `add_subdirectory(tests)` completo não ter sido escolhido aqui.
//
//     Exercita deliberadamente a API pública dos dois módulos mais expostos a esta exata
//     classe de bug (só `core`, zero dependência de GL/janela/RmlUi, então buildam sob a
//     configuração degradada GLFW=OFF deste job): glintfx::Audio (o módulo onde a regressão do
//     m.lib foi de fato encontrada) e glintfx::I18n (mesma classe de independência "só core"
//     pela ADR-0015 (b), ampliando a superfície de símbolo linkado a custo extra
//     quase-zero). Usa AudioConfig::null_backend=true -- exatamente como os testes
//     audio_*_sanity -- então não precisa de dispositivo de som real (CI headless, também no
//     Windows). Toda chamada aqui deve TER SUCESSO ou falhar DE FORMA SEGURA (pelos próprios
//     contratos sem-crash de audio.hpp/i18n.hpp); isto é um link smoke, não um teste
//     comportamental, então nenhuma assertiva é feita sobre valor de retorno -- código de saída
//     0 é o único sinal que o passo de CI deste executável verifica (compilar E linkar com
//     sucesso é o ponto inteiro).

#include <glintfx/audio.hpp>
#include <glintfx/i18n.hpp>

int main() {
  glintfx::Audio audio;
  glintfx::AudioConfig cfg;
  cfg.null_backend = true;
  audio.init(cfg);
  audio.is_initialized();
  audio.shutdown();

  glintfx::I18n i18n;
  i18n.load_catalog_string("[en-US]\nhello = Hello\n");
  i18n.set_locale("en-US");
  i18n.tr("hello");

  return 0;
}
