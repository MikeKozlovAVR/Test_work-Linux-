cmake_minimum_required(VERSION 3.16.3)

project(math_model_cmake)




file(MAKE_DIRECTORY applications)




find_package (Threads)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/applications)
add_executable(math_model source/math_model.cpp)
add_executable(control_system source/control_system.cpp)
add_executable(logger source/logger.cpp)


