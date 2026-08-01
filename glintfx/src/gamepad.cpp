// SPDX-License-Identifier: Apache-2.0
// EN: Device layer of A2-GAMEPAD (Agent B, framework-2D, wave 2) -- the ONLY translation unit in
//     this module that ever touches a real file descriptor: directory scan of
//     `GamepadsConfig::dev_dir`, `open()`/`ioctl()`/`read()`/inotify, slot bookkeeping. Everything
//     below sits on top of the two PURE, headless-testable modules Agent A delivered
//     (`gamepad_decode.hpp`'s `GamepadDecoder`, `gamepad_mapping.hpp`'s GUID/layout/DB/
//     default-kernel-spec) -- this file never reimplements decode or mapping logic, it only
//     supplies them with real bytes and real capability bitmaps. See glintfx/gamepad.hpp's own
//     header comment for the full public lifecycle/RAII/slots/permissions/hotplug/fail-high
//     contract this file implements; comments here focus on the "how".
//
//     SYNTHETIC-EVENT RESYNC (D8 of the plan): `GamepadDecoder` (Agent A's header) exposes no
//     "set this key/axis state directly" setter -- by design, its ONLY input is `feed_bytes()`,
//     fed real wire-format bytes (dependency-injection-by-construction, see gamepad_decode.hpp's
//     own comment). Rather than asking Agent A to grow a second, device-layer-only entry point,
//     this file builds tiny SYNTHETIC 24-byte `input_event` wire buffers from `EVIOCGKEY`/
//     `EVIOCGABS` ioctl results and feeds THOSE through the exact same `feed_bytes()` path a real
//     `read()` would use -- the 16/18/20 byte offsets used here are not a peek into
//     `GamepadDecoder`'s private implementation, they are the actual, kernel-defined
//     `struct input_event` wire layout on x86-64 (documented in gamepad_decode.hpp's own header
//     comment, and independently derivable from the kernel UAPI struct itself) -- both files
//     arrive at the same 16/18/20 from the same public ABI fact, neither leans on the other's
//     internals. This buys resync (D8) and "seed already-held buttons at connect time" (a
//     connect-time enhancement beyond the plan's literal SYN_DROPPED-only wording, but the exact
//     same code path, see `resync_pad_state()` below) for free, with zero new API surface on the
//     decode core.
//
//     CAPABILITY BITMAPS: `EVIOCGBIT` fills a bitmap of `unsigned long`s (kernel convention, not a
//     type this file invents) -- `nbits()`/`bit_is_set()` below are the same small,
//     publicly-documented bit-twiddling idiom every evdev-reading tool (jstest, evtest, SDL's own
//     Linux joystick backend) reimplements locally, since `<linux/input.h>` deliberately does not
//     ship an `NBITS`/`test_bit` helper of its own for userspace.
// PT: Camada de device do A2-GAMEPAD (Agente B, framework-2D, onda 2) -- a ÚNICA unidade de
//     tradução deste módulo que de fato toca um descritor de arquivo real: varredura de
//     diretório de `GamepadsConfig::dev_dir`, `open()`/`ioctl()`/`read()`/inotify, contabilidade
//     de slot. Tudo abaixo fica em cima dos dois módulos PUROS, testáveis headless, que o Agente
//     A entregou (`GamepadDecoder` do `gamepad_decode.hpp`, GUID/layout/DB/default-kernel-spec do
//     `gamepad_mapping.hpp`) -- este arquivo nunca reimplementa lógica de decode ou mapeamento,
//     só fornece a eles bytes reais e bitmaps de capacidade reais. Ver o próprio comentário de
//     cabeçalho do glintfx/gamepad.hpp pro contrato público completo de ciclo-de-vida/RAII/
//     slots/permissões/hotplug/fail-high que este arquivo implementa; os comentários aqui focam
//     no "como".
//
//     RESYNC POR EVENTO SINTÉTICO (D8 do plano): `GamepadDecoder` (header do Agente A) não expõe
//     nenhum setter de "define este estado de tecla/eixo diretamente" -- por design, sua ÚNICA
//     entrada é `feed_bytes()`, alimentada com bytes reais em formato de fio
//     (injeção-de-dependência-por-construção, ver o próprio comentário do gamepad_decode.hpp). Em
//     vez de pedir pro Agente A crescer um segundo ponto de entrada só-da-camada-de-device, este
//     arquivo constrói pequenos buffers SINTÉTICOS de 24 bytes em formato de fio `input_event` a
//     partir de resultados de ioctl `EVIOCGKEY`/`EVIOCGABS` e alimenta ESSES pelo EXATO mesmo
//     caminho `feed_bytes()` que um `read()` real usaria -- os offsets de byte 16/18/20 usados
//     aqui não são um espiar na implementação privada do `GamepadDecoder`, são o layout de fio
//     `struct input_event` de verdade, definido pelo kernel, em x86-64 (documentado no próprio
//     comentário de cabeçalho do gamepad_decode.hpp, e derivável independentemente da própria
//     struct UAPI do kernel) -- os dois arquivos chegam nos mesmos 16/18/20 a partir do mesmo fato
//     de ABI pública, nenhum se apoia nos internos do outro. Isso compra resync (D8) e "semear
//     botões já segurados em tempo de conexão" (um aprimoramento em tempo de conexão além da
//     redação literal do plano, que só fala de SYN_DROPPED, mas é o EXATO mesmo caminho de
//     código, ver `resync_pad_state()` abaixo) de graça, com zero superfície de API nova no
//     núcleo de decode.
//
//     BITMAPS DE CAPACIDADE: `EVIOCGBIT` preenche um bitmap de `unsigned long`s (convenção do
//     kernel, não um tipo que este arquivo inventa) -- `nbits()`/`bit_is_set()` abaixo são o mesmo
//     idioma pequeno e publicamente documentado de manipulação de bit que toda ferramenta leitora
//     de evdev (jstest, evtest, o próprio backend de joystick Linux do SDL) reimplementa
//     localmente, já que `<linux/input.h>` deliberadamente não traz um helper `NBITS`/`test_bit`
//     próprio pra userspace.
// Copyright (c) 2026 Petrus Silva Costa
#include <glintfx/gamepad.hpp>

