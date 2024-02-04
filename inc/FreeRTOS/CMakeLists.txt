cmake_minimum_required(VERSION 3.13)

set(INCLUDE_DIRS
  .
)

add_library(FreeRTOS-Config INTERFACE)

target_include_directories(FreeRTOS-Config  SYSTEM INTERFACE
    ${INCLUDE_DIRS}
)

target_compile_definitions(FreeRTOS-Config 
  INTERFACE
    projCOVERAGE_TEST=0
)