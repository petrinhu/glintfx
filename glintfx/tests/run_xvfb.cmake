# SPDX-License-Identifier: MPL-2.0
execute_process(COMMAND xvfb-run -a ${EXE} RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "test failed rc=${rc}")
endif()
