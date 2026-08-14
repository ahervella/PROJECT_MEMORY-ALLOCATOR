#pragma once
#include "iMemoryManager.h"
#include <map>
#include <set>
#include <vector>

#include <cstdint>

#define BIT_MAP_SIZE 32

template <class T>
class BitMapMM : public IMemoryManager
{
public:
    BitMapMM() {}
    ~BitMapMM() {}

    void* allocate(size_t size);
    void* allocateArray(size_t size);

    // could this technically be a T* ? if an array is just that with offsets?
    // Or what's the difference between T[]* and T*?
    void free(void*);
    std::vector<void*> GetMemoryPool();

private:
    class BitMapEntry
    {
    public:
        int MemPoolListIndex;

        BitMapEntry(int memoryPoolIndex) : MemPoolListIndex(memoryPoolIndex)
        {
            memset(m_bitMap, 0xff, BIT_MAP_SIZE / sizeof(uint_fast8_t));
        }

        void SetBit(int pos, bool flag);
        void SetMultipleBits(int pos, bool flag, int count);
        int SetFirstFreeBlockPos();

        void GetIndexAndOffset(int pos, int &index, int &offset)
        {
            index = pos / sizeof(uint_fast8_t);
            offset = pos % sizeof(uint_fast8_t);
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

        int GetShiftFromPos(int offset) { return BIT_MAP_SIZE - offset - 1; }

        int m_blocksAvailable = BIT_MAP_SIZE;
        uint_fast8_t m_bitMap[BIT_MAP_SIZE / sizeof(uint_fast8_t)];
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

    // hierarchy goes: chunks (given by OS) which have a corresponding
    // BitMapEntry, divided into blocks, which each correspond to 1 bit. One
    // block is the size of one object.

    // chunks pool
    std::vector<void*> m_memoryPoolList;
    std::vector<BitMapEntry> m_bitMapEntryList;
    // Both will always be 1:1 in size

    std::map<void*, ArrayMemoryInfo> m_arrayInfoMap;

    std::set<BitMapEntry*> m_freeSingleEntries;
    std::set<ArrayMemoryInfo*> m_freeArrayInfos;
};

inline BitMapMM<class Complex> gBitMapMM;