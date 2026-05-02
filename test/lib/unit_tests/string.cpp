#include <doctest.h>
#include <types/string/string.hpp>
#include <types/string/string_internal.hpp>

namespace sy {
namespace internal {
struct Test_String {
    static const AtomicStringHeader* header(const String& s) { return s.impl_; }
};

struct Test_StringBuilder {
    static const AtomicStringHeader* header(const StringBuilder& s) { return s.impl_; }
    static size_t capacity(const StringBuilder& s) { return s.fullAllocatedCapacity_; }
};
} // namespace internal
} // namespace sy

using namespace sy;
using sy::internal::Test_String;
using sy::internal::Test_StringBuilder;

TEST_CASE_FIXTURE(Test_String, "[sy::String] empty string") {
    String s{};
    auto header = Test_String::header(s);
    REQUIRE_EQ(header, nullptr); // didn't allocate memory
    REQUIRE_EQ(s.len(), 0);
    REQUIRE_EQ(s.asSlice(), "");
    REQUIRE_EQ(s.cstr(), nullptr);
    REQUIRE_EQ(s, "");
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] empty string copy") {
    String s1{};
    String s2 = s1;
    auto s1header = Test_String::header(s1);
    auto s2header = Test_String::header(s2);

    REQUIRE_EQ(s2header, nullptr); // didn't allocate memory
    REQUIRE_EQ(s2.len(), 0);
    REQUIRE_EQ(s2.asSlice(), "");
    REQUIRE_EQ(s2.cstr(), nullptr);
    REQUIRE_EQ(s2, "");
    REQUIRE_EQ(s1, s2);

    REQUIRE_EQ(s1header, s2header);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] empty string hash") {
    String s{};
    const size_t first = s.hash();
    const size_t second = s.hash();
    REQUIRE_EQ(first, second);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] one character string") {
    String s = String::init("a").takeValue();
    auto header = Test_String::header(s);
    REQUIRE_NE(header, nullptr); // did allocate memory
    REQUIRE_EQ(s.len(), 1);
    REQUIRE_EQ(s.asSlice(), "a");
    REQUIRE_NE(s.cstr(), nullptr);
    REQUIRE_EQ(s, "a");

    REQUIRE_EQ(header->refCount.load(), 1);
    REQUIRE_EQ(header->hasHashStored.load(), false);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] one character string copy") {
    String s1 = String::init("a").takeValue();
    String s2 = s1;
    auto s1header = Test_String::header(s1);
    auto s2header = Test_String::header(s2);

    REQUIRE_EQ(s2.len(), 1);
    REQUIRE_EQ(s2.asSlice(), "a");
    REQUIRE_NE(s2.cstr(), nullptr);
    REQUIRE_EQ(s2, "a");
    REQUIRE_EQ(s1, s2);

    REQUIRE_EQ(s1header, s2header);
    REQUIRE_EQ(s1header->refCount.load(), 2);
    REQUIRE_EQ(s1header->hasHashStored.load(), false);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] one character cache hash") {
    String s = String::init("a").takeValue();
    auto header = Test_String::header(s);

    REQUIRE_EQ(header->hasHashStored.load(), false);

    const size_t receivedHash = s.hash();

    REQUIRE_EQ(header->hasHashStored.load(), true);
    REQUIRE_EQ(header->hashCode.load(), receivedHash);

    const size_t secondHash = s.hash();

    // hash retains second time
    REQUIRE_EQ(secondHash, receivedHash);
    REQUIRE_EQ(header->hasHashStored.load(), true);
    REQUIRE_EQ(header->hashCode.load(), receivedHash);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] small string") {
    String s = String::init("hello world!").takeValue();
    auto header = Test_String::header(s);
    REQUIRE_NE(header, nullptr); // did allocate memory
    REQUIRE_EQ(s.len(), 12);
    REQUIRE_EQ(s.asSlice(), "hello world!");
    REQUIRE_NE(s.cstr(), nullptr);
    REQUIRE_EQ(s, "hello world!");

    REQUIRE_EQ(header->refCount.load(), 1);
    REQUIRE_EQ(header->hasHashStored.load(), false);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] small string copy") {
    String s1 = String::init("hello world!").takeValue();
    String s2 = s1;
    auto s1header = Test_String::header(s1);
    auto s2header = Test_String::header(s2);
    REQUIRE_NE(s2header, nullptr); // did allocate memory
    REQUIRE_EQ(s2.len(), 12);
    REQUIRE_EQ(s2.asSlice(), "hello world!");
    REQUIRE_NE(s2.cstr(), nullptr);
    REQUIRE_EQ(s2, "hello world!");
    REQUIRE_EQ(s1, s2);

    REQUIRE_EQ(s1header, s2header);
    REQUIRE_EQ(s1header->refCount.load(), 2);
    REQUIRE_EQ(s1header->hasHashStored.load(), false);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] small cache hash") {
    String s = String::init("hello world!").takeValue();
    auto header = Test_String::header(s);

    REQUIRE_EQ(header->hasHashStored.load(), false);

    const size_t receivedHash = s.hash();

    REQUIRE_EQ(header->hasHashStored.load(), true);
    REQUIRE_EQ(header->hashCode.load(), receivedHash);

    const size_t secondHash = s.hash();

    // hash retains second time
    REQUIRE_EQ(secondHash, receivedHash);
    REQUIRE_EQ(header->hasHashStored.load(), true);
    REQUIRE_EQ(header->hashCode.load(), receivedHash);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] inline sized string") {
    char buf[] = "hello to this world!@!";
    String s = String::init(StringSlice(buf)).takeValue();
    auto header = Test_String::header(s);
    REQUIRE_NE(header, nullptr); // did allocate memory
    REQUIRE_EQ(s.len(), 22);
    REQUIRE_EQ(s.asSlice(), "hello to this world!@!");
    REQUIRE_NE(s.cstr(), nullptr);
    REQUIRE_EQ(s, "hello to this world!@!");

    REQUIRE_EQ(header->refCount.load(), 1);
    REQUIRE_EQ(header->hasHashStored.load(), false);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] inline sized string copy") {
    char buf[] = "hello to this world!@!";
    String s1 = String::init(StringSlice(buf)).takeValue();
    String s2 = s1;
    auto s1header = Test_String::header(s1);
    auto s2header = Test_String::header(s2);
    REQUIRE_NE(s2header, nullptr); // did allocate memory
    REQUIRE_EQ(s2.len(), 22);
    REQUIRE_EQ(s2.asSlice(), "hello to this world!@!");
    REQUIRE_NE(s2.cstr(), nullptr);
    REQUIRE_EQ(s2, "hello to this world!@!");
    REQUIRE_EQ(s1, s2);

    REQUIRE_EQ(s1header, s2header);
    REQUIRE_EQ(s1header->refCount.load(), 2);
    REQUIRE_EQ(s1header->hasHashStored.load(), false);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] inline sized cache hash") {
    char buf[] = "hello to this world!@!";
    String s = String::init(StringSlice(buf)).takeValue();
    auto header = Test_String::header(s);

    REQUIRE_EQ(header->hasHashStored.load(), false);

    const size_t receivedHash = s.hash();

    REQUIRE_EQ(header->hasHashStored.load(), true);
    REQUIRE_EQ(header->hashCode.load(), receivedHash);

    const size_t secondHash = s.hash();

    // hash retains second time
    REQUIRE_EQ(secondHash, receivedHash);
    REQUIRE_EQ(header->hasHashStored.load(), true);
    REQUIRE_EQ(header->hashCode.load(), receivedHash);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] big string") {
    StringSlice slice = "1234567890123456789012345678901234567890";
    String s = String::init(slice).takeValue();
    auto header = Test_String::header(s);
    REQUIRE_NE(header, nullptr); // did allocate memory
    REQUIRE_EQ(s.len(), 40);
    REQUIRE_EQ(s.asSlice(), slice);
    REQUIRE_NE(s.cstr(), nullptr);
    REQUIRE_EQ(s, slice);

    REQUIRE_EQ(header->refCount.load(), 1);
    REQUIRE_EQ(header->hasHashStored.load(), false);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] big string copy") {
    StringSlice slice = "1234567890123456789012345678901234567890";
    String s1 = String::init(slice).takeValue();
    String s2 = s1;
    auto s1header = Test_String::header(s1);
    auto s2header = Test_String::header(s2);
    REQUIRE_NE(s2header, nullptr); // did allocate memory
    REQUIRE_EQ(s2.len(), 40);
    REQUIRE_EQ(s2.asSlice(), slice);
    REQUIRE_NE(s2.cstr(), nullptr);
    REQUIRE_EQ(s2, slice);
    REQUIRE_EQ(s1, s2);

    REQUIRE_EQ(s1header, s2header);
    REQUIRE_EQ(s1header->refCount.load(), 2);
    REQUIRE_EQ(s1header->hasHashStored.load(), false);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] big string cache hash") {
    StringSlice slice = "1234567890123456789012345678901234567890";
    String s = String::init(slice).takeValue();
    auto header = Test_String::header(s);

    REQUIRE_EQ(header->hasHashStored.load(), false);

    const size_t receivedHash = s.hash();

    REQUIRE_EQ(header->hasHashStored.load(), true);
    REQUIRE_EQ(header->hashCode.load(), receivedHash);

    const size_t secondHash = s.hash();

    // hash retains second time
    REQUIRE_EQ(secondHash, receivedHash);
    REQUIRE_EQ(header->hasHashStored.load(), true);
    REQUIRE_EQ(header->hashCode.load(), receivedHash);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] concat two empty") {
    String s{};
    String concat = s.concat("").takeValue();

    auto header = Test_String::header(concat);
    REQUIRE_EQ(header, nullptr); // didn't allocate memory
    REQUIRE_EQ(s.len(), 0);
    REQUIRE_EQ(s.asSlice(), "");
    REQUIRE_EQ(s.cstr(), nullptr);
    REQUIRE_EQ(s, "");
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] concat other empty just copy") {
    String s = String::init("a").takeValue();
    String concat = s.concat("").takeValue();

    auto sHeader = Test_String::header(s);
    auto concatHeader = Test_String::header(concat);
    REQUIRE_NE(concatHeader, nullptr);
    REQUIRE_EQ(concat.len(), 1);
    REQUIRE_EQ(concat.asSlice(), "a");
    REQUIRE_NE(concat.cstr(), nullptr);
    REQUIRE_EQ(concat, "a");

    REQUIRE_EQ(sHeader, concatHeader);
    REQUIRE_EQ(concatHeader->refCount.load(), 2);
    REQUIRE_EQ(concatHeader->hasHashStored.load(), false);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] concat two non-empty small strings") {
    String s = String::init("a").takeValue();
    String concat = s.concat("a").takeValue();

    auto sHeader = Test_String::header(s);
    auto concatHeader = Test_String::header(concat);
    REQUIRE_NE(concatHeader, nullptr);
    REQUIRE_EQ(concat.len(), 2);
    REQUIRE_EQ(concat.asSlice(), "aa");
    REQUIRE_NE(concat.cstr(), nullptr);
    REQUIRE_EQ(concat, "aa");

    REQUIRE_NE(sHeader, concatHeader);
    REQUIRE_EQ(sHeader->refCount.load(), 1);
    REQUIRE_EQ(concatHeader->refCount.load(), 1);
    REQUIRE_EQ(sHeader->hasHashStored.load(), false);
    REQUIRE_EQ(concatHeader->hasHashStored.load(), false);
}

TEST_CASE_FIXTURE(Test_String, "[sy::String] concat two small strings make big") {
    String s = String::init("12345678901234567890").takeValue();
    String concat = s.concat("abcdefghijklmnopqrstuvwxyz").takeValue();

    auto sHeader = Test_String::header(s);
    auto concatHeader = Test_String::header(concat);
    REQUIRE_NE(concatHeader, nullptr);
    REQUIRE_EQ(concat.len(), 46);
    REQUIRE_EQ(concat.asSlice(), "12345678901234567890abcdefghijklmnopqrstuvwxyz");
    REQUIRE_NE(concat.cstr(), nullptr);
    REQUIRE_EQ(concat, "12345678901234567890abcdefghijklmnopqrstuvwxyz");

    REQUIRE_NE(sHeader, concatHeader);
    REQUIRE_EQ(sHeader->refCount.load(), 1);
    REQUIRE_EQ(concatHeader->refCount.load(), 1);
    REQUIRE_EQ(sHeader->hasHashStored.load(), false);
    REQUIRE_EQ(concatHeader->hasHashStored.load(), false);
}

TEST_CASE_FIXTURE(Test_StringBuilder, "[sy::StringBuilder] init") {
    StringBuilder b = StringBuilder::init().takeValue();
    auto bHeader = Test_StringBuilder::header(b);
    REQUIRE_NE(bHeader, nullptr);
    REQUIRE_EQ(bHeader->len, 0);

    // builds empty
    String s = b.build().takeValue();
    auto sHeader = Test_String::header(s);
    REQUIRE_EQ(sHeader, nullptr); // didn't allocate memory
    REQUIRE_EQ(s.len(), 0);
    REQUIRE_EQ(s.asSlice(), "");
    REQUIRE_EQ(s.cstr(), nullptr);
    REQUIRE_EQ(s, "");
}

TEST_CASE_FIXTURE(Test_StringBuilder, "[sy::StringBuilder] initWithCapacity small") {
    StringBuilder b = StringBuilder::initWithCapacity(2).takeValue();
    auto bHeader = Test_StringBuilder::header(b);
    REQUIRE_NE(bHeader, nullptr);
    REQUIRE_EQ(bHeader->len, 0);
    REQUIRE_GE(Test_StringBuilder::capacity(b), 2);

    // builds empty
    String s = b.build().takeValue();
    auto sHeader = Test_String::header(s);
    REQUIRE_EQ(sHeader, nullptr); // didn't allocate memory
    REQUIRE_EQ(s.len(), 0);
    REQUIRE_EQ(s.asSlice(), "");
    REQUIRE_EQ(s.cstr(), nullptr);
    REQUIRE_EQ(s, "");
}

TEST_CASE_FIXTURE(Test_StringBuilder, "[sy::StringBuilder] initWithCapacity bigger than inline") {
    const size_t minCapacity = internal::AtomicStringHeader::HEADER_INLINE_STRING_SIZE * 2;
    StringBuilder b = StringBuilder::initWithCapacity(minCapacity).takeValue();
    auto bHeader = Test_StringBuilder::header(b);
    REQUIRE_NE(bHeader, nullptr);
    REQUIRE_EQ(bHeader->len, 0);
    REQUIRE_GE(Test_StringBuilder::capacity(b), minCapacity);

    // builds empty
    String s = b.build().takeValue();
    auto sHeader = Test_String::header(s);
    REQUIRE_EQ(sHeader, nullptr); // didn't allocate memory
    REQUIRE_EQ(s.len(), 0);
    REQUIRE_EQ(s.asSlice(), "");
    REQUIRE_EQ(s.cstr(), nullptr);
    REQUIRE_EQ(s, "");
}

