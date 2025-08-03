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
    virtual void *allocate(size_t size, int processId) = 0;
    virtual void deallocate(int processId) = 0;
    virtual std::string visualizeMemory() = 0;
};

class FlatMemoryAllocator : public IMemoryAllocator
{
public:
    FlatMemoryAllocator(size_t maximumSize, size_t pageSize = 4096, size_t maxPages = 4)
        : maxSize(maximumSize),
          pageSize(pageSize),
          maxPagesPerProcess(maxPages),
          memory(maximumSize, '.'),
          allocationMap(maximumSize, false)
    {
        for (size_t i = 0; i <= maxSize - pageSize; i += pageSize)
        {
            freeFrames.push_back(i);
        }
    }

    void setMaxMemorySize(int maximumSize)
    {
        maxSize = maximumSize;
    }

    void *allocate(size_t size, int processId) override
    {
        size_t numPages = (size + pageSize - 1) / pageSize;
        if (numPages > maxPagesPerProcess)
            numPages = maxPagesPerProcess;

        ProcessInfo procInfo;
        procInfo.totalPages = numPages;

        for (size_t page = 0; page < numPages; ++page)
        {
            size_t index = findFreePage();
            if (index == SIZE_MAX)
            {
                deallocate(processId);
                return nullptr;
            }
            markPageAllocated(index, pageSize);
            procInfo.pages.push_back({index, true});
        }

        processAllocations[processId] = procInfo;
        return &memory[processAllocations[processId].pages[0].startIndex];
    }

    void deallocate(int processId) override
    {
        auto it = processAllocations.find(processId);
        if (it != processAllocations.end())
        {
            for (const auto &page : it->second.pages)
            {
                if (page.inMemory)
                {
                    for (size_t i = page.startIndex; i < page.startIndex + pageSize; ++i)
                    {
                        memory[i] = '.';
                        allocationMap[i] = false;
                    }
                    freeFrames.push_back(page.startIndex);
                }
            }
            processAllocations.erase(it);
        }

        std::remove("csopesy-backing-store.txt");
    }

    std::string visualizeMemory() override
    {
        return std::string(memory.begin(), memory.end());
    }

    void allocateDemandPaged(size_t size, int processId)
    {
        size_t numPages = (size + pageSize - 1) / pageSize;
        if (numPages > maxPagesPerProcess)
            numPages = maxPagesPerProcess;

        ProcessInfo procInfo;
        procInfo.totalPages = numPages;

        for (size_t page = 0; page < numPages; ++page)
        {
            procInfo.pages.push_back({SIZE_MAX, false});
        }

        processAllocations[processId] = procInfo;
    }

    size_t getProcessCount() const
    {
        return processAllocations.size();
    }

    char *accessPage(int processId, size_t pageIndex)
    {
        auto it = processAllocations.find(processId);
        if (it == processAllocations.end())
            return nullptr;

        ProcessInfo &proc = it->second;
        if (pageIndex >= proc.pages.size())
            return nullptr;

        PageInfo &page = proc.pages[pageIndex];

        if (!page.inMemory)
        {
            // First try to swap in from backing store
            if (!swapInFromBackstore(processId, pageIndex))
            {
                // Not in backing store → allocate blank frame
                size_t index = findFreePage();
                if (index == SIZE_MAX)
                    return nullptr;

                markPageAllocated(index, pageSize);
                page.startIndex = index;
            }

            page.inMemory = true;
        }

        return &memory[page.startIndex];
    }

    bool hasAllocation(int processId) const
    {
        return processAllocations.find(processId) != processAllocations.end();
    }

    void *getProcessMemoryPointer(int processId) const
    {
        auto it = processAllocations.find(processId);
        if (it != processAllocations.end() && !it->second.pages.empty())
        {
            return const_cast<char *>(&memory[it->second.pages[0].startIndex]);
        }
        return nullptr;
    }

