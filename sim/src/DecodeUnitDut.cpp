#include "DecodeUnitDut.hpp"

using namespace cat;

constexpr auto kTimeStep = 10;

auto DecodeUnitDut::tick() -> void {
  this->clockSignal() = 0;
  this->eval();
  trace_ptr_->dump(main_time_ + 1);

  this->clockSignal() = 1;
  this->eval();
  main_time_ += kTimeStep;
  trace_ptr_->dump(main_time_);
}

auto DecodeUnitDut::getCyclesNum() -> vluint64_t {
  return main_time_ / kTimeStep;
}

auto DecodeUnitDut::reset() -> void {
  this->resetSignal() = resetActiveLevel();
  this->tick(10);
  this->resetSignal() = !resetActiveLevel();
  this->tick(10);
}
