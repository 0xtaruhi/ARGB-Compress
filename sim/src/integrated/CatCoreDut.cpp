#include "CatCoreDut.hpp"
#include "CatLog.hpp"
#include <cassert>

using namespace cat;

CatCoreDut::CatCoreDut() : VCatCore(), Dut() {
  this->trace(trace_ptr_.get(), 99);
  this->openTraceFile("CatCoreTrace.vcd");
}

auto CatCoreDut::fallEdge() -> void {
  this->clockSignal() = 0;
  this->eval();
  trace_ptr_->dump(main_time_ + 1);
  regsSync();
}

auto CatCoreDut::riseEdge() -> void {
  this->clockSignal() = 1;
  this->eval();
  main_time_ += kTimeStep;
  trace_ptr_->dump(main_time_);
}

auto CatCoreDut::regsSync() -> void {
  // Data Regs
  if (this->io_dataReg_writeEnable) {
    setDataReg(this->io_dataReg_writeData, this->io_dataReg_regIndex);
  }

  this->io_dataReg_readData =
      static_cast<SData>(getDataReg(this->io_dataReg_regIndex));

  // CS Reg
  cs_reg_ =
      cs_reg_ & 0xFF00FFFF | (static_cast<uint8_t>(this->io_status) << 16);
  if (this->io_info_writeEnable) {
    setInfo(this->io_info_writeData);
  }

  this->io_control = static_cast<CData>((cs_reg_ & 0xFF000000) >> 24);
  this->io_info_readData = static_cast<SData>(cs_reg_ & 0x0000FFFF);
}

auto CatCoreDut::setInfo(uint16_t info) -> void {
  cs_reg_ = cs_reg_ & 0xFFFF0000 | info;
  this->io_info_readData = cs_reg_ & 0xff;
  cat::CatLog::logDebug("Info set to: " + std::to_string(info));
}

auto CatCoreDut::getInfo() -> uint16_t const {
  return static_cast<uint16_t>(cs_reg_ & 0x0000FFFF);
}

auto CatCoreDut::setDataReg(uint32_t data, uint8_t reg_index) -> void {
  assert(reg_index < 8 && "Register index out of range");
  data_reg_[reg_index] = data;
}

auto CatCoreDut::getDataReg(uint8_t reg_index) -> uint32_t const {
  assert(reg_index < 8 && "Register index out of range");
  return data_reg_[reg_index];
}

auto CatCoreDut::setControlCode(ControlCode code) -> void {
  cs_reg_ = cs_reg_ & 0x00FFFFFF | (static_cast<uint8_t>(code) << 24);
  this->io_control = static_cast<CData>((cs_reg_ & 0xFF000000) >> 24);
  cat::CatLog::logDebug("Control code set to: " +
                        std::to_string(static_cast<uint8_t>(code)));
}

auto CatCoreDut::getControlCode() -> ControlCode const {
  return static_cast<ControlCode>((cs_reg_ & 0xFF000000) >> 24);
}

auto CatCoreDut::getStatus() -> StatusCode const {
  return static_cast<StatusCode>(this->io_status);
}
