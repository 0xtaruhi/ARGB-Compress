#include <iostream>
#include <string>

#include "CatLog.hpp"

auto simDecodeUnit(const std::string& filepath, int decode_length) -> void;

int main(int argc, char** argv) {
  cat::CatLog::setLogLevel(cat::CatLog::LogLevel::Debug);

  int decode_length = std::stoi(argv[2]);

  simDecodeUnit(argv[1], decode_length);
  return 0;
}
