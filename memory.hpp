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
#include <mutex>
#include <cstdint>

class IMemoryAllocator
{
public:
    virtual ~IMemoryAllocator() = default;
    virtual void *allocate(size_t size, int processId) = 0;
    virtual void deallocate(int processId) = 0;
    virtual std::string visualizeMemory() = 0;
};

class FlatMemoryAllocator : public IMemoryAllocator
{
public:
    // --- CORRECTED CONSTRUCTOR IMPLEMENTATION ---
    // The definition is now inside the class header.
    FlatMemoryAllocator(size_t maximumSize, size_t pageSize, size_t maxPages)
        : maxSize(maximumSize),
          pageSize(pageSize),
          maxPagesPerProcess(maxPages),
          memory(maximumSize, '.'),
          allocationMap(maximumSize, false)
    {
        // This loop is the critical part that was missing.
        // It populates the freeFrames list with all available frames at the start.
        for (size_t i = 0; i < maximumSize; i += pageSize)
        {
            freeFrames.push_back(i);
        }
    }

    void *allocate(size_t size, int processId) override
    {
        size_t numPages = (size + pageSize - 1) / pageSize;
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
        std::lock_guard<std::recursive_mutex> lock(memMutex);
        auto it = processAllocations.find(processId);
        if (it != processAllocations.end())
        {
            for (const auto &page : it->second.pages)
            {
                if (page.inMemory)
                {
                    freeFrames.push_back(page.startIndex);
                    auto &queue = loadedFramesQueue;
                    queue.erase(std::remove(queue.begin(), queue.end(), page.startIndex), queue.end());

                    for (size_t i = page.startIndex; i < page.startIndex + pageSize; ++i)
                    {
                        if (i < memory.size())
                        {
                            memory[i] = '.';
                            allocationMap[i] = false;
                        }
                    }
                }
            }
            processAllocations.erase(it);
        }
    }

    std::string visualizeMemory() override
    {
        return std::string(memory.begin(), memory.end());
    }

