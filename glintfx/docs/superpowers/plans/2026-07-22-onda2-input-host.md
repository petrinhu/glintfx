# ONDA 2 — Host physical-input channel + window lifecycle + log throttle / canal de input físico pro host + ciclo de vida da janela + throttle de log

> EN first, PT below (house convention). Plan author: Caetano (CTO). Baseline: glintfx
> **v0.18.0** on `main` (program baseline; Wave-1 hygiene may tag v0.18.1 independently).
> Target tag: **v0.19.0**. Governing architecture: [ADR-0015](../../../../docs/adr/0015-framework-2d-atomized-architecture.md)
> (input atom: "keyboard/mouse backend depends on window; polling/callbacks for the game").
> Program: [2026-07-22-programa-completo.md](2026-07-22-programa-completo.md).
> Wave item ID prefix for commits: `HOSTIN-*` / `LOGTHR-1` / `DOC-HOSTIN` (per-slice IDs below).
> STATUS: PLANNED — awaiting orchestrator fan-out.

---

## Part A — EN

### 0. Reordering record (orchestrator decision, communicated to leader + consumer)

The program plan's Wave 2 was **Draw2D core (D2D-1)**. The consumer (GusWorld) closed its
v0.18.0 integration and reported **exactly one production blocker** for migrating to App mode:
no way to read PHYSICAL keyboard/mouse for the game scene (everything is routed into RmlUi
only; their spike worked around it by linking glfw themselves and calling
`glfwGetKey(glfwGetCurrentContext(), ...)`). Under the house consumer-driven cadence
(`feedback_cadencia_releases_consumidor`: a consumer finding becomes the immediate next
release), the blocker jumps the queue:

| Program wave (old) | Now | Tag |
|---|---|---|
| Wave 2 — Draw2D core (D2D-1) | **Wave 3** | v0.20.0 |
| Wave 3 — Draw2D camera (D2D-2) | **Wave 4** | v0.21.0 |
| **NEW Wave 2 — this plan** (consumer unblock) | **Wave 2** | **v0.19.0** |
| Waves 4..9 (SVG, gamepad, window, audio, D2D-3, CBDT) | shift +1 each | +1 each |

The orchestrator should append this table as an addendum to the program plan when the wave
closes (single source of truth there; this plan records the local rationale).

Everything in this wave came from the consumer's consolidated inventory; the spike also
VALIDATED our `frame_callback` contract in production terms (their own GL renderer inside our
hook, no conflict with the internal gl3w, dt error p95 = 20.8 µs over 8633 frames) — that
result is folded into the 3.1/3.2 docs below as verified behaviour, not marketing.

### 1. Scope (what ships, what explicitly does not)

**Ships (5 code slices + 1 doc slice):**

- **HOSTIN-1** — physical input STATE channel on `App`: `is_key_down(Key)`,
  `is_mouse_button_down(int)`, `get_cursor_pos(float&, float&)` + the `Key` enum extension
  that makes it useful for a game (WASD, letters, digits, F-keys, modifiers L/R, numpad,
  punctuation, World1/2).
- **HOSTIN-2** — edge-detected key callback `set_key_callback(fn(Key, KeyAction, mods))`
  (lossless press+release even within one frame). Same tap point and same files as HOSTIN-1
  — doing it later would mean a second pass through identical code; it rides in-wave.
- **HOSTIN-3** — `request_close()` (programmatic quit for hosts inside `run()`).
- **HOSTIN-4** — close-request VETO callback (X button / Alt+F4 → "save before exit?").
- **HOSTIN-5** — WINDOW focus + iconify (minimize) observation: 2 callbacks + 2 polling
  queries (auto-pause / volume-duck on alt-tab).
- **LOGTHR-1** — dedup/throttle of repeated RmlUi warnings (the "No font face defined"
  per-element-per-frame flood) at OUR `SystemInterface` layer, both modes (App + embed).
