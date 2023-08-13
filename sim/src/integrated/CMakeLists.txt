add_executable(sim_core
SimCore.cpp
CatCoreDut.cpp 
IntegratedSimulator.cpp
${SIM_COMMON_SRCS}
)

verilate(sim_core SOURCES ${CMAKE_SOURCE_DIR}/rtl/verilog/CatCore.v TRACE)
target_include_directories(sim_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common)
