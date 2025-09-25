#define DOCTEST_CONFIG_IMPLEMENT
#include "../src/doctest.h"
#include "../src/testing/child_process.hpp"
#include <cstring>

int main(int argc, char** argv) {

    std::cout << "hi" << std::endl;

    return doctest::Context(argc, argv).run();
}
