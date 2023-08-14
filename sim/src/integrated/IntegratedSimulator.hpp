#ifndef INTEGRATED_SIMULATOR_HPP
#define INTEGRATED_SIMULATOR_HPP

#include "CatCoreDut.hpp"
#include "VirtualMemory.hpp"
#include <memory>

namespace cat {

class IntegratedSimulator {

public:
  IntegratedSimulator();

  static auto getInstance() -> IntegratedSimulator & {
    static IntegratedSimulator instance;
    return instance;
  }

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

  auto tick() -> void;

  auto checkEqual() -> bool const;

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
