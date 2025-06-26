#pragma once
#include <unordered_map>
#include <string>
#include <mutex>

namespace GlobalSymbols {
    inline std::unordered_map<std::string, uint16_t> symbolTable;
    inline std::mutex symbolTableMutex;
}