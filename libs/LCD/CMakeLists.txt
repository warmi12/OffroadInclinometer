cmake_minimum_required(VERSION 3.13)

set(INCLUDE_DIRS
  inc
)

add_library(LCD STATIC
  src/LCD_1in28.c
) 

target_link_libraries(LCD PUBLIC Config)
target_include_directories(LCD PUBLIC ${INCLUDE_DIRS})