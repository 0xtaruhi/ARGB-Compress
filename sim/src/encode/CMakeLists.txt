add_executable(sim_encode
SimEncode.cpp
EncodeUnitDut.cpp
${SIM_COMMON_SRCS}
)

verilate(sim_encode SOURCES ${CMAKE_SOURCE_DIR}/rtl/verilog/EncodeUnit.v TRACE)
target_include_directories(sim_encode PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common)
