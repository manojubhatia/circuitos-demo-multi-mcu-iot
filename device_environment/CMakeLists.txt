cmake_minimum_required(VERSION 3.16)

# Set target chip
set(IDF_TARGET esp32c3)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(thespeaker_3_boards CXX C ASM)
