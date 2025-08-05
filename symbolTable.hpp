#pragma once
#include <unordered_map>
#include <string>
#include <mutex>
#include <cstdint> // Include for uint32_t

namespace GlobalSymbols {
    // The symbol table now maps a variable name to its virtual memory address.
    extern std::unordered_map<std::string, uint32_t> symbolTable;
    extern std::recursive_mutex symbolTableMutex;
}