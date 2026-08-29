#include <gtest/gtest.h>

#include "allocators/customMemoryAllocator.h"
#include "allocators/badAllocWithMsg.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

struct TestObj
{
    int a;
    int b;
    TestObj() {}
    TestObj(int a_, int b_) : a(a_), b(b_) {}
};

template <class AllocatorT>
class AllocatorTest : public ::testing::Test
{
protected:
    AllocatorT allocator;
};

using AllocatorTypes = ::testing::Types<
    BasicPoolMM<TestObj>,
    BasicPoolTSMM<TestObj>,
    BitMapMM<TestObj>,
    BitMapTSMM<TestObj>>;
TYPED_TEST_SUITE(AllocatorTest, AllocatorTypes);

TYPED_TEST(AllocatorTest, AllocateReturnsUsableMemory)
{
    TestObj* obj = new (this->allocator) TestObj(3, 4);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->a, 3);
    EXPECT_EQ(obj->b, 4);

    operator delete<TestObj>(obj, this->allocator);
}

TYPED_TEST(AllocatorTest, AllocationsAreDistinctAndNonOverlapping)
{
    constexpr int COUNT = 10;
    std::vector<void*> ptrs;

    for (int i = 0; i < COUNT; i++)
    {
        void* p = this->allocator.allocate(sizeof(TestObj));
        ASSERT_NE(p, nullptr);

        ptrs.push_back(p);
    }

    std::sort(ptrs.begin(), ptrs.end());

    for (size_t i = 1; i < ptrs.size(); i++)
    {
        uintptr_t prevAddr = reinterpret_cast<uintptr_t>(ptrs[i - 1]);
        uintptr_t currAddr = reinterpret_cast<uintptr_t>(ptrs[i]);
        EXPECT_GE(currAddr - prevAddr, sizeof(TestObj));
    }

    for (void* p : ptrs)
    {
        this->allocator.free(p);
    }
}

TYPED_TEST(AllocatorTest, FreedSlotIsReusable)
{
    TestObj* obj1 = new (this->allocator) TestObj(1, 2);
    ASSERT_NE(obj1, nullptr);
    operator delete<TestObj>(obj1, this->allocator);

    TestObj* obj2 = new (this->allocator) TestObj(5, 6);
    ASSERT_NE(obj2, nullptr);
    EXPECT_EQ(obj2->a, 5);
    EXPECT_EQ(obj2->b, 6);

    operator delete<TestObj>(obj2, this->allocator);
}

TYPED_TEST(AllocatorTest, AllocatingPastOneChunkStillWorks)
{
    //BasicPoolMM::POOLSIZE and BitMapMM::BIT_MAP_SIZE are 32
    constexpr int COUNT = 40;
    std::vector<void*> ptrs;
    for (int i = 0; i < COUNT; i++)
    {
        TestObj* p = new (this->allocator) TestObj(i, i * 2);
        ASSERT_NE(p, nullptr);
        ptrs.push_back(p);
    }

    for (int i = 0; i < COUNT; i++)
    {
        TestObj* obj = static_cast<TestObj*>(ptrs[i]);
        EXPECT_EQ(obj->a, i);
        EXPECT_EQ(obj->b, i * 2);
    }

    for (void* p : ptrs)
    {
        operator delete<TestObj>(p, this->allocator);
    }
}

TYPED_TEST(AllocatorTest, OversizedRequestThrows)
{
    EXPECT_THROW(this->allocator.allocate(sizeof(TestObj) + 1), BadAllocWithMsg);
}

TYPED_TEST(AllocatorTest, ArrayAllocateAndFreeRoundTrips)
{
    constexpr int kCount = 5;
    
    TestObj* arr = new (this->allocator) TestObj[kCount];
    ASSERT_NE(arr, nullptr);

    for (int i = 0; i < kCount; i++)
    {
        new (&arr[i]) TestObj(i, i + 100);
    }

    for (int i = 0; i < kCount; i++)
    {
        EXPECT_EQ(arr[i].a, i);
        EXPECT_EQ(arr[i].b, i + 100);

    }

    operator delete<TestObj>(arr, this->allocator);
}

template <class AllocatorT>
class ThreadSafeAllocatorTest : public ::testing::Test
{
protected:
    AllocatorT allocator;
};

using ThreadSafeAllocatorTypes = ::testing::Types<BasicPoolTSMM<TestObj>, BitMapTSMM<TestObj>>;
TYPED_TEST_SUITE(ThreadSafeAllocatorTest, ThreadSafeAllocatorTypes);

TYPED_TEST(ThreadSafeAllocatorTest, ConcurrentAllocateFreeIsSafe)
{
    constexpr int kThreads = 8;
    constexpr int kItersPerThread = 500;

    std::mutex liveMutex;
    std::unordered_set<void*> livePointers;
    std::atomic<bool> failed{false};

    auto worker = [&]()
    {
        for (int i = 0; i < kItersPerThread && !failed; i++)
        {
            TestObj* obj = new (this->allocator) TestObj(i, i);
            if (!obj)
            {
                failed = true;
                return;
            }

            {
                std::lock_guard<std::mutex> lock(liveMutex);
                if (!livePointers.insert(obj).second)
                {
                    // Same address handed out while already live elsewhere -
                    // indicates a race in the allocator's locking.
                    failed = true;
                    return;
                }
            }

            EXPECT_EQ(obj->a, i);

            {
                std::lock_guard<std::mutex> lock(liveMutex);
                livePointers.erase(obj);
            }

            operator delete<TestObj>(obj, this->allocator);
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; i++)
    {
        threads.emplace_back(worker);
    }
    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_FALSE(failed);
}