#include "gamepad_decode.hpp"
#include "gamepad_mapping.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <string>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/input.h>

// EN: The embedded, vendored Linux subset of gamecontrollerdb.txt (D6 of the plan, Agent C) --
//     gated at RUNTIME by GamepadsConfig::use_builtin_db (below), but always compiled in whenever
//     this whole translation unit is (i.e. whenever GLINTFX_MODULE_GAMEPAD=ON): the include
//     directory for this file is only added to the glintfx_gamepad target
//     (CMakeLists.txt, PRIVATE third_party/gamecontrollerdb), so module OFF removes this array and
//     its symbol from the archive entirely, same as every other module-gated data blob.
// PT: O subconjunto Linux vendorizado embutido do gamecontrollerdb.txt (D6 do plano, Agente C) --
//     gateado em RUNTIME por GamepadsConfig::use_builtin_db (abaixo), mas sempre compilado
//     sempre que esta unidade de tradução inteira está (ou seja, sempre que
//     GLINTFX_MODULE_GAMEPAD=ON): o diretório de include deste arquivo só é acrescentado ao
//     alvo glintfx_gamepad (CMakeLists.txt, PRIVATE third_party/gamecontrollerdb), então módulo
//     OFF remove este array e o símbolo dele do archive por completo, igual a todo outro blob de
//     dado gateado por módulo.
#include "gamecontrollerdb_linux.inc"

