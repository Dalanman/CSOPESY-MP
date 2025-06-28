#include "symbolTable.hpp"

namespace GlobalSymbols {
    std::unordered_map<std::string, uint16_t> symbolTable;
    std::mutex symbolTableMutex;
}