#include "IntegratedSimulator.hpp"
#include "CatCoreDut.hpp"
#include "CatLog.hpp"

#include <iostream>
#include <memory>

using namespace cat;

IntegratedSimulator::IntegratedSimulator()
    : dut_(std::make_unique<CatCoreDut>()), unencoded_memory_(512),
      undecoded_memory_(512), hash_memory_(16384) {
  unencoded_memory_.newBlock(0x0);
  undecoded_memory_.newBlock(0x0);
  hash_memory_.newBlock(0x0);
}

auto IntegratedSimulator::run() -> void {
  start_time_ = dut_->getMainTime();
  dut_->setControlCode(CatCoreDut::ControlCode::Idle);
  dut_->setInfo(0x0);
  dut_->reset();
  runEncode();
  runDecode();
  end_time_ = dut_->getMainTime();

  auto encode_cycles =
      (encode_end_time_ - encode_start_time_) / dut_->kTimeStep;
  auto decode_cycles =
      (decode_end_time_ - decode_start_time_) / dut_->kTimeStep;
  auto total_cycles = (end_time_ - start_time_) / dut_->kTimeStep;

  CatLog::logInfo("========= Final result =========");
  CatLog::logInfo("Encode cycles: " + std::to_string(encode_cycles));
  CatLog::logInfo("Decode cycles: " + std::to_string(decode_cycles));
  CatLog::logInfo("Total cycles: " + std::to_string(total_cycles));
}

auto IntegratedSimulator::runEncode() -> void {
  if (!dut_->isIdle()) {
    CatLog::logError("Cannot run encode when the core is not idle.");
    return;
  }

  encode_start_time_ = dut_->getMainTime();
  dut_->setControlCode(CatCoreDut::ControlCode::Encode);
  dut_->setInfo(encode_length_);
  waitUntilDone();
  
  encode_end_time_ = dut_->getMainTime();
  encoded_length_ = dut_->getInfo();
  CatLog::logInfo("Encoded length: " + std::to_string(encoded_length_));
  returnToIdle();
}

auto IntegratedSimulator::runDecode() -> void {
  if (!dut_->isIdle()) {
    CatLog::logError("Cannot run decode when the core is not idle.");
    return;
  }

  decode_start_time_ = dut_->getMainTime();
  dut_->setControlCode(CatCoreDut::ControlCode::Decode);
  dut_->setInfo(encoded_length_);
  waitUntilDone();
  decoded_length_ = dut_->getInfo();
  CatLog::logInfo("Decoded length: " + std::to_string(decoded_length_));
  returnToIdle();
  decode_end_time_ = dut_->getMainTime();
}

auto IntegratedSimulator::dumpMemory() -> void const {
  cat::CatLog::logInfo("Unencoded memory:");
  unencoded_memory_.dump();
  cat::CatLog::logInfo("Undecoded memory:");
  undecoded_memory_.dump();
  cat::CatLog::logInfo("Hash memory:");
  hash_memory_.dump();
}

auto IntegratedSimulator::waitUntilDone() -> void {
  CatLog::logInfo("Waiting until done...");
  while (!dut_->isDone()) {
    tick();
  }
  CatLog::logInfo("Done.");
}

auto IntegratedSimulator::returnToIdle() -> void {
  if (dut_->isBusy()) {
    CatLog::logError("Cannot return to idle when the core is busy.");
    return;
  }
  dut_->setControlCode(CatCoreDut::ControlCode::ReturnToIdle);

  while (!dut_->isIdle()) {
    tick();
  }
  CatLog::logInfo("Returned to idle.");
  dut_->setControlCode(CatCoreDut::ControlCode::Idle);
}

auto IntegratedSimulator::tick() -> void {
  dut_->fallEdge();
  hash_memory_read_addr_ = dut_->io_hashMemory_read_address;
  unencoded_memory_read_addr_0_ = dut_->io_unencodedMemory_0_read_address;
  unencoded_memory_read_addr_1_ = dut_->io_unencodedMemory_1_read_address;
  undecoded_memory_read_addr_ = dut_->io_undecodedMemory_read_address;

  if (dut_->io_hashMemory_write_enable) {
    hash_memory_.write(dut_->io_hashMemory_write_address << 2,
                       dut_->io_hashMemory_write_data);
  }
  unencoded_memory_.write(dut_->io_unencodedMemory_0_write_address,
                          dut_->io_unencodedMemory_0_write_data,
                          dut_->io_unencodedMemory_0_write_mask);
  undecoded_memory_.write(dut_->io_undecodedMemory_write_address,
                          dut_->io_undecodedMemory_write_data,
                          dut_->io_undecodedMemory_write_mask);

  dut_->riseEdge();
  dut_->io_unencodedMemory_0_read_data =
      unencoded_memory_.read(unencoded_memory_read_addr_0_);
  dut_->io_unencodedMemory_1_read_data =
      unencoded_memory_.read(unencoded_memory_read_addr_1_);
  dut_->io_undecodedMemory_read_data =
      undecoded_memory_.read(undecoded_memory_read_addr_);
  dut_->io_hashMemory_read_data =
      hash_memory_.read(hash_memory_read_addr_ << 2);
}
