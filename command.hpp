#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include "symbolTable.hpp"
#include <vector>
#include "memory.hpp"
#include <regex>
#include <utility>
using namespace GlobalSymbols;

enum CommandType
{
    IO,    // ADD, SUBTRACT, DECLARE, SLEEP, READ, WRITE
    PRINT, // PRINT
    FOR    // FOR LOOP
};

class Command
{
public:
    CommandType type;

    Command(CommandType t) : type(t) {}

    virtual void printExecute(std::string timestamp, int coreIndex, std::vector<std::string> *logList, int pid, std::shared_ptr<FlatMemoryAllocator> memoryAllocator) { /* do nothing */ }
    virtual void IOExecute(int pid, std::shared_ptr<FlatMemoryAllocator> memoryAllocator) { /* do nothing */ }
    virtual std::string toString() const = 0;

    virtual ~Command() = default;
};

class PrintCommand : public Command
{
public:
    enum class PrintPartType { LITERAL, VARIABLE };

    struct PrintPart {
        PrintPartType type;
        std::string data;
    };

private:
    std::vector<PrintPart> parts;

public:
    // The constructor is now simpler
    PrintCommand() : Command(PRINT) {}

    void addPart(PrintPartType type, std::string data) {
        parts.push_back({type, data});
    }

    void printExecute(std::string timestamp, int coreIndex, std::vector<std::string> *logList, int pid, std::shared_ptr<FlatMemoryAllocator> memoryAllocator) override
    {
        std::ostringstream oss; // Use a string stream to build the final output

        for (const auto& part : parts)
        {
            if (part.type == PrintPartType::LITERAL)
            {
                // If it's a literal, just append it
                oss << part.data;
            }
            else if (part.type == PrintPartType::VARIABLE)
            {
                // If it's a variable, look up its value in memory
                std::lock_guard<std::recursive_mutex> lock(GlobalSymbols::symbolTableMutex);
                uint16_t val = 0; // Default to 0 if not found

                if (GlobalSymbols::symbolTable.count(part.data))
                {
                    uint32_t varAddr = GlobalSymbols::symbolTable.at(part.data);
                    val = memoryAllocator->readValueFromVirtualAddress(pid, varAddr);
                }
                oss << val;
            }
        }

        std::string coreIndexStr = std::to_string(coreIndex);
        std::string log = "(" + timestamp + ")" + " " + "Core:" + coreIndexStr + " " + "\"" + oss.str() + "\"";
        logList->push_back(log);
    }

    std::string toString() const override
    {
        // Reconstruct a string representation for logging/debugging
        std::string result = "PRINT(";
        for (size_t i = 0; i < parts.size(); ++i) {
            if (parts[i].type == PrintCommand::PrintPartType::LITERAL) {
                result += "\"" + parts[i].data + "\"";
            } else {
                result += parts[i].data;
            }
            if (i < parts.size() - 1) {
                result += " + ";
            }
        }
        result += ")";
        return result;
    }

    static std::string randomCommand()
    {
        static std::vector<std::string> samples = {
            "PRINT(Hello World)",
            "PRINT(Value is correct)",
            "PRINT(Looping...)",
            "PRINT(Execution done)",
            "PRINT(Error occurred)"};
        return samples[rand() % samples.size()];
    }
};

class IOCommand : public Command
{
    std::string operation;
    std::string lhsVar;
    std::string rhsVar;
    std::string extraVar;

    uint16_t rhsValue = 0;
    uint8_t sleepTicks = 0;
    bool isSleeping = false;

public:
    IOCommand(const std::string &op, const std::string &lhs = "", const std::string &rhs = "", const std::string &extra = "", uint16_t value = 0)
        : Command(IO), operation(op), lhsVar(lhs), rhsVar(rhs), extraVar(extra), rhsValue(value) {}

    std::string getOperation() { return operation; }

    void IOExecute(int pid, std::shared_ptr<FlatMemoryAllocator> memoryAllocator)
    {
        std::lock_guard<std::recursive_mutex> lock(GlobalSymbols::symbolTableMutex);
        auto &table = GlobalSymbols::symbolTable;

        if (operation == "DECLARE")
        {
            if (table.find(lhsVar) == table.end())
            {
                
                uint32_t newAddr = memoryAllocator->allocateVariable(pid, sizeof(uint16_t));
                table[lhsVar] = newAddr;
                memoryAllocator->writeValueAtVirtualAddress(pid, newAddr, rhsValue);
            }
        }
        else if (operation == "ADD" || operation == "SUBTRACT")
        {
            auto getValue = [&](const std::string &varOrLiteral) -> uint16_t
            {
                if (isalpha(varOrLiteral[0]))
                { // It's a variable
                    if (table.count(varOrLiteral))
                    {
                        uint32_t addr = table.at(varOrLiteral);
                        return memoryAllocator->readValueFromVirtualAddress(pid, addr);
                    }
                    return 0; // Variable not found, return 0 as default
                }
                return static_cast<uint16_t>(std::stoi(varOrLiteral)); // It's a literal
            };

            uint16_t val1 = getValue(rhsVar);
            uint16_t val2 = getValue(extraVar);
            uint16_t result = (operation == "ADD") ? (val1 + val2) : (val1 - val2);

            // Check if destination variable exists and write the result to its address.
            if (table.count(lhsVar))
            {
                uint32_t destAddr = table.at(lhsVar);
                memoryAllocator->writeValueAtVirtualAddress(pid, destAddr, result);
            }
        }
        else if (operation == "READ") // Syntax: READ(<variable_to_store_in>, <hex_address_to_read_from>)
        {
            if (table.count(lhsVar))
            {
                uint32_t destVarAddr = table.at(lhsVar);
                uint32_t sourceMemAddr = std::stoul(rhsVar, nullptr, 16);

                uint16_t valueRead = memoryAllocator->readValueFromVirtualAddress(pid, sourceMemAddr);
                memoryAllocator->writeValueAtVirtualAddress(pid, destVarAddr, valueRead);
            }
        }
        else if (operation == "WRITE") // Syntax: WRITE(<hex_address_to_write_to>, <variable_to_read_from>)
        {
            if (table.count(rhsVar))
            {
                uint32_t sourceVarAddr = table.at(rhsVar);
                uint32_t destMemAddr = std::stoul(lhsVar, nullptr, 16);

                uint16_t valueToWrite = memoryAllocator->readValueFromVirtualAddress(pid, sourceVarAddr);
                memoryAllocator->writeValueAtVirtualAddress(pid, destMemAddr, valueToWrite);
            }
        }
        else if (operation == "SLEEP")
        {
            sleepTicks = static_cast<uint8_t>(std::stoi(lhsVar));
            isSleeping = true;
        }
    }

