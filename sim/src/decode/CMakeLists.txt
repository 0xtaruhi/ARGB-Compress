add_executable(sim_decode
SimDecode.cpp
DecodeUnitDut.cpp
${SIM_COMMON_SRCS}
)

verilate(sim_decode SOURCES ${CMAKE_SOURCE_DIR}/rtl/verilog/DecodeUnit.v TRACE)
target_include_directories(sim_decode PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common)
