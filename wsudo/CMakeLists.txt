# privexec

add_executable(wsudo wsudo.cc wsudoalias.cc wsudo.rc wsudo.manifest)

if(PRIVEXEC_ENABLE_LTO)
  set_property(TARGET wsudo PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_libraries(
  wsudo
  ProcessCore
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

add_executable(wsudo-tie wsudo-tie.cc wsudo-tie.rc wsudo.manifest)

if(PRIVEXEC_ENABLE_LTO)
  set_property(TARGET wsudo-tie PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_libraries(
  wsudo-tie
  ProcessCore
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

install(TARGETS wsudo-tie DESTINATION .)
