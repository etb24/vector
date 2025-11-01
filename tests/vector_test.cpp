#include <gtest/gtest.h>
#include <stdexcept>
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

struct MoveOnly {
    int value;

    explicit MoveOnly(int v) : value(v) {}
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

    MoveOnly(MoveOnly&& other) noexcept : value(other.value)
    {
        other.value = -1;
    }

    MoveOnly& operator=(MoveOnly&&) = delete;
};

struct CopyError : std::runtime_error {
    CopyError() : std::runtime_error("copy error") {}
};

struct ThrowOnCopy {
    static inline int live_objects{0};
    static inline int copy_count{0};
    static inline int throw_on_copy{-1};

    int value;

    explicit ThrowOnCopy(int v) : value(v)
    {
        ++live_objects;
    }

    ThrowOnCopy(const ThrowOnCopy& other) : value(other.value)
    {
        if (throw_on_copy >= 0 && copy_count >= throw_on_copy) {
            ++copy_count;
            throw CopyError{};
        }
        ++copy_count;
        ++live_objects;
    }

    ThrowOnCopy(ThrowOnCopy&& other) noexcept(false) : value(other.value)
    {
        ++live_objects;
    }

    ~ThrowOnCopy()
    {
        --live_objects;
    }

    static void reset()
    {
        live_objects = 0;
        copy_count = 0;
        throw_on_copy = -1;
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

TEST_F(VectorTest, SubscriptThrowsOnOutOfRangeAccess)
{
    Vector<int> vec;
    vec.push_back(42);

    EXPECT_THROW(vec[1], std::out_of_range);

    const Vector<int>& cvec = vec;
    EXPECT_THROW(cvec[1], std::out_of_range);
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

TEST_F(VectorTest, ReallocatePrefersMoveConstruction)
{
    Vector<AllocCounter> vec;
    vec.push_back(AllocCounter(1));

    AllocCounter::reset();
    vec.push_back(AllocCounter(2)); // triggers reallocate and move existing element

    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec[0].value, 1);
    EXPECT_EQ(vec[1].value, 2);
    EXPECT_EQ(AllocCounter::copy_ctor_count, 0u);
    EXPECT_GE(AllocCounter::move_ctor_count, 2u);
}

TEST_F(VectorTest, ReallocateRollsBackWhenCopyThrows)
{
    ThrowOnCopy::reset();
    {
        Vector<ThrowOnCopy> vec;
        vec.emplace_back(1);
        vec.emplace_back(2);

        EXPECT_EQ(vec.size(), 2u);
        EXPECT_EQ(ThrowOnCopy::live_objects, 2);

        ThrowOnCopy::throw_on_copy = 1;
        EXPECT_THROW(vec.emplace_back(3), CopyError);

        EXPECT_EQ(vec.size(), 2u);
        EXPECT_EQ(vec[0].value, 1);
        EXPECT_EQ(vec[1].value, 2);
        EXPECT_EQ(ThrowOnCopy::live_objects, 2);
        EXPECT_EQ(ThrowOnCopy::copy_count, 2);
    }
    EXPECT_EQ(ThrowOnCopy::live_objects, 0);
    ThrowOnCopy::reset();
}

TEST_F(VectorTest, EmplaceBackConstructsInPlace)
{
    Vector<AllocCounter> vec;

    AllocCounter::reset();
    auto& ref = vec.emplace_back(42);

    EXPECT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec[0].value, 42);
    EXPECT_EQ(&ref, &vec.back());
    EXPECT_EQ(AllocCounter::copy_ctor_count, 0u);
    EXPECT_EQ(AllocCounter::copy_assignment_count, 0u);
}

TEST_F(VectorTest, EmplaceBackSupportsMoveOnlyTypes)
{
    Vector<MoveOnly> vec;

    vec.emplace_back(7);
    EXPECT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec[0].value, 7);

    auto& second = vec.emplace_back(9);
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(second.value, 9);
    EXPECT_EQ(vec[0].value, 7);
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

TEST_F(VectorTest, PopBackDestroysLastElement)
{
    Vector<AllocCounter> vec;
    vec.push_back(AllocCounter(1));
    vec.push_back(AllocCounter(2));

    AllocCounter::reset();
    vec.pop_back();

    EXPECT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec[0].value, 1);
    EXPECT_EQ(AllocCounter::dtor_count, 1u);
}

TEST_F(VectorTest, MoveAssignmentTransfersOwnership)
{
    Vector<AllocCounter> source;
    source.emplace_back(3);
    source.emplace_back(4);
    auto* original_data = source.data();

    Vector<AllocCounter> target;
    target.emplace_back(1);
    target.emplace_back(2);

    AllocCounter::reset();
    target = std::move(source);

    EXPECT_EQ(target.size(), 2u);
    EXPECT_EQ(target[0].value, 3);
    EXPECT_EQ(target[1].value, 4);
    EXPECT_EQ(target.data(), original_data);
    EXPECT_EQ(AllocCounter::copy_ctor_count, 0u);
    EXPECT_EQ(AllocCounter::move_ctor_count, 0u);
    EXPECT_EQ(AllocCounter::dtor_count, 2u); // old target elements destroyed

    EXPECT_EQ(source.size(), 0u);
    EXPECT_EQ(source.capacity(), 0u);
    EXPECT_EQ(source.begin(), source.end());
}

TEST_F(VectorTest, CopyConstructorCreatesIndependentVector)
{
    Vector<AllocCounter> original;
    original.push_back(AllocCounter(1));
    original.push_back(AllocCounter(2));

    AllocCounter::reset();
    Vector<AllocCounter> copy(original);

    EXPECT_EQ(copy.size(), 2u);
    EXPECT_EQ(copy[0].value, 1);
    EXPECT_EQ(copy[1].value, 2);
    EXPECT_GE(copy.capacity(), copy.size());
    EXPECT_EQ(AllocCounter::copy_ctor_count, 2u);
    EXPECT_EQ(AllocCounter::move_ctor_count, 0u);
    EXPECT_NE(copy.data(), original.data());

    original[0].value = 10;
    EXPECT_EQ(copy[0].value, 1);
}

TEST_F(VectorTest, CopyAssignmentPerformsDeepCopy)
{
    Vector<AllocCounter> source;
    source.push_back(AllocCounter(5));
    source.push_back(AllocCounter(6));

    Vector<AllocCounter> target;
    target.push_back(AllocCounter(1));
    target.push_back(AllocCounter(2));

    AllocCounter::reset();
    target = source;

    EXPECT_EQ(target.size(), 2u);
    EXPECT_EQ(target[0].value, 5);
    EXPECT_EQ(target[1].value, 6);
    EXPECT_GE(target.capacity(), target.size());
    EXPECT_EQ(AllocCounter::copy_ctor_count, 2u);
    EXPECT_EQ(AllocCounter::dtor_count, 2u);
    EXPECT_NE(target.data(), source.data());

    source[0].value = 9;
    EXPECT_EQ(target[0].value, 5);
}

TEST_F(VectorTest, CopyAssignmentHandlesSelfAssignment)
{
    Vector<int> vec;
    vec.push_back(7);
    vec.push_back(8);

    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec[0], 7);
    EXPECT_EQ(vec[1], 8);
}

TEST_F(VectorTest, StdSwapExchangesContents)
{
    Vector<int> a;
    a.push_back(1);
    a.push_back(2);
    auto* a_data = a.data();

    Vector<int> b;
    b.push_back(10);
    b.push_back(20);
    b.push_back(30);
    auto* b_data = b.data();

    std::swap(a, b);

    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a[0], 10);
    EXPECT_EQ(a[1], 20);
    EXPECT_EQ(a[2], 30);
    EXPECT_EQ(a.data(), b_data);

    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 2);
    EXPECT_EQ(b.data(), a_data);
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

TEST_F(VectorTest, DestructorDestroysAllElementsAtScopeExit)
{
    {
        Vector<AllocCounter> vec;
        vec.push_back(AllocCounter(5));
        vec.push_back(AllocCounter(10));
        AllocCounter::reset();
    }

    EXPECT_EQ(AllocCounter::dtor_count, 2u);
}

// Iterators
TEST_F(VectorTest, IteratorTraversalMatchesSequence)
{
    Vector<int> vec;
    vec.push_back(4);
    vec.push_back(8);
    vec.push_back(15);

    std::size_t index = 0;
    for (int value : vec) {
        constexpr int expected[] = {4, 8, 15};
        ASSERT_LT(index, 3u);
        EXPECT_EQ(value, expected[index]);
        ++index;
    }
    EXPECT_EQ(index, 3u);
}

TEST_F(VectorTest, EmptyReflectsContainerState)
{
    Vector<int> vec;
    EXPECT_TRUE(vec.empty());

    vec.push_back(5);
    EXPECT_FALSE(vec.empty());

    vec.pop_back();
    EXPECT_TRUE(vec.empty());
}

TEST_F(VectorTest, FrontAndBackProvideAccess)
{
    Vector<int> vec;
    vec.push_back(11);
    vec.push_back(22);

    EXPECT_EQ(vec.front(), 11);
    EXPECT_EQ(vec.back(), 22);

    const Vector<int>& cvec = vec;
    EXPECT_EQ(cvec.front(), 11);
    EXPECT_EQ(cvec.back(), 22);
}

TEST_F(VectorTest, DataExposesContiguousStorage)
{
    Vector<int> vec;
    vec.push_back(10);
    vec.push_back(20);

    int* raw = vec.data();
    ASSERT_NE(raw, nullptr);
    EXPECT_EQ(raw[0], 10);
    EXPECT_EQ(raw[1], 20);
}

TEST_F(VectorTest, ConstIteratorsDereferenceValues)
{
    Vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);

    const Vector<int>& cvec = vec;
    auto it = cvec.cbegin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(it, cvec.cend());
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

TEST_F(VectorTest, IteratorEqualityChecks)
{
    Vector<int> a;
    a.push_back(7);
    a.push_back(14);

    auto begin = a.begin();
    auto end = a.end();
    EXPECT_NE(begin, end);
    ++begin;
    ++begin;
    EXPECT_EQ(begin, end);

    const Vector<int>& ca = a;
    auto cbegin = ca.cbegin();
    auto cend = ca.cend();
    EXPECT_NE(cbegin, cend);
    ++cbegin;
    ++cbegin;
    EXPECT_EQ(cbegin, cend);

    Vector<int> b;
    b.push_back(7);
    EXPECT_NE(a.begin(), b.begin());
}

TEST_F(VectorTest, IteratorArrowAccessProvidesMember)
{
    struct Point {
        int x;
        int y;
    };

    Vector<Point> vec;
    vec.emplace_back(Point{1, 2});
    vec.emplace_back(Point{3, 4});

    auto it = vec.begin();
    EXPECT_EQ(it->x, 1);
    EXPECT_EQ(it->y, 2);

    const Vector<Point>& cvec = vec;
    auto cit = cvec.cbegin();
    EXPECT_EQ(cit->x, 1);
    EXPECT_EQ(cit->y, 2);
}
