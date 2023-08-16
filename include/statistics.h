#ifndef STATISTICS_H
#define STATISTICS_H
#include <cstdint>
#include <iostream>
#include <limits>
#include <ostream>
#include <vector>

class Statistics {
public:
  struct SingleData {
    SingleData() : encode_cycles(0), decode_cycles(0), encoded_bytes(0) {}
    SingleData(uint64_t encode_cycles, uint64_t decode_cycles,
               uint64_t encoded_bytes)
        : encode_cycles(encode_cycles), decode_cycles(decode_cycles),
          encoded_bytes(encoded_bytes) {}

    uint64_t encode_cycles;
    uint64_t decode_cycles;
    uint64_t encoded_bytes;
  };

  Statistics() = default;
  Statistics(const Statistics &) = delete;

  static auto getInstance() -> Statistics & {
    static Statistics instance;
    return instance;
  }

  template <typename T> auto getAverage(T SingleData::*member) const -> double {
    double sum = 0;
    for (const auto &d : data_) {
      sum += d.*member;
    }
    return sum / data_.size();
  }

  template <typename T>
  auto getMax(const T SingleData::*member) const -> uint64_t {
    uint64_t max = 0;
    for (const auto &d : data_) {
      max = std::max(max, d.*member);
    }
    return max;
  }

  template <typename T>
  auto getMin(const T SingleData::*member) const -> uint64_t {
    uint64_t min = std::numeric_limits<uint64_t>::max();
    for (const auto &d : data_) {
      min = std::min(min, d.*member);
    }
    return min;
  }

  auto getAverageEncodeCycles() const -> double {
    return getAverage(&SingleData::encode_cycles);
  }
  auto getAverageDecodeCycles() const -> double {
    return getAverage(&SingleData::decode_cycles);
  }
  auto getAverageEncodedBytes() const -> double {
    return getAverage(&SingleData::encoded_bytes);
  }

  auto getMaxEncodeCycles() const -> uint64_t {
    return getMax(&SingleData::encode_cycles);
  }

  auto getMaxDecodeCycles() const -> uint64_t {
    return getMax(&SingleData::decode_cycles);
  }

  auto getMaxEncodedBytes() const -> uint64_t {
    return getMax(&SingleData::encoded_bytes);
  }

  auto getMinEncodeCycles() const -> uint64_t {
    return getMin(&SingleData::encode_cycles);
  }

  auto getMinDecodeCycles() const -> uint64_t {
    return getMin(&SingleData::decode_cycles);
  }

  auto getMinEncodedBytes() const -> uint64_t {
    return getMin(&SingleData::encoded_bytes);
  }

  auto getDataSize() const { return data_.size(); }

  auto clear() -> void { data_.clear(); }

  auto dumpInfos(std::ostream &os) {
    if (encode_) {
      os << "Average encode cycles: " << getAverageEncodeCycles() << std::endl;
      os << "Max encode cycles: " << getMaxEncodeCycles() << std::endl;
      os << "Min encode cycles: " << getMinEncodeCycles() << std::endl;
      std::cout << "---" << std::endl;
      os << "Average encoded bytes: " << getAverageEncodedBytes() << std::endl;
      os << "Max encoded bytes: " << getMaxEncodedBytes() << std::endl;
      os << "Min encoded bytes: " << getMinEncodedBytes() << std::endl;
    }
    if (decode_) {
      std::cout << "---" << std::endl;
      os << "Average decode cycles: " << getAverageDecodeCycles() << std::endl;
      os << "Max decode cycles: " << getMaxDecodeCycles() << std::endl;
      os << "Min decode cycles: " << getMinDecodeCycles() << std::endl;
    }
  }

  auto addData(const SingleData &data) -> void { data_.push_back(data); }

  auto addData(uint64_t encode_cycles, uint64_t decode_cycles,
               uint64_t encoded_bytes) -> void {
    data_.emplace_back(encode_cycles, decode_cycles, encoded_bytes);
  }

  auto setEncode(bool encode = true) -> void { encode_ = encode; }
  auto setDecode(bool decode = true) -> void { decode_ = decode; }

private:
  std::vector<SingleData> data_;

  bool decode_ = false;
  bool encode_ = false;
}; // class Statistics

#endif // STATISTICS_H
