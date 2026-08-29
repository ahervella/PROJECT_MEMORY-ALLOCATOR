#include "bitMapMM.h"
#include "badAllocWithMsg.h"

template <class T>
void BitMapMM<T>::BitMapEntry::SetBit(int pos, bool flag)
{
    int index, offset;
    GetIndexAndOffset(pos, index, offset);

    int shift = GetShiftFromPos(offset);
    bool prevFlag = GetBitFromShift(index, shift);

    if (prevFlag == flag)
    {
        return;
    }

    if (flag)
    {
        m_bitMap[index] |= (0b1 << shift);
        m_blocksAvailable++;
    }
    else
    {
        m_bitMap[index] &= (~(0b1 << shift));
        m_blocksAvailable--;
    }
}

template <class T>
inline void BitMapMM<T>::BitMapEntry::SetMultipleBits(int pos, bool flag, int count)
{
    for (int i = 0; i < count; i++)
    {
        SetBit(pos + i, flag);
    }
}

template <class T>
int BitMapMM<T>::BitMapEntry::SetFirstFreeBlockPos()
{
    for (int i = 0; i < BIT_MAP_SIZE; i++)
    {
        bool val = GetBit(i);
        if (val)
        {
            SetBit(i, false);
            return i;
        }
    }

    throw BadAllocWithMsg(std::string("Error, Set First Free Block was not able to find a first free block!"));

    return -1;
}