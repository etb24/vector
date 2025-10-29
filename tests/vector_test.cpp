#include <gtest/gtest.h>
#include <utility>
#include "../Vector.h"

namespace {
struct AllocCounter {
    static inline unsigned default_ctor_count{0};
    static inline unsigned copy_ctor_count{0};
    static inline unsigned move_ctor_count{0};
    static inline unsigned copy_assignment_count{0};
    static inline unsigned dtor_count{0};

    int value;

    AllocCounter() : value{0}
    {
        ++default_ctor_count;
    }

    explicit AllocCounter(int v) : value{v} {}

    AllocCounter(const AllocCounter& other) : value(other.value)
    {
        ++copy_ctor_count;
    }

    AllocCounter& operator=(const AllocCounter& other)
    {
        AllocCounter(other).swap(*this);
        ++copy_assignment_count;
        return *this;
    }

    void swap(AllocCounter& other) noexcept
    {
        std::swap(value, other.value);
    }

    AllocCounter(AllocCounter&& other) noexcept : value{other.value}
    {
        ++move_ctor_count;
    }

    ~AllocCounter()
    {
        ++dtor_count;
    }

    static void reset()
    {
        default_ctor_count = 0;
        copy_ctor_count = 0;
        move_ctor_count = 0;
        copy_assignment_count = 0;
        dtor_count = 0;
    }

    bool operator==(const AllocCounter& other) const
    {
        return value == other.value;
    }
};

class VectorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        AllocCounter::reset();
    }

    void TearDown() override
    {
        AllocCounter::reset();
    }
};
} // namespace

// Constructors
TEST_F(VectorTest, DefaultConstructedVectorIsEmpty)
{
    Vector<int> vec;

    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
    EXPECT_EQ(vec.begin(), vec.end());
}

TEST_F(VectorTest, MoveConstructorTransfersOwnership)
{
    Vector<int> original;
    original.push_back(11);
    original.push_back(22);

    const auto original_capacity = original.capacity();

    Vector<int> moved(std::move(original));

    EXPECT_EQ(moved.size(), 2);
    EXPECT_EQ(moved[0], 11);
    EXPECT_EQ(moved[1], 22);
    EXPECT_EQ(moved.capacity(), original_capacity);

    EXPECT_EQ(original.size(), 0);
    EXPECT_EQ(original.capacity(), 0);
    EXPECT_EQ(original.begin(), original.end());
}

// Element access
TEST_F(VectorTest, ConstSubscriptProvidesConstAccess)
{
    Vector<int> vec;
    vec.push_back(7);
    vec.push_back(9);

    const Vector<int>& const_ref = vec;
    EXPECT_EQ(const_ref[0], 7);
    EXPECT_EQ(const_ref[1], 9);
}

// Capacity
TEST_F(VectorTest, CapacityExpandsAsElementsAreAdded)
{
    Vector<int> vec;
    std::size_t last_capacity = vec.capacity();

    for (int i = 0; i < 32; ++i) {
        vec.push_back(i);
        EXPECT_EQ(vec[i], i);
        EXPECT_GE(vec.capacity(), vec.size());
        if (vec.capacity() != last_capacity) {
            EXPECT_GT(vec.capacity(), last_capacity);
            last_capacity = vec.capacity();
        }
    }
}

// Modifiers
TEST_F(VectorTest, PushBackWithLValueStoresElementsAndUpdatesSize)
{
    Vector<int> vec;
    int value = 21;
    vec.push_back(value);

    EXPECT_EQ(vec.size(), 1);
    EXPECT_GE(vec.capacity(), 1u);
    EXPECT_EQ(vec[0], 21);
}

TEST_F(VectorTest, PushBackWithRValuePrefersMoveConstruction)
{
    Vector<AllocCounter> vec;

    const AllocCounter payload(7);
    vec.push_back(payload); // copy
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(AllocCounter::copy_ctor_count, 1u);
    EXPECT_EQ(AllocCounter::move_ctor_count, 0u);

    AllocCounter::reset();
    vec.push_back(AllocCounter(9)); // move
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(AllocCounter::copy_ctor_count, 0u);
    EXPECT_GE(AllocCounter::move_ctor_count, 1u);
    EXPECT_EQ(vec[1].value, 9);
}

TEST_F(VectorTest, PopBackRemovesLastElementWhenNotEmpty)
{
    Vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    vec.pop_back();
    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[1], 2);
}

TEST_F(VectorTest, PopBackOnEmptyVectorIsNoOp)
{
    Vector<int> vec;
    vec.pop_back();

    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.begin(), vec.end());
}

TEST_F(VectorTest, ClearDestroysAllElementsAndPreservesCapacity)
{
    Vector<AllocCounter> vec;
    vec.push_back(AllocCounter(1));
    vec.push_back(AllocCounter(2));
    auto original_capacity = vec.capacity();

    AllocCounter::reset();
    vec.clear();

    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), original_capacity);
    EXPECT_EQ(AllocCounter::dtor_count, 2u);

    vec.push_back(AllocCounter(42));
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0].value, 42);
}

// Iterators
TEST_F(VectorTest, IteratorTraversalMatchesSequence)
{
    Vector<int> vec;
    vec.push_back(4);
    vec.push_back(8);
    vec.push_back(15);

    int expected[] = {4, 8, 15};
    std::size_t index = 0;
    for (int value : vec) {
        ASSERT_LT(index, 3u);
        EXPECT_EQ(value, expected[index]);
        ++index;
    }
    EXPECT_EQ(index, 3u);
}

TEST_F(VectorTest, IteratorSupportsRandomAccessLikeOperations)
{
    Vector<int> vec;
    vec.push_back(100);
    vec.push_back(200);
    vec.push_back(300);

    auto it = vec.begin();
    EXPECT_EQ(*it, 100);
    ++it;
    EXPECT_EQ(*it, 200);
    it++;
    EXPECT_EQ(*it, 300);
    --it;
    EXPECT_EQ(*it, 200);
    it--;
    EXPECT_EQ(*it, 100);
    EXPECT_EQ(it[2], 300);
}
