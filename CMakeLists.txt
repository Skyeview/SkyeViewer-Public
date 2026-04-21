cmake_minimum_required(VERSION 3.15)
project(SkyeviewPopup LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(skyeview WIN32 main.cpp resource.rc)
add_executable(skyeview-mal WIN32 main1.cpp resource.rc)

if(MSVC)
  target_compile_options(skyeview PRIVATE /W4 /permissive-)
endif()

target_link_libraries(skyeview PRIVATE gdiplus ole32 winmm)
