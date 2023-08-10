#ifndef DECODE_UNIT_DUT_HPP
#define DECODE_UNIT_DUT_HPP

#include "Dut.hpp"
#include "VDecodeUnit.h"
#include <string>

namespace cat {

class DecodeUnitDut : public Dut, public VDecodeUnit {
public:
  DecodeUnitDut() : VDecodeUnit(), Dut() {
    this->trace(trace_ptr_.get(), 99);
    this->openTraceFile("DecodeUnitTrace.vcd");
  }

  auto clockSignal() -> CData & override { return this->clk; }
  auto resetSignal() -> CData & override { return this->resetn; }

  auto resetActiveLevel() -> bool override { return false; }

  auto tick() -> void;
  auto tick(int n) -> void {
    for (int i = 0; i < n; i++) {
      this->tick();
    }
  }
  auto reset() -> void;

  auto getCyclesNum() -> vluint64_t;

private:
};

} // namespace cat

#endif // DECODE_UNIT_DUT_HPP
