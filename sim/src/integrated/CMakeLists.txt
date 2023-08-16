add_executable(sim_core
SimCore.cpp
CatCoreDut.cpp 
IntegratedSimulator.cpp
${SIM_COMMON_SRCS}
)

verilate(sim_core SOURCES ${CMAKE_SOURCE_DIR}/rtl/verilog/CatCore.v TRACE)
target_include_directories(sim_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common)

add_library(sim_core_lib STATIC
CatCoreDut.cpp
IntegratedSimulator.cpp
${SIM_COMMON_SRCS}
)

target_include_directories(sim_core_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common)
verilate(sim_core_lib SOURCES ${CMAKE_SOURCE_DIR}/rtl/verilog/CatCore.v TRACE)
