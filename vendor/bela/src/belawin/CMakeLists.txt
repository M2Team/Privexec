# bela win libaray

add_library(belawin STATIC
  env.cc
  path.cc
  realpath.cc
  reparsepoint.cc
  appexeclink.cc
  peimage.cc
)

target_link_libraries(belawin
  bela
)
