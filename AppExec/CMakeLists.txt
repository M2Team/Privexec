# privexec

add_executable(AppExec WIN32
    appsettings.cc
    apputils.cc
    capabilities.cc
    app.cc
    appx.cc
    main.cc
    Appexec.manifest
    Appexec.rc
)

if(lto_supported)
  set_property(TARGET AppExec PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

target_link_libraries(AppExec
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

install(TARGETS AppExec
    DESTINATION .
)
