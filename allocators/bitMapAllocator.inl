#include "bitMapAllocator.h"
#include "badAllocWithMsg.h"

template <class T>
void* BitMapAllocator<T>::allocate(size_t size)
{
    TestForConcreteClass<T>( size );

    auto freeMapI = m_freeSingleEntries.begin();
    if (freeMapI != m_freeSingleEntries.end())
    {
        return AllocateFirstFreeBlock(*freeMapI);
    }

    AllocateChunkAndInitBitMap();
    size_t lastIndex = m_bitMapEntryList.size() - 1;
    BitMapEntry* entry = &(m_bitMapEntryList[lastIndex]);

    return AllocateFirstFreeBlock(entry);
}

template <class T>
void* BitMapAllocator<T>::allocateArray(size_t size)
{
    // TODO: allow for array chunks to have multiple arrays if fit
    // TODO: allow for array's bigger than chunks by wrapping to next available
    // array space

    size_t length = size / sizeof(T);

    auto itr = m_freeArrayInfos.begin();
    for (; itr != m_freeArrayInfos.end(); itr++)
    {
        ArrayMemoryInfo* info = *itr;

        info->Length = length;
        SetMultipleBlockBits(info, true);
        T* head = static_cast<T*>(m_memoryPoolList[info->MemPoolListIndex]);
        head += info->StartPosition;
        return static_cast<void*>(head);
    }

    return AllocateArrayMemoryAndInitInfo(size);
}

template <class T>
void BitMapAllocator<T>::free(void* p)
{
    auto kvp = m_arrayInfoMap.find(p);
    if (kvp == m_arrayInfoMap.end())
    {
        SetBlockBit(p, true);
        return;
    }

    ArrayMemoryInfo* info = &kvp->second;
    SetMultipleBlockBits(info, true);
}

template <class T>
void* BitMapAllocator<T>::AllocateArrayMemoryAndInitInfo(size_t size)
{
    // TODO: wrap to another chunk if size exceeds chunk size
    size_t length = size / sizeof(T);
    void* ptr = AllocateChunkAndInitBitMap();
    m_arrayInfoMap[ptr] = ArrayMemoryInfo({m_arrayInfoMap.size(), 0, length});
    return ptr;
}

template <class T>
void* BitMapAllocator<T>::AllocateChunkAndInitBitMap()
{
    size_t size = sizeof(T) * BIT_MAP_SIZE;
    void* head = malloc(size);

    m_memoryPoolList.push_back(head);

    BitMapEntry entry(m_memoryPoolList.size() - 1);
    m_bitMapEntryList.push_back(entry);
    return head;
}

template <class T>
void* BitMapAllocator<T>::AllocateFirstFreeBlock(BitMapEntry* entry)
{
    T* p = static_cast<T*>(m_memoryPoolList[entry->MemPoolListIndex]);
    p += entry->SetFirstFreeBlockPos();

    UpdateFreeSingleEntriesList(entry);

    return static_cast<void*>(p);
}

template <class T>
void BitMapAllocator<T>::SetBlockBit(void* object, bool flag)
{
    for (size_t i = 0; i < m_bitMapEntryList.size(); i++)
    {
        BitMapEntry &mapEntry = m_bitMapEntryList[i];
        void* entryHead = m_memoryPoolList[i];

        bool withinLowerBound = entryHead <= object;
        if (!withinLowerBound)
        {
            continue;
        }

        T* headTypedPtr = static_cast<T*>(entryHead);
        bool withinUpperBound = (headTypedPtr + BIT_MAP_SIZE - 1) >= object;
        if (!withinUpperBound)
        {
            continue;
        }

        int position = static_cast<T*>(object) - headTypedPtr;
        mapEntry.SetBit(position, flag);

        UpdateFreeSingleEntriesList( &mapEntry);
        break;
    }
}

template <class T>
inline void BitMapAllocator<T>::SetMultipleBlockBits(ArrayMemoryInfo* info, bool flag)
{
    m_bitMapEntryList[info->MemPoolListIndex].SetMultipleBits(
        info->StartPosition, flag, info->Length);

    UpdateFreeArrayInfosList(info, flag);
}

template <class T>
void BitMapAllocator<T>::UpdateFreeSingleEntriesList( BitMapEntry* entry )
{
    if (entry->HasFreeBlock())
    {
        m_freeSingleEntries.insert(entry);
    }
    else
    {
        m_freeSingleEntries.erase(entry);
    }
}

template <class T>
void BitMapAllocator<T>::UpdateFreeArrayInfosList(ArrayMemoryInfo* arrayInfo, bool free)
{
    if (free)
    {
        m_freeArrayInfos.insert(arrayInfo);
    }
    else
    {
        m_freeArrayInfos.erase(arrayInfo);
    }
}