#ifndef ENCODE_UNIT_DUT_HPP
#define ENCODE_UNIT_DUT_HPP

#include "Dut.hpp"
#include "VEncodeUnit.h"
#include <string>

namespace cat {

class EncodeUnitDut : public Dut, public VEncodeUnit {
public:
  EncodeUnitDut() : VEncodeUnit(), Dut() {
    this->trace(trace_ptr_.get(), 99);
    this->openTraceFile("EncodeUnitTrace.vcd");
  }

  auto clockSignal() -> CData & override { return this->clk; }
  auto resetSignal() -> CData & override { return this->resetn; }

  auto resetActiveLevel() -> bool override { return false; }

  auto fallEdge() -> void override;
  auto riseEdge() -> void override;
};

} // namespace cat

#endif // ENCODE_UNIT_DUT_HPP
