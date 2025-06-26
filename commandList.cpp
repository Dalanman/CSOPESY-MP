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

bool CommandList::parseCommands(std::vector<std::string> inputCommands) {
    totalCommands = inputCommands.size(); // Start with unparsed size

    for (std::string line : inputCommands) {
        line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());

        // PRINT(HELLO WORLD FROM PROCESS)
        if (line.find("PRINT(") == 0 && line.back() == ')') {
            std::string msg = line.substr(6, line.size() - 7);
            commands.push_back(std::make_shared<PrintCommand>(msg));
        }

        // DECLARE(x,100)
        else if (line.find("DECLARE(") == 0 && line.back() == ')') {
            std::string args = line.substr(8, line.size() - 9);
            size_t comma = args.find(',');
            if (comma == std::string::npos) return false;

            std::string var = args.substr(0, comma);
            std::string val = args.substr(comma + 1);
            commands.push_back(std::make_shared<IOCommand>("DECLARE", var, "", "", std::stoi(val)));
        }

        // ADD(x,y,z) or SUBTRACT(a,b,c)
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

        // SLEEP(5)
        else if (line.find("SLEEP(") == 0 && line.back() == ')') {
            std::string ticks = line.substr(6, line.size() - 7);
            commands.push_back(std::make_shared<IOCommand>("SLEEP", ticks));
        }

        // FOR([...],N)
        else if (line.find("FOR([") == 0 && line.back() == ')') {
            size_t bodyStart = line.find("[") + 1;
            size_t bodyEnd = line.find("]");
            size_t commaAfterBody = line.find(",", bodyEnd);

            if (bodyStart == std::string::npos || bodyEnd == std::string::npos || commaAfterBody == std::string::npos)
                return false;

            std::string bodyStr = line.substr(bodyStart, bodyEnd - bodyStart);
            std::string repeatStr = line.substr(commaAfterBody + 1, line.size() - commaAfterBody - 2);
            int repeatCount = std::stoi(repeatStr);

            // Split nested commands by ")," — same assumption: each ends with )
            std::vector<std::string> nestedCommands;
            while (true) {
                size_t endPos = bodyStr.find("),");
                if (endPos == std::string::npos) break;
                std::string part = bodyStr.substr(0, endPos + 1);
                nestedCommands.push_back(part);
                bodyStr.erase(0, endPos + 2);
            }
            if (!bodyStr.empty()) nestedCommands.push_back(bodyStr);

            // Parse nested commands
            CommandList tempList;
            if (!tempList.parseCommands(nestedCommands)) return false;

            // Adjust totalCommands
            totalCommands -= 1; // Removing FOR
            totalCommands += repeatCount * tempList.commands.size();

            for (int i = 0; i < repeatCount; ++i) {
                for (const auto& cmd : tempList.commands) {
                    commands.push_back(cmd);
                }
            }
        }

        else {
            return false;
        }
    }

    return true;
}
