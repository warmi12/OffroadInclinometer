set(LVGL_OUT_DIR ${CMAKE_SOURCE_DIR}/libs/lvgl)

# set(LV_CONF_PATH
#     ${CMAKE_SOURCE_DIR}/inc/lvgl/lv_conf.h
#     CACHE STRING "" FORCE)

include(FetchContent)

FetchContent_Declare(
  lvgl
  GIT_REPOSITORY https://github.com/lvgl/lvgl
  GIT_TAG        release/v8.1
  SOURCE_DIR     ${LVGL_OUT_DIR}/src
  BINARY_DIR     ${LVGL_OUT_DIR}/bin
)
set(FETCHCONTENT_BASE_DIR ${LVGL_OUT_DIR})

include_directories(${CMAKE_SOURCE_DIR}/inc/lvgl)

FetchContent_GetProperties(lvgl)
if (NOT lvgl_POPULATED)
    FetchContent_Populate(lvgl)
endif ()
add_subdirectory(${lvgl_SOURCE_DIR} ${lvgl_BINARY_DIR})
