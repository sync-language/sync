#include <assert.h>
#include <core_internal.h>
#include <filesystem>
#include <iostream>

int main() {
    const std::string cwd = std::filesystem::current_path().string();
    if (cwd == "/") {
        std::cerr << "Skipping relative path test running in root dir";
        return 0;
    }

    size_t foundLastSeparator = cwd.find_last_of('/');
    if (foundLastSeparator == std::string::npos) {
        foundLastSeparator = cwd.find_last_of('\\');
        assert(foundLastSeparator != std::string::npos);
    }

    const std::string parentFolderName = cwd.substr(foundLastSeparator + 1);
    const std::string relativeDir = "../" + parentFolderName;
    char absoluteBuf[4096];
    assert(sy_relative_to_absolute_path(relativeDir.c_str(), relativeDir.size(), absoluteBuf, 256));
    std::string absolute = absoluteBuf;
    assert(absolute == cwd);
}