#pragma once
#include <string>

using namespace std;

class ConfigReader
{
public:
	enum SchedulerType { FCFS, RR }; 
	ConfigReader();
	~ConfigReader();

	void readConfig();
	void debug();

	int getNumCpu() { return numCpu; };
	SchedulerType getSchedulerType() { return schedulerType; };
	int getQuantum() { return quantum; };
	int getBPF() { return batchProcessFreq; };
	int getDelayPerExec() { return delayPerExec; };
    int getMinIns() { return minIns; };
	int getMaxIns() { return maxIns; };

private:
	int numCpu;
	SchedulerType schedulerType;
	int quantum;
	int batchProcessFreq;
	int minIns;
	int maxIns;
	int delayPerExec;
};