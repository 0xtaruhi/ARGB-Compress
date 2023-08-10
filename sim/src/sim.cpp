#include <iostream>

#include "CatLog.hpp"
#include "DecodeUnitDut.hpp"
#include "VirtualMemory.hpp"

int main() {
  cat::VirtualMemory vm(1024);
  cat::CatLog::setLogLevel(cat::CatLog::LogLevel::Debug);

  auto dut = std::make_unique<cat::DecodeUnitDut>();

  dut->reset();
  dut->tick(10);

  return 0;
}
