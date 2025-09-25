#include <iostream>
#define DOCTEST_CONFIG_IMPLEMENT
#include "../src/doctest.h"

int main(int argc, char** argv) {
    std::cout << "hi\n";

    return doctest::Context(argc, argv).run();
}

TEST_CASE("guh") { CHECK(true); }