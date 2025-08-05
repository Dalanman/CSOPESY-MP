#include "symbolTable.hpp"

namespace GlobalSymbols {
    // Definition updated to store uint32_t addresses.
    std::unordered_map<std::string, uint32_t> symbolTable;
    std::recursive_mutex symbolTableMutex;
}