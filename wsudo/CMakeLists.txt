# privexec

add_executable(wsudo
    wsudo.cc
    wsudoalias.cc
    wsudo.rc
	wsudo.manifest
)

if(lto_supported)
  set_property(TARGET wsudo PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_libraries(wsudo
    Comctl32
    Shlwapi
    Pathcch
    wtsapi32
    Userenv
    Taskschd
    Kernel32
    Gdi32
    Advapi32
)

install(TARGETS wsudo
    DESTINATION .
)
