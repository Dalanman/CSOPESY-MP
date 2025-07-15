#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstddef>
#include <optional>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <algorithm>

class IMemoryAllocator
{
public:
    virtual void* allocate(size_t size, int processId) = 0;
    virtual void deallocate(int processId) = 0;
    virtual std::string visualizeMemory() = 0;
};

class FlatMemoryAllocator : public IMemoryAllocator
{
public:
    FlatMemoryAllocator(size_t maximumSize)
        : maxSize(maximumSize),
        memory(maximumSize, '.'),
        allocationMap(maximumSize, false) {
    }

    void setMaxMemorySize(int maximumSize) {
        maxSize = maximumSize;
    }

    void* allocate(size_t size, int processId) override
    {
        for (size_t i = 0; i <= maxSize - size; ++i)
        {
            if (!allocationMap[i] && canAllocateAt(i, size))
            {
                allocateAt(i, size, processId);
                return &memory[i];
            }
        }
        return nullptr;
    }

    void deallocate(int processId) override
    {
        auto it = processAllocations.find(processId);
        if (it != processAllocations.end())
        {
            size_t start = it->second.startIndex;
            size_t size = it->second.size;
            for (size_t i = start; i < start + size; ++i)
            {
                allocationMap[i] = false;
                memory[i] = '.';
            }
            processAllocations.erase(it);
        }
    }

    std::string visualizeMemory() override
    {
        return std::string(memory.begin(), memory.end());
    }

    size_t getProcessCount() const
    {
        return processAllocations.size();
    }

    size_t getExternalFragmentation(size_t minBlockSize) const
    {
        size_t totalFragmented = 0;
        size_t currentFreeBlock = 0;

        for (bool allocated : allocationMap)
        {
            if (!allocated)
            {
                ++currentFreeBlock;
            }
            else
            {
                if (currentFreeBlock > 0 && currentFreeBlock < minBlockSize)
                {
                    totalFragmented += currentFreeBlock;
                }
                currentFreeBlock = 0;
            }
        }

        if (currentFreeBlock > 0 && currentFreeBlock < minBlockSize)
        {
            totalFragmented += currentFreeBlock;
        }

        return totalFragmented;
    }

    bool hasAllocation(int processId) const {
        return processAllocations.find(processId) != processAllocations.end();
    }

    void* getProcessMemoryPointer(int processId) const {
        auto it = processAllocations.find(processId);
        if (it != processAllocations.end()) {
            return const_cast<char*>(&memory[it->second.startIndex]);
        }
        return nullptr;
    }

    void getMemorySnapshot(int quantum) {
        std::ofstream outFile("memory_stamp_" + std::to_string(quantum) + ".txt");

        auto now = std::chrono::system_clock::now();
        std::time_t timeStamp = std::chrono::system_clock::to_time_t(now);
        std::tm timeInfo;
        localtime_s(&timeInfo, &timeStamp);
        outFile << "Timestamp: " << std::put_time(&timeInfo, "%F %T") << "\n";

        outFile << "Number of processes in memory: " << getProcessCount() << "\n";

        int tempExternalFragmentation = (4 - getProcessCount()) * 4096;
        // getExternalFragmentation(16) returns 0 kaya ganito muna
        outFile << "Total external fragmentation in KB: " << tempExternalFragmentation << " \n\n";

        // Print upper and lower memory address limits for each process
        outFile << "----end---- = 16384\n" << std::endl;
  
        std::vector<std::tuple<size_t, int, size_t>> blocks;
        for (const auto& entry : processAllocations) {
            int processId = entry.first;
            const ProcessInfo& info = entry.second;
            size_t lower_limit = info.startIndex;
            size_t upper_limit = info.startIndex + info.size;
            blocks.emplace_back(upper_limit, processId, lower_limit);
        }
        std::sort(blocks.begin(), blocks.end(), [](const auto& a, const auto& b) {
            return std::get<0>(a) > std::get<0>(b); // descending by upper_limit
        });
        for (const auto& block : blocks) {
            size_t upper_limit = std::get<0>(block);
            int processId = std::get<1>(block);
            size_t lower_limit = std::get<2>(block);
            outFile << upper_limit << "\n";
            outFile << "P" << processId << "\n";
            outFile << lower_limit << "\n";
            outFile << "\n";
        }
        outFile << "----start---- = 0" << std::endl;
        outFile.close();
    }


private:
    struct ProcessInfo
    {
        size_t startIndex;
        size_t size;
    };

    size_t maxSize;
    std::vector<char> memory;
    std::vector<bool> allocationMap;
    std::unordered_map<int, ProcessInfo> processAllocations;  // changed to int

    bool canAllocateAt(size_t index, size_t size) const
    {
        if (index + size > maxSize)
            return false;
        for (size_t i = index; i < index + size; ++i)
        {
            if (allocationMap[i])
                return false;
        }
        return true;
    }

    void allocateAt(size_t index, size_t size, int processId)  // changed to int
    {
        std::fill(allocationMap.begin() + index, allocationMap.begin() + index + size, true);
        std::fill(memory.begin() + index, memory.begin() + index + size, '#');
        processAllocations[processId] = { index, size };
    }
};