    void getMemorySnapshot(int quantum)
    {
        std::ofstream outFile("memory_stamp_" + std::to_string(quantum) + ".txt");

        auto now = std::chrono::system_clock::now();
        std::time_t timeStamp = std::chrono::system_clock::to_time_t(now);
        std::tm timeInfo;
        localtime_s(&timeInfo, &timeStamp);
        outFile << "Timestamp: (" << std::put_time(&timeInfo, "%m/%d/%Y %I:%M:%S%p") << ")\n";

        outFile << "Number of processes in memory: " << getProcessCount() << "\n";
        int tempExternalFragmentation = (4 - getProcessCount()) * 4096;
        outFile << "Total external fragmentation in KB: " << tempExternalFragmentation << " \n\n";
        outFile << "----end---- = 16384\n"
                << std::endl;

        std::vector<std::tuple<size_t, int, size_t>> blocks;
        for (const auto &entry : processAllocations)
        {
            int processId = entry.first;
            for (const auto &page : entry.second.pages)
            {
                blocks.emplace_back(page.startIndex + pageSize, processId, page.startIndex);
            }
        }

        std::sort(blocks.begin(), blocks.end(), [](const auto &a, const auto &b)
                  { return std::get<0>(a) > std::get<0>(b); });

        for (const auto &block : blocks)
        {
            size_t upper_limit = std::get<0>(block);
            int processId = std::get<1>(block);
            size_t lower_limit = std::get<2>(block);
            outFile << upper_limit << "\n";
            outFile << "P" << processId << "\n";
            outFile << lower_limit << "\n\n";
        }

        outFile << "----start----- = 0" << std::endl;
        outFile.close();
    }

    size_t getTotalFreeMemory() const
    {
        size_t free = 0;
        for (bool frame : freeFrames)
        {
            if (!frame)
                free += pageSize;
        }
        return free;
    }

    void swapOutToBackstore(int processId, size_t pageIndex)
    {
        auto &proc = processAllocations[processId];
        if (pageIndex >= proc.pages.size() || !proc.pages[pageIndex].inMemory)
            return;

        std::ofstream outFile("csopesy-backing-store.txt", std::ios::app);
        size_t start = proc.pages[pageIndex].startIndex;
        outFile << "P" << processId << "_page" << pageIndex << " ";
        for (size_t i = start; i < start + pageSize; ++i)
        {
            outFile << memory[i];
            memory[i] = '.';
            allocationMap[i] = false;
        }
        outFile << "\n";

        proc.pages[pageIndex].inMemory = false;
        totalPagesPagedIn++;
        freeFrames.push_back(start);
    }

    bool swapInFromBackstore(int processId, size_t pageIndex)
    {
        auto &proc = processAllocations[processId];
        if (pageIndex >= proc.pages.size() || proc.pages[pageIndex].inMemory)
            return false;

        size_t index = findFreePage();
        if (index == SIZE_MAX)
            return false;

        std::ifstream inFile("csopesy-backing-store.txt");
        std::string line;
        std::string targetPrefix = "P" + std::to_string(processId) + "_page" + std::to_string(pageIndex) + " ";

        while (std::getline(inFile, line))
        {
            if (line.rfind(targetPrefix, 0) == 0)
            {
                std::string content = line.substr(targetPrefix.length());
                for (size_t i = 0; i < pageSize && i < content.size(); ++i)
                {
                    memory[index + i] = content[i];
                    allocationMap[index + i] = true;
                }
                break;
            }
        }

        proc.pages[pageIndex].startIndex = index;
        proc.pages[pageIndex].inMemory = true;
        totalPagesPagedIn++;
        return true;
    }

    size_t getTotalPagesPagedIn() const
    {
        return totalPagesPagedIn;
    }

    size_t getTotalPagesPagedOut() const
    {
        return totalPagesPagedOut;
    }

private:
    struct PageInfo
    {
        size_t startIndex;
        bool inMemory;
    };

    struct ProcessInfo
    {
        std::vector<PageInfo> pages;
        size_t totalPages;
    };

    size_t maxSize;
    size_t pageSize;
    size_t maxPagesPerProcess;
    std::vector<char> memory;
    std::vector<bool> allocationMap;
    std::unordered_map<int, ProcessInfo> processAllocations;
    std::vector<size_t> freeFrames;
    size_t totalPagesPagedIn = 0;
    size_t totalPagesPagedOut = 0;

    size_t findFreePage()
    {
        if (freeFrames.empty())
            return SIZE_MAX;
        size_t index = freeFrames.back();
        freeFrames.pop_back();
        return index;
    }

    void markPageAllocated(size_t index, size_t size)
    {
        std::fill(allocationMap.begin() + index, allocationMap.begin() + index + size, true);
        std::fill(memory.begin() + index, memory.begin() + index + size, '#');
    }
};
