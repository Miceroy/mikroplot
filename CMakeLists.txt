#//// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= ////
#//// MikRoPlot - C++ Plotting made easy.
#////
#//// MIT Licanse
#////
#//// Copyright (c) 2022 Mikko Romppainen.
#////
#//// Permission is hereby granted, free of charge, to any person obtaining
#//// a copy of this software and associated documentation files (the
#//// "Software"), to deal in the Software without restriction, including
#//// without limitation the rights to use, copy, modify, merge, publish,
#//// distribute, sublicense, and/or sell copies of the Software, and to
#//// permit persons to whom the Software is furnished to do so, subject to
#//// the following conditions:
#////
#//// The above copyright notice and this permission notice shall be included
#//// in all copies or substantial portions of the Software.
#////
#//// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#//// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#//// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#//// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#//// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#//// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#//// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#//// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= ////

# Specify minimum supported cmake version
cmake_minimum_required(VERSION 3.16)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use Interprocedural optimization: Interprocedural optimization (IPO) is a collection
# of compiler techniques used in computer programming to improve performance in
# programs containing many frequently used functions of small or medium length.
# IPO differs from other compiler optimization because it analyzes the entire
# program; other optimizations look at only a single function, or even a single
# block of code.
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
# visibility=hidden by default
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

##
## Dependencies
##
# add_subdirectorys to be CMaked
option(GLFW_BUILD_EXAMPLES "" OFF)
option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
add_subdirectory("./ext/glfw-3.3.6")
# Specify include directories
include_directories("ext/glfw-3.3.6/include")
include_directories("ext/stb-master")
include_directories("ext/miniaudio")
include_directories("ext/freeglut-3.2.1/include")
include_directories("${GLFW_SOURCE_DIR}/deps")
include_directories("include")

# Define glad source files
set(GLAD_GL "${GLFW_SOURCE_DIR}/deps/glad/gl.h" "${GLFW_SOURCE_DIR}/deps/glad_gl.c")

## Project mikroplot
project (mikroplot)
option(MIKROPLOT_BUILD_EXAMPLES "Build MikRoPlot examples" ON )

file(GLOB_RECURSE MIKROPLOT_INC_FILES "./include/mikroplot/*.h")
file(GLOB_RECURSE MIKROPLOT_SRC_FILES "./src/*.cpp")

add_library(mikroplot "ext/miniaudio/miniaudio.h" ${MIKROPLOT_SRC_FILES} ${MIKROPLOT_INC_FILES} ${GLAD_GL})
target_link_libraries(mikroplot PUBLIC glfw)
target_include_directories(mikroplot PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

if(MIKROPLOT_BUILD_EXAMPLES)
    add_executable (mikroplot_demo examples/main_mikroplot_demo.cpp)
    target_link_libraries(mikroplot_demo PUBLIC mikroplot)
endif()


