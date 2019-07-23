# privexec

add_executable(edge_test 
    edge.cc
)

target_link_libraries(edge_test
    bela
    belawin
)

