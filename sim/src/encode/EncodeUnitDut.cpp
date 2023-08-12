#include "EncodeUnitDut.hpp"

using namespace cat;

auto EncodeUnitDut::fallEdge() -> void {
  this->clockSignal() = 0;
  this->eval();
  trace_ptr_->dump(main_time_ + 1);
}

auto EncodeUnitDut::riseEdge() -> void {
  this->clockSignal() = 1;
  this->eval();
  main_time_ += kTimeStep;
  trace_ptr_->dump(main_time_);
}