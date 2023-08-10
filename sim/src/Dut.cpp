#include "Dut.hpp"

using namespace cat;

Dut::Dut() {
  Verilated::traceEverOn(true);
  trace_ptr_ = std::make_unique<VerilatedVcdC>();
}

Dut::~Dut() { trace_ptr_->close(); }
