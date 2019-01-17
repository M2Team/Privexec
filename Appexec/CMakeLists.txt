# privexec

add_executable(Appexec WIN32
    appalias.cc
    apputils.cc
    capabilities.cc
    app.cc
    main.cc
    Appexec.manifest
    Appexec.rc
)

if(lto_supported)
  set_property(TARGET Appexec PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_libraries(Appexec
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


install(TARGETS Appexec
    DESTINATION bin
)
