# SPDX-License-Identifier: MPL-2.0
add_executable(window_smoke window_smoke.cpp ${CMAKE_SOURCE_DIR}/src/window_glfw.cpp)
target_link_libraries(window_smoke PRIVATE glfw OpenGL::GL gl3w)
target_include_directories(window_smoke PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME window_smoke COMMAND ${CMAKE_COMMAND}
  -DEXE=$<TARGET_FILE:window_smoke> -P ${CMAKE_SOURCE_DIR}/tests/run_xvfb.cmake)
