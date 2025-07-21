cmake_minimum_required(VERSION 3.30)
project(sdk)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
set(CMAKE_CXX_FLAGS /bigobj)

if (MSVC)
  add_compile_options(/EHsc)
endif()

add_subdirectory(external/libhat)

add_library(sdk STATIC 
  src/finder.cpp
)

target_include_directories(sdk PUBLIC include)
target_link_libraries(sdk PUBLIC libhat)