find_package(verilator HINTS $ENV{VERILATOR_ROOT})

add_executable(sim
src/sim.cpp
src/CatLog.cpp 
src/VirtualMemory.cpp
src/DecodeUnitDut.cpp
src/Dut.cpp
src/SimDecode.cpp
)

verilate(sim SOURCES ${CMAKE_SOURCE_DIR}/rtl/verilog/DecodeUnit.v TRACE)

target_include_directories(sim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
