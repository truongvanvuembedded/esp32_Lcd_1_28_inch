# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
include_directories(${CMAKE_SOURCE_DIR}/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Esp_Lcd_1_28Inch)