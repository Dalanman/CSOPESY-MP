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
            if (!swapInFromBackstore(processId, pageIndex))
            {
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

    char readByteAtAddress(uint32_t address)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);
        if (address >= memory.size())
        {
            throw std::out_of_range("Read address out of range");
        }
        return memory[address];
    }

    void writeByteAtAddress(uint32_t address, char value)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);
        if (address >= memory.size())
        {
            throw std::out_of_range("Write address out of range");
        }
        memory[address] = value;
    }

    char readFromHexAddress(int pid, const std::string &hexAddr)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);

        // Convert hex string (e.g., "0x1A3F") to integer address
        size_t virtualAddr = std::stoul(hexAddr, nullptr, 16);
        size_t pageNumber = virtualAddr / pageSize;
        size_t offset = virtualAddr % pageSize;

        ensurePageIsLoaded(pid, pageNumber); // ensure it's paged in

        auto &proc = processAllocations.at(pid);
        PageInfo &page = proc.pages.at(pageNumber);
        size_t physicalAddr = page.startIndex + offset;

        if (physicalAddr >= memory.size())
            throw std::out_of_range("Physical address out of bounds");

        return memory[physicalAddr];
    }

    void writeToHexAddress(int pid, const std::string &hexAddr, char value)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);

        size_t virtualAddr = std::stoul(hexAddr, nullptr, 16);
        size_t pageNumber = virtualAddr / pageSize;
        size_t offset = virtualAddr % pageSize;

        ensurePageIsLoaded(pid, pageNumber); // ensure it's paged in

        auto &proc = processAllocations.at(pid);
        PageInfo &page = proc.pages.at(pageNumber);
        size_t physicalAddr = page.startIndex + offset;

        if (physicalAddr >= memory.size())
            throw std::out_of_range("Physical address out of bounds");

        memory[physicalAddr] = value;
    }

    void ensurePageIsLoaded(int pid, size_t virtualPageNumber)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);

        auto it = processAllocations.find(pid);

        /* 
        std::cout << "ensurePageIsLoaded: pid=" << pid
            << ", found=" << (it != processAllocations.end() ? "yes" : "no")
            << std::endl;
        std::cout << "virtualPageNumber=" << virtualPageNumber
            << ", allocated pages=" << it->second.pages.size()
            << std::endl;
        */
        if (it == processAllocations.end())
            throw std::runtime_error("Invalid process");

        while (virtualPageNumber >= it->second.pages.size())
        {
            it->second.pages.push_back({ SIZE_MAX, false }); // Add unmapped page
            it->second.totalPages++;
        }

        PageInfo &page = it->second.pages[virtualPageNumber];

        if (page.inMemory)
            return;

        if (page.startIndex == SIZE_MAX) {
            // Fresh allocation
            size_t index = findFreePage();
            if (index == SIZE_MAX)
                throw std::runtime_error("Out of memory");
            markPageAllocated(index, pageSize);
            page.startIndex = index;
            page.inMemory = true;
            totalPagesPagedIn++;
        }
        else {
            // Swapping in
            if (swapInFromBackstore(pid, virtualPageNumber)) {
                totalPagesPagedIn++;
            }
        }
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
