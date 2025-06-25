#include "commandList.hpp"

CommandList::CommandList()
{
}

void CommandList::removeCommandAt(int index) {
    if (index >= 0 && index < commands.size())
        commands.erase(commands.begin() + index);
}

void CommandList::insertCommandsAt(int index, const std::vector<std::shared_ptr<Command>>& newCommands) {
    commands.insert(commands.begin() + index, newCommands.begin(), newCommands.end());
}