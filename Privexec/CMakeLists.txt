# privexec

add_executable(
  Privexec WIN32
  alias.cc
  capabilities.cc
  app.cc
  main.cc
  Privexec.manifest
  Privexec.rc)

if(PRIVEXEC_ENABLE_LTO)
  set_property(TARGET Privexec PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_libraries(
  Privexec
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

install(TARGETS Privexec DESTINATION .)
