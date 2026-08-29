#pragma once
#include "iMemoryAllocator.h"
#include <map>
#include <set>
#include <vector>

#include <cstdint>

#define BIT_MAP_SIZE 32

template <class T>
class BitMapAllocator : public IMemoryAllocator
{
public:
    BitMapAllocator<T>() {}
    ~BitMapAllocator<T>() {}

    void* allocate(size_t size) override;
    void* allocateArray(size_t) override;
    void free(void*) override;
    //need to specify this free so we don't trigger a possibly overriden free version
    void freeArray(void* p) override { BitMapAllocator<T>::free(p); };

    std::vector<void*> GetMemoryPool();

private:
    class BitMapEntry
    {
    public:
        int MemPoolListIndex;

        BitMapEntry(int memoryPoolIndex) : MemPoolListIndex(memoryPoolIndex)
        {
            memset(m_bitMap, 0xff, BIT_MAP_ARRAY_LEN);
        }

        void SetBit(int pos, bool flag);
        void SetMultipleBits(int pos, bool flag, int count);
        int SetFirstFreeBlockPos();

        void GetIndexAndOffset(int pos, int &index, int &offset)
        {
            int bitCount = sizeof(uint_fast8_t) * 8;
            index = pos / bitCount;
            offset = pos % bitCount;
        }

        bool GetBit(int pos)
        {
            int index, offset;
            GetIndexAndOffset(pos, index, offset);
            return GetBitFromShift(index, GetShiftFromPos(offset));
        }

        bool HasFreeBlock() { return m_blocksAvailable > 0; }

    private:
        bool GetBitFromShift(int index, int shiftAmount)
        {
            return (m_bitMap[index] >> shiftAmount & 0b1) > 0;
        }

        int GetShiftFromPos(int offset) { return 8 - offset - 1; }

        int m_blocksAvailable = BIT_MAP_SIZE;
        
        static constexpr int BIT_MAP_ARRAY_LEN = BIT_MAP_SIZE / (sizeof(uint_fast8_t) * 8);
        uint_fast8_t m_bitMap[BIT_MAP_ARRAY_LEN];
    };

    typedef struct ArrayMemoryInfo
    {
        size_t MemPoolListIndex;
        size_t StartPosition;
        size_t Length;
    } ArrayMemoryInfo;

    void* AllocateArrayMemoryAndInitInfo(size_t size);
    void* AllocateChunkAndInitBitMap();
    void* AllocateFirstFreeBlock(BitMapEntry* bitMapEntry);

    void SetBlockBit(void* object, bool flag);
    void SetMultipleBlockBits(ArrayMemoryInfo* info, bool flag);

    void UpdateFreeSingleEntriesList(BitMapEntry* bitMapEntry);
    void UpdateFreeArrayInfosList( ArrayMemoryInfo* freeArrayInfo, bool free);

    // hierarchy goes: chunks (given by OS) which have a corresponding
    // BitMapEntry, divided into blocks, which each correspond to 1 bit. One
    // block is the size of one object.

    // chunks pool
    std::vector<void*> m_memoryPoolList;
    std::vector<BitMapEntry> m_bitMapEntryList;
    // Both will always be 1:1 in size

    std::map<void*, ArrayMemoryInfo> m_arrayInfoMap;

    // only save to do with respect to the m_bitMapEntryList because that only
    // ever changes size once this free list is empty, otherwise would need a
    // pointer stable upon growth structure like std::deque
    
    std::set<BitMapEntry*> m_freeSingleEntries;
    std::set<ArrayMemoryInfo*> m_freeArrayInfos;
};

#include "bitMapAllocator.inl"
#include "bitMapAllocatorEntry.inl"