# exec lib
add_library(exec STATIC
    appcontainer.cc
    argv.cc
    elevator.cc
    exec.cc
    system.cc
)

target_link_libraries(exec
  belawin
)
