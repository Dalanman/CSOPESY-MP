#pragma once
#include <unordered_map>
#include <string>
#include <mutex>

namespace GlobalSymbols {
    extern std::unordered_map<std::string, uint16_t> symbolTable;
    extern std::mutex symbolTableMutex;
}