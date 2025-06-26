#include "commandList.hpp"

CommandList::CommandList() {

}

CommandList::CommandList( int total)
{
    totalCommands = total;
}

void CommandList::removeCommandAt(int index) {
    if (index >= 0 && index < commands.size())
        commands.erase(commands.begin() + index);
}

void CommandList::insertCommandsAt(int index, const std::vector<std::shared_ptr<Command>>& newCommands) {
    commands.insert(commands.begin() + index, newCommands.begin(), newCommands.end());
}

void CommandList::executeCommand(int index) {

}

void CommandList::addCommand(std::string line) {
    
}

// Helper function to split nested command strings properly
std::vector<std::string> splitNestedCommands(const std::string& input) {
    std::vector<std::string> result;
    std::string current;
    int parenDepth = 0;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        current += c;

        if (c == '(') {
            parenDepth++;
        }
        else if (c == ')') {
            parenDepth--;
        }

        if (parenDepth == 0 && (i + 1 == input.size() || input[i + 1] == ',')) {
            result.push_back(current);
            current.clear();
            if (i + 1 < input.size() && input[i + 1] == ',') i++; // skip the comma
        }
    }

    return result;
}

bool CommandList::parseCommands(std::vector<std::string> inputCommands) {
    totalCommands = inputCommands.size();

    for (std::string line : inputCommands) {
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

        if (line.find("PRINT(") == 0 && line.back() == ')') {
            std::string msg = line.substr(6, line.size() - 7);
            commands.push_back(std::make_shared<PrintCommand>(msg));
        }

        else if (line.find("DECLARE(") == 0 && line.back() == ')') {
            std::string args = line.substr(8, line.size() - 9);
            size_t comma = args.find(',');
            if (comma == std::string::npos) return false;

            std::string var = args.substr(0, comma);
            std::string val = args.substr(comma + 1);
            commands.push_back(std::make_shared<IOCommand>("DECLARE", var, "", "", std::stoi(val)));
        }

        else if ((line.find("ADD(") == 0 || line.find("SUBTRACT(") == 0) && line.back() == ')') {
            std::string op = line.substr(0, line.find('('));
            std::string args = line.substr(op.size() + 1, line.size() - op.size() - 2);

            std::vector<std::string> parts;
            std::stringstream ss(args);
            std::string part;
            while (std::getline(ss, part, ',')) parts.push_back(part);
            if (parts.size() != 3) return false;

            commands.push_back(std::make_shared<IOCommand>(op, parts[0], parts[1], parts[2]));
        }

        else if (line.find("SLEEP(") == 0 && line.back() == ')') {
            std::string ticks = line.substr(6, line.size() - 7);
            commands.push_back(std::make_shared<IOCommand>("SLEEP", ticks));
        }

        else if (line.find("FOR([") == 0 && line.back() == ')') {
            size_t bodyStart = line.find("[") + 1;
            size_t bodyEnd = line.find("]");
            size_t commaAfterBody = line.find(",", bodyEnd);

            if (bodyStart == std::string::npos || bodyEnd == std::string::npos || commaAfterBody == std::string::npos)
                return false;

            std::string bodyStr = line.substr(bodyStart, bodyEnd - bodyStart);
            std::string repeatStr = line.substr(commaAfterBody + 1, line.size() - commaAfterBody - 2);
            int repeatCount = std::stoi(repeatStr);

            std::vector<std::string> nestedCommands = splitNestedCommands(bodyStr);

            CommandList tempList;
            if (!tempList.parseCommands(nestedCommands)) return false;

            totalCommands -= 1; // account for the FOR command being replaced
            totalCommands += repeatCount * tempList.commands.size();

            for (int i = 0; i < repeatCount; ++i) {
                for (const auto& cmd : tempList.commands) {
                    commands.push_back(cmd);
                }
            }
        }

        else {
            return false; // Invalid command
        }
    }

    return true;
}
