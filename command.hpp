#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include "symbolTable.hpp"
#include <vector>
using namespace GlobalSymbols;

extern std::unordered_map<std::string, int> symbolTable; // Global symbol table

enum CommandType
{
    IO,    // ADD, SUBTRACT, DECLARE, SLEEP
    PRINT, // PRINT
    FOR    // FOR LOOP
};

class Command
{
public:
    CommandType type;

    Command(CommandType t) : type(t) {}

    virtual void printExecute(std::ofstream &out, std::vector<std::string>* logList) { /* do nothing */ }
    virtual void IOExecute() { /* do nothing */ }
    virtual std::string toString() const = 0;

    virtual ~Command() = default;
};

class PrintCommand : public Command
{
    std::string message;

public:
    PrintCommand(const std::string &msg)
        : Command(PRINT), message(msg) {}

    void printExecute(std::ofstream &out, std::vector<std::string>* logList) override
    {
        out << message << std::endl;
        logList->push_back(message);
    }

    std::string toString() const override
    {
        return "PRINT \"" + message + "\"";
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
    std::string operation; // e.g., "ADD", "SUBTRACT", "DECLARE", "SLEEP"
    std::string lhsVar;    // For target variable or sleep duration
    std::string rhsVar;    // First operand (or value for DECLARE)
    std::string extraVar;  // Second operand for ADD/SUBTRACT

    uint16_t rhsValue = 0;  // Used for DECLARE
    uint8_t sleepTicks = 0; // Used for SLEEP
    bool isSleeping = false;

public:
    IOCommand(const std::string &op, const std::string &lhs = "", const std::string &rhs = "", const std::string &extra = "", uint16_t value = 0)
        : Command(IO), operation(op), lhsVar(lhs), rhsVar(rhs), extraVar(extra), rhsValue(value) {}

    void IOExecute() override
    {
        std::lock_guard<std::mutex> lock(GlobalSymbols::symbolTableMutex); // Ensure thread-safe access

        if (operation == "DECLARE")
        {
            if (GlobalSymbols::symbolTable.find(lhsVar) == GlobalSymbols::symbolTable.end())
            {
                GlobalSymbols::symbolTable[lhsVar] = rhsValue;
            }
        }
        else if (operation == "ADD" || operation == "SUBTRACT")
        {
            if (isalpha(rhsVar[0]) && GlobalSymbols::symbolTable.find(rhsVar) == GlobalSymbols::symbolTable.end())
                GlobalSymbols::symbolTable[rhsVar] = 0;
            if (isalpha(extraVar[0]) && GlobalSymbols::symbolTable.find(extraVar) == GlobalSymbols::symbolTable.end())
                GlobalSymbols::symbolTable[extraVar] = 0;
            if (GlobalSymbols::symbolTable.find(lhsVar) == GlobalSymbols::symbolTable.end())
                GlobalSymbols::symbolTable[lhsVar] = 0;

            uint16_t rhsVal = isalpha(rhsVar[0]) ? GlobalSymbols::symbolTable[rhsVar] : static_cast<uint16_t>(std::stoi(rhsVar));
            uint16_t extraVal = isalpha(extraVar[0]) ? GlobalSymbols::symbolTable[extraVar] : static_cast<uint16_t>(std::stoi(extraVar));

            if (operation == "ADD")
                GlobalSymbols::symbolTable[lhsVar] = rhsVal + extraVal;
            else
                GlobalSymbols::symbolTable[lhsVar] = rhsVal - extraVal;
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
        else
            return operation + " " + lhsVar + ", " + rhsVar + ", " + extraVar;
    }

    static std::string randomCommand()
    {
        static std::vector<std::string> samples = {
            "DECLARE(x, 100)",
            "DECLARE(y, 200)",
            "ADD(z, x, y)",
            "ADD(total, 50, 25)",
            "SUBTRACT(diff, x, 20)",
            "SUBTRACT(balance, y, z)",
            "SLEEP(5)",
            "SLEEP(10)"};
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

    static std::string randomCommand()
    {
        static std::vector<std::string> samples = {
            "FOR([PRINT(Hello World), PRINT(Execution done)], 2)",
            "FOR([ADD(x, y, z), SUBTRACT(z, x, y)], 3)",
            "FOR([PRINT(Loop Start), SLEEP(2), PRINT(Loop End)], 4)",
            "FOR([FOR([PRINT(Nested Loop), SLEEP(1)], 2)], 2)",
            "FOR([DECLARE(a, 10), ADD(b, a, 5), PRINT(b)], 3)"};
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

    void printExecute(std::ofstream &out, std::vector<std::string>* logs) override
    {
        for (int i = 0; i < repeatCount; ++i)
        {
            for (const auto &cmd : body)
            {
                if (cmd->type == PRINT)
                {
                    cmd->printExecute(out, logs);
                }
                else if (cmd->type == IO)
                {
                    cmd->IOExecute();
                }
                else if (cmd->type == FOR)
                {
                    cmd->printExecute(out, logs); // Recursive call for nested FORs
                }
            }
        }
    }

    int getNestingDepth() const
    {
        return nestingDepth;
    }

    void IOExecute() override
    {
        for (int i = 0; i < repeatCount; ++i)
        {
            for (const auto &cmd : body)
            {
                if (cmd->type == IO)
                {
                    cmd->IOExecute();
                }
                else if (cmd->type == FOR)
                {
                    cmd->IOExecute(); // Recursive call for nested FORs
                }
                // else if (cmd->type == PRINT)
                // {
                //     std::ofstream dummyOut("/dev/null"); // Optional: discard output
                //     cmd->printExecute(dummyOut, logs);
                // }
            }
        }
    }

    std::string toString() const override
    {
        return "FOR " + std::to_string(repeatCount) + " TIMES";
    }
};
