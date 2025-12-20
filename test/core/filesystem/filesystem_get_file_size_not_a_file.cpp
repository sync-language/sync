#include <assert.h>
#include <core_internal.h>
#include <string>

int main() {
    std::string fileName = __FILE__;
    fileName += "a"; // .cppa

    size_t fileSize;
    assert(sy_get_file_info(fileName.c_str(), fileName.size(), &fileSize) == false);
}