#pragma once
#ifndef COMMANDLIST_H
#define COMMANDLIST_H

#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <windows.h>
#include "command.hpp"

class CommandList
{
public:
CommandList();
CommandList(int total);
void executeCommand(int index);
void addCommand(std::string line);
void removeCommandAt(int index);
void insertCommandsAt(int index, const std::vector<std::shared_ptr<Command>>& newCommands);
bool parseCommands(std::vector<std::string> inputCommands);
int getSize() { return commands.size(); }

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