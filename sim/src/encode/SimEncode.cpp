#include <iostream>
#include <memory>
#include <string>

#include "CatLog.hpp"
#include "EncodeUnitDut.hpp"
#include "VirtualMemory.hpp"

class EncodeUnitSimulator {
public:
  EncodeUnitSimulator()
      : dut_(std::make_unique<cat::EncodeUnitDut>()), unencoded_memory_(512),
        encoded_memory_(512), hash_memory_(4096) {
    unencoded_memory_.newBlock(0x0);
    encoded_memory_.newBlock(0x0);
    hash_memory_.newBlock(0x0);
  }

  auto loadUnEncodedMemory(const std::string &filepath) -> bool {
    return unencoded_memory_.readFromFile(filepath);
  }

  auto run() -> void;

  static auto getInstance() -> EncodeUnitSimulator & {
    static EncodeUnitSimulator instance;
    return instance;
  }

  auto setEncodeLength(int encode_length) -> void {
    this->encode_length_ = encode_length;
  }

  auto dumpMemory() -> void {
    cat::CatLog::logInfo("Unencoded memory:");
    unencoded_memory_.dump();
    cat::CatLog::logInfo("Encoded memory:");
    encoded_memory_.dump();
    // cat::CatLog::logInfo("Hash memory:");
    // hash_memory_.dump();
  }

private:
  std::unique_ptr<cat::EncodeUnitDut> dut_;
  cat::VirtualMemory unencoded_memory_;
  cat::VirtualMemory encoded_memory_;
  cat::VirtualMemory hash_memory_;

  int encode_length_ = 0;
  vluint64_t max_time_ = 100000;
};

auto EncodeUnitSimulator::run() -> void {
  dut_->reset();

  dut_->io_control_reset = 1;
  dut_->tick();
  dut_->io_control_reset = 0;
  dut_->tick();

  dut_->io_encodeLength = this->encode_length_;

  auto startTime = dut_->getMainTime();
  dut_->io_control_start = 1;

  dut_->tick();
  dut_->io_control_start = 0;

  auto hash_memory_read_addr = dut_->io_hashMemory_read_address;
  auto unencoded_memory_read_addr_0 = dut_->io_unencodedMemory_0_read_address;
  auto unencoded_memory_read_addr_1 = dut_->io_unencodedMemory_1_read_address;

  auto encoded_memory_write_addr = dut_->io_encodedMemory_write_address;
  auto encoded_memory_write_data = dut_->io_encodedMemory_write_data;
  auto encoded_memory_write_mask = dut_->io_encodedMemory_write_mask;
  auto hash_memory_write_addr = dut_->io_hashMemory_write_address;
  auto hash_memory_write_data = dut_->io_hashMemory_write_data;
  auto hash_memory_write_enable = dut_->io_hashMemory_write_enable;

  while (true) {
    dut_->io_unencodedMemory_0_read_data =
        unencoded_memory_.read(unencoded_memory_read_addr_0);
    dut_->io_unencodedMemory_1_read_data =
        unencoded_memory_.read(unencoded_memory_read_addr_1);
    dut_->io_hashMemory_read_data =
        hash_memory_.read(hash_memory_read_addr << 2);

    dut_->fallEdge();

    hash_memory_read_addr = dut_->io_hashMemory_read_address;
    unencoded_memory_read_addr_0 = dut_->io_unencodedMemory_0_read_address;
    unencoded_memory_read_addr_1 = dut_->io_unencodedMemory_1_read_address;

    encoded_memory_write_addr = dut_->io_encodedMemory_write_address;
    encoded_memory_write_data = dut_->io_encodedMemory_write_data;
    encoded_memory_write_mask = dut_->io_encodedMemory_write_mask;
    hash_memory_write_addr = dut_->io_hashMemory_write_address;
    hash_memory_write_data = dut_->io_hashMemory_write_data;
    hash_memory_write_enable = dut_->io_hashMemory_write_enable;

    if (hash_memory_write_enable) {
      hash_memory_.write(hash_memory_write_addr << 2, hash_memory_write_data);
    }
    encoded_memory_.write(encoded_memory_write_addr, encoded_memory_write_data,
                          encoded_memory_write_mask);

    dut_->riseEdge();
    if (dut_->io_control_done) {
      break;
    }

    if (dut_->getMainTime() > max_time_) {
      cat::CatLog::logError("Timeout");
      break;
    }
  }

  std::string result = "Simulation finished in " +
                       std::to_string((dut_->getMainTime() - startTime) / 10) +
                       " cycles";
  cat::CatLog::logInfo(result);
  result = "Encoded length: " + std::to_string(dut_->io_encodedLength);
  cat::CatLog::logInfo(result);
}

auto simEncodeUnit(const std::string &filepath, int encode_length) {
  auto &sim = EncodeUnitSimulator::getInstance();
  sim.setEncodeLength(encode_length);
  sim.loadUnEncodedMemory(filepath);
  sim.run();
  sim.dumpMemory();
}

int main(int argc, char **argv) {
  cat::CatLog::setLogLevel(cat::CatLog::LogLevel::Info);

  int encode_length = std::stoi(argv[2]);
  simEncodeUnit(argv[1], encode_length);

  return 0;
}
