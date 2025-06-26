#pragma once
#ifndef COMMANDLIST_H
#define COMMANDLIST_H

#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "command.hpp"
class CommandList
{
public:
CommandList(int total);
void executeCommand(int index);
void addCommand(std::string line);
void removeCommandAt(int index);
void insertCommandsAt(int index, const std::vector<std::shared_ptr<Command>>& newCommands);
std::shared_ptr<Command> getCommand(int index){
    return commands[index];
}
int getTotalCommands() {
    return totalCommands;
}
void setTotalCommands(int value) {
    totalCommands = value;
}

private:
std::vector<std::shared_ptr<Command>> commands;
int totalCommands;
};

#endif