#include "Dut.hpp"

using namespace cat;

Dut::Dut() {
  Verilated::traceEverOn(true);
  trace_ptr_ = std::make_unique<VerilatedVcdC>();
}

Dut::~Dut() { trace_ptr_->close(); }

auto Dut::tick() -> void {
  fallEdge();
  riseEdge();
}

auto Dut::tick(int cycles) -> void {
  for (int i = 0; i != cycles; ++i) {
    tick();
  }
}

auto Dut::getCyclesNum() const -> vluint64_t {
  return main_time_ / this->kTimeStep;
}

auto Dut::reset() -> void {
  this->resetSignal() = resetActiveLevel();
  this->tick(10);
  this->resetSignal() = !resetActiveLevel();
  this->tick(10);
}
