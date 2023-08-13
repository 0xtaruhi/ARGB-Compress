find_package(verilator HINTS $ENV{VERILATOR_ROOT})

add_subdirectory(common)
add_subdirectory(encode)
add_subdirectory(decode)
add_subdirectory(integrated)