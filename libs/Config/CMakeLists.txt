cmake_minimum_required(VERSION 3.13)

set(INCLUDE_DIRS
  inc
  ${CMAKE_SOURCE_DIR}/inc/IMUTask
)

add_library(Config STATIC
  src/DEV_Config.c
) 

target_link_libraries(Config PUBLIC pico_stdlib hardware_spi hardware_i2c hardware_pwm hardware_adc hardware_dma)
target_include_directories(Config PUBLIC ${INCLUDE_DIRS})
