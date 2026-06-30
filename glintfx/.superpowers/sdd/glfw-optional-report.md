# glfw-optional-report — patch v0.2.1

**Branch:** `fix/glfw-optional-backend`
**Data:** 2026-06-30
**Agent:** backend-engineer

## Resumo executivo

GLFW agora é opcional no build do glintfx. A opção `GLINTFX_BACKEND_GLFW` (default `ON`) controla se o backend de janela GLFW + `glintfx::App` standalone são compilados. Em modo `OFF`, a lib é embed-only (`UiLayer` + Engine + RenderGl3), sem nenhuma referência a glfw na lib.

## Resultados dos builds

### Build ON (GLINTFX_BACKEND_GLFW=ON) - padrão

```
cmake -S glintfx -B glintfx/build -DGLINTFX_BUILD_TESTS=ON -DGLINTFX_BACKEND_GLFW=ON
cmake --build glintfx/build --parallel 4
ctest --test-dir glintfx/build --output-on-failure -j1
```

Resultado: **10/10 VERDE**

```
 1/10 window_smoke    Passed  3.30s
 2/10 render_smoke    Passed  3.26s
 3/10 engine_smoke    Passed  3.23s
 4/10 app_smoke       Passed  3.25s
 5/10 render_sanity   Passed  3.29s
 6/10 ui_layer_attach Passed  3.28s
 7/10 ui_layer_compose Passed 3.27s
 8/10 gl_state_guard  Passed  3.27s
 9/10 ui_layer_events Passed  3.27s
10/10 ui_layer_sanity Passed  3.28s
Total: 32.70s
```

### Build OFF (GLINTFX_BACKEND_GLFW=OFF) - embed-only

```
cmake -S glintfx -B glintfx/build-noglfw -DGLINTFX_BUILD_TESTS=ON -DGLINTFX_BACKEND_GLFW=OFF
cmake --build glintfx/build-noglfw --parallel 4
ctest --test-dir glintfx/build-noglfw --output-on-failure -j1
```

Resultado: **5/5 VERDE** (testes App/window excluidos conforme esperado)

```
1/5 ui_layer_attach  Passed  3.26s
2/5 ui_layer_compose Passed  3.26s
3/5 gl_state_guard   Passed  3.28s
4/5 ui_layer_events  Passed  3.26s
5/5 ui_layer_sanity  Passed  3.30s
Total: 16.36s
```

### Verificacao de ausencia de glfw na lib (build OFF)

```
nm -u build-noglfw/libglintfx.a | grep -i glfw | wc -l
=> 0
```

Simbolos glfw undefined na libglintfx.a em build OFF: **0** (confirmado limpo).

Comparacao: build ON tem 27 refs glfw na lib (esperado, `App`/`WindowGlfw` compilados nela).

## Arquivos modificados

| Arquivo | Tipo | Descricao |
|---|---|---|
| `glintfx/include/glintfx/config.hpp.in` | NOVO | Header gerado com `#cmakedefine01 GLINTFX_BACKEND_GLFW` |
| `glintfx/include/glintfx/glintfx.hpp` | EDITADO | Guard `#if GLINTFX_BACKEND_GLFW` em torno de `app.hpp` |
| `glintfx/CMakeLists.txt` | EDITADO | `option(GLINTFX_BACKEND_GLFW ON)`, sources e links condicionais, `configure_file` do config.hpp |
| `glintfx/cmake/glintfxConfig.cmake.in` | EDITADO | `find_dependency(glfw3)` e `glfw` em INTERFACE_LINK_LIBRARIES condicionais via `@GLINTFX_BACKEND_GLFW@` |
| `glintfx/tests/CMakeLists.txt` | EDITADO | Testes App/GLFW em `if(GLINTFX_BACKEND_GLFW)`, helper `_glintfx_test_ctx` para testes embed em OFF |
| `docs/adr/0008-embed-guest-mode.md` | EDITADO | Nova secao "Optional GLFW build" documentando a opcao e a estrategia de teste |
| `README.md` | EDITADO | Limitacao "GLFW only" substituida por "GLFW backend opcional" (EN + PT) |
| `CHANGELOG.md` | EDITADO | Entrada `[Unreleased]` com Added + Fixed |

## Invariantes verificados

- [x] Build ON: comportamento identico ao anterior; 10/10 verde
- [x] Build OFF: `libglintfx.a` tem 0 simbolos glfw undefined
- [x] Build OFF: testes embed passam usando `_glintfx_test_ctx` (GLFW so-de-teste)
- [x] Build OFF: testes App/window excluidos do ctest (nao falham por ausencia)
- [x] `config.hpp` gerado na build tree; instalavel com `install(FILES ...)`
- [x] `glintfxConfig.cmake.in` propaga a opcao via `@GLINTFX_BACKEND_GLFW@`
- [x] `glintfx.hpp` inclui `app.hpp` somente quando `GLINTFX_BACKEND_GLFW=1`
- [x] SPDX MPL-2.0 em todos os arquivos novos/editados
- [x] Identificadores en-intl; comentarios e docs bilIngues
