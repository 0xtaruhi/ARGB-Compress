#ifndef INTEGRATED_SIMULATOR_HPP
#define INTEGRATED_SIMULATOR_HPP

#include "CatCoreDut.hpp"
#include "VirtualMemory.hpp"
#include <cstdint>
#include <memory>

namespace cat {

class IntegratedSimulator {

public:
  IntegratedSimulator();

  static auto getInstance() -> IntegratedSimulator & {
    static IntegratedSimulator instance;
    return instance;
  }

  using addr_t = VirtualMemory::addr_t;
  using mask_t = VirtualMemory::mask_t;

  auto run() -> void;
  auto dumpMemory() -> void const;

  auto runEncode() -> void;
  auto runDecode() -> void;

  auto loadUnencodedMemory(const std::string &filepath) -> bool {
    return original_memory_.readFromFile(filepath) &&
           unencoded_memory_.readFromFile(filepath);
  }

  auto getEncodeLength() -> int const { return encode_length_; }
  auto getEncodedLength() -> int const { return encoded_length_; }

  auto setEncodeLength(int encode_length) -> void {
    this->encode_length_ = encode_length;
  }

  auto setEncodedLength(int encoded_length) -> void {
    this->encoded_length_ = encoded_length;
  }

  auto writeUndecodedMemory(addr_t addr, uint32_t data, mask_t mask = 0b1111)
      -> void {
    undecoded_memory_.write(addr, data, mask);
  }

  auto readUndecodedMemory(addr_t addr) -> uint32_t {
    return undecoded_memory_.read(addr);
  }

  auto readHashMemory(addr_t addr) -> uint32_t {
    return hash_memory_.read(addr);
  }

  auto writeUnencodedMemory(addr_t addr, uint32_t data, mask_t mask = 0b1111)
      -> void {
    unencoded_memory_.write(addr, data, mask);
  }

  auto readUnencodedMemory(addr_t addr) -> uint32_t {
    return unencoded_memory_.read(addr);
  }

  auto writeHashMemory(addr_t addr, uint32_t data, mask_t mask = 0b1111)
      -> void {
    hash_memory_.write(addr, data, mask);
  }

  auto tick() -> void;

  auto checkEqual() -> bool const;

  auto reset() -> void;

  auto getDecodeCyclesNum() -> uint64_t const {
    return (decode_end_time_ - decode_start_time_) / dut_->kTimeStep;
  }

  auto getEncodeCyclesNum() -> uint64_t const {
    return (encode_end_time_ - encode_start_time_) / dut_->kTimeStep;
  }

private:
  auto waitUntilDone() -> void;
  auto returnToIdle() -> void;

private:
  std::unique_ptr<CatCoreDut> dut_;
  cat::VirtualMemory original_memory_;
  cat::VirtualMemory unencoded_memory_;
  cat::VirtualMemory undecoded_memory_;
  cat::VirtualMemory hash_memory_;

  int encode_length_ = 0;
  int encoded_length_ = 0;
  int decoded_length_ = 0;

  SData unencoded_memory_read_addr_0_;
  SData unencoded_memory_read_addr_1_;
  SData undecoded_memory_read_addr_;
  SData hash_memory_read_addr_;

  vluint64_t start_time_ = 0;
  vluint64_t encode_start_time_ = 0;
  vluint64_t encode_end_time_ = 0;
  vluint64_t decode_start_time_ = 0;
  vluint64_t decode_end_time_ = 0;
  vluint64_t end_time_ = 0;
};

}; // namespace cat

#endif // INTEGRATED_SIMULATOR_HPP
