// SPDX-License-Identifier: MPL-2.0
// EN: Public window-mode selector for glintfx::App (A4-WINMODES, framework-2D). A plain,
//     engine-agnostic enum -- no GLFW type appears here, or anywhere else in glintfx's public
//     headers (the encapsulation invariant every header under glintfx/include/glintfx/
//     upholds; see AGENTS.md "gate de encapsulamento"). App-standalone ONLY: glintfx::UiLayer
//     (embed/guest mode) does not own the window and has no equivalent -- the host is the
//     window-mode authority there (ADR-0008).
// PT: Seletor público de modo de janela para glintfx::App (A4-WINMODES, framework-2D). Um enum
//     simples, agnóstico de engine -- nenhum tipo GLFW aparece aqui, nem em nenhum outro header
//     público do glintfx (o invariante de encapsulamento que todo header sob
//     glintfx/include/glintfx/ sustenta; ver AGENTS.md "gate de encapsulamento"). Só App
//     standalone: o glintfx::UiLayer (modo embed/convidado) não é dono da janela e não tem
//     equivalente -- o host é a autoridade de modo de janela lá (ADR-0008).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <cstdint>

namespace glintfx {

// EN: Selects how glintfx::App's window occupies the screen. Four modes, primary-monitor-only
//     in this slice (multi-monitor selection -- INBOX). Set the INITIAL mode via
//     AppConfig::window_mode (App::App()); switch at RUNTIME via App::set_window_mode().
//
//     Windowed (the default): decorated window at width×height, movable/resizable by the
//     window manager (WM), current behaviour unchanged.
//
//     Maximized: maximized BY THE WM, respecting the work area -- panels/taskbars stay
//     visible. glintfx never hand-sizes the window to the monitor's full dimensions; the WM
//     applies its own panel struts, the exact GusWorld consumer-driven fix this mode exists
//     for. Headless (no WM, e.g. Xvfb): the request may not be honoured -- see
//     App::window_mode()'s doc-comment.
//
//     FullscreenDesktop: borderless fullscreen at the monitor's CURRENT video mode -- no mode
//     switch. Covers EVERYTHING, panels included, BY DESIGN (this is the standard "windowed
//     full screen" semantics every 2D game framework offers); an app that wants the panel to
//     stay visible uses Maximized instead, not this mode.
//
//     FullscreenExclusive: a REAL video-mode switch to the mode closest to the requested
//     width×height. Under native Wayland this is compositor-EMULATED (GLFW's documented
//     behaviour: no real mode-set, the compositor scales) -- it behaves like
//     FullscreenDesktop with viewport scaling there; under X11/XWayland it is literal. See
//     docs/window-modes.md for the full per-platform breakdown.
//
// PT: Seleciona como a janela do glintfx::App ocupa a tela. Quatro modos, só monitor primário
//     nesta fatia (seleção multi-monitor -- INBOX). Define o modo INICIAL via
//     AppConfig::window_mode (App::App()); troca em RUNTIME via App::set_window_mode().
//
//     Windowed (o padrão): janela decorada em width×height, movível/redimensionável pelo
//     window manager (WM), comportamento atual inalterado.
//
//     Maximized: maximizada PELO WM, respeitando a work area -- painéis/barras de tarefas
//     continuam visíveis. A glintfx nunca dimensiona a janela à mão pro tamanho cheio do
//     monitor; o WM aplica os próprios struts de painel, exatamente o fix consumer-driven do
//     GusWorld que este modo existe para resolver. Headless (sem WM, ex.: Xvfb): o pedido pode
//     não ser honrado -- ver o doc-comment de App::window_mode().
//
//     FullscreenDesktop: fullscreen sem borda no video mode ATUAL do monitor -- sem troca de
//     modo. Cobre TUDO, painéis inclusive, BY DESIGN (é a semântica padrão de "windowed full
//     screen" que todo framework de jogo 2D oferece); um app que quer o painel visível usa
//     Maximized, não este modo.
//
//     FullscreenExclusive: uma troca de video mode REAL para o modo mais próximo de
//     width×height. Sob Wayland nativo é EMULADO pelo compositor (comportamento documentado do
//     GLFW: sem troca de modo real, o compositor escala) -- se comporta como FullscreenDesktop
//     com escala de viewport lá; sob X11/XWayland é literal. Ver docs/window-modes.md para o
//     detalhamento completo por plataforma.
enum class WindowMode : std::uint8_t {
  Windowed,
  Maximized,
  FullscreenDesktop,
  FullscreenExclusive,
};

} // namespace glintfx
