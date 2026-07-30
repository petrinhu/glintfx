// SPDX-License-Identifier: MPL-2.0
// EN: Umbrella public header — include this one instead of individual headers.
//     version.hpp is always included (L1.9-VERSEMBED: glintfx::version() must be available
//     regardless of backend); log.hpp is always included too (FW-LOG, W20 -- same independence
//     class as version.hpp: zero RmlUi/GLFW/GL, no GLINTFX_MODULE_* flag, must be reachable from
//     an embed-only build); app.hpp is only included when GLINTFX_BACKEND_GLFW is enabled
//     (standalone build); ui_layer.hpp is always included (embed mode is always available);
//     i18n.hpp is included when GLINTFX_MODULE_I18N is enabled (default ON, ADR-0015 module
//     "i18n" -- depends on core only, independent of GLINTFX_BACKEND_GLFW either way); audio.hpp
//     is included when GLINTFX_MODULE_AUDIO is enabled (default ON, ADR-0015 module "audio",
//     framework-2D slice A3-AUDIO -- same independence class as i18n); gamepad.hpp is included
//     when GLINTFX_MODULE_GAMEPAD is enabled (default ON on Linux, forced OFF elsewhere -- see
//     CMakeLists.txt's platform gate -- ADR-0015 (b)/ADR-0016 module "gamepad", framework-2D
//     slice A2-GAMEPAD; same independence class as i18n/audio, but Linux-only by nature).
//     gl_proc.hpp (DOC-GLCOHAB) is included UNCONDITIONALLY, outside app.hpp's own
//     `#if GLINTFX_BACKEND_GLFW` block (GLPROC-EMBED, 2026-07-30) -- glintfx::gl_proc_address()
//     resolves symbols against WHATEVER GL context is current, not only an App's; a UiLayer/
//     embed host is exactly who needs it most, to resolve its own function pointers without
//     linking a second loader library. See gl_proc.hpp's own top comment for the full
//     GLPROC-EMBED writeup (the old gate's premise -- "an embed host already owns its own
//     loader" -- was right, its conclusion -- "so there is nothing to resolve" -- was backwards).
// PT: Header público guarda-chuva — inclua este em vez dos headers individuais.
//     version.hpp sempre incluído (L1.9-VERSEMBED: glintfx::version() deve estar disponível
//     independente do backend); log.hpp também sempre incluído (FW-LOG, W20 -- mesma classe de
//     independência do version.hpp: zero RmlUi/GLFW/GL, sem flag GLINTFX_MODULE_*, precisa ser
//     alcançável de um build embed-only); app.hpp só é incluído quando GLINTFX_BACKEND_GLFW está
//     habilitado (build standalone); ui_layer.hpp sempre incluído (embed mode sempre disponível);
//     i18n.hpp é incluído quando GLINTFX_MODULE_I18N está habilitado (padrão ON, módulo "i18n"
//     da ADR-0015 -- depende só de core, independente de GLINTFX_BACKEND_GLFW de qualquer jeito);
//     audio.hpp é incluído quando GLINTFX_MODULE_AUDIO está habilitado (padrão ON, módulo
//     "audio" da ADR-0015, fatia framework-2D A3-AUDIO -- mesma classe de independência do i18n);
//     gamepad.hpp é incluído quando GLINTFX_MODULE_GAMEPAD está habilitado (padrão ON no Linux,
//     forçado OFF em outro lugar -- ver o gate de plataforma do CMakeLists.txt -- módulo
//     "gamepad" da ADR-0015 (b)/ADR-0016, fatia framework-2D A2-GAMEPAD; mesma classe de
//     independência do i18n/audio, porém Linux-only por natureza); draw2d.hpp é incluído quando
//     GLINTFX_MODULE_DRAW2D está habilitado (padrão ON, átomo "draw2d" do ADR-0017, fatia
//     D2D-1B -- depende de core + a costura interna de decode de imagem + o gl-loader interno;
//     NÃO de ui/fx/window, mesmo padrão de independência do i18n/audio/gamepad acima).
//     gl_proc.hpp (DOC-GLCOHAB) é incluído INCONDICIONALMENTE, fora do próprio bloco
//     `#if GLINTFX_BACKEND_GLFW` do app.hpp (GLPROC-EMBED, 2026-07-30) --
//     glintfx::gl_proc_address() resolve símbolos contra QUALQUER contexto GL corrente, não só
//     o de um App; um host UiLayer/embed é exatamente quem mais precisa dela, pra resolver os
//     próprios ponteiros de função sem linkar uma segunda lib de loader. Ver o próprio
//     comentário de topo de gl_proc.hpp pro relato completo do GLPROC-EMBED (a premissa da
//     guarda antiga -- "um host embed já é dono do próprio loader" -- estava certa, a conclusão
//     -- "então não há nada pra resolver" -- estava invertida).
#pragma once
#include <glintfx/config.hpp>
#include <glintfx/version.hpp>
#include <glintfx/log.hpp>
#include <glintfx/gl_proc.hpp>
#if GLINTFX_BACKEND_GLFW
#include <glintfx/app.hpp>
#endif
#include <glintfx/ui_layer.hpp>
#if GLINTFX_MODULE_I18N
#include <glintfx/i18n.hpp>
#endif
#if GLINTFX_MODULE_AUDIO
#include <glintfx/audio.hpp>
#endif
#if GLINTFX_MODULE_GAMEPAD
#include <glintfx/gamepad.hpp>
#endif
#if GLINTFX_MODULE_DRAW2D
#include <glintfx/draw2d.hpp>
#endif