- **DOC-HOSTIN** — 3 doc items: `set_frame_callback` hosting-your-own-renderer contract
  (GL loader is the HOST's job), `render()` vsync/swap-interval honesty note, `Gamepads`+`App`
  coexistence note.

**Does NOT ship (recorded, with trigger):**

- Text input / IME forwarding — **leader-confirmed deferral** (program section 6.3; trigger:
  GusWorld SAVE-NOMEAR). The key-state channel here is PHYSICAL polling, not text.
- Raw scancode escape hatch (`is_scancode_down(int)`) for keys GLFW has no token for
  (notably the ABNT2 `/` key, scancode 97, a known GLFW gap) — INBOX seed, trigger: a
  consumer actually hits an unnamed key. See D4.
- Mouse buttons beyond GLFW's 8-button ceiling (12-button mice) — INBOX seed, see D3.
- vsync SETTER — already scheduled in WM-EXTRAS (window-modes finish wave); 3.2 is doc-only.
- Mouse-button edge callback / per-frame "pressed this frame" helpers — polling + key
  callback cover the consumer's stated need; seed.

### 2. Design decisions (D1–D10)

**D1 — Extend the existing `enum class Key`, do NOT create a second enum.** One key
vocabulary for the whole library (UiEvent synthetic channel, physical state channel, edge
callback) — two enums would force every consumer to hold a mapping between our own types.
Constraints that make this safe:

- **Append-only**: the existing enumerators (`None` … `PageDown`) keep their exact positions
  and numeric values (compat with any consumer that serialized them). New entries are
  appended AFTER `PageDown`, ending with a `Count` sentinel (documented: never a key, never
  emitted; future additions go BEFORE `Count`).
- New entries (mirrors GLFW's nameable set, ~92 additions): `A`–`Z`; `Digit0`–`Digit9`;
  `F1`–`F12`; `LeftShift`, `RightShift`, `LeftControl`, `RightControl`, `LeftAlt`,
  `RightAlt`, `LeftSuper`, `RightSuper`; `Insert`, `CapsLock`, `ScrollLock`, `NumLock`,
  `PrintScreen`, `Pause`, `Menu`; `Apostrophe`, `Comma`, `Minus`, `Period`, `Slash`,
  `Semicolon`, `Equal`, `LeftBracket`, `Backslash`, `RightBracket`, `GraveAccent`,
  `World1`, `World2`; `Kp0`–`Kp9`, `KpDecimal`, `KpDivide`, `KpMultiply`, `KpSubtract`,
  `KpAdd`, `KpEqual`. `GLFW_KEY_KP_ENTER` keeps folding into `Key::Enter` (existing
  documented behaviour, unchanged).
- `glfw_translate_key()` grows the same entries (mechanical switch; the unit test pins the
  full table — see section 4). **This is the slice that is bigger than it looks** (~100-case
  switch + tests); it is still mechanical, single-file, one-agent work. The minimum
  defensible slice if review pressure demands a cut: drop `Kp*` + lock keys (~17 entries) —
  letters/digits/F-keys/modifiers are NOT cuttable (they are the consumer's literal ask).

**D2 — Own input-state table fed by the EXISTING GLFW callbacks; NOT `glfwGetKey` passthrough.**
Three reasons over the (simpler-looking) `glfwGetKey`/`glfwGetCursorPos` forwarding:
(1) **testability** — a pure `InputState` struct (`src/input_state.hpp`: fixed `bool` array
indexed by `Key`, 8 mouse-button bools, cursor x/y) is unit-testable headless, while
`glfwGetKey` state can only be driven by real platform events; (2) the **edge callback
(HOSTIN-2) needs the callback tap anyway** — one mechanism serves both; (3) no reverse
`Key → GLFW_KEY_*` map needed (we reuse the existing forward translation at the tap).
Tap point: `WindowGlfw::handle_key/handle_mouse_button/handle_cursor_pos` update the state
FIRST, then dispatch to the sink as today (so a UI callback that polls `is_key_down` sees
fresh state). All real logic in pure functions (A1's own testability contract, kept).
`GLFW_REPEAT` does not re-set an already-down key (state is level, not edge).

**D3 — Mouse buttons: plain `int`, range `[0, 7]`, NO 3-button restriction (leader
requirement, memory `feedback_input_multibutton_teclados`).** The UI route keeps its 3-button
contract untouched (RmlUi never dispatches clicks for side buttons — documented since
AUD-PUB-4); the STATE channel tracks all 8 buttons GLFW can report
(`GLFW_MOUSE_BUTTON_LAST` = button index 7). `handle_mouse_button` therefore forks: state
update accepts 0..7; UiEvent emission keeps the existing 0..2 filter. Honest limit,
documented in the header: **GLFW itself cannot see buttons beyond 8** — a 12-button mouse's
extra buttons never reach this API. Recorded as an INBOX seed (route: evdev side-channel,
same pattern as the gamepad atom; trigger: a consumer needs button ≥ 8). `is_mouse_button_down`
outside `[0,7]` → `false`, fail-high, never UB.

**D4 — Keyboard layouts (leader requirement: US-Intl + BR ABNT2 as base).** GLFW key tokens
are PHYSICAL-POSITION tokens named after the US layout — for game input (WASD as a movement
diamond) this is exactly the right semantics and is layout-agnostic by construction: the key
labelled `W` on ABNT2 is the same physical key and reports `GLFW_KEY_W`. Consequences,
documented verbatim in the header: (a) US-Intl is fully covered by the named set; (b) ABNT2's
`ç` arrives as the physical `Semicolon` position (position semantics, not label semantics);
(c) ABNT2's second `/` key (right of the right Shift, scancode 97) is a **known GLFW gap**
(`GLFW_KEY_UNKNOWN`, no token) — it CANNOT be polled through this API today; the escape
hatch (`is_scancode_down`) is a seeded follow-up with a real-consumer trigger, per the house
YAGNI discipline. `World1`/`World2` ARE included (the ISO extra key next to left Shift on
ABNT2/European boards reports as one of these on X11).

**D5 — Synthetic and physical channels stay SEPARATE.** `App::process_event()` (the
synthetic/replay/test channel) does NOT write the physical state table — `is_key_down`
answers "is the physical key held on the real keyboard", and letting synthetic events forge
that answer would make the two channels lie to each other (a replay harness would leave
phantom held keys behind). Documented on both methods. (Tests inject at the pure seam
instead — section 4.)

**D6 — `request_close()` bypasses the veto; the veto guards USER-initiated close only.**
`request_close()` = `glfwSetWindowShouldClose(win, TRUE)`: programmatic quit is an
intentional act by the code that owns the loop; routing it through the veto would deadlock
"menu Sair" against its own confirmation dialog. The GLFW close callback
(`glfwSetWindowCloseCallback`) fires only on user attempts (X button, Alt+F4); our handler
asks the registered `std::function<bool()>`; `false` → `glfwSetWindowShouldClose(win, FALSE)`
(veto: window stays open, host shows its "save?" dialog and later calls `request_close()`
for real). No callback registered = current behaviour (close proceeds). AUD-TEC-3
copy-before-invoke; callback runs inside `poll_events()` on the main thread (documented).

**D7 — Window focus/iconify: names must not collide with DOM focus.** `set_focus_callback`
(DOM element focus) already exists; the window-level API is prefixed: 
`set_window_focus_callback(fn(bool focused))`, `set_window_iconify_callback(fn(bool iconified))`
+ polling twins `is_window_focused()`, `is_window_iconified()` (via `glfwGetWindowAttrib`,
`GLFW_FOCUSED`/`GLFW_ICONIFIED`). Fail-high: `false` when `!ok()`. Headless honesty
(documented, same class as `window_mode()`'s Xvfb note): under Xvfb there is no WM — iconify
may never be honoured and focus reporting is degenerate; the callbacks are exercised for
real only in the nested-compositor QA leg.

**D8 — Log throttle lives in OUR `SystemInterface` layer, policy is count-based dedup.**
Verified origin: the flood comes from RmlUi core (`ElementText.cpp:86`, warning logged per
element per frame) through `Rml::SystemInterface::LogMessage`. Upstream is pinned — patching
RmlUi is not on the table. Our tap: **App mode** constructs the RmlUi backend's
`SystemInterface_GLFW` raw (`app.cpp:129`) and **embed mode** uses our `SystemClock`
(`src/system_clock.hpp`); neither overrides `LogMessage`, so both fall to RmlUi's default
stderr print. Fix: one pure helper, `src/log_dedup.hpp` (`LogDedup::should_print(type, msg)
→ bool` + suppressed-count bookkeeping), used by (a) a `LogMessage` override added to
`SystemClock` and (b) a new thin `src/system_glfw_dedup.hpp` subclassing
`SystemInterface_GLFW` with the same override (App switches to it — one line in `app.cpp`).
**Policy (deterministic, testable):** first occurrence of an identical (type, message) pair
always prints; repeats are suppressed; occurrence counts re-emit one summary line at counts
10, 100, 1000, … (`"... repeated 100x (suppressed)"`). Errors (`LT_ERROR`/`LT_ASSERT`) are
NEVER suppressed. Memory bound: cap the table (e.g. 256 distinct messages, LRU evict) so a
hostile/degenerate message stream cannot grow it unbounded. Note the message here contains
the element address (varies) — dedup keys on the full formatted string, which still
collapses the per-frame repetition of the SAME element (the actual flood); throttling
by-prefix was considered and rejected (would mask genuinely distinct warnings).

**D9 — vsync: document, don't touch.** Verified: `glfwSwapInterval` is called NOWHERE in
`glintfx/src/` — the swap-synchronized `dt` the consumer measured is the **driver/compositor
default**, not a glintfx guarantee. 3.2 documents exactly that on `render()`/`run()`:
"glintfx neither enables nor disables vsync; the platform default governs; a vsync setter is
scheduled in the window-modes finish wave (WM-EXTRAS)". No setter here (no scope creep into
Onda "window finish").

**D10 — UiEvent→RmlUi routing is UNCHANGED this wave.** The extended `Key` entries are
tracked in the state table and reported by the edge callback, but `glfw_translate_key`'s
CALLERS keep emitting UiEvents to the UI engine exactly as before, and `Engine`'s
`to_rml_key` mapping is untouched — zero behaviour change for documents/focus navigation.
(Extending RmlUi key forwarding — e.g. letters for UI shortcuts — is a separate, deliberate
change with its own review; seeded, not smuggled in.) Consequence for implementation: the
translate function grows, but `WindowGlfw::handle_key` forwards to the sink ONLY the subset
`to_rml_key` understands today (a small "UI-forwardable" predicate pins this; test-covered).

### 3. Public API contract (literal)

```cpp
// ui_event.hpp — Key gains ~92 appended enumerators (D1 list) + Count sentinel.
// New, next to Mod:
enum class KeyAction { Press, Release, Repeat };  // EN/PT doc-comment per house style

// app.hpp — all new methods App-only (UiLayer/embed: host owns the window and its input).
// Physical input state (HOSTIN-1). Fail-high: false/0 when !ok(); invalid enum/button → false.
bool  is_key_down(Key k) const;                  // level state; GLFW_REPEAT does not re-edge
bool  is_mouse_button_down(int button) const;    // [0,7]; UI 3-button contract untouched (D3)
void  get_cursor_pos(float& x, float& y) const;  // framebuffer px, top-left, y-down (same
                                                 // space as UiEvent/get_element_box); (0,0) when !ok()
// Edge-detected key events (HOSTIN-2). Fires inside poll_events(); AUD-TEC-3 copy-before-invoke.
void  set_key_callback(std::function<void(Key key, KeyAction action, int modifiers)> cb);
// Lifecycle (HOSTIN-3/4/5).
void  request_close();                           // programmatic; BYPASSES the veto (D6)
void  set_close_request_callback(std::function<bool()> cb);   // false = veto (D6)
void  set_window_focus_callback(std::function<void(bool focused)> cb);
void  set_window_iconify_callback(std::function<void(bool iconified)> cb);
bool  is_window_focused() const;                 // false when !ok()
bool  is_window_iconified() const;               // false when !ok()
```

No third-party type crosses the boundary (house rule, `tools/check_encapsulation.sh` gate);
no `GLFW_KEY_*` constant appears anywhere public.

### 4. Testing (headless answer included)

**Is physical input testable headless? Honestly: at three levels, and level 3 is manual.**

1. **Pure units (the bulk, deterministic, no display):**
   - `glfw_event_translate_sanity` (extended): full GLFW→Key table pinned (every named key →
     its enumerator; unknown → false; `KP_ENTER`→`Enter` fold); **enum-freeze assertions**
     (`static_assert(int(Key::PageDown) == <today's value>)` etc.) so the append-only rule
     (D1) is machine-enforced, not reviewed-by-eye.
   - NEW `input_state_sanity` (pure, no window): press/release/edge semantics, repeat does
     not double-edge, mouse 0..7, hostile applies (button 8, −1, huge int; invalid Key cast)
     are ignored fail-high; cursor updates; the UI-forwardable predicate (D10) pinned.
   - NEW `log_dedup_sanity` (pure): first-print, suppression, 10/100/1000 re-emit with
     count, per-message independence, LT_ERROR never suppressed, LRU cap under 10k distinct
     hostile messages, empty/huge message safe.
2. **App-level under Xvfb (automated, in-suite):**
   - NEW `app_input_query_smoke`: fresh App → all `is_key_down` false, cursor (0,0),
     out-of-range buttons false; `request_close()` → `running()` false → `run()` returns
     immediately (bounded, no timeout race — this finally gives `run()` a real test, closing
     the AUD-TEC-7 note honestly); callbacks registerable pre/post `load()`, null-callback
     no-op; `process_event` does NOT alter physical state (D5 pinned by test).
   - Close-veto e2e (best-effort, time-boxed): under X11 send `WM_DELETE_WINDOW` via
     `XSendEvent` to the GLFW window (`glfwGetX11Window`, `GLFW_EXPOSE_NATIVE_X11` — tests
     may touch native APIs; the PUBLIC boundary may not) and assert veto-false keeps
     `running()` true, veto-true closes. If flaky under Xvfb after a time-boxed attempt
     (half a day), fall back to the pure seam (`WindowGlfw`'s close-decision extracted as a
     testable function) + the manual leg, and record the downgrade in the PR.
   - LOGTHR-1 integration: embed scene with a font-family-less label rendered 100 frames;
     stderr captured (freopen/pipe); assert ≤ a small constant of lines (vs ~100+ today).
3. **Nested-compositor manual QA leg (house rule: NEVER the leader's live session):**
   scripted checklist run by the `qa-engineer` inside a nested compositor (Xephyr or
   `weston --nested`): real keystrokes via `xdotool`/XTest → `is_key_down` demo readout;
   12-key rollover sanity; window X-button veto dialog flow; alt-tab focus callback; minimize
   iconify callback; ABNT2 layout loaded (setxkbmap br) → WASD still correct, `ç` reports
   `Semicolon` position, scancode-97 gap CONFIRMED and logged as the seeded limitation.
   Deliverable: the QA agent's pass/fail report, re-checked by the orchestrator.

### 5. House gates (all mandatory)

- Build dirs in **`/var/tmp`** (`/var/tmp/glx-onda2*`) with `export TMPDIR=/var/tmp`; **one
  heavy build at a time**; ASan leg included before review sign-off.
- `tools/preci.sh` (clang-tidy + cppcheck + gitleaks) green before any push.
- **MSVC-safe** (the 3 layers that already bit us): no raw `-Wall`/`-Wextra` (generator
  expression guard); no unguarded `min`/`max` usage patterns (NOMINMAX is directory-scope,
  don't regress); no `filesystem::path::c_str()` into `const char*`; note `ui_event.hpp`,
  `input_state.hpp`, `log_dedup.hpp`, `system_clock.hpp` ARE compiled by the MSVC job
  (embed build) — keep them libc-portable (no Unix-only header without a guard).
  `window_glfw.cpp`/`app.cpp` are GLFW=ON-only (not in the MSVC job until CI-MSVCGLFW).
- `link_smoke` now LINKS for real: every new public method must have its definition in a TU
  that is part of the embed build **or** be App-only and excluded — all new methods here are
  App-only (`app.cpp`, GLFW=ON), and `KeyAction`/enum growth is header-only; the embed
  `link_smoke` must stay green with GLFW=OFF.
- Window/input stress in a **nested compositor only** (section 4.3).
- Push / merge / tag: **only with explicit leader authorization in-context**. Commits cite
  the slice ID and touch TODO.md status in the same commit (`🔍` on delivery).
- Version bump `glintfx/CMakeLists.txt` `project(VERSION 0.19.0)` at wave close, per
  `docs/RELEASE_CHECKLIST.md`; CHANGELOG entry; consumer relay block for the leader.

### 6. Agent map (4 agents, disjoint ownership, sequential where files collide)

| # | Agent | Slices | Owns (files) | Runs |
|---|---|---|---|---|
| 1 | `backend-engineer` (implementer A) | HOSTIN-1..5 | `ui_event.hpp`, `glfw_event_translate.hpp`, NEW `src/input_state.hpp`, `window_glfw.{hpp,cpp}`, `app.{hpp,cpp}`, tests: `glfw_event_translate_sanity.cpp` (extend), NEW `input_state_sanity.cpp`, NEW `app_input_query_smoke.cpp`, `tests/CMakeLists.txt` (their block) | first |
| 2 | `backend-engineer` (implementer B, different agent) | LOGTHR-1 | NEW `src/log_dedup.hpp`, NEW `src/system_glfw_dedup.hpp`, `src/system_clock.hpp`, one-line swap in `app.cpp` (after A lands), tests: NEW `log_dedup_sanity.cpp`, NEW `log_throttle_embed_sanity.cpp`, `tests/CMakeLists.txt` (their block) | after A (app.cpp collision) |
| 3 | `technical-writer` | DOC-HOSTIN (3.1/3.2/3.3) + CHANGELOG draft | doc-comments in `app.hpp` (`set_frame_callback`, `render()`/`run()` vsync note — after A lands), `docs/embed-integration.md` §frame-hook, NEW short section in `docs/` for Gamepads+App coexistence (or extend `docs/gamepad.md`), `CHANGELOG.md` | after A |
| 4 | `qa-engineer` (adversarial reviewer, EXECUTES) | whole wave | read+run everything; mutation testing on translate table + dedup policy + veto logic; ASan suite; nested-compositor manual leg (section 4.3); domino sweep: "does this guard exist in a sibling?" across the new fail-high surface | last |

Orchestrator (main thread) re-verifies: clean build from scratch in `/var/tmp`, spot-check
of claims (file:line), `git log` for unauthorized pushes, suite green GLFW=ON + embed, then
takes the wave to the leader.

### 7. Briefs (ready to paste)

#### 7.1 Brief — implementer A (`backend-engineer`) — HOSTIN-1..5

```
Você é backend-engineer no glintfx (repo loucura_c_asm, pasta glintfx/). NÃO faça push. NÃO
faça merge. Commits locais em Conventional Commits pt-br citando o ID da fatia (HOSTIN-1..5)
e tocando o Status do TODO.md no mesmo commit (→ 🔍 Pendente verificação).

Leia ANTES: docs/superpowers/plans/2026-07-22-onda2-input-host.md (este plano, seções D1-D7,
D10, 3, 4, 5 — o contrato é literal, não invente assinatura); AGENTS.md (gotchas + governança);
glintfx/include/glintfx/ui_event.hpp; src/glfw_event_translate.hpp; src/window_glfw.{hpp,cpp};
src/app.cpp; include/glintfx/app.hpp; include/glintfx/gamepad.hpp (o padrão fail-high da casa).

Entregue, sob TDD (red/green/refactor), na ordem:
1. HOSTIN-1: extensão APPEND-ONLY do enum Key (lista D1, sentinela Count) + extensão de
   glfw_translate_key + struct pura InputState (src/input_state.hpp) + tap nos handle_* do
   WindowGlfw (estado ANTES do sink; GLFW_REPEAT não re-edge; mouse 0..7 no estado, filtro
   0..2 do UiEvent INTOCADO; predicado "UI-forwardable" preservando o subconjunto atual
   encaminhado ao RmlUi — D10) + App::is_key_down/is_mouse_button_down/get_cursor_pos
   (fail-high, coordenadas = pixels de framebuffer, mesmo espaço do UiEvent).
2. HOSTIN-2: KeyAction + App::set_key_callback (dispara de dentro do poll_events, cópia-
   antes-de-invocar AUD-TEC-3, null = no-op).
3. HOSTIN-3: App::request_close (BYPASSA o veto — doc-comment explícito, D6).
4. HOSTIN-4: set_close_request_callback (false = veto via glfwSetWindowShouldClose(FALSE);
   só close INICIADO PELO USUÁRIO; lógica de decisão extraída em função pura testável).
5. HOSTIN-5: set_window_focus_callback / set_window_iconify_callback +
   is_window_focused/is_window_iconified (glfwGetWindowAttrib; fail-high; honestidade
   headless documentada — sob Xvfb sem WM iconify pode não ser honrado).
Testes (seção 4 do plano): estender glfw_event_translate_sanity (tabela completa + static_
   asserts de congelamento dos valores atuais do enum), novo input_state_sanity (puro),
   novo app_input_query_smoke (Xvfb; inclui request_close→run() retorna; process_event NÃO
   altera estado físico; veto e2e via XSendEvent WM_DELETE_WINDOW GLFW_EXPOSE_NATIVE_X11 —
   TIME-BOX de meio dia, se flaky caia pro seam puro e REGISTRE o downgrade).
Doc-comments bilíngues (EN primeiro, PT depois) em TUDO que é público, incluindo: semântica
   de posição física do GLFW (US-Intl ok; ç do ABNT2 = posição Semicolon; scancode 97 do "/"
   ABNT2 = lacuna GLFW conhecida, seed registrada), teto de 8 botões do GLFW (mouse 12
   botões = seed com gatilho), Count nunca emitido.
Gates: build em /var/tmp/glx-onda2 com TMPDIR=/var/tmp; 1 build pesado por vez; suíte
   GLFW=ON e embed (GLFW=OFF: nada disto pode vazar pro build embed — App-only);
   tools/preci.sh limpo; headers públicos e input_state.hpp/ui_event.hpp MSVC-portáveis
   (sem -Wall cru, sem min/max desprotegido, sem header Unix-only sem guard);
   check_encapsulation.sh limpo (zero GLFW_* público). NADA de stress de janela na sessão
   viva do líder — teste automatizado só sob Xvfb.
Ao final: relatório com file:line de cada entrega + saída do ctest + o que ficou de fora.
```

#### 7.2 Brief — implementer B (`backend-engineer`, agente DIFERENTE do A) — LOGTHR-1

```
Você é backend-engineer no glintfx. NÃO faça push/merge. Commits locais Conventional pt-br
citando LOGTHR-1 + Status do TODO.md (→ 🔍) no mesmo commit. Rode APÓS o implementer A
mergear na branch da onda (vocês colidem em app.cpp numa linha).

Leia ANTES: o plano (D8, seções 4-5); src/system_clock.hpp; app.cpp:129 (construção do
SystemInterface_GLFW); a origem do flood: build/_deps/rmlui-src/Source/Core/ElementText.cpp:86.

Entregue sob TDD:
1. src/log_dedup.hpp — helper PURO (sem RmlUi na interface do helper): should_print(type,
   msg) com política determinística: 1ª ocorrência imprime; repetidas suprimem; re-emite
   sumário em 10/100/1000/... ("repeated Nx (suppressed)"); LT_ERROR/LT_ASSERT NUNCA
   suprimidos; tabela com cap LRU (256) contra stream hostil de mensagens distintas.
2. Override de LogMessage no SystemClock (embed) usando o helper.
3. src/system_glfw_dedup.hpp — subclasse fina de SystemInterface_GLFW com o mesmo override;
   troca de uma linha no app.cpp.
Testes: log_dedup_sanity (puro: política completa + hostil: msg vazia/enorme/10k distintas)
   e log_throttle_embed_sanity (cena embed com label sem font-family, 100 frames, stderr
   capturado, assert ≤ constante pequena de linhas).
Gates: build em /var/tmp com TMPDIR=/var/tmp (1 pesado por vez); preci.sh; MSVC-safe (esses
   arquivos COMPILAM no job MSVC embed — nada Unix-only sem guard); link_smoke embed verde;
   doc-comments bilíngues. Relatório final com file:line + ctest.
```

#### 7.3 Brief — `technical-writer` — DOC-HOSTIN

```
Você é technical-writer no glintfx. NÃO faça push/merge. Commit local citando DOC-HOSTIN.
Rode após o implementer A (colisão em app.hpp). Docs bilíngues (EN primeiro, PT depois).
1. set_frame_callback (app.hpp + docs/embed-integration.md §frame-hook): hospedar um renderer
   GL PRÓPRIO dentro do hook é suportado e VALIDADO em produção pelo consumidor (GusWorld:
   renderer próprio + loader do host, sem conflito com o gl3w interno; dt p95 20,8µs/8633
   frames) — o contrato: o HOST traz o próprio GL loader (símbolos GL do glintfx são
   internos, ADR-0013), estado GL restaurado automaticamente após o hook (contrato já
   documentado no engine.hpp — referencie, não duplique).
2. render()/run() (app.hpp): nota de vsync HONESTA — glintfx NÃO chama glfwSwapInterval em
   lugar nenhum (verificado); o default do driver/compositor governa; setter virá na onda
   window-modes finish (WM-EXTRAS). Não prometa sincronização.
3. Coexistência Gamepads+App (docs/gamepad.md, seção nova curta): ordem de init livre
   (Gamepads não toca GLFW — evdev puro; App é dono do glfwInit); poll() do Gamepads no
   mesmo lugar do frame step; referencie o doc-comment de gamepad.hpp.
4. CHANGELOG.md: rascunho da entrada v0.19.0 (todas as fatias da onda).
Nada de claims não verificáveis; cite file:line ao referenciar contratos.
```

#### 7.4 Brief — `qa-engineer` (review adversarial, EXECUTA) — onda inteira

```
Você é qa-engineer, revisor ADVERSARIAL da onda HOSTIN-*/LOGTHR-1/DOC-HOSTIN do glintfx.
Você NÃO implementou nada disto (implementer ≠ reviewer). NÃO faça push/merge.
Método: você EXECUTA, não só lê. Build limpo do zero em /var/tmp/glx-onda2-review
(TMPDIR=/var/tmp, 1 build pesado por vez), suíte GLFW=ON + embed + leg ASan.
1. Mutation testing dirigido: inverta casos da tabela glfw_translate_key (o teste pega?);
   quebre o append-only do enum (os static_asserts pegam?); mude a política do dedup
   (10→11 — o teste pega?); inverta o sentido do veto (pega?).
2. Hostil: is_mouse_button_down(INT_MAX/-1), Key cast fora de faixa, callbacks que se
   re-registram dentro da própria invocação (AUD-TEC-3), process_event tentando forjar
   estado físico (D5), stream de 10k mensagens distintas no dedup (cap LRU segura?).
3. Dominó: cada guard novo tem gêmeo? (ex.: fail-high do is_window_focused existe no
   is_window_iconified? o filtro 0..2 do UiEvent sobreviveu intacto? o embed build
   GLFW=OFF continua sem NADA disto vazando?)
4. Leg manual em compositor ANINHADO (Xephyr ou weston --nested; NUNCA a sessão viva do
   líder): xdotool/XTest → is_key_down; veto no botão X; alt-tab (focus cb); minimizar
   (iconify cb); setxkbmap br → WASD ok, ç = posição Semicolon, scancode 97 confirma a
   lacuna documentada. Roteiro + pass/fail item a item.
5. Docs: cada claim do DOC-HOSTIN é verificável? (ex.: grep glfwSwapInterval vazio).
Relatório: Critical/Major/Minor com file:line + comando de repro. Critical/Major bloqueiam
a tag. O orquestrador vai re-verificar suas claims — seja preciso.
```

### 8. TODO.md rows (for the orchestrator to insert)

| ID | Item | Onda | Esforço | Status |
|---|---|---|---|---|
| HOSTIN-1 | Canal de estado de input físico no App (Key estendido append-only + InputState + 3 queries) | 2 | M | ⬜ |
| HOSTIN-2 | set_key_callback edge-detectado (KeyAction) | 2 | P | ⬜ |
| HOSTIN-3 | request_close() (bypassa veto, documentado) | 2 | P | ⬜ |
| HOSTIN-4 | Veto de close do usuário (set_close_request_callback) | 2 | P | ⬜ |
| HOSTIN-5 | Focus/iconify de JANELA (2 cbs + 2 queries) | 2 | P | ⬜ |
| LOGTHR-1 | Dedup/throttle de warnings RmlUi no SystemInterface (ambos os modos) | 2 | P | ⬜ |
| DOC-HOSTIN | Docs: frame_callback renderer-hosting, vsync honesto, Gamepads+App | 2 | P | ⬜ |

INBOX seeds to add: `SEED-SCANCODE` (is_scancode_down; trigger: consumer hits an unnamed
key, e.g. ABNT2 scancode 97), `SEED-MOUSE9PLUS` (buttons ≥ 8 via evdev side-channel;
trigger: consumer needs them), `SEED-UIKEYFWD` (forward extended keys to RmlUi for UI
shortcuts; trigger: real UI demand), `SEED-MOUSEEDGE` (mouse-button edge callback; trigger:
consumer ask).

---

## Part B — PT

### 0. Registro da reordenação (decisão do orquestrador, comunicada ao líder e ao consumidor)

A Onda 2 do programa era o Draw2D (D2D-1). O consumidor (GusWorld) fechou a integração da
v0.18.0 e reportou **exatamente um bloqueio de produção** pra migrar ao App mode: não há como
ler teclado/mouse FÍSICOS pra cena do jogo (tudo é roteado só pro RmlUi; o workaround do
spike deles foi linkar glfw e chamar `glfwGetKey` direto — funciona, mas exige dep extra e
conhecimento das nossas entranhas). Pela cadência consumer-driven da casa
(`feedback_cadencia_releases_consumidor`), o bloqueio fura a fila: **esta onda vira a Onda 2
(v0.19.0)** e o Draw2D desliza pra Onda 3 (v0.20.0); ondas seguintes +1 (tabela na Parte A,
seção 0 — o orquestrador anexa o adendo no plano do programa ao fechar a onda).

O mesmo inventário do consumidor VALIDOU o contrato do `frame_callback` em produção
(renderer GL próprio deles dentro do nosso hook, sem conflito com o gl3w interno, erro de
`dt` p95 = 20,8µs em 8633 frames) — isso vira doc verificada nos itens 3.1/3.2.

### 1. Escopo

**Entra (5 fatias de código + 1 de doc):** HOSTIN-1 (canal de ESTADO de input físico:
`is_key_down(Key)`, `is_mouse_button_down(int)`, `get_cursor_pos` + extensão do enum `Key`);
HOSTIN-2 (callback de tecla edge-detectado — mesmo ponto de tap, mesmos arquivos: entra na
onda pra não passar duas vezes pelo mesmo código); HOSTIN-3 (`request_close()`); HOSTIN-4
(veto do close do usuário); HOSTIN-5 (focus/iconify de JANELA: 2 callbacks + 2 queries);
LOGTHR-1 (throttle do flood "No font face defined" na NOSSA camada de SystemInterface, App e
embed); DOC-HOSTIN (3 docs: hospedar renderer no frame_callback, vsync honesto, coexistência
Gamepads+App).

**NÃO entra (registrado com gatilho):** text-input/IME (adiamento confirmado pelo líder no
programa, seção 6.3); `is_scancode_down` (válvula de escape pra teclas sem token GLFW —
notavelmente o `/` do ABNT2, scancode 97; seed com gatilho de consumidor real); botões de
mouse além do teto de 8 do GLFW (mouse de 12 botões — seed, rota evdev, ver D3); SETTER de
vsync (já agendado no WM-EXTRAS — aqui é doc-only); edge callback de botão de mouse (seed).

### 2. Decisões de desenho (resumo — o normativo completo está na Parte A)

- **D1 — Estender o `enum class Key` existente, não criar um segundo enum.** Um vocabulário
  só pra biblioteca inteira. **Append-only**: os enumeradores atuais mantêm valor numérico
  (congelado por static_assert em teste); ~92 entradas novas (A–Z, Digit0–9, F1–F12,
  modificadores L/R, Super, locks, pontuação, World1/2, numpad) + sentinela `Count`.
  `KP_ENTER` continua dobrando em `Enter` (comportamento já documentado). **É a fatia maior
  do que parece** (~100 casos de switch + testes) — mas mecânica, um arquivo, um agente.
  Fatia mínima se o review apertar: cortar `Kp*` + locks (~17); letras/dígitos/F/modifiers
  NÃO são cortáveis (são o pedido literal do consumidor).
- **D2 — Tabela de estado PRÓPRIA alimentada pelos callbacks GLFW existentes, NÃO repasse de
  `glfwGetKey`.** Testabilidade (struct pura `InputState` testável headless), o HOSTIN-2
  precisa do tap de qualquer jeito, e reusa a tradução forward existente (sem mapa reverso).
  Estado atualizado ANTES do despacho ao sink; `GLFW_REPEAT` não re-edge.
- **D3 — Botão de mouse é `int` em `[0,7]`, SEM restrição a 3 botões** (requisito do líder,
  memória `feedback_input_multibutton_teclados`). A rota de UI mantém o contrato de 3 botões
  intocado. Limite honesto documentado: o PRÓPRIO GLFW não enxerga além de 8 botões — mouse
  de 12 botões é seed com gatilho (rota evdev, padrão do átomo gamepad). Fora de faixa →
  `false`, fail-high.
- **D4 — Layouts (requisito do líder: US-Intl + ABNT2).** Tokens GLFW são de POSIÇÃO física
  (semântica certa pra WASD, agnóstica de layout). US-Intl coberto integralmente; `ç` do
  ABNT2 chega como posição `Semicolon` (documentado); o segundo `/` do ABNT2 (scancode 97) é
  lacuna CONHECIDA do GLFW (`GLFW_KEY_UNKNOWN`) — documentada + seed `is_scancode_down`.
  `World1`/`World2` incluídos (a tecla ISO extra dos teclados ABNT2/europeus).
- **D5 — Canal sintético e físico SEPARADOS**: `process_event()` NÃO escreve a tabela física
  (replay não pode forjar tecla segurada). Documentado nos dois métodos e fixado por teste.
- **D6 — `request_close()` BYPASSA o veto** (quit programático é intencional; passar pelo
  veto travaria o "menu Sair" contra o próprio diálogo). O veto guarda só o close INICIADO
  PELO USUÁRIO (X/Alt+F4): callback `bool()`; `false` → janela fica aberta.
- **D7 — Prefixo `window_` nos nomes** pra não colidir com o `set_focus_callback` (DOM) já
  existente; queries de polling gêmeas via `glfwGetWindowAttrib`; honestidade headless
  documentada (sob Xvfb sem WM, iconify pode não ser honrado).
- **D8 — Throttle de log na NOSSA camada.** Origem verificada: `ElementText.cpp:86` do RmlUi
  → `SystemInterface::LogMessage`. Helper puro `src/log_dedup.hpp` + override no
  `SystemClock` (embed) + subclasse fina do `SystemInterface_GLFW` (App). Política: 1ª
  ocorrência imprime; repetidas suprimem; sumário em 10/100/1000...; erros NUNCA suprimidos;
  cap LRU (256) contra stream hostil.
- **D9 — vsync: documentar, não tocar.** Verificado: `glfwSwapInterval` não é chamado em
  lugar NENHUM do `src/` — o comportamento medido pelo consumidor é default do
  driver/compositor. Doc honesta em `render()`/`run()`; setter fica no WM-EXTRAS.
- **D10 — Roteamento UiEvent→RmlUi INALTERADO nesta onda**: as teclas novas entram no estado
  e no edge callback, mas o subconjunto encaminhado à UI é o mesmo de hoje (predicado
  fixado por teste). Encaminhar teclas estendidas à UI é mudança deliberada futura (seed).

### 3. Contrato de API, testes, gates, agentes e briefs

Normativos na Parte A (seções 3–7; o contrato de API é literal e os briefs estão
prontos-pra-colar em pt-br). Resumo operacional:

- **Resposta à pergunta "input físico sintético é testável headless?"**: em 3 níveis —
  (1) unidades puras (tabela de tradução congelada + `InputState` + política do dedup);
  (2) Xvfb automatizado (`app_input_query_smoke` incluindo `request_close` — que finalmente
  dá um teste REAL ao `run()` — e o e2e de veto via `XSendEvent WM_DELETE_WINDOW`,
  time-boxed com fallback pro seam puro); (3) leg MANUAL em **compositor aninhado**
  (Xephyr/weston --nested, xdotool/XTest, teclado BR carregado), executada pelo
  `qa-engineer`, NUNCA na sessão viva do líder.
- **4 agentes** (< 5, posse disjunta): implementer A (HOSTIN-1..5) → implementer B
  (LOGTHR-1; colide com A em `app.cpp`, roda depois) → technical-writer (DOC-HOSTIN;
  colide em `app.hpp`, roda depois de A) → qa-engineer adversarial que EXECUTA (mutation na
  tabela/política/veto + hostil + dominó + leg aninhada). Orquestrador re-verifica build
  limpo, claims file:line e `git log` (relatório de agent não é prova).
- **Gates**: build em `/var/tmp` (`TMPDIR=/var/tmp`), 1 build pesado por vez, `preci.sh`
  (clang-tidy+cppcheck+gitleaks), MSVC-safe (headers novos compilam no job MSVC embed),
  `link_smoke` embed verde (tudo aqui é App-only — nada vaza pro GLFW=OFF),
  `check_encapsulation.sh` (zero tipo/constante GLFW público), push/tag só com autorização
  explícita do líder, bump `project(VERSION 0.19.0)` + CHANGELOG + bloco de relay pro
  consumidor no fechamento.

### 4. Contra-argumentos do CTO (registro honesto)

1. **O enum de teclado É maior do que parece** (a suspeita do orquestrador procede): ~100
   entradas + tabela de tradução + testes de congelamento. Segue defensável em-onda porque é
   mecânico, um arquivo, e é o pedido literal do consumidor (WASD/letras/números/F1-F12).
   A fatia mínima (cortar numpad+locks) está nomeada caso o review derrape — mas NÃO
   recomendo cortar de antemão: o custo marginal é baixo e evita uma 3ª passada.
2. **ABNT2 pleno não fecha nesta onda por limite do GLFW, e não é vergonha**: a tecla
   scancode-97 não tem token upstream. A resposta certa é a documentação honesta + seed com
   gatilho (`is_scancode_down`), não uma rota evdev de teclado inteira agora (seria um átomo
   novo de input-sem-janela, custo M/G, zero demanda concreta).
3. **HOSTIN-2 (edge callback) DEVE entrar agora** (o item 2.1 perguntava): é irmão siamês do
   HOSTIN-1 (mesmo tap, mesmos arquivos, mesma disciplina AUD-TEC-3) — separá-lo dobraria o
   custo de review da superfície.
4. **Nada da onda deveria sair**: todos os itens vêm do inventário do consumidor, são P/M, e
   o conjunto fecha coeso e taggeável (v0.19.0 = "App mode desbloqueado pra produção do
   GusWorld"). O único corte que eu faria sob pressão é o e2e de veto por XSendEvent
   (downgrade documentado pro seam puro + leg manual) — nunca o veto em si.
