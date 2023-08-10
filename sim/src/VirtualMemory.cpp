#include "VirtualMemory.hpp"

#include "CatLog.hpp"
#include <cassert>
#include <iomanip>
#include <ios>
#include <iostream>

using namespace cat;

VirtualMemoryBlock::VirtualMemoryBlock(uint32_t page_size)
    : VirtualMemoryBlock(page_size, true) {}

VirtualMemoryBlock::VirtualMemoryBlock(uint32_t page_size, bool do_initialize) {
  assert(page_size % 4 == 0);

  page_size_ = page_size;
  data_.resize(page_size_ / 4);

  if (do_initialize) {
    for (auto &word : data_) {
      word = 0;
    }
  }
}

auto VirtualMemoryBlock::read(addr_t addr) -> uint32_t const {
  addr_t addr_in_page = addr & (page_size_ - 1);

  addr_t access_word_addr = addr_in_page / 4;
  addr_t access_word_offset = addr_in_page % 4;

  switch (access_word_offset) {
  case 0:
    return data_[access_word_addr];
  case 1:
    return ((data_[access_word_addr] >> 8) & 0x00ffffff) |
           (data_[access_word_addr + 1] << 24);
  case 2:
    return ((data_[access_word_addr] >> 16) & 0x0000ffff) |
           (data_[access_word_addr + 1] << 16);
  case 3:
    return ((data_[access_word_addr] >> 24) & 0x000000ff) |
           (data_[access_word_addr + 1] << 8);
  default:
    assert(false && "unreachable");
  }
}

auto VirtualMemoryBlock::write(addr_t addr, uint32_t data, mask_t mask)
    -> void {
  addr_t addr_in_page = addr & (page_size_ - 1);

  addr_t access_word_addr = addr_in_page / 4;
  addr_t access_word_offset = addr_in_page % 4;

  auto mask_extend = [](mask_t mask) {
    uint32_t ret = 0;
    for (int i = 0; i < 4; ++i) {
      if (mask & (1 << i)) {
        ret |= 0xff << (i * 8);
      }
    }
    return ret;
  };

  auto writeWithByteMask = [&](uint32_t &word, uint8_t byte_mask,
                               uint32_t data) {
    auto extended_mask = mask_extend(byte_mask);
    word = (word & ~extended_mask) | (data & extended_mask);
  };

  if (access_word_offset == 0) {
    writeWithByteMask(data_[access_word_addr], mask, data);
    return;
  }

  mask_t mask0 = (mask << access_word_offset) & 0xf;
  mask_t mask1 = ((mask << access_word_offset) >> 4) & 0xf;
  uint32_t data0 = data << (access_word_offset * 8);
  uint32_t data1 = data >> ((4 - access_word_offset) * 8);

  writeWithByteMask(data_[access_word_addr], mask0, data0);
  writeWithByteMask(data_[access_word_addr + 1], mask1, data1);
}

auto VirtualMemoryBlock::dump() const -> void {
  std::cout << std::hex << std::setfill('0') << std::setw(8) << 0 << ": ";

  for (int i = 0; i < data_.size(); ++i) {
    std::cout << std::setw(8) << data_[i] << " ";
    if ((i + 1) % 4 == 0 && i != data_.size() - 1) {
      std::cout << std::endl;
      std::cout << std::setw(8) << (i + 1) * 4 << ": ";
    }
  }
  std::cout << std::endl;
}

VirtualMemory::VirtualMemory(uint32_t page_size) { page_size_ = page_size; }

auto VirtualMemory::newBlock(addr_t addr) -> VirtualMemoryBlock & {
  assert(getPageBase(addr) == addr && "address must be page-aligned");

  auto it = blocks_.find(addr);
  if (it != blocks_.end()) {
    CatLog::logWarning("VirtualMemory::newBlock: block already exists");
    return *it->second;
  }

  it =
      blocks_
          .emplace(addr, std::make_unique<VirtualMemoryBlock>(page_size_, true))
          .first;
  CatLog::logDebug("VirtualMemory::newBlock: created new block");
  return *it->second;
}

auto VirtualMemory::getBlock(addr_t addr) -> VirtualMemoryBlock & {
  addr_t page_base = getPageBase(addr);

  auto it = blocks_.find(page_base);
  if (it == blocks_.end()) {
    it = blocks_
             .emplace(page_base,
                      std::make_unique<VirtualMemoryBlock>(page_size_, true))
             .first;
    CatLog::logWarning("VirtualMemory::getBlock: block does not exist, "
                       "creating empty block");
  }

  return *it->second;
}

auto VirtualMemory::getBlock(addr_t addr) const -> VirtualMemoryBlock const & {
  addr_t page_base = getPageBase(addr);

  auto it = blocks_.find(page_base);
  assert(it != blocks_.end());

  return *it->second;
}

auto VirtualMemory::getPageBase(addr_t addr) const -> addr_t {
  return addr & ~(page_size_ - 1);
}

auto VirtualMemory::read(addr_t addr) -> uint32_t {
  bool cross_page = (addr & (page_size_ - 1)) > (page_size_ - 4);

  if (!cross_page) {
    [[likely]] return getBlock(addr).read(addr);
  } else {
    uint32_t data = getBlock(addr).read(addr);
    uint32_t data2 = getBlock(addr + 4).read(addr + 4);

    addr_t byte_offset = addr & 3;
    switch (byte_offset) {
    case 0:
      return data;
    case 1:
      return (data & 0x00ffffff) | (data2 << 24);
    case 2:
      return (data & 0x0000ffff) | (data2 << 16);
    case 3:
      return (data & 0x000000ff) | (data2 << 8);
    default:
      assert(false && "unreachable");
    }
  }
}

auto VirtualMemory::write(addr_t addr, uint32_t data, mask_t mask) -> void {
  bool cross_page = (addr & (page_size_ - 1)) > (page_size_ - 4);

  if (!cross_page) {
    [[likely]] getBlock(addr).write(addr, data, mask);
  } else {
    CatLog::logError("VirtualMemory::write: unaligned write across page "
                     "boundaries not supported");
  }
}
