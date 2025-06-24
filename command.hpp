#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

extern std::unordered_map<std::string, int> symbolTable; // Global symbol table

enum CommandType
{
    IO,
    PRINT,
    FOR
};

class Command
{
public:
    CommandType type;

    Command(CommandType t) : type(t) {}

    virtual void printExecute(std::ofstream &out) { /* do nothing */ }
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

    void printExecute(std::ofstream &out) override
    {
        out << message << std::endl;
    }

    std::string toString() const override
    {
        return "PRINT \"" + message + "\"";
    }
};

class IOCommand : public Command
{
    std::string lhsVar, rhsVar, resultVar;
    std::string operation; // e.g., "ADD", "SUB", etc.

public:
    IOCommand(const std::string &op, const std::string &lhs, const std::string &rhs, const std::string &res)
        : Command(IO), lhsVar(lhs), rhsVar(rhs), resultVar(res), operation(op) {}

    void IOExecute() override
    {
        int lhs = symbolTable[lhsVar];
        int rhs = symbolTable[rhsVar];

        if (operation == "ADD")
        {
            symbolTable[resultVar] = lhs + rhs;
        }
        else if (operation == "SUB")
        {
            symbolTable[resultVar] = lhs - rhs;
        }
        // You can extend to support MUL, DIV, etc.
    }

    std::string toString() const override
    {
        return operation + " " + lhsVar + ", " + rhsVar + ", " + resultVar;
    }
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

    void addCommand(std::shared_ptr<Command> cmd)
    {
        if (cmd->type == FOR)
        {
            auto innerFor = std::dynamic_pointer_cast<ForCommand>(cmd);
            if (innerFor)
            {
                // Enforce nesting limit when adding nested FORs
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

    void printExecute(std::ofstream &out) override
    {
        for (int i = 0; i < repeatCount; ++i)
        {
            for (const auto &cmd : body)
            {
                if (cmd->type == PRINT)
                {
                    cmd->printExecute(out);
                }
                else if (cmd->type == IO)
                {
                    cmd->IOExecute();
                }
                else if (cmd->type == FOR)
                {
                    cmd->printExecute(out); // Recursive print
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
                    cmd->IOExecute(); // Recursive IO
                }
            }
        }
    }

    std::string toString() const override
    {
        return "FOR " + std::to_string(repeatCount) + " TIMES";
    }
};
