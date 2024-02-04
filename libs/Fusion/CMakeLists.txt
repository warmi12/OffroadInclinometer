cmake_minimum_required(VERSION 3.13)

set(INCLUDE_DIRS
  inc
)

add_library(Fusion STATIC
  src/FusionAhrs.c
  src/FusionCompass.c
  src/FusionOffset.c
) 

target_compile_options(Fusion PRIVATE "-Wall")
target_include_directories(Fusion PUBLIC ${INCLUDE_DIRS})
