#include <assert.h>
#include <core_internal.h>
#include <cstring>

int main() {
    const char* fileName = __FILE__;
    size_t len = std::strlen(fileName);

    size_t fileSize;
    assert(sy_get_file_info(fileName, len, &fileSize));
    assert(fileSize < 512); // this source file is pretty small
}