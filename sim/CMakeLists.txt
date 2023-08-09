find_package(verilator HINTS $ENV{VERILATOR_ROOT})

add_executable(sim src/sim.cc)
