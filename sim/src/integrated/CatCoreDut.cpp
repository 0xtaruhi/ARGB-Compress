#include "CatCoreDut.hpp"
#include "CatLog.hpp"
#include <cassert>

using namespace cat;

CatCoreDut::CatCoreDut()
    : VCatCore(), Dut(),
      data_reg_bundles_(std::array<RegsBundle, 8>{
          RegsBundle(this->io_data_0_readData, this->io_data_0_writeData,
                     this->io_data_0_writeEnable),
          RegsBundle(this->io_data_1_readData, this->io_data_1_writeData,
                     this->io_data_1_writeEnable),
          RegsBundle(this->io_data_2_readData, this->io_data_2_writeData,
                     this->io_data_2_writeEnable),
          RegsBundle(this->io_data_3_readData, this->io_data_3_writeData,
                     this->io_data_3_writeEnable),
          RegsBundle(this->io_data_4_readData, this->io_data_4_writeData,
                     this->io_data_4_writeEnable),
          RegsBundle(this->io_data_5_readData, this->io_data_5_writeData,
                     this->io_data_5_writeEnable),
          RegsBundle(this->io_data_6_readData, this->io_data_6_writeData,
                     this->io_data_6_writeEnable),
          RegsBundle(this->io_data_7_readData, this->io_data_7_writeData,
                     this->io_data_7_writeEnable)}) {
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
  for (int i = 0; i != 8; ++i) {
    if (data_reg_bundles_[i].write_enable) {
      data_reg_[i] = data_reg_bundles_[i].write_data;
    }
  }

  for (int i = 0; i != 8; ++i) {
    data_reg_bundles_[i].read_data = data_reg_[i];
  }

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
  cat::CatLog::logDebug("Control code set to: " +
                       std::to_string(static_cast<uint8_t>(code)));
}

auto CatCoreDut::getStatus() -> StatusCode const {
  return static_cast<StatusCode>((cs_reg_ & 0x00FF0000) >> 16);
}
