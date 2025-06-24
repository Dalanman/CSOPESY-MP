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
CommandList();
void executeCommand(int index);
void addCommand(std::string line);

private:
std::vector<Command> commands;
int totalCommands;
};

#endif