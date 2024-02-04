cmake_minimum_required(VERSION 3.13)

set(INCLUDE_DIRS
  inc
)

add_library(QMI8658 STATIC
  src/QMI8658.c
) 

target_link_libraries(QMI8658 PUBLIC Config)
target_include_directories(QMI8658 PUBLIC ${INCLUDE_DIRS})