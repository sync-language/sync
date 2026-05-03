#include <atomic>
#include <doctest.h>
#include <string>
#include <string_view>
#include <thread>
#include <threading/generation/gen_pool.hpp>
#include <threading/generation/gen_pool_internal.hpp>
#include <types/string/string.hpp>
#include <types/type_info.hpp>
#include <vector>

namespace sy {
namespace internal {
struct Test_GenPool {
    static internal::GenPoolImpl* impl(GenPool& self) { return self.impl_; }
};

struct Test_GenOwner {
    template <typename T> static uint64_t gen(const GenOwner<T>& g) { return g.gen_; }
    template <typename T> static GenTypedPool::Chunk* chunk(const GenOwner<T>& g) {
        return reinterpret_cast<GenTypedPool::Chunk*>(g.chunk_);
    }
    template <typename T> static uint64_t index(const GenRef<T>& g) { return g.objectIndex_; }
};

struct Test_GenRef {
    template <typename T> static uint64_t gen(const GenRef<T>& g) { return g.gen_; }
    template <typename T> static GenTypedPool::Chunk* chunk(const GenOwner<T>& g) {
        return reinterpret_cast<GenTypedPool::Chunk*>(g.chunk_);
    }
    template <typename T> static uint64_t index(const GenRef<T>& g) { return g.objectIndex_; }
};
} // namespace internal
} // namespace sy

using namespace sy;
using sy::internal::GenPoolImpl;
using sy::internal::GenTypedPool;
using Chunk = sy::internal::GenTypedPool::Chunk;
using sy::String;

TEST_CASE("[sy::GenPool] add int and mutate via owner and ref") {
    GenPool pool = GenPool::init().takeValue();

    GenOwner<int> owner = pool.add<int>(5).takeValue();
    REQUIRE_EQ(owner.load().takeValue(), 5);

    REQUIRE(owner.store(10).hasValue());
    REQUIRE_EQ(owner.load().takeValue(), 10);

    GenRef<int> ref = owner.ref();
    REQUIRE(ref.store(15).hasValue());
    REQUIRE_EQ(owner.load().takeValue(), 15);
    REQUIRE_EQ(ref.load().takeValue(), 15);
}

TEST_CASE("[sy::GenPool] add String and mutate via owner and ref") {
    GenPool pool = GenPool::init().takeValue();

    GenOwner<String> owner = pool.add<String>(String::init("a").takeValue()).takeValue();
    REQUIRE_EQ(owner.load().takeValue(), "a");

    REQUIRE(owner.store(String::init("b").takeValue()).hasValue());
    REQUIRE_EQ(owner.load().takeValue(), "b");

    GenRef<String> ref = owner.ref();
    REQUIRE(ref.store(String::init("c").takeValue()).hasValue());
    REQUIRE_EQ(owner.load().takeValue(), "c");
    REQUIRE_EQ(ref.load().takeValue(), "c");
}

TEST_CASE("[sy::GenPool] int high contention many threads") {
    constexpr int NUM_THREADS = 3;
    constexpr int ITERATIONS = 10000;
    constexpr int RANGE_SIZE = 1000000;

    GenPool pool = GenPool::init().takeValue();
    GenOwner<int> owner = pool.add<int>(0).takeValue();

    auto threadFn = [&owner, NUM_THREADS](int tid) {
        GenRef<int> ref = owner.ref();
        for (int i = 0; i < ITERATIONS; i++) {
            const int toWrite = tid * RANGE_SIZE + i;
            REQUIRE(ref.store(toWrite).hasValue());

            const int loaded = ref.load().takeValue();
            const int decodedTid = loaded / RANGE_SIZE;
            REQUIRE_GE(decodedTid, 0);
            REQUIRE_LT(decodedTid, NUM_THREADS);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);
    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back(threadFn, t);
    }
    for (auto& th : threads) {
        th.join();
    }

    const int finalLoaded = owner.load().takeValue();
    const int finalTid = finalLoaded / RANGE_SIZE;
    REQUIRE_GE(finalTid, 0);
    REQUIRE_LT(finalTid, NUM_THREADS);
}

TEST_CASE("[sy::GenPool] String high contention many threads") {
    constexpr int NUM_THREADS = 3;
    constexpr int ITERATIONS = 10000;
    constexpr size_t MAX_FILLER_LEN = 128;

    GenPool pool = GenPool::init().takeValue();
    GenOwner<String> owner = pool.add<String>(String::init(",0,0").takeValue()).takeValue();

    auto threadFn = [&owner, NUM_THREADS, ITERATIONS, MAX_FILLER_LEN](int tid) {
        GenRef<String> ref = owner.ref();
        for (int i = 0; i < ITERATIONS; i++) {
            const size_t fillerLen = static_cast<size_t>((i + tid)) % MAX_FILLER_LEN;
            const char fillerChar = static_cast<char>('a' + tid);
            const std::string toWrite = std::string(fillerLen, fillerChar) + "," +
                                        std::to_string(tid) + "," + std::to_string(i);
            REQUIRE(ref.store(String::init(StringSlice(toWrite.data(), toWrite.size())).takeValue())
                        .hasValue());

            const String loaded = ref.load().takeValue();
            const std::string_view sv = static_cast<std::string_view>(loaded);

            const size_t lastComma = sv.rfind(',');
            REQUIRE_NE(lastComma, std::string_view::npos);
            const size_t secondLastComma = sv.rfind(',', lastComma - 1);
            REQUIRE_NE(secondLastComma, std::string_view::npos);

            const int decodedTid = std::stoi(
                std::string(sv.substr(secondLastComma + 1, lastComma - secondLastComma - 1)));
            REQUIRE_GE(decodedTid, 0);
            REQUIRE_LT(decodedTid, NUM_THREADS);

            const int decodedCounter = std::stoi(std::string(sv.substr(lastComma + 1)));
            REQUIRE_GE(decodedCounter, 0);
            REQUIRE_LT(decodedCounter, ITERATIONS);

            const std::string_view fillerView = sv.substr(0, secondLastComma);
            REQUIRE_LT(fillerView.size(), MAX_FILLER_LEN);
            const char expectedChar = static_cast<char>('a' + decodedTid);
            for (char c : fillerView) {
                REQUIRE_EQ(c, expectedChar);
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);
    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back(threadFn, t);
    }
    for (auto& th : threads) {
        th.join();
    }

    const String finalLoaded = owner.load().takeValue();
    const std::string_view finalSv = static_cast<std::string_view>(finalLoaded);
    const size_t finalLastComma = finalSv.rfind(',');
    REQUIRE_NE(finalLastComma, std::string_view::npos);
    const size_t finalSecondLastComma = finalSv.rfind(',', finalLastComma - 1);
    REQUIRE_NE(finalSecondLastComma, std::string_view::npos);
    const int finalTid = std::stoi(std::string(
        finalSv.substr(finalSecondLastComma + 1, finalLastComma - finalSecondLastComma - 1)));
    REQUIRE_GE(finalTid, 0);
    REQUIRE_LT(finalTid, NUM_THREADS);
}
