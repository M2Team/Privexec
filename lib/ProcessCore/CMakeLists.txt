

add_library(ProcessCore STATIC
    appcontainer.cc
    capabilities.cc
    exec.cc
    systemti.cc
)

target_link_libraries(ProcessCore
  belawin
)