namespace glintfx {

namespace {

// ---------------------------------------------------------------------------
// EN: Bitmap helpers -- see this file's header comment.
// PT: Auxiliares de bitmap -- ver o comentário de cabeçalho deste arquivo.
// ---------------------------------------------------------------------------

constexpr std::size_t kBitsPerLong = sizeof(unsigned long) * 8;

constexpr std::size_t nbits(int max_bit) {
  return static_cast<std::size_t>(max_bit) / kBitsPerLong + 1;
}

bool bit_is_set(const unsigned long* bits, int bit) {
  const std::size_t b = static_cast<std::size_t>(bit);
  return ((bits[b / kBitsPerLong] >> (b % kBitsPerLong)) & 1UL) != 0UL;
}

constexpr std::size_t kKeyBitsSize = static_cast<std::size_t>(KEY_MAX) + 1;
constexpr std::size_t kAbsBitsSize = static_cast<std::size_t>(ABS_MAX) + 1;

// EN: Builds a synthetic 24-byte input_event wire buffer -- see this file's header comment for
//     why the fixed 16/18/20 offsets are safe to replicate here (kernel ABI fact, not a peek into
//     GamepadDecoder's implementation). The leading 16 bytes (timeval) are left zeroed -- the
//     decode core never reads them.
// PT: Constrói um buffer sintético de 24 bytes em formato de fio input_event -- ver o comentário
//     de cabeçalho deste arquivo pro porquê dos offsets fixos 16/18/20 serem seguros de replicar
//     aqui (fato de ABI do kernel, não um espiar na implementação do GamepadDecoder). Os 16 bytes
//     iniciais (timeval) ficam zerados -- o núcleo de decode nunca os lê.
std::array<unsigned char, 24> make_synthetic_event(std::uint16_t type, std::uint16_t code, std::int32_t value) {
  std::array<unsigned char, 24> buf{};
  std::memcpy(buf.data() + 16, &type, sizeof(type));
  std::memcpy(buf.data() + 18, &code, sizeof(code));
  std::memcpy(buf.data() + 20, &value, sizeof(value));
  return buf;
}

// EN: GAMEPAD-NOTHROW (W22, 2026-07-30) -- byte-count cap, checked INCREMENTALLY (block-by-
//     block), not via a single `ss << f.rdbuf()` slurp (the pre-fix shape). MEASURED, not
//     theoretical: `read_whole_file("/dev/zero")` under the pre-fix `rdbuf()` slurp grew the
//     destination `std::string` without bound (a character device has no EOF, so the stream
//     extraction never stops on its own) until `std::bad_alloc` escaped uncaught all the way to
//     `std::terminate()`/`abort()` -- reproduced under `ulimit -v` to keep the process' own
//     memory footprint bounded during the repro, not the cause of the crash itself; the SAME
//     class of failure is reachable with any sufficiently large or infinite-appearing path
//     (a legitimately huge file, a sparse file, another zero-like special device), no fault
//     injection needed, contradicting `gamepad.hpp`'s own documented `load_mappings_file()`
//     contract ("Never crashes on a hostile/binary-garbage file"). A post-hoc size check via
//     `seek`/`tell` would NOT have caught this: `/dev/zero` and other character devices are not
//     seekable in the way a regular file is (no reliable `st_size`), so the only guard that
//     actually bounds an EOF-less stream is one applied WHILE reading, not before or after.
//     `kMaxMappingsFileBytes` reuses the SAME 256 MiB VALUE `BaseUrlFileInterface::kMaxFileBytes`
//     (AUD-L1-PARSE, `base_url_file_interface.hpp`) already established as this codebase's own
//     generic per-asset-file ceiling -- declared locally here (not shared/`#include`d), same
//     "reuse the VALUE, not the symbol" discipline `CAPTURE-NOTHROW`'s own `kMaxCaptureBytes`
//     already used for `kMaxImageDecodeBytes`. The community `gamecontrollerdb.txt` this format
//     mirrors is on the order of a few hundred KiB to a few MiB in the wild -- 256 MiB is
//     generous headroom over any plausible legitimate file, not a tight fit.
// PT: GAMEPAD-NOTHROW (W22, 2026-07-30) -- teto de contagem de bytes, checado
//     INCREMENTALMENTE (bloco a bloco), não via um único slurp `ss << f.rdbuf()` (a forma
//     pré-conserto). MEDIDO, não teórico: `read_whole_file("/dev/zero")` sob o slurp `rdbuf()`
//     pré-conserto crescia a `std::string` de destino sem limite (um device de caractere não
//     tem EOF, então a extração do stream nunca para sozinha) até o `std::bad_alloc` escapar
//     sem captura até `std::terminate()`/`abort()` -- reproduzido sob `ulimit -v` só pra manter
//     a própria pegada de memória do processo limitada durante a reprodução, não a causa do
//     crash em si; a MESMA classe de falha é alcançável com qualquer path suficientemente
//     grande ou aparentemente infinito (um arquivo legitimamente enorme, um arquivo esparso,
//     outro device especial parecido com zero), sem injeção de falha nenhuma necessária,
//     contradizendo o próprio contrato documentado de `load_mappings_file()` em `gamepad.hpp`
//     ("Never crashes on a hostile/binary-garbage file"). Uma checagem de tamanho a posteriori
//     via `seek`/`tell` NÃO teria pego isto: `/dev/zero` e outros devices de caractere não são
//     seekable do jeito que um arquivo comum é (sem `st_size` confiável), então a única guarda
//     que de fato limita um stream sem EOF é uma aplicada DURANTE a leitura, não antes nem
//     depois. `kMaxMappingsFileBytes` reusa o MESMO VALOR de 256 MiB que
//     `BaseUrlFileInterface::kMaxFileBytes` (AUD-L1-PARSE, `base_url_file_interface.hpp`) já
//     estabeleceu como o próprio teto genérico por-arquivo-de-asset deste código-base --
//     declarado localmente aqui (não compartilhado/`#include`do), mesma disciplina "reusa o
//     VALOR, não o símbolo" que o próprio `kMaxCaptureBytes` do `CAPTURE-NOTHROW` já usou pro
//     `kMaxImageDecodeBytes`. O `gamecontrollerdb.txt` da comunidade que este formato espelha
//     fica na ordem de algumas centenas de KiB a poucos MiB no mundo real -- 256 MiB é folga
//     generosa sobre qualquer arquivo legítimo plausível, não um ajuste apertado.
constexpr std::size_t kMaxMappingsFileBytes = 256u * 1024u * 1024u; // 256 MiB.

// EN: Reads the whole file into memory, binary-safe (embedded NUL bytes survive the read, though
//     MappingDb::parse_text()'s own std::string(text) construction from the returned c_str()
//     later stops at the first one -- documented truncation, never a crash). Returns an empty
//     string on any open/read failure, OR once the running byte count exceeds
//     `kMaxMappingsFileBytes` (see this function's own top comment) -- indistinguishable from
//     "empty file" either way, which is fine: all cases legitimately parse to 0 entries.
// PT: Lê o arquivo inteiro pra memória, binary-safe (bytes NUL embutidos sobrevivem à leitura,
//     embora a própria construção std::string(text) do MappingDb::parse_text() a partir do
//     c_str() retornado depois pare no primeiro -- truncamento documentado, nunca um crash).
//     Retorna uma string vazia em qualquer falha de open/read, OU assim que a contagem de bytes
//     corrente ultrapassar `kMaxMappingsFileBytes` (ver o próprio comentário de topo desta
//     função) -- indistinguível de "arquivo vazio" de qualquer jeito, o que é aceitável: todos
//     os casos legitimamente fazem parse pra 0 entradas.
std::string read_whole_file(const char* path) {
  std::ifstream f(path, std::ios::binary);
  if (!f.is_open()) {
    return {};
  }
  std::string content;
  char buf[65536];
  std::size_t total = 0;
  while (f) {
    f.read(buf, sizeof(buf));
    const std::streamsize n = f.gcount();
    if (n <= 0) break;
    total += static_cast<std::size_t>(n);
    if (total > kMaxMappingsFileBytes) {
      return {}; // EN: hostile/unbounded stream -- reject, never accumulate past the cap.
                 // PT: stream hostil/sem limite -- rejeita, nunca acumula além do teto.
    }
    content.append(buf, static_cast<std::size_t>(n));
  }
  return content;
}

} // namespace

struct Gamepads::Impl {
  struct Pad {
    bool           connected = false;
    int            fd        = -1;
    std::string    dev_name; // EN: bare "eventN" filename, for hotplug IN_DELETE/dedup matching. PT: nome de arquivo nu "eventN", pro casamento de IN_DELETE/dedup do hotplug.
    std::string    name;     // EVIOCGNAME
    std::string    guid;     // build_guid()
    GamepadDecoder decoder;
    JoystickLayout layout;
    GamepadMapping mapping;
    std::array<bool, kKeyBitsSize> key_bits{}; // EN: capability bitmap at scan time. PT: bitmap de capacidade em tempo de varredura.
    std::array<bool, kAbsBitsSize> abs_bits{};
  };

  bool        initialized = false;
  std::string dev_dir     = "/dev/input";
  bool        hotplug     = true;
  int         inotify_fd  = -1;

  MappingDb db;

  std::array<Pad, static_cast<std::size_t>(Gamepads::kMaxPads)> pads{};

  std::function<void(int, bool)> connection_cb;

