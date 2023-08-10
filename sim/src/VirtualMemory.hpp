#ifndef VIRTUAL_MEMORY_H
#define VIRTUAL_MEMORY_H

#include <map>
#include <memory>
#include <vector>


namespace cat {

class VirtualMemoryBlock {
  using addr_t = uint32_t;
  using mask_t = uint8_t;

public:
  VirtualMemoryBlock(uint32_t page_size);

  VirtualMemoryBlock(uint32_t page_size, bool do_initialize);

  auto getPageSize() const { return page_size_; }

  // read 4 bytes, unaligned
  auto read(addr_t addr) -> uint32_t const; // read 4 bytes

  // write 4 bytes, unaligned.
  // mask is a 4-bit value, where each bit corresponds to a byte in the word.
  auto write(addr_t addr, uint32_t data, mask_t mask) -> void;

  auto dump() const -> void;

private:
  uint32_t page_size_; // in bytes
  std::vector<uint32_t> data_;
};

class VirtualMemory {
  using addr_t = uint32_t;
  using mask_t = uint8_t;

public:
  VirtualMemory(uint32_t page_size);

  auto newBlock(addr_t addr) -> VirtualMemoryBlock &;

  // read 4 bytes, unaligned
  auto read(addr_t addr) -> uint32_t; // read 4 bytes
  
  // write 4 bytes, unaligned.
  auto write(addr_t addr, uint32_t data, mask_t mask) -> void;

  auto getBlock(addr_t addr) const -> VirtualMemoryBlock const &;
  auto getBlock(addr_t addr) -> VirtualMemoryBlock &;

private:
  uint32_t page_size_; // in bytes
  std::map<addr_t, std::unique_ptr<VirtualMemoryBlock>> blocks_;

  auto getPageBase(addr_t addr) const -> addr_t;
};

} // namespace cat

#endif // VIRTUAL_MEMORY_H
