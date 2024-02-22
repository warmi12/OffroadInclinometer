cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_SDK_FETCH_FROM_GIT on)
set(FREERTOS_KERNEL_PATH "${CMAKE_SOURCE_DIR}/libs/FreeRTOS")

include(${CMAKE_SOURCE_DIR}/libs/pico-sdk/pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(offroad C CXX ASM)

add_subdirectory(${CMAKE_SOURCE_DIR}/libs/Fusion Fusion)
add_subdirectory(${CMAKE_SOURCE_DIR}/inc/FreeRTOS FreeRTOS-Config)
add_subdirectory(${CMAKE_SOURCE_DIR}/libs/LCD LCD)
add_subdirectory(${CMAKE_SOURCE_DIR}/libs/Config Config)
add_subdirectory(${CMAKE_SOURCE_DIR}/libs/QMI8658 QMI8658)
add_subdirectory(${CMAKE_SOURCE_DIR}/libs/lvgl lvgl)

pico_sdk_init()

set(GLOBAL_DEFINES
    PICO_STACK_SIZE=4096
    #PICO_HEAP_SIZE=16384
)

add_executable(${CMAKE_PROJECT_NAME}
    ${CMAKE_SOURCE_DIR}/src/main.c
    ${CMAKE_SOURCE_DIR}/src/IMUTask/IMUTask.c
    ${CMAKE_SOURCE_DIR}/src/DisplayTask/DisplayTask.c
    ${CMAKE_SOURCE_DIR}/src/DisplayTask/LvglProcess.c
    ${CMAKE_SOURCE_DIR}/src/DisplayTask/ImageData.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/inc/IMUTask
    ${CMAKE_SOURCE_DIR}/inc/DisplayTask
    ${CMAKE_SOURCE_DIR}/inc/lvgl
    ${CMAKE_SOURCE_DIR}/libs/lvgl/src
    ${CMAKE_SOURCE_DIR}/inc/ImageData

)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ${GLOBAL_DEFINES})
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra)
target_link_libraries(${CMAKE_PROJECT_NAME} FreeRTOS-Kernel FreeRTOS-Kernel-Heap3 FreeRTOS-Config Fusion LCD Config QMI8658 lvgl)

pico_enable_stdio_usb(${CMAKE_PROJECT_NAME} 1)
pico_enable_stdio_uart(${CMAKE_PROJECT_NAME} 1)
pico_add_extra_outputs(${CMAKE_PROJECT_NAME})