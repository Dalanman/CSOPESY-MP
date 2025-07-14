#include <string>
#include <vector>
#include <unordered_map>
#include <cstddef>
class IMemoryAllocator
{
public:
    virtual void *allocate(size_t size) = 0;
    virtual void deallocate(void *ptr) = 0;
    virtual std::string visualizeMemory() = 0;
};

class FlatMemoryAllocator : public IMemoryAllocator
{
public:
    FlatMemoryAllocator(size_t maximumSize) : maxSize(maximumSize), allocatedSize(0),
                                              memory(maximumSize, '.'), allocationMap(maximumSize, false)
    {
    }

    void *allocate(size_t size) override
    {
        for (size_t i = 0; i <= maxSize - size; ++i)
        {
            if (!allocationMap[i] && canAllocateAt(i, size))
            {
                allocateAt(i, size);
                return &memory[i];
            }
        }
        return nullptr;
    }

    void deallocate(void *ptr) override
    {
        size_t index = static_cast<char *>(ptr) - &memory[0];
        if (allocationMap[index])
        {
            deallocateAt(index);
        }
    }

    std::string visualizeMemory() override
    {
        return std::string(memory.begin(), memory.end());
    }

    size_t calculateExternalFragmentation(size_t minBlockSize) const
    {
        size_t totalFragmented = 0;
        size_t currentFreeBlockSize = 0;

        for (bool allocated : allocationMap)
        {
            if (!allocated)
            {
                ++currentFreeBlockSize;
            }
            else
            {
                if (currentFreeBlockSize > 0 && currentFreeBlockSize < minBlockSize)
                {
                    totalFragmented += currentFreeBlockSize;
                }
                currentFreeBlockSize = 0;
            }
        }

        if (currentFreeBlockSize > 0 && currentFreeBlockSize < minBlockSize)
        {
            totalFragmented += currentFreeBlockSize;
        }

        return totalFragmented;
    }

private:
    size_t maxSize;
    size_t allocatedSize;
    std::vector<char> memory;
    std::vector<bool> allocationMap;

    bool canAllocateAt(size_t index, size_t size) const
    {
        if (index + size > maxSize)
            return false;
        for (size_t i = index; i < index + size; ++i)
            if (allocationMap[i])
                return false;
        return true;
    }

    void allocateAt(size_t index, size_t size)
    {
        std::fill(allocationMap.begin() + index, allocationMap.begin() + index + size, true);
        std::fill(memory.begin() + index, memory.begin() + index + size, '#');
        allocatedSize += size;
    }

    void deallocateAt(size_t index)
    {
        allocationMap[index] = false;
        memory[index] = '.';
    }
};
