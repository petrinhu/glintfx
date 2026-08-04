// SPDX-License-Identifier: Apache-2.0
// EN: TETO-DEDUP (W26, 2026-08-04) -- the shared VALUE behind glintfx's "hard ceiling on any
//     untrusted byte stream read into memory" policy. Born from a precision-of-language finding
//     (TETO-CALIBRE, W25): a CHANGELOG entry described gamepad.cpp's old 256 MiB cap as "reuse of
//     the VALUE, not the SYMBOL" of BaseUrlFileInterface::kMaxFileBytes -- a distinction that,
//     followed to its conclusion, revealed FIVE independently-declared "256 MiB" literals across
//     the codebase (frame_capture.cpp, image_decode.hpp, render_gl3.cpp,
//     base_url_file_interface.hpp, and gamepad.cpp before TETO-CALIBRE right-sized it to 1 MiB).
//     Five equal numbers were a COINCIDENCE waiting to be found out as either policy or drift; this
//     header answers that question: it IS policy, and this constant is where it now lives.
//
//     WHAT THIS CONSTANT MEANS: a default hard ceiling, in bytes, for any single untrusted
//     byte-stream a glintfx call site reads/allocates in one shot BEFORE it has been validated --
//     an asset file, a decoded-image buffer, a framebuffer capture, a config/mapping file. It
//     exists to turn an unbounded-allocation DoS (hostile input, corrupt file, a symlink/FUSE mount
//     lying about its own size) into a graceful, logged rejection instead of `bad_alloc`/OOM
//     crossing a library boundary into the host application. 256 MiB was derived, not invented:
//     generous for any single UI asset (an 8192x8192 RGBA8 texture, already past what most GPUs'
//     own `GL_MAX_TEXTURE_SIZE` allows, is 256 MiB raw) or an 8K UHD framebuffer capture (~126.6
//     MiB, ~2x headroom) that a UI or a frame-capture call plausibly needs, while staying far below
//     what pressures a typical host's memory budget.
//
//     WHAT DEDUP DOES AND DOES NOT CHANGE: the four call sites that reference this constant
//     (frame_capture.cpp's `kMaxCaptureBytes`, image_decode.hpp's `kMaxImageDecodeBytes`,
//     render_gl3.cpp's `kMaxAssetFileBytes`, base_url_file_interface.hpp's `kMaxFileBytes`) keep
//     their own domain-named local constant -- readability at each call site is preserved, and so
//     is every EXISTING belt-and-suspenders CHECKPOINT (multiple independent call sites still each
//     enforce their own guard, at their own point in the pipeline; a host that bypasses one check
//     -- e.g. replacing Rml::SetFileInterface() after Bootstrap::load() runs -- still hits the
//     others). What changes is that each of those four local constants is now INITIALISED FROM
//     this single named symbol instead of independently retyping the literal `256u * 1024u *
//     1024u` -- a future policy change (raising or lowering the DEFAULT ceiling) is one edit here,
//     verified at compile time everywhere a call site's own `static_assert` checks its local
//     constant against this one, instead of four textually-identical-by-coincidence literals that
//     could silently drift apart one at a time.
//
//     DIVERGENCE IS ALLOWED, BUT MUST BE CALIBRATED AND WRITTEN DOWN: this default is NOT a floor
//     every domain must share forever. `gamepad.cpp`'s `kMaxMappingsFileBytes` diverges from it ON
//     PURPOSE (TETO-CALIBRE, W25, 2026-08-04) -- 1 MiB, derived from the REAL vendored
//     `gamecontrollerdb_linux.txt` artifact (200 882 bytes, ~5.2x headroom), not from this default,
//     because a gamepad-mappings file has a real, known, much smaller upper bound than "any UI
//     asset in general". The lesson TETO-CALIBRE names is the one this header exists to prevent
//     recurring: a value inherited from another domain (or a shared default like this one) carries
//     the CALIBRATION of the domain it was derived for, not necessarily the domain it lands in --
//     if a future domain's real ceiling is provably smaller (or larger) than this default, the fix
//     is to derive ITS OWN constant from ITS OWN real artifact/traffic, exactly as `gamepad.cpp`
//     did, and to WRITE the derivation in the comment next to it (not just cite this header as
//     precedent for "why 256 MiB" -- the same trap TETO-CALIBRE's own postmortem calls out: "de
//     onde veio o número" answers a different question than "por que este número é o certo AQUI").
// PT: TETO-DEDUP (W26, 2026-08-04) -- o VALOR compartilhado por trás da política glintfx de "teto
//     rígido sobre qualquer stream de bytes não confiável lido pra memória". Nasceu de um achado de
//     precisão de linguagem (TETO-CALIBRE, W25): uma entrada de CHANGELOG descreveu o antigo teto
//     de 256 MiB de gamepad.cpp como "reuso do VALOR, não do SÍMBOLO" de
//     BaseUrlFileInterface::kMaxFileBytes -- uma distinção que, levada até o fim, revelou CINCO
//     literais "256 MiB" declarados independentemente pela base de código (frame_capture.cpp,
//     image_decode.hpp, render_gl3.cpp, base_url_file_interface.hpp, e gamepad.cpp antes do
//     TETO-CALIBRE recalibrá-lo pra 1 MiB). Cinco números iguais eram uma COINCIDÊNCIA esperando
//     pra ser revelada como política ou como deriva; este header responde essa pergunta: É
//     política, e esta constante é onde ela agora mora.
//
//     O QUE ESTA CONSTANTE SIGNIFICA: um teto rígido default, em bytes, pra qualquer stream de
//     bytes não confiável que um ponto de chamada da glintfx lê/aloca de uma vez ANTES de ter sido
//     validado -- um arquivo de asset, um buffer de imagem decodificada, uma captura de
//     framebuffer, um arquivo de config/mapeamento. Existe pra transformar um DoS de alocação sem
//     teto (input hostil, arquivo corrompido, um symlink/mount FUSE mentindo sobre o próprio
//     tamanho) numa recusa graciosa e logada em vez de um `bad_alloc`/OOM cruzando a fronteira da
//     lib até a aplicação host. 256 MiB foi derivado, não inventado: generoso pra qualquer asset
//     único de UI (uma textura RGBA8 8192x8192, já além do que a maioria das GPUs permite via
//     `GL_MAX_TEXTURE_SIZE`, são 256 MiB brutos) ou pra uma captura de framebuffer 8K UHD (~126,6
//     MiB, ~2x de folga) que uma UI ou uma chamada de captura de frame realisticamente precisa,
//     ficando ainda bem abaixo do que pressionaria o orçamento de memória de um host típico.
//
//     O QUE O DEDUP MUDA E O QUE NÃO MUDA: os quatro pontos de chamada que referenciam esta
//     constante (`kMaxCaptureBytes` de frame_capture.cpp, `kMaxImageDecodeBytes` de
//     image_decode.hpp, `kMaxAssetFileBytes` de render_gl3.cpp, `kMaxFileBytes` de
//     base_url_file_interface.hpp) mantêm a própria constante local com nome de domínio --
//     legibilidade em cada ponto de chamada é preservada, e também é preservado todo CHECKPOINT
//     cinto-e-suspensório EXISTENTE (múltiplos pontos de chamada independentes continuam cada um
//     aplicando a própria guarda, no próprio ponto do pipeline; um host que contorna uma checagem
//     -- ex.: substituindo Rml::SetFileInterface() depois que Bootstrap::load() roda -- ainda
//     esbarra nas outras). O que muda é que cada uma dessas quatro constantes locais agora é
//     INICIALIZADA A PARTIR deste único símbolo nomeado em vez de re-tipar independentemente o
//     literal `256u * 1024u * 1024u` -- uma mudança futura de política (elevar ou baixar o teto
//     DEFAULT) é uma edição aqui, verificada em tempo de compilação em todo lugar onde o próprio
//     `static_assert` de um ponto de chamada checa a constante local dele contra esta, em vez de
//     quatro literais textualmente-idênticos-por-coincidência que poderiam divergir em silêncio um
//     de cada vez.
//
//     DIVERGÊNCIA É PERMITIDA, MAS TEM DE SER CALIBRADA E ESCRITA: este default NÃO é um piso que
//     todo domínio precisa compartilhar pra sempre. `kMaxMappingsFileBytes` de `gamepad.cpp`
//     diverge dele DE PROPÓSITO (TETO-CALIBRE, W25, 2026-08-04) -- 1 MiB, derivado do artefato REAL
//     vendorizado `gamecontrollerdb_linux.txt` (200.882 bytes, ~5,2x de folga), não deste default,
//     porque um arquivo de mapeamentos de gamepad tem um teto superior real, conhecido, muito menor
//     que "qualquer asset de UI em geral". A lição que o TETO-CALIBRE nomeia é a que este header
//     existe pra prevenir que se repita: um valor herdado de outro domínio (ou de um default
//     compartilhado como este) carrega a CALIBRAÇÃO do domínio do qual foi derivado, não
//     necessariamente a do domínio onde aterrissa -- se o teto real de um domínio futuro for
//     provadamente menor (ou maior) que este default, o conserto é derivar A PRÓPRIA constante do
//     PRÓPRIO artefato/tráfego real, exatamente como `gamepad.cpp` fez, e ESCREVER a derivação no
//     comentário ao lado dela (não só citar este header como precedente pro "por que 256 MiB" -- a
//     mesma armadilha que o próprio post-mortem do TETO-CALIBRE nomeia: "de onde veio o número"
//     responde uma pergunta diferente de "por que este número é o certo AQUI").
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <cstddef>

namespace glintfx {

// EN: Default hard ceiling, in bytes, on any single untrusted byte-stream read/allocated in one
//     shot. See this file's own top comment for the full policy and why divergence is allowed but
//     must be calibrated and written down.
// PT: Teto rígido default, em bytes, sobre qualquer stream de bytes não confiável lido/alocado de
//     uma vez. Ver o próprio comentário de topo deste arquivo pra política completa e por que
//     divergência é permitida, mas tem de ser calibrada e escrita.
inline constexpr std::size_t kMaxUntrustedFileBytes = 256u * 1024u * 1024u; // 256 MiB.

} // namespace glintfx
