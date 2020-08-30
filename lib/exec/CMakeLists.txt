# exec lib
add_library(Exec STATIC
    appcontainer.cc
    argv.cc
    elevator.cc
    exec.cc
    system.cc
)

target_link_libraries(Exec
  belawin
)
