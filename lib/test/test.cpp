#define DOCTEST_CONFIG_IMPLEMENT
#include "../src/doctest.h"
#include "../src/testing/child_process.hpp"
#include <cstring>

int main(int argc, char** argv) {
    if (argc > 1) {
        if (std::strcmp(argv[1], SYNC_CHILD_PROCESS_ARGV_1_NAME)) {
            for (int i = 0; i < argc; i++) {
                std::cout << argv[i] << std::endl;
            }
            return 0;
        }
    }

    return doctest::Context(argc, argv).run();
}