    void allocateDemandPaged(size_t size, int processId)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);
        size_t numPages = (size + pageSize - 1) / pageSize;
        ProcessInfo procInfo;
        procInfo.totalPages = numPages;
        for (size_t page = 0; page < numPages; ++page)
        {
            procInfo.pages.push_back({SIZE_MAX, false});
        }
        processAllocations[processId] = procInfo;
    }

    bool hasAllocation(int processId) const
    {
        return processAllocations.find(processId) != processAllocations.end();
    }

    char *accessPage(int processId, size_t pageIndex)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);
        ensurePageIsLoaded(processId, pageIndex);
        PageInfo &page = processAllocations.at(processId).pages.at(pageIndex);
        return &memory[page.startIndex];
    }

    uint32_t allocateVariable(int pid, size_t varSize)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);
        auto it = processAllocations.find(pid);
        if (it == processAllocations.end())
        {
            allocateDemandPaged(0, pid);
            it = processAllocations.find(pid);
        }
        uint32_t allocatedAddress = it->second.nextVirtualAddress;
        it->second.nextVirtualAddress += varSize;
        return allocatedAddress;
    }

    void writeValueAtVirtualAddress(int pid, uint32_t virtualAddr, uint16_t value)
    {
        char byte1 = value & 0xFF;
        char byte2 = (value >> 8) & 0xFF;
        writeToHexAddress(pid, virtualAddr, byte1);
        writeToHexAddress(pid, virtualAddr + 1, byte2);
    }

    uint16_t readValueFromVirtualAddress(int pid, uint32_t virtualAddr)
    {
        char byte1 = readFromHexAddress(pid, virtualAddr);
        char byte2 = readFromHexAddress(pid, virtualAddr + 1);
        uint16_t value = static_cast<uint16_t>(static_cast<unsigned char>(byte1)) |
                         (static_cast<uint16_t>(static_cast<unsigned char>(byte2)) << 8);
        return value;
    }

    void ensurePageIsLoaded(int pid, size_t virtualPageNumber)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);
        auto it = processAllocations.find(pid);
        if (it == processAllocations.end())
            throw std::runtime_error("Invalid process ID in ensurePageIsLoaded: " + std::to_string(pid));

        while (virtualPageNumber >= it->second.pages.size())
        {
            it->second.pages.push_back({SIZE_MAX, false});
            it->second.totalPages++;
        }

        PageInfo &page = it->second.pages.at(virtualPageNumber);
        if (page.inMemory)
            return;

        size_t index = findFreePage();
        if (index == SIZE_MAX)
        {
            index = evictPage();
        }

        if (findAndLoadFromBackstore(pid, virtualPageNumber, &memory[index]))
        {
            std::fill(allocationMap.begin() + index, allocationMap.begin() + index + pageSize, true);
        }
        else
        {
            markPageAllocated(index, pageSize);
        }

        loadedFramesQueue.push_back(index);
        page.startIndex = index;
        page.inMemory = true;
        totalPagesPagedIn++;
    }

    size_t getTotalFreeMemory() const
    {
        return freeFrames.size() * pageSize;
    }

    size_t getTotalPagesPagedIn() const
    {
        return totalPagesPagedIn;
    }

    size_t getTotalPagesPagedOut() const
    {
        return totalPagesPagedOut;
    }

    size_t getPageSize() const { return pageSize; }

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
        uint32_t nextVirtualAddress = 0;
    };

    size_t maxSize;
    size_t pageSize;
    size_t maxPagesPerProcess;
    std::vector<char> memory;
    std::vector<bool> allocationMap;
    std::unordered_map<int, ProcessInfo> processAllocations;
    std::vector<size_t> freeFrames;
    std::recursive_mutex memMutex;
    size_t totalPagesPagedIn = 0;
    size_t totalPagesPagedOut = 0;
    std::vector<size_t> loadedFramesQueue;

    void writeToHexAddress(int pid, uint32_t virtualAddr, char value)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);
        size_t pageNumber = virtualAddr / pageSize;
        size_t offset = virtualAddr % pageSize;
        ensurePageIsLoaded(pid, pageNumber);
        PageInfo &page = processAllocations.at(pid).pages.at(pageNumber);
        size_t physicalAddr = page.startIndex + offset;
        if (physicalAddr < memory.size())
        {
            memory[physicalAddr] = value;
        }
    }
    char readFromHexAddress(int pid, uint32_t virtualAddr)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);
        size_t pageNumber = virtualAddr / pageSize;
        size_t offset = virtualAddr % pageSize;
        ensurePageIsLoaded(pid, pageNumber);
        PageInfo &page = processAllocations.at(pid).pages.at(pageNumber);
        size_t physicalAddr = page.startIndex + offset;
        if (physicalAddr >= memory.size())
            throw std::out_of_range("Physical address out of bounds");
        return memory[physicalAddr];
    }

    bool findAndLoadFromBackstore(int processId, size_t pageIndex, char *destination)
    {
        std::ifstream inFile("csopesy-backing-store.txt");
        if (!inFile)
            return false;
        std::string line;
        std::string targetPrefix = "P" + std::to_string(processId) + "_page" + std::to_string(pageIndex) + " ";
        while (std::getline(inFile, line))
        {
            if (line.rfind(targetPrefix, 0) == 0)
            {
                std::string content = line.substr(targetPrefix.length());
                for (size_t i = 0; i < pageSize && i < content.size(); ++i)
                {
                    destination[i] = content[i];
                }
                return true;
            }
        }
        return false;
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
        totalPagesPagedOut++;
        freeFrames.push_back(start);
    }

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

    size_t evictPage()
    {
        if (loadedFramesQueue.empty())
        {
            throw std::runtime_error("CRITICAL: Eviction called but no frames are loaded. State is inconsistent.");
        }
        size_t victimFrameIndex = loadedFramesQueue.front();
        loadedFramesQueue.erase(loadedFramesQueue.begin());
        bool found = false;
        for (auto &proc_pair : processAllocations)
        {
            int pid = proc_pair.first;
            auto &proc_info = proc_pair.second;
            for (size_t i = 0; i < proc_info.pages.size(); ++i)
            {
                if (proc_info.pages[i].inMemory && proc_info.pages[i].startIndex == victimFrameIndex)
                {
                    swapOutToBackstore(pid, i);
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }
        if (!found)
        {
            // This case can happen if a process was deallocated without cleaning the queue.
            // We can just return the frame index, as it's effectively free now.
        }
        return victimFrameIndex;
    }
};