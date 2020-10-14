# privexec

add_executable(wsudo delegate.cc wsudo.cc wsudoalias.cc wsudo.rc wsudo.manifest)

if(PRIVEXEC_ENABLE_LTO)
  set_property(TARGET wsudo PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_libraries(
  wsudo
  Exec
  Exec
  Comctl32
  Shlwapi
  Pathcch
  wtsapi32
  Userenv
  Taskschd
  Kernel32
  Gdi32
  Advapi32)

install(TARGETS wsudo DESTINATION .)

add_executable(wsudo-bridge wsudo-bridge.cc wsudo-bridge.trace.cc wsudo-bridge.rc wsudo.manifest)

if(PRIVEXEC_ENABLE_LTO)
  set_property(TARGET wsudo-bridge PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_libraries(
  wsudo-bridge
  Exec
  belashl
  Comctl32
  Shlwapi
  Pathcch
  wtsapi32
  Userenv
  Taskschd
  Kernel32
  Gdi32
  Advapi32)

install(TARGETS wsudo-bridge DESTINATION .)
