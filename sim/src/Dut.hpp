#ifndef DUT_HPP
#define DUT_HPP

#include <memory>
#include <string>
#include <verilated.h>
#include <verilated_vcd_c.h>

namespace cat {

class Dut {
public:
  Dut();
  virtual ~Dut();

  virtual auto clockSignal() -> CData & = 0;
  virtual auto resetSignal() -> CData & = 0;
  virtual auto resetActiveLevel() -> bool = 0;

  auto openTraceFile(const std::string &trace_filename) -> void {
    trace_ptr_->open(trace_filename.c_str());
  }

  auto resetActive() -> bool { return resetSignal() == resetActiveLevel(); }

  auto dumpTrace() -> void { trace_ptr_->dump(main_time_); }
protected:
  std::unique_ptr<VerilatedVcdC> trace_ptr_;
  vluint64_t main_time_ = 0;
};

} // namespace cat

#endif // DUT_HPP