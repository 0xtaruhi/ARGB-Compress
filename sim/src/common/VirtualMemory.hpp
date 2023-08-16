#ifndef VIRTUAL_MEMORY_H
#define VIRTUAL_MEMORY_H

#include <map>
#include <memory>
#include <stdint.h>
#include <vector>

namespace cat {

class VirtualMemoryBlock {
public:
  using addr_t = uint32_t;
  using mask_t = uint8_t;

  VirtualMemoryBlock(uint32_t page_size);

  VirtualMemoryBlock(uint32_t page_size, bool do_initialize);

  auto getPageSize() const { return page_size_; }

  // read 4 bytes, unaligned
  auto read(addr_t addr) -> uint32_t const; // read 4 bytes

  // write 4 bytes, unaligned.
  // mask is a 4-bit value, where each bit corresponds to a byte in the word.
  auto write(addr_t addr, uint32_t data, mask_t mask = 0b1111) -> void;

  auto dump() const -> void;

  auto loadFromBuffer(addr_t addr, const uint8_t *data, uint32_t size) -> void;
  auto writeToBuffer(addr_t addr, uint8_t *data) -> uint32_t const;

  friend auto operator==(const VirtualMemoryBlock &lhs,
                         const VirtualMemoryBlock &rhs) -> bool;

  friend auto operator!=(const VirtualMemoryBlock &lhs,
                         const VirtualMemoryBlock &rhs) -> bool {
    return !(lhs == rhs);
  }

private:
  uint32_t page_size_; // in bytes
  std::vector<uint32_t> data_;
};

class VirtualMemory {
public:
  using addr_t = uint32_t;
  using mask_t = uint8_t;

  VirtualMemory(uint32_t page_size);

  VirtualMemory(const VirtualMemory &other);

  auto newBlock(addr_t addr) -> VirtualMemoryBlock &;

  // read 4 bytes, unaligned
  auto read(addr_t addr) -> uint32_t; // read 4 bytes

  // write 4 bytes, unaligned.
  auto write(addr_t addr, uint32_t data, mask_t mask = 0b1111) -> void;

  auto getBlock(addr_t addr) const -> VirtualMemoryBlock const &;
  auto getBlock(addr_t addr) -> VirtualMemoryBlock &;

  auto readFromFile(const std::string &filename) -> bool;

  auto dump() const -> void;

  friend bool operator==(const VirtualMemory &lhs, const VirtualMemory &rhs);
  friend bool operator!=(const VirtualMemory &lhs, const VirtualMemory &rhs) {
    return !(lhs == rhs);
  }

private:
  uint32_t page_size_; // in bytes
  std::map<addr_t, std::unique_ptr<VirtualMemoryBlock>> blocks_;

  auto getPageBase(addr_t addr) const -> addr_t;
};

auto operator==(const cat::VirtualMemoryBlock &lhs,
                const cat::VirtualMemoryBlock &rhs) -> bool;
auto operator==(const cat::VirtualMemory &lhs, const cat::VirtualMemory &rhs)
    -> bool;

} // namespace cat

#endif // VIRTUAL_MEMORY_H