    std::string toString() const override
    {
        if (operation == "DECLARE")
            return "DECLARE " + lhsVar + ", " + std::to_string(rhsValue);
        else if (operation == "SLEEP")
            return "SLEEP " + lhsVar;
        else if (operation == "READ")
            return "READ " + lhsVar + ", " + rhsVar;
        else if (operation == "WRITE")
            return "WRITE " + lhsVar + ", " + rhsVar;
        else
            return operation + " " + lhsVar + ", " + rhsVar + ", " + extraVar;
    }

    static std::string randomCommand()
    {
        static std::vector<std::string> samples = {
            "DECLARE(x, 100)",
            "DECLARE(y, 200)",
            "ADD(z, x, y)",
            "SUBTRACT(diff, x, 20)",
            "SLEEP(5)",
            "READ(x, 0x001)",
            "WRITE(0x001, y)"};
        return samples[rand() % samples.size()];
    }

    bool sleeping() const { return isSleeping; }
    uint8_t getSleepTicks() const { return sleepTicks; }
};

class ForCommand : public Command
{
    int repeatCount;
    std::vector<std::shared_ptr<Command>> body;
    int nestingDepth;

public:
    ForCommand(int count, int depth = 1)
        : Command(FOR), repeatCount(count), nestingDepth(depth)
    {
        if (nestingDepth > 3)
        {
            throw std::runtime_error("Nesting limit exceeded (max 3 levels)");
        }
    }

    static std::string randomCommand(std::string name)
    {
        static std::vector<std::string> samples = {
            "FOR([PRINT(Hello World from " + name + "), PRINT(Hello World from " + name + ")], 2)",
            "FOR([ADD(x, y, z), SUBTRACT(z, x, y)], 3)",
            "FOR([PRINT(Hello World from " + name + "), SLEEP(2), PRINT(Hello World from " + name + ")], 4)",
            "FOR([FOR([PRINT(Hello World from " + name + "), SLEEP(1)], 2)], 2)",
            "FOR([DECLARE(a, 10), ADD(b, a, 5), PRINT(Value from:b)], 3)"};
        return samples[rand() % samples.size()];
    }
    void addCommand(std::shared_ptr<Command> cmd)
    {
        // If the command is a FOR loop, validate nesting
        if (cmd->type == FOR)
        {
            auto innerFor = std::dynamic_pointer_cast<ForCommand>(cmd);
            if (innerFor)
            {
                if (nestingDepth + 1 > 3)
                {
                    throw std::runtime_error("Nesting depth exceeds 3");
                }
                innerFor->setNestingDepth(nestingDepth + 1);
            }
        }

        body.push_back(cmd);
    }

    std::vector<std::shared_ptr<Command>> unrollBody() const
    {
        std::vector<std::shared_ptr<Command>> unrolled;

        for (int i = 0; i < repeatCount; ++i)
        {
            for (auto &cmd : body)
            {
                if (cmd->type == FOR)
                {
                    auto nestedFor = std::dynamic_pointer_cast<ForCommand>(cmd);
                    auto nestedUnrolled = nestedFor->unrollBody();
                    unrolled.insert(unrolled.end(), nestedUnrolled.begin(), nestedUnrolled.end());
                }
                else
                {
                    unrolled.push_back(cmd);
                }
            }
        }

        return unrolled;
    }
    void setNestingDepth(int depth)
    {
        nestingDepth = depth;
        if (nestingDepth > 3)
        {
            throw std::runtime_error("Nesting limit exceeded (max 3 levels)");
        }

        // Also push depth to any nested FORs already added
        for (auto &cmd : body)
        {
            if (cmd->type == FOR)
            {
                auto nested = std::dynamic_pointer_cast<ForCommand>(cmd);
                if (nested)
                {
                    nested->setNestingDepth(depth + 1);
                }
            }
        }
    }

    void printExecute(std::string timestamp, int coreIndex, std::vector<std::string>* logs, int pid, std::shared_ptr<FlatMemoryAllocator> memoryAllocator) override
    {
        for (int i = 0; i < repeatCount; ++i)
        {
            for (const auto &cmd : body)
            {
                // Pass the pid down in the recursive call
                cmd->printExecute(timestamp, coreIndex, logs, pid, memoryAllocator);
            }
        }
    }

    int getNestingDepth() const
    {
        return nestingDepth;
    }

    void IOExecute(int pid, std::shared_ptr<FlatMemoryAllocator> memoryAllocator) override
    {
        for (int i = 0; i < repeatCount; ++i)
        {
            for (const auto &cmd : body)
            {
                // Pass the pid down in the recursive call
                cmd->IOExecute(pid, memoryAllocator);
            }
        }
    }

    std::string toString() const override
    {
        return "FOR " + std::to_string(repeatCount) + " TIMES";
    }
};
