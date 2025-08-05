#include "commandList.hpp"

CommandList::CommandList()
{
}

void CommandList::removeCommandAt(int index)
{
    if (index >= 0 && index < commands.size())
        commands.erase(commands.begin() + index);
}

void CommandList::insertCommandsAt(int index, const std::vector<std::shared_ptr<Command>> &newCommands)
{
    commands.insert(commands.begin() + index, newCommands.begin(), newCommands.end());
}

void CommandList::executeCommand(int index)
{
}

void CommandList::addCommand(std::string line)
{
}

// Helper function to split nested command strings properly
std::vector<std::string> splitNestedCommands(const std::string &input)
{
    std::vector<std::string> result;
    std::string current;
    int parenDepth = 0;

    for (size_t i = 0; i < input.size(); ++i)
    {
        char c = input[i];
        current += c;

        if (c == '(')
        {
            parenDepth++;
        }
        else if (c == ')')
        {
            parenDepth--;
        }

        if (parenDepth == 0 && (i + 1 == input.size() || input[i + 1] == ','))
        {
            result.push_back(current);
            current.clear();
            if (i + 1 < input.size() && input[i + 1] == ',')
                i++; // skip the comma
        }
    }

    return result;
}

std::string trim(const std::string &str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");

    if (start == std::string::npos)
        return ""; // string is all spaces
    return str.substr(start, end - start + 1);
}

bool CommandList::parseCommands(std::vector<std::string> inputCommands)
{
    for (std::string line : inputCommands)
    {
        // For PRINT, we need to preserve spaces inside the parentheses
        std::string trimmedLine = trim(line);
        if (trimmedLine.rfind("PRINT(", 0) == 0 && trimmedLine.back() == ')')
        {
            size_t start = trimmedLine.find('(');
            size_t end = trimmedLine.rfind(')');
            if (start == std::string::npos || end == std::string::npos || start >= end)
                return false;

            std::string content = trimmedLine.substr(start + 1, end - start - 1);

            auto printCmd = std::make_shared<PrintCommand>();

            // Split the content by the '+' character
            std::stringstream ss(content);
            std::string segment;
            while (std::getline(ss, segment, '+'))
            {
                std::string partStr = trim(segment);
                if (partStr.front() == '"' && partStr.back() == '"')
                {
                    // It's a literal string
                    std::string literal = partStr.substr(1, partStr.length() - 2);
                    printCmd->addPart(PrintCommand::PrintPartType::LITERAL, literal);
                }
                else if (partStr.rfind("Valuefrom:", 0) == 0)
                {
                    // Support the old format for backward compatibility
                    std::string varName = partStr.substr(10);
                    printCmd->addPart(PrintCommand::PrintPartType::VARIABLE, trim(varName));
                }
                else
                {
                    // It's a variable name
                    printCmd->addPart(PrintCommand::PrintPartType::VARIABLE, partStr);
                }
            }
            commands.push_back(printCmd);
            continue; // Continue to the next line
        }
        // For all other commands, remove spaces to simplify parsing
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

        if (line.find("DECLARE(") == 0 && line.back() == ')')
        {
            std::string args = line.substr(8, line.size() - 9);
            size_t comma = args.find(',');
            if (comma == std::string::npos)
                return false;

            std::string var = args.substr(0, comma);
            std::string val = args.substr(comma + 1);
            commands.push_back(std::make_shared<IOCommand>("DECLARE", var, "", "", std::stoi(val)));
        }
        else if ((line.find("ADD(") == 0 || line.find("SUBTRACT(") == 0) && line.back() == ')')
        {
            std::string op = line.substr(0, line.find('('));
            std::string args = line.substr(op.size() + 1, line.size() - op.size() - 2);

            std::vector<std::string> parts;
            std::stringstream ss(args);
            std::string part;
            while (std::getline(ss, part, ','))
                parts.push_back(part);
            if (parts.size() != 3)
                return false;

            commands.push_back(std::make_shared<IOCommand>(op, parts[0], parts[1], parts[2]));
        }
        else if (line.find("SLEEP(") == 0 && line.back() == ')')
        {
            std::string ticks = line.substr(6, line.size() - 7);
            commands.push_back(std::make_shared<IOCommand>("SLEEP", ticks));
        }
        else if (line.find("FOR([") == 0 && line.back() == ')')
        {
            size_t bodyStart = line.find("[") + 1;
            size_t bodyEnd = line.rfind("]");
            size_t commaAfterBody = line.find(",", bodyEnd);

            if (bodyStart == std::string::npos || bodyEnd == std::string::npos || commaAfterBody == std::string::npos)
                return false;

            std::string bodyStr = line.substr(bodyStart, bodyEnd - bodyStart);
            std::string repeatStr = line.substr(commaAfterBody + 1, line.size() - commaAfterBody - 2);
            int repeatCount = std::stoi(repeatStr);

            std::vector<std::string> nestedCommands = splitNestedCommands(bodyStr);

            CommandList tempList;
            if (!tempList.parseCommands(nestedCommands))
                return false;

            // FIX: The lines that manually managed 'totalCommands' are now removed.
            // The command count is now handled automatically by the vector's size.

            for (int i = 0; i < repeatCount; ++i)
            {
                for (int j = 0; j < tempList.getTotalCommands(); ++j)
                {
                    commands.push_back(tempList.getCommand(j));
                }
            }
        }
        else if ((line.find("READ(") == 0 || line.find("WRITE(") == 0) && line.back() == ')')
        {
            std::string op = line.substr(0, line.find('('));
            std::string args = line.substr(op.size() + 1, line.size() - op.size() - 2);

            size_t comma = args.find(',');
            if (comma == std::string::npos)
                return false;

            std::string arg1 = args.substr(0, comma);
            std::string arg2 = args.substr(comma + 1);

            // FIX: This now works correctly for both READ(var, addr) and WRITE(addr, var)
            commands.push_back(std::make_shared<IOCommand>(op, arg1, arg2));
        }
        else
        {
            // Invalid command
            return false;
        }
    }
    return true;
}
