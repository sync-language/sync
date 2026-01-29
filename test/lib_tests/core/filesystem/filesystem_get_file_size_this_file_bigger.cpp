#include <assert.h>
#include <core_internal.h>
#include <cstring>

int main() {
    const char* fileName = __FILE__;
    size_t len = std::strlen(fileName);

    size_t fileSize;
    assert(sy_get_file_info(fileName, len, &fileSize));
    assert(fileSize > 512);  // this source file is not as small as the other one
    assert(fileSize < 1024); // this source file is not more than 1KB
}

// add extra stuff
// 12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890