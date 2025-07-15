#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstddef>
#include <optional>

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
