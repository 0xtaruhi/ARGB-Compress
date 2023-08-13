#ifndef CAT_CORE_DUT_HPP
#define CAT_CORE_DUT_HPP

#include <_types/_uint8_t.h>
#include <array>

#include "Dut.hpp"
#include "VCatCore.h"

namespace cat {

class CatCoreDut : public Dut, public VCatCore {
public:
  CatCoreDut();

  // Override the virtual methods from the Dut class.
  auto clockSignal() -> CData & override { return this->clk; }
  auto resetSignal() -> CData & override { return this->resetn; }
  auto resetActiveLevel() -> bool override { return false; }

  auto fallEdge() -> void override;
  auto riseEdge() -> void override;

  enum class ControlCode : uint8_t {
    Idle = 0,
    ReadUnencodedMemory = 1,
    ReadUndecodedMemory = 2,
    WriteUnencodedMemory = 3,
    WriteUndecodedMemory = 4,
    Decode = 5,
    Encode = 6,
    ReturnToIdle = 7,
  };

  enum class StatusCode : uint8_t {
    Idle = 0,
    Busy = 1,
    Done = 2,
  };

  auto regsSync() -> void;

  auto setInfo(uint16_t info) -> void;

  auto getInfo() -> uint16_t const;

  auto setDataReg(uint32_t data, uint8_t reg_index) -> void;

  auto getDataReg(uint8_t reg_index) -> uint32_t const;

  auto setControlCode(ControlCode code) -> void;

  auto getStatus() -> StatusCode const;

  auto isIdle() -> bool const { return getStatus() == StatusCode::Idle; }
  auto isBusy() -> bool const { return getStatus() == StatusCode::Busy; }
  auto isDone() -> bool const { return getStatus() == StatusCode::Done; }

private:
  std::array<uint32_t, 8> data_reg_;
  uint32_t cs_reg_;

  struct RegsBundle {
    IData &read_data;
    IData &write_data;
    CData &write_enable;

    RegsBundle(IData &read_data, IData &write_data, CData &write_enable)
        : read_data(read_data), write_data(write_data),
          write_enable(write_enable) {}
  };

  std::array<RegsBundle, 8> data_reg_bundles_;
};

} // namespace cat

#endif // CAT_CORE_DUT_HPP
