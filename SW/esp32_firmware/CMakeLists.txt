# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(PROJECT_VER "VERSION_0.207a")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wm210e_esp32)

list(APPEND EXTRA_COMPONENT_DIRS components)
