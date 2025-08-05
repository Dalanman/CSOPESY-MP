#include "commandList.hpp"

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
std::string trim(const std::string& str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return ""; // string contains only whitespace
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}



std::vector<std::string> splitNestedCommands(const std::string& input)
{
    std::vector<std::string> result;
    std::string current;
    int parenDepth = 0;

    for (char c : input)
    {
        // Split on comma only if we are not inside parentheses or brackets
        if (parenDepth == 0 && c == ',')
        {
            result.push_back(trim(current));
            current.clear();
            continue;
        }

        current += c;

        if (c == '(' || c == '[')
        {
            parenDepth++;
        }
        else if (c == ')' || c == ']')
        {
            parenDepth--;
        }
    }
    // Add the last command to the list
    if (!current.empty()) {
        result.push_back(trim(current));
    }

    return result;
}

bool CommandList::parseCommands(std::vector<std::string> inputCommands)
{
    for (std::string line : inputCommands)
    {
        std::string trimmedLine = trim(line);
        if (trimmedLine.empty()) continue;

        if (trimmedLine.rfind("PRINT(", 0) == 0 && trimmedLine.back() == ')')
        {
            size_t start = trimmedLine.find('(');
            size_t end = trimmedLine.rfind(')');
            if (start == std::string::npos || end == std::string::npos || start >= end) return false;

            std::string content = trimmedLine.substr(start + 1, end - start - 1);
            auto printCmd = std::make_shared<PrintCommand>();

            std::stringstream ss(content);
            std::string segment;
            while (std::getline(ss, segment, '+'))
            {
                std::string partStr = trim(segment);
                if (!partStr.empty()) {
                    if (partStr.front() == '"' && partStr.back() == '"') {
                        printCmd->addPart(PrintCommand::PrintPartType::LITERAL, partStr.substr(1, partStr.length() - 2));
                    }
                    else if (partStr.rfind("Valuefrom:", 0) == 0) {
                        printCmd->addPart(PrintCommand::PrintPartType::VARIABLE, trim(partStr.substr(10)));
                    }
                    else {
                        printCmd->addPart(PrintCommand::PrintPartType::VARIABLE, partStr);
                    }
                }
            }
            commands.push_back(printCmd);
            continue; // Go to the next command string
        }

        if (trimmedLine.find('(') == std::string::npos) {
            size_t first_space = trimmedLine.find(' ');
            if (first_space != std::string::npos) {
                std::string op = trimmedLine.substr(0, first_space);
                std::string args = trim(trimmedLine.substr(first_space + 1));

                std::replace(args.begin(), args.end(), ' ', ',');

                line = op + "(" + args + ")";
            }
            else {
                line = trimmedLine + "()";
            }
        }
        else {
            line = trimmedLine;
        }

        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

        if (line.find("DECLARE(") == 0 && line.back() == ')')
        {
            std::string args = line.substr(8, line.size() - 9);
            size_t comma = args.find(',');
            if (comma == std::string::npos) return false;
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
            while (std::getline(ss, part, ',')) parts.push_back(part);
            if (parts.size() != 3) return false;
            commands.push_back(std::make_shared<IOCommand>(op, parts[0], parts[1], parts[2]));
        }
        else if (line.find("SLEEP(") == 0 && line.back() == ')')
        {
            std::string ticks = line.substr(6, line.size() - 7);
            commands.push_back(std::make_shared<IOCommand>("SLEEP", ticks));
        }
        else if ((line.find("READ(") == 0 || line.find("WRITE(") == 0) && line.back() == ')')
        {
            std::string op = line.substr(0, line.find('('));
            std::string args = line.substr(op.size() + 1, line.size() - op.size() - 2);
            size_t comma = args.find(',');
            if (comma == std::string::npos) return false;
            std::string arg1 = args.substr(0, comma);
            std::string arg2 = args.substr(comma + 1);
            commands.push_back(std::make_shared<IOCommand>(op, arg1, arg2));
        }
        else if (line.find("FOR([") == 0 && line.back() == ')')
        {
            size_t bodyStart = line.find('[') + 1;
            size_t bodyEnd = line.rfind(']');
            size_t commaAfterBody = line.find(',', bodyEnd);
            if (bodyStart == std::string::npos || bodyEnd == std::string::npos || commaAfterBody == std::string::npos) return false;

            std::string bodyStr = line.substr(bodyStart, bodyEnd - bodyStart);
            std::string repeatStr = line.substr(commaAfterBody + 1, line.size() - commaAfterBody - 2);
            int repeatCount = std::stoi(repeatStr);
            std::vector<std::string> nestedCommands = splitNestedCommands(bodyStr);

            CommandList tempList;
            if (!tempList.parseCommands(nestedCommands)) return false;

            for (int i = 0; i < repeatCount; ++i)
            {
                for (int j = 0; j < tempList.getTotalCommands(); ++j)
                {
                    commands.push_back(tempList.getCommand(j));
                }
            }
        }
        else
        {
            return false;
        }
    }
    return true;
}
