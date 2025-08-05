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
    FlatMemoryAllocator(size_t maximumSize, size_t pageSize, size_t maxPages);

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
    }

    std::string visualizeMemory() override
    {
        return std::string(memory.begin(), memory.end());
    }

    void allocateDemandPaged(size_t size, int processId)
    {
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
        // To ensure thread safety, we should lock the mutex for this operation.
        std::lock_guard<std::recursive_mutex> lock(memMutex);

        // 1. Call our main function to handle all the complex logic.
        //    This function will handle the page fault, load from disk,
        //    or call evictPage() if necessary.
        ensurePageIsLoaded(processId, pageIndex);

        // 2. After ensurePageIsLoaded returns, the page is GUARANTEED to be in memory.
        //    We can now safely get its physical address.
        //    Using .at() is safe here because ensurePageIsLoaded would have already
        //    thrown an error if the process or page was invalid.
        PageInfo &page = processAllocations.at(processId).pages.at(pageIndex);

        // 3. Return the pointer to the start of the page's physical frame.
        return &memory[page.startIndex];
    }

    // NEW: Function to allocate virtual address space for a variable
    uint32_t allocateVariable(int pid, size_t varSize)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);
        auto it = processAllocations.find(pid);
        if (it == processAllocations.end())
        {
            // If process info doesn't exist, create it.
            allocateDemandPaged(0, pid); // Allocate with 0 pages initially
            it = processAllocations.find(pid);
        }

        uint32_t allocatedAddress = it->second.nextVirtualAddress;
        it->second.nextVirtualAddress += varSize; // Increment heap pointer
        return allocatedAddress;
    }

    // NEW: Function to write a 16-bit value to a virtual address
    void writeValueAtVirtualAddress(int pid, uint32_t virtualAddr, uint16_t value)
    {
        // Writes a 2-byte value, little-endian
        char byte1 = value & 0xFF;        // Low byte
        char byte2 = (value >> 8) & 0xFF; // High byte

        writeToHexAddress(pid, virtualAddr, byte1);
        writeToHexAddress(pid, virtualAddr + 1, byte2);
    }

    // NEW: Function to read a 16-bit value from a virtual address
    uint16_t readValueFromVirtualAddress(int pid, uint32_t virtualAddr)
    {
        // Reads a 2-byte value, little-endian
        char byte1 = readFromHexAddress(pid, virtualAddr);
        char byte2 = readFromHexAddress(pid, virtualAddr + 1);

        // Combine bytes back into a uint16_t
        uint16_t value = static_cast<uint16_t>(static_cast<unsigned char>(byte1)) |
                         (static_cast<uint16_t>(static_cast<unsigned char>(byte2)) << 8);
        return value;
    }

    // MODIFIED: Overloaded to accept uint32_t for virtual address.
    char readFromHexAddress(int pid, uint32_t virtualAddr)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);

        size_t pageNumber = virtualAddr / pageSize;
        size_t offset = virtualAddr % pageSize;

        ensurePageIsLoaded(pid, pageNumber);

        auto &proc = processAllocations.at(pid);
        PageInfo &page = proc.pages.at(pageNumber);
        size_t physicalAddr = page.startIndex + offset;

        if (physicalAddr >= memory.size())
            throw std::out_of_range("Physical address out of bounds");

        return memory[physicalAddr];
    }

    // MODIFIED: Overloaded to accept uint32_t for virtual address.
    void writeToHexAddress(int pid, uint32_t virtualAddr, char value)
    {
        std::lock_guard<std::recursive_mutex> lock(memMutex);

        size_t pageNumber = virtualAddr / pageSize;
        size_t offset = virtualAddr % pageSize;

        ensurePageIsLoaded(pid, pageNumber);

        auto &proc = processAllocations.at(pid);
        PageInfo &page = proc.pages.at(pageNumber);
        size_t physicalAddr = page.startIndex + offset;

        if (physicalAddr >= memory.size())
            throw std::out_of_range("Physical address out of bounds");

        memory[physicalAddr] = value;
    }

    // Kept for compatibility but recommend using the uint32_t versions directly.
    char readFromHexAddress(int pid, const std::string &hexAddr)
    {
        size_t virtualAddr = std::stoul(hexAddr, nullptr, 16);
        return readFromHexAddress(pid, (uint32_t)virtualAddr);
    }
    void writeToHexAddress(int pid, const std::string &hexAddr, char value)
    {
        size_t virtualAddr = std::stoul(hexAddr, nullptr, 16);
        writeToHexAddress(pid, (uint32_t)virtualAddr, value);
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

        PageInfo &page = it->second.pages[virtualPageNumber];

        if (page.inMemory)
            return;

        // --- Start of New, Clean Logic ---

        // 1. Get a frame, either by finding a free one or by evicting.
        size_t index = findFreePage();
        if (index == SIZE_MAX)
        {
            index = evictPage();
        }

        // 2. Try to load data from the backing store into our new frame.
        //    The destination is memory[index].
        if (findAndLoadFromBackstore(pid, virtualPageNumber, &memory[index]))
        {
            // Data was successfully loaded from disk.
            // We just need to mark the allocation map.
            std::fill(allocationMap.begin() + index, allocationMap.begin() + index + pageSize, true);
        }
        else
        {
            // Page is new, so mark it as allocated (fills with '#').
            markPageAllocated(index, pageSize);
        }

        // 3. Update all data structures. This now happens for ALL loaded pages.
        loadedFramesQueue.push_back(index); // This is now called for every case!
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
                // Copy the content into the destination frame
                for (size_t i = 0; i < pageSize && i < content.size(); ++i)
                {
                    destination[i] = content[i];
                }
                return true; // Found and loaded!
            }
        }
        return false; // Did not find it.
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
        // NEW: "Heap" pointer for the process's virtual address space, tracking next available address.
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
        std::fill(memory.begin() + index, memory.begin() + index + size, '#'); // Fill with a placeholder
    }

    size_t evictPage()
    {

        // 1. Select the victim frame (the first one that was loaded)
        size_t victimFrameIndex = loadedFramesQueue.front();
        loadedFramesQueue.erase(loadedFramesQueue.begin());

        // 2. Find which process and page corresponds to this victim frame
        bool found = false;
        for (auto &proc_pair : processAllocations)
        {
            int pid = proc_pair.first;
            auto &proc_info = proc_pair.second;
            for (size_t i = 0; i < proc_info.pages.size(); ++i)
            {
                if (proc_info.pages[i].inMemory && proc_info.pages[i].startIndex == victimFrameIndex)
                {
                    // 3. Swap the victim page out to the backing store
                    swapOutToBackstore(pid, i); // This also marks the page as not in memory
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }

        // 4. Return the now-free frame index
        return victimFrameIndex;
    }
};
