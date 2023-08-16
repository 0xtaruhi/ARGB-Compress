cmake_minimum_required(VERSION 3.10)

project(argb_compress)

set(EXECUTABLE_NAME fblcd.out)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC -O0 -g -DIN_DEVELOP")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC -O3")

aux_source_directory(src SRC_LIST)
add_executable(${EXECUTABLE_NAME} ${SRC_LIST})
target_include_directories(${EXECUTABLE_NAME} PRIVATE include)

add_subdirectory(sim)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE sim_core_lib)
target_include_directories(${EXECUTABLE_NAME} PRIVATE sim/src/integrated)
