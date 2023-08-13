#include <string>

#include "CatLog.hpp"
#include "DecodeUnitDut.hpp"
#include "VirtualMemory.hpp"

class DecodeUnitSimulator {
public:
  DecodeUnitSimulator()
      : dut_(std::make_unique<cat::DecodeUnitDut>()), undecoded_memory_(512),
        decodedMemory_(512) {}

  auto loadUndecodedMemory(const std::string &filepath) -> bool {
    return undecoded_memory_.readFromFile(filepath);
  }

  auto run() -> void;

  auto dumpMemory() -> void {
    cat::CatLog::logInfo("Undecoded memory:");
    undecoded_memory_.dump();
    cat::CatLog::logInfo("Decoded memory:");
    decodedMemory_.dump();
  }

  auto setDecodeLength(int decode_length) -> void {
    this->decode_length_ = decode_length;
  }

private:
  std::unique_ptr<cat::DecodeUnitDut> dut_;
  cat::VirtualMemory undecoded_memory_;
  cat::VirtualMemory decodedMemory_;

  int decode_length_ = 0;

  vluint64_t max_time_ = 10000;
};

auto DecodeUnitSimulator::run() -> void {
  dut_->reset();

  dut_->io_control_reset = 1;
  dut_->tick();
  dut_->io_control_reset = 0;
  dut_->tick();

  dut_->io_decodeLength = this->decode_length_;
  auto undecoded_memory_read_addr = dut_->io_undecodedMemory_read_address;
  dut_->io_undecodedMemory_read_data =
      undecoded_memory_.read(undecoded_memory_read_addr);
  dut_->io_control_start = 1;

  auto startTime = dut_->getMainTime();

  while (true) {
    auto undecoded_memory_read_addr = dut_->io_undecodedMemory_read_address;
    auto decoded_memory_read_addr = dut_->io_decodedMemory_read_address;

    dut_->fallEdge();
    if (dut_->io_control_done) {
      break;
    }

    if (dut_->getMainTime() > max_time_) {
      cat::CatLog::logError("Timeout");
      break;
    }

    auto decoded_memory_write_addr = dut_->io_decodedMemory_write_address;
    auto decoded_memory_write_data = dut_->io_decodedMemory_write_data;
    auto decoded_memory_write_mask = dut_->io_decodedMemory_write_mask;
    decodedMemory_.write(decoded_memory_write_addr, decoded_memory_write_data,
                         decoded_memory_write_mask);

    dut_->riseEdge();

    dut_->io_undecodedMemory_read_data =
        undecoded_memory_.read(undecoded_memory_read_addr);
    dut_->io_decodedMemory_read_data =
        decodedMemory_.read(decoded_memory_read_addr);
  }

  std::string result = "Simulation finished in " +
                       std::to_string((dut_->getMainTime() - startTime) / 10) +
                       " cycles";

  cat::CatLog::logInfo(result);
}

auto simDecodeUnit(const std::string &filepath, int decode_length) -> void {
  DecodeUnitSimulator sim;
  sim.setDecodeLength(decode_length);
  sim.loadUndecodedMemory(filepath);
  sim.run();
  // uncomment to dump memory
  sim.dumpMemory();
}

int main(int argc, char** argv) {
  cat::CatLog::setLogLevel(cat::CatLog::LogLevel::Debug);

  int decode_length = std::stoi(argv[2]);

  simDecodeUnit(argv[1], decode_length);
  return 0;
}