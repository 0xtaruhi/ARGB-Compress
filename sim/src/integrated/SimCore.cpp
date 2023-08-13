#include <iostream>

#include "CatLog.hpp"
#include "IntegratedSimulator.hpp"

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cout << "Usage: " << argv[0]
              << " <unencoded memory file> <encode length>" << std::endl;
    return 1;
  }

  auto &sim = cat::IntegratedSimulator::getInstance();
  sim.loadUnencodedMemory(argv[1]);
  sim.setEncodeLength(std::stoi(argv[2]));

#ifdef IN_DEVELOP
  cat::CatLog::setLogLevel(cat::CatLog::LogLevel::Debug);
#else
  cat::CatLog::setLogLevel(cat::CatLog::LogLevel::Info);
#endif

  cat::CatLog::logInfo("Starting simulation...");
  sim.run();
  return 0;
}