  // EN: Device I/O methods -- MEMBERS of Impl (not free functions taking `Gamepads::Impl&`):
  //     `Impl` is a PRIVATE nested type of `Gamepads` (same as `Audio::Impl`), so a free function
  //     outside the class cannot even NAME the type `Gamepads::Impl` in its own signature
  //     (unlike defining the class body itself out-of-line, which this file also does below --
  //     providing a member's definition is not an access-controlled "use" of the member, but
  //     independently naming that member's type in an unrelated function's signature is). Making
  //     these ordinary member functions of `Impl` sidesteps the issue entirely: `Gamepads::`'s own
  //     methods (which DO have access to their own private nested `Impl` type) call them via
  //     `impl_->methodName(...)`.
  // PT: Métodos de I/O de device -- MEMBROS de Impl (não funções livres recebendo
  //     `Gamepads::Impl&`): `Impl` é um tipo aninhado PRIVADO de `Gamepads` (igual ao
  //     `Audio::Impl`), então uma função livre fora da classe nem consegue NOMEAR o tipo
  //     `Gamepads::Impl` na própria assinatura (diferente de definir o corpo da classe fora de
  //     linha, que este arquivo também faz abaixo -- fornecer a definição de um membro não é um
  //     "uso" controlado por acesso, mas nomear o tipo desse membro independentemente na
  //     assinatura de uma função não relacionada é). Tornar estes métodos comuns de `Impl`
  //     contorna o problema por completo: os próprios métodos de `Gamepads::` (que TÊM acesso ao
  //     próprio tipo aninhado privado `Impl`) os chamam via `impl_->methodName(...)`.
  void invoke_connection_cb(int pad, bool is_connected);
  static void resync_pad_state(Pad& pad);
  void disconnect_pad(int index);
  bool try_register(const std::string& filename);
  void drain_pad_fd(int index);
  void drain_inotify();
};

void Gamepads::Impl::invoke_connection_cb(int pad, bool is_connected) {
  if (!connection_cb) {
    return;
  }
  // EN: Copy-before-invoke (AUD-TEC-3) -- see gamepad.hpp's header comment.
  // PT: Cópia-antes-de-invocar (AUD-TEC-3) -- ver o comentário de cabeçalho do gamepad.hpp.
  auto cb = connection_cb;
  cb(pad, is_connected);
}

// EN: Re-reads a pad's COMPLETE key/abs state via EVIOCGKEY/EVIOCGABS and feeds it into the
//     decoder as synthetic events (D8 of the plan) -- called both right after a pad is newly
//     registered (seeds any button already held down at connect time -- no future EV_KEY event
//     would otherwise ever report an already-pressed button) and whenever the decode core flags
//     SYN_DROPPED (poll()'s resync path, drain_pad_fd() below). Only codes `pad`'s OWN capability
//     bitmap (`key_bits`/`abs_bits`, captured at scan time) actually reports are ever queried or
//     fed -- never a code this device does not have.
// PT: Relê o estado completo de tecla/abs de um pad via EVIOCGKEY/EVIOCGABS e alimenta isso no
//     decoder como eventos sintéticos (D8 do plano) -- chamado tanto logo depois de um pad ser
//     recém-registrado (semeia qualquer botão já segurado em tempo de conexão -- nenhum evento
//     EV_KEY futuro reportaria um botão já pressionado de outra forma) quanto sempre que o
//     núcleo de decode sinaliza SYN_DROPPED (caminho de resync do poll(), drain_pad_fd() abaixo).
//     Só códigos que o PRÓPRIO bitmap de capacidade de `pad` (`key_bits`/`abs_bits`, capturado em
//     tempo de varredura) de fato reporta são consultados ou alimentados -- nunca um código que
//     este device não tem.
void Gamepads::Impl::resync_pad_state(Pad& pad) {
  std::vector<unsigned long> key_state_raw(nbits(KEY_MAX), 0UL);
  if (::ioctl(pad.fd, EVIOCGKEY(static_cast<int>(sizeof(unsigned long) * key_state_raw.size())),
              key_state_raw.data()) >= 0) {
    for (int code = 0; code <= KEY_MAX; ++code) {
      if (!pad.key_bits[static_cast<std::size_t>(code)]) {
        continue;
      }
      const bool pressed = bit_is_set(key_state_raw.data(), code);
      const auto ev       = make_synthetic_event(EV_KEY, static_cast<std::uint16_t>(code), pressed ? 1 : 0);
      pad.decoder.feed_bytes(ev.data(), ev.size());
    }
  }
  // EN: ioctl failure here: leave key state as it was (whatever feed_bytes() already applied
  //     from real events) -- never a crash, never a guess.
  // PT: falha de ioctl aqui: deixa o estado de tecla como estava (o que quer que o feed_bytes()
  //     já tenha aplicado de eventos reais) -- nunca um crash, nunca um chute.

  for (int code = 0; code <= ABS_MAX; ++code) {
    if (!pad.abs_bits[static_cast<std::size_t>(code)]) {
      continue;
    }
    input_absinfo absinfo{};
    if (::ioctl(pad.fd, EVIOCGABS(code), &absinfo) >= 0) {
      pad.decoder.configure_abs(static_cast<std::uint16_t>(code),
                                 EvAbsInfo{absinfo.value, absinfo.minimum, absinfo.maximum, absinfo.fuzz,
                                           absinfo.flat, absinfo.resolution});
    }
  }
}

void Gamepads::Impl::disconnect_pad(int index) {
  Pad& pad = pads[static_cast<std::size_t>(index)];
  if (!pad.connected) {
    return;
  }
  ::close(pad.fd);
  pad = Pad{};
  invoke_connection_cb(index, false);
}

// EN: Opens `filename` (a bare "eventN" name inside impl.dev_dir), runs the D7 capability check,
//     and -- if it passes -- registers a new connected pad in the LOWEST free slot (D4). Returns
//     `false` (no-op, never a crash) when: `filename` is already a connected pad's dev_name
//     (dedupes a redundant IN_CREATE+IN_ATTRIB pair for the same node), no slot is free,
//     `open()`/`ioctl()` fails for ANY reason (permission denied, not an evdev node at all, no
//     BTN_JOYSTICK..BTN_DIGI capability). See section 2.5 of the A2-GAMEPAD plan for the exact
//     scan order this mirrors.
// PT: Abre `filename` (um nome nu "eventN" dentro de impl.dev_dir), roda a checagem de
//     capacidade D7, e -- se passar -- registra um pad novo conectado no MENOR slot livre (D4).
//     Retorna `false` (no-op, nunca um crash) quando: `filename` já é o dev_name de um pad
//     conectado (deduplica um par redundante IN_CREATE+IN_ATTRIB pro mesmo node), nenhum slot
//     está livre, `open()`/`ioctl()` falha por QUALQUER motivo (permissão negada, nem de longe um
//     node evdev, sem capacidade BTN_JOYSTICK..BTN_DIGI). Ver a seção 2.5 do plano A2-GAMEPAD pra
//     ordem exata de varredura que isto espelha.
bool Gamepads::Impl::try_register(const std::string& filename) {
  const bool already_tracked = std::any_of(pads.begin(), pads.end(), [&filename](const Pad& pad) {
    return pad.connected && pad.dev_name == filename;
  });
  if (already_tracked) {
    return false; // EN: already tracked -- no-op. PT: já rastreado -- no-op.
  }

  const std::string path = dev_dir + "/" + filename;
  const int         fd   = ::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
  if (fd < 0) {
    return false; // EN: EACCES/EPERM/ENOENT/... -- graceful skip, no root required, no crash.
                  // PT: EACCES/EPERM/ENOENT/... -- pula gracioso, sem exigir root, sem crash.
  }

  std::vector<unsigned long> ev_bits(nbits(EV_MAX), 0UL);
  if (::ioctl(fd, EVIOCGBIT(0, static_cast<int>(sizeof(unsigned long) * ev_bits.size())), ev_bits.data()) < 0) {
    ::close(fd);
    return false; // EN: ENOTTY -- not an evdev node (regular file/garbage). PT: ENOTTY -- não é um node evdev (arquivo regular/lixo).
  }
  if (!bit_is_set(ev_bits.data(), EV_ABS)) {
    ::close(fd);
    return false; // EN: no absolute axes at all -- not a gamepad. PT: nenhum eixo absoluto -- não é um gamepad.
  }

  std::vector<unsigned long> key_bits_raw(nbits(KEY_MAX), 0UL);
  if (::ioctl(fd, EVIOCGBIT(EV_KEY, static_cast<int>(sizeof(unsigned long) * key_bits_raw.size())),
              key_bits_raw.data()) < 0) {
    ::close(fd);
    return false;
  }
  bool has_joystick_button = false;
  for (int code = BTN_JOYSTICK; code < BTN_DIGI; ++code) {
    if (bit_is_set(key_bits_raw.data(), code)) {
      has_joystick_button = true;
      break;
    }
  }
  if (!has_joystick_button) {
    ::close(fd);
    return false; // EN: D7 -- never grabs a keyboard/mouse. PT: D7 -- nunca sequestra teclado/mouse.
  }

  std::vector<unsigned long> abs_bits_raw(nbits(ABS_MAX), 0UL);
  // EN: EV_ABS presence already confirmed above on this same fd -- a failure here would be
  //     exceptional; an all-zero abs_bits_raw degrades to "no axes resolved", never a crash.
  // PT: A presença de EV_ABS já foi confirmada acima neste mesmo fd -- uma falha aqui seria
  //     excepcional; um abs_bits_raw todo zero degrada pra "nenhum eixo resolvido", nunca um
  //     crash.
  (void)::ioctl(fd, EVIOCGBIT(EV_ABS, static_cast<int>(sizeof(unsigned long) * abs_bits_raw.size())),
                abs_bits_raw.data());

  int slot = -1;
  for (int i = 0; i < Gamepads::kMaxPads; ++i) {
    if (!pads[static_cast<std::size_t>(i)].connected) {
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    ::close(fd);
    return false; // EN: D4 -- all kMaxPads slots occupied. PT: D4 -- todos os kMaxPads slots ocupados.
  }

  Pad& pad     = pads[static_cast<std::size_t>(slot)];
  pad          = Pad{};
  pad.fd       = fd;
  pad.dev_name = filename;

  input_id id{};
  (void)::ioctl(fd, EVIOCGID, &id); // EN: failure leaves id all-zero -- build_guid() still produces a deterministic (if generic) GUID. PT: falha deixa id todo zero -- build_guid() ainda produz um GUID determinístico (se genérico).

  char name_buf[256] = {};
  if (::ioctl(fd, EVIOCGNAME(sizeof(name_buf) - 1), name_buf) < 0) {
    name_buf[0] = '\0';
  }
  name_buf[sizeof(name_buf) - 1] = '\0'; // EN: hard guard regardless of what the ioctl wrote. PT: guarda dura independente do que o ioctl escreveu.
  pad.name                       = name_buf;

  for (int code = 0; code <= KEY_MAX; ++code) {
    pad.key_bits[static_cast<std::size_t>(code)] = bit_is_set(key_bits_raw.data(), code);
  }
  for (int code = 0; code <= ABS_MAX; ++code) {
    pad.abs_bits[static_cast<std::size_t>(code)] = bit_is_set(abs_bits_raw.data(), code);
  }

  pad.guid   = build_guid(id, pad.name);
  pad.layout = derive_layout(pad.key_bits, pad.abs_bits);

  if (const GamepadMapping* found = db.find(pad.guid); found != nullptr) {
    pad.mapping = *found;
  } else {
    const bool has_gas   = pad.abs_bits[static_cast<std::size_t>(ABS_GAS)];
    const bool has_brake = pad.abs_bits[static_cast<std::size_t>(ABS_BRAKE)];
    pad.mapping          = default_kernel_spec_mapping(has_gas, has_brake);
  }

  resync_pad_state(pad); // EN: seeds abs ranges/values + any already-held button. PT: semeia faixas/valores de abs + qualquer botão já segurado.

  pad.connected = true;
  invoke_connection_cb(slot, true);
  return true;
}

// EN: Drains pad `index`'s fd (read() in a loop until EAGAIN), feeding every chunk into the
//     decoder; disconnects the pad on EOF/ENODEV/EBADF/EIO; resyncs (D8) if the decoder flagged
//     SYN_DROPPED while draining.
// PT: Drena o fd do pad `index` (read() em laço até EAGAIN), alimentando todo chunk no decoder;
//     desconecta o pad em EOF/ENODEV/EBADF/EIO; ressincroniza (D8) se o decoder sinalizou
//     SYN_DROPPED durante a drenagem.
void Gamepads::Impl::drain_pad_fd(int index) {
  Pad&          pad = pads[static_cast<std::size_t>(index)];
  unsigned char buf[24 * 32];
  while (true) {
    const ssize_t n = ::read(pad.fd, buf, sizeof(buf));
    if (n > 0) {
      pad.decoder.feed_bytes(buf, static_cast<std::size_t>(n));
      continue; // EN: keep draining until EAGAIN. PT: continua drenando até EAGAIN.
    }
    if (n == 0) {
      disconnect_pad(index); // EN: EOF -- device node closed/removed. PT: EOF -- node de device fechado/removido.
      return;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      break; // EN: fully drained this frame. PT: totalmente drenado neste frame.
    }
    if (errno == ENODEV || errno == EBADF || errno == EIO) {
      disconnect_pad(index);
      return;
    }
    break; // EN: any other unexpected errno -- stop reading this pad this frame, never spin/crash.
           // PT: qualquer outro errno inesperado -- para de ler este pad neste frame, nunca gira/crasha.
  }

  if (pad.connected && pad.decoder.sync_lost()) {
    resync_pad_state(pad);
    pad.decoder.clear_sync_lost();
  }
}

// EN: Drains the inotify fd (if any) and reacts to IN_CREATE/IN_ATTRIB (attempt to
//     register/re-register the node -- see try_register()'s own dedup check) and IN_DELETE
//     (disconnect the matching connected pad, if any). No-op when hotplug is off or the watch
//     failed to set up at init() time.
// PT: Drena o fd de inotify (se houver) e reage a IN_CREATE/IN_ATTRIB (tenta registrar/
//     re-registrar o node -- ver a própria checagem de dedup do try_register()) e IN_DELETE
//     (desconecta o pad conectado correspondente, se houver). No-op quando o hotplug está
//     desligado ou o watch falhou ao montar em tempo de init().
void Gamepads::Impl::drain_inotify() {
  if (inotify_fd < 0) {
    return;
  }
  alignas(struct inotify_event) unsigned char buf[4096];
  while (true) {
    const ssize_t n = ::read(inotify_fd, buf, sizeof(buf));
    if (n <= 0) {
      break; // EN: EAGAIN (drained) or an error -- nothing more to react to now.
             // PT: EAGAIN (drenado) ou um erro -- nada mais pra reagir agora.
    }
    std::size_t offset = 0;
    while (offset + sizeof(struct inotify_event) <= static_cast<std::size_t>(n)) {
      const auto* ev = reinterpret_cast<const struct inotify_event*>(buf + offset);
      if (ev->len > 0) {
        const std::string filename(ev->name);
        if (filename.rfind("event", 0) == 0) {
          if ((ev->mask & (IN_CREATE | IN_ATTRIB)) != 0U) {
            try_register(filename);
          }
          if ((ev->mask & IN_DELETE) != 0U) {
            for (int i = 0; i < Gamepads::kMaxPads; ++i) {
              if (pads[static_cast<std::size_t>(i)].connected &&
                  pads[static_cast<std::size_t>(i)].dev_name == filename) {
                disconnect_pad(i);
                break;
              }
            }
          }
        }
      }
      offset += sizeof(struct inotify_event) + ev->len;
    }
  }
}

// ---------------------------------------------------------------------------
// EN: Gamepads -- public facade. See glintfx/gamepad.hpp for the full contract.
// PT: Gamepads -- fachada pública. Ver glintfx/gamepad.hpp pro contrato completo.
// ---------------------------------------------------------------------------

Gamepads::Gamepads() : impl_(std::make_unique<Impl>()) {}

Gamepads::~Gamepads() {
  shutdown();
}

bool Gamepads::init(const GamepadsConfig& cfg) {
  if (impl_->initialized) {
    return false; // EN: already initialized -- call shutdown() first. PT: já inicializado -- chame shutdown() primeiro.
  }

  impl_->dev_dir = (cfg.dev_dir != nullptr) ? cfg.dev_dir : "/dev/input";
  impl_->hotplug = cfg.hotplug;

  if (cfg.use_builtin_db) {
    impl_->db.parse_text(glintfx_gamecontrollerdb_linux);
  }

  if (impl_->hotplug) {
    const int ino_fd = ::inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (ino_fd >= 0) {
      const int watch =
          ::inotify_add_watch(ino_fd, impl_->dev_dir.c_str(), IN_CREATE | IN_ATTRIB | IN_DELETE);
      if (watch >= 0) {
        impl_->inotify_fd = ino_fd;
      } else {
        ::close(ino_fd); // EN: watch failed (e.g. dev_dir absent) -- degrade gracefully, init() still succeeds.
                          // PT: watch falhou (ex.: dev_dir ausente) -- degrada gracioso, init() ainda tem sucesso.
      }
    }
    // EN: inotify_init1() failure: degrade gracefully too (documented in gamepad.hpp).
    // PT: falha do inotify_init1(): degrada gracioso também (documentado no gamepad.hpp).
  }

  DIR* dir = ::opendir(impl_->dev_dir.c_str());
  if (dir != nullptr) {
    const struct dirent* entry = nullptr;
    while ((entry = ::readdir(dir)) != nullptr) {
      const std::string filename = entry->d_name;
      if (filename.rfind("event", 0) == 0) {
        impl_->try_register(filename);
      }
    }
    ::closedir(dir);
  }
  // EN: opendir() failure (e.g. dev_dir does not exist): 0 pads found, not an init() failure
  //     (documented in gamepad.hpp).
  // PT: falha do opendir() (ex.: dev_dir não existe): 0 pads encontrados, não é uma falha de
  //     init() (documentado no gamepad.hpp).

  impl_->initialized = true;
  return true;
}

void Gamepads::shutdown() {
  if (!impl_->initialized) {
    return; // EN: idempotent no-op. PT: no-op idempotente.
  }
  for (auto& pad : impl_->pads) {
    if (pad.connected) {
      ::close(pad.fd);
    }
    pad = Impl::Pad{};
  }
  if (impl_->inotify_fd >= 0) {
    ::close(impl_->inotify_fd);
    impl_->inotify_fd = -1;
  }
  impl_->initialized = false;
}

bool Gamepads::is_initialized() const {
  return impl_->initialized;
}

void Gamepads::poll() {
  if (!impl_->initialized) {
    return;
  }
  for (int i = 0; i < kMaxPads; ++i) {
    if (impl_->pads[static_cast<std::size_t>(i)].connected) {
      impl_->drain_pad_fd(i);
    }
  }
  if (impl_->hotplug) {
    impl_->drain_inotify();
  }
}

int Gamepads::connected_count() const {
  return static_cast<int>(
      std::count_if(impl_->pads.begin(), impl_->pads.end(), [](const Impl::Pad& pad) { return pad.connected; }));
}

bool Gamepads::connected(int pad) const {
  if (pad < 0 || pad >= kMaxPads) {
    return false;
  }
  return impl_->pads[static_cast<std::size_t>(pad)].connected;
}

const char* Gamepads::name(int pad) const {
  if (pad < 0 || pad >= kMaxPads || !impl_->pads[static_cast<std::size_t>(pad)].connected) {
    return "";
  }
  return impl_->pads[static_cast<std::size_t>(pad)].name.c_str();
}

const char* Gamepads::guid(int pad) const {
  if (pad < 0 || pad >= kMaxPads || !impl_->pads[static_cast<std::size_t>(pad)].connected) {
    return "";
  }
  return impl_->pads[static_cast<std::size_t>(pad)].guid.c_str();
}

bool Gamepads::button(int pad, GamepadButton b) const {
  if (pad < 0 || pad >= kMaxPads || !impl_->pads[static_cast<std::size_t>(pad)].connected) {
    return false;
  }
  const Impl::Pad& p = impl_->pads[static_cast<std::size_t>(pad)];
  return resolve_button(p.mapping, p.layout, p.decoder, b);
}

float Gamepads::axis(int pad, GamepadAxis a) const {
  if (pad < 0 || pad >= kMaxPads || !impl_->pads[static_cast<std::size_t>(pad)].connected) {
    return 0.0f;
  }
  const Impl::Pad& p = impl_->pads[static_cast<std::size_t>(pad)];
  return resolve_axis(p.mapping, p.layout, p.decoder, a);
}

int Gamepads::raw_button_count(int pad) const {
  if (pad < 0 || pad >= kMaxPads || !impl_->pads[static_cast<std::size_t>(pad)].connected) {
    return 0;
  }
  return static_cast<int>(impl_->pads[static_cast<std::size_t>(pad)].layout.button_codes.size());
}

bool Gamepads::raw_button(int pad, int index) const {
  if (pad < 0 || pad >= kMaxPads || !impl_->pads[static_cast<std::size_t>(pad)].connected) {
    return false;
  }
  const Impl::Pad& p = impl_->pads[static_cast<std::size_t>(pad)];
  if (index < 0 || static_cast<std::size_t>(index) >= p.layout.button_codes.size()) {
    return false;
  }
  return p.decoder.key_state(p.layout.button_codes[static_cast<std::size_t>(index)]);
}

std::uint16_t Gamepads::raw_button_code(int pad, int index) const {
  if (pad < 0 || pad >= kMaxPads || !impl_->pads[static_cast<std::size_t>(pad)].connected) {
    return 0;
  }
  const Impl::Pad& p = impl_->pads[static_cast<std::size_t>(pad)];
  if (index < 0 || static_cast<std::size_t>(index) >= p.layout.button_codes.size()) {
    return 0;
  }
  return p.layout.button_codes[static_cast<std::size_t>(index)];
}

int Gamepads::raw_axis_count(int pad) const {
  if (pad < 0 || pad >= kMaxPads || !impl_->pads[static_cast<std::size_t>(pad)].connected) {
    return 0;
  }
  return static_cast<int>(impl_->pads[static_cast<std::size_t>(pad)].layout.axis_codes.size());
}

float Gamepads::raw_axis(int pad, int index) const {
  if (pad < 0 || pad >= kMaxPads || !impl_->pads[static_cast<std::size_t>(pad)].connected) {
    return 0.0f;
  }
  const Impl::Pad& p = impl_->pads[static_cast<std::size_t>(pad)];
  if (index < 0 || static_cast<std::size_t>(index) >= p.layout.axis_codes.size()) {
    return 0.0f;
  }
  return p.decoder.abs_normalized(p.layout.axis_codes[static_cast<std::size_t>(index)]);
}

std::int32_t Gamepads::raw_axis_value(int pad, int index) const {
  if (pad < 0 || pad >= kMaxPads || !impl_->pads[static_cast<std::size_t>(pad)].connected) {
    return 0;
  }
  const Impl::Pad& p = impl_->pads[static_cast<std::size_t>(pad)];
  if (index < 0 || static_cast<std::size_t>(index) >= p.layout.axis_codes.size()) {
    return 0;
  }
  return p.decoder.abs_value(p.layout.axis_codes[static_cast<std::size_t>(index)]);
}

std::uint16_t Gamepads::raw_axis_code(int pad, int index) const {
  if (pad < 0 || pad >= kMaxPads || !impl_->pads[static_cast<std::size_t>(pad)].connected) {
    return 0;
  }
  const Impl::Pad& p = impl_->pads[static_cast<std::size_t>(pad)];
  if (index < 0 || static_cast<std::size_t>(index) >= p.layout.axis_codes.size()) {
    return 0;
  }
  return p.layout.axis_codes[static_cast<std::size_t>(index)];
}

// EN: GAMEPAD-NOTHROW (W22, 2026-07-30) -- ONE function-wide try/catch, not several call-scoped
//     ones (same rationale FONT-NOTHROW's own load_font() states, draw2d.cpp): both
//     `read_whole_file()`'s own `content.append()` growth (even under `kMaxMappingsFileBytes`,
//     a machine already low on memory can still fail an allocation well below the cap) and
//     `MappingDb::parse_text()`'s own `std::string full(text)` COPY of the entire (up to 256
//     MiB) content plus its own per-line `substr()`/`trimmed` copies and `by_guid_[guid] =
//     mapping` map insertion (gamepad_mapping.cpp) can throw `std::bad_alloc` -- AUDITED, not
//     assumed: neither this file's own parser helpers nor gamepad_mapping.cpp's own
//     `parse_line()`/`parse_value_spec()` use `std::stoi`/`.at()`/anything else that could throw
//     a DIFFERENT exception type; every numeric field is parsed via `std::from_chars` (never
//     throws, reports failure through its own return value), so `std::bad_alloc` is the ONLY
//     exception class reachable on this call path -- `catch (const std::exception&)` therefore
//     covers the real risk, `catch (...)` is belt-and-suspenders for the same reason every other
//     `never a crash` sibling in this codebase keeps it (a future third-party dependency this
//     seam might grow). `MappingDb::parse_text()`'s own body is left BYTE-FOR-BYTE UNTOUCHED
//     (`load_mappings_text()`'s own direct call to it, below, is unaffected -- it receives
//     caller-owned memory already in hand, no read/cap concern of its own, per this fatia's own
//     scope decision) -- only the CALL to it from inside this function's own try is what changes.
// PT: GAMEPAD-NOTHROW (W22, 2026-07-30) -- UM try/catch do tamanho da função, não vários
//     escopados por chamada (mesmo racional que o próprio load_font() do FONT-NOTHROW afirma,
//     draw2d.cpp): tanto o próprio crescimento `content.append()` de `read_whole_file()` (mesmo
//     sob `kMaxMappingsFileBytes`, uma máquina já com pouca memória ainda pode falhar uma
//     alocação bem abaixo do teto) quanto a própria cópia `std::string full(text)` do conteúdo
//     inteiro (até 256 MiB) de `MappingDb::parse_text()`, mais as próprias cópias `substr()`/
//     `trimmed` por-linha e a inserção de mapa `by_guid_[guid] = mapping` (gamepad_mapping.cpp)
//     podem lançar `std::bad_alloc` -- AUDITADO, não presumido: nem os próprios helpers de
//     parser deste arquivo nem o próprio `parse_line()`/`parse_value_spec()` de
//     gamepad_mapping.cpp usam `std::stoi`/`.at()`/qualquer outra coisa que pudesse lançar um
//     tipo de exceção DIFERENTE -- todo campo numérico é parseado via `std::from_chars` (nunca
//     lança, reporta falha pelo próprio valor de retorno), então `std::bad_alloc` é a ÚNICA
//     classe de exceção alcançável neste caminho de chamada -- `catch (const std::exception&)`
//     portanto cobre o risco real, `catch (...)` é cinto-e-suspensório pelo mesmo motivo que
//     todo outro irmão `never a crash` deste código-base mantém (uma dependência de terceiro
//     futura que esta costura possa ganhar). O próprio corpo do `MappingDb::parse_text()` fica
//     BYTE-A-BYTE INTOCADO (a própria chamada direta do `load_mappings_text()` a ele, abaixo,
//     fica sem efeito -- ele recebe memória de posse do chamador já em mãos, sem preocupação de
//     leitura/teto própria, pela própria decisão de escopo desta fatia) -- só a CHAMADA a ele de
//     dentro do próprio try desta função é o que muda.
int Gamepads::load_mappings_file(const char* path) noexcept {
  if (!impl_->initialized || path == nullptr) {
    return 0;
  }
  try {
    const std::string content = read_whole_file(path);
    if (content.empty()) {
      return 0; // EN: unreadable/nonexistent/empty file -- 0, never a crash. PT: arquivo ilegível/inexistente/vazio -- 0, nunca um crash.
    }
    return impl_->db.parse_text(content.c_str());
  } catch (const std::exception&) {
    // EN: std::bad_alloc (the expected case, see this function's own top comment) or any other
    //     std::exception -- degrade to 0, the SAME clean signal every other rejection above
    //     already returns.
    // PT: std::bad_alloc (o caso esperado, ver o próprio comentário de topo desta função) ou
    //     qualquer outro std::exception -- degrada pra 0, o MESMO sinal limpo que toda outra
    //     rejeição acima já retorna.
    return 0;
  } catch (...) {
    return 0;
  }
}

int Gamepads::load_mappings_text(const char* text) {
  if (!impl_->initialized) {
    return 0;
  }
  return impl_->db.parse_text(text); // EN: parse_text() itself returns 0 on nullptr. PT: o próprio parse_text() retorna 0 em nullptr.
}

void Gamepads::set_connection_callback(std::function<void(int, bool)> cb) {
  impl_->connection_cb = std::move(cb);
}

} // namespace glintfx