TEST_CASE_FIXTURE(Test_StringBuilder, "[sy::StringBuilder] write once small") {
    StringBuilder b = StringBuilder::init().takeValue();
    REQUIRE(b.write("a"));
    auto bHeader = Test_StringBuilder::header(b);
    REQUIRE_NE(bHeader, nullptr);
    REQUIRE_EQ(bHeader->len, 1);

    String s = b.build().takeValue();
    auto sHeader = Test_String::header(s);
    REQUIRE_NE(sHeader, nullptr);
    REQUIRE_EQ(s.len(), 1);
    REQUIRE_EQ(s.asSlice(), "a");
    REQUIRE_NE(s.cstr(), nullptr);
    REQUIRE_EQ(s, "a");
}

TEST_CASE_FIXTURE(Test_StringBuilder, "[sy::StringBuilder] write once big") {
    StringBuilder b = StringBuilder::init().takeValue();
    REQUIRE(b.write("12345678901234567890abcdefghijklmnopqrstuvwxyz"));
    auto bHeader = Test_StringBuilder::header(b);
    REQUIRE_NE(bHeader, nullptr);
    REQUIRE_EQ(bHeader->len, 46);

    // builds empty
    String s = b.build().takeValue();
    auto sHeader = Test_String::header(s);
    REQUIRE_NE(sHeader, nullptr);
    REQUIRE_EQ(s.len(), 46);
    REQUIRE_EQ(s.asSlice(), "12345678901234567890abcdefghijklmnopqrstuvwxyz");
    REQUIRE_NE(s.cstr(), nullptr);
    REQUIRE_EQ(s, "12345678901234567890abcdefghijklmnopqrstuvwxyz");
}

TEST_CASE_FIXTURE(Test_StringBuilder, "[sy::StringBuilder] write twice small") {
    StringBuilder b = StringBuilder::init().takeValue();
    REQUIRE(b.write("a"));
    REQUIRE(b.write("a"));
    auto bHeader = Test_StringBuilder::header(b);
    REQUIRE_NE(bHeader, nullptr);
    REQUIRE_EQ(bHeader->len, 2);

    String s = b.build().takeValue();
    auto sHeader = Test_String::header(s);
    REQUIRE_NE(sHeader, nullptr);
    REQUIRE_EQ(s.len(), 2);
    REQUIRE_EQ(s.asSlice(), "aa");
    REQUIRE_NE(s.cstr(), nullptr);
    REQUIRE_EQ(s, "aa");
}

TEST_CASE_FIXTURE(Test_StringBuilder, "[sy::StringBuilder] write twice makes big") {
    StringBuilder b = StringBuilder::init().takeValue();
    REQUIRE(b.write("12345678901234567890"));
    REQUIRE(b.write("abcdefghijklmnopqrstuvwxyz"));
    auto bHeader = Test_StringBuilder::header(b);
    REQUIRE_NE(bHeader, nullptr);
    REQUIRE_EQ(bHeader->len, 46);

    String s = b.build().takeValue();
    auto sHeader = Test_String::header(s);
    REQUIRE_NE(sHeader, nullptr);
    REQUIRE_EQ(s.len(), 46);
    REQUIRE_EQ(s.asSlice(), "12345678901234567890abcdefghijklmnopqrstuvwxyz");
    REQUIRE_NE(s.cstr(), nullptr);
    REQUIRE_EQ(s, "12345678901234567890abcdefghijklmnopqrstuvwxyz");
}
