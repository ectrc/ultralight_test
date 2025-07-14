cmake_minimum_required(VERSION 3.30)
project(ul_test)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
set(CMAKE_CXX_FLAGS /bigobj)

if (MSVC)
  add_compile_options(/EHsc)
endif()

add_subdirectory(external/libhat)
add_subdirectory(external/minhook)

add_library(ul_test SHARED 
  library.cpp 
  src/render.cpp
  src/view.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_demo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_dx11.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_win32.cpp
)

target_include_directories(ul_test PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}/include 
  ${CMAKE_CURRENT_SOURCE_DIR}/src

  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends

  ${CMAKE_CURRENT_SOURCE_DIR}/external/ultralight/include
)
set(UL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/ultralight/lib")

target_link_libraries(ul_test PUBLIC 
  libhat 
  d3d11 
  minhook
  ${UL_LIB_DIR}/AppCore.lib
  ${UL_LIB_DIR}/Ultralight.lib
  ${UL_LIB_DIR}/UltralightCore.lib
  ${UL_LIB_DIR}/WebCore.lib
)