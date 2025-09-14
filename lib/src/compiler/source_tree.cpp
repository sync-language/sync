#include "source_tree.hpp"
#include <cstdlib>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <string>

using namespace sy;

void SourceFile::destroy(Allocator& alloc) noexcept {
    this->absolutePath_.destroy(alloc);
    this->relativePath_.destroy(alloc);
    this->fileName_.destroy(alloc);
    this->fileContents_.destroy(alloc);
}

sy::SourceTree::~SourceTree() noexcept {
    for (size_t i = 0; i < this->files_.len(); i++) {
        this->files_[i].destroy(this->alloc_);
    }
    this->files_.destroy(this->alloc_);
}

Result<SourceTree, SourceTreeError> sy::SourceTree::fromDirectory(Allocator alloc, StringSlice dir) noexcept {
    SourceTree self(alloc);
    SourceFile file;

    try {
        const std::filesystem::path syncExtension = ".sync";

        const std::filesystem::path path(dir.data(), dir.data() + dir.len());
        if (!std::filesystem::exists(path)) {
            file.destroy(alloc);
            return Error(SourceTreeError::DirectoryNotExist);
        }
        if (!std::filesystem::is_directory(path)) {
            file.destroy(alloc);
            return Error(SourceTreeError::NotDirectory);
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (!std::filesystem::is_regular_file(entry.status()))
                continue;

            const auto entryPath = entry.path();
            if (entryPath.extension() != syncExtension)
                continue;

            std::string u8str = entryPath.u8string();

            { // absolute path
                auto pathResult = StringUnmanaged::copyConstructSlice(StringSlice(u8str.c_str(), u8str.size()), alloc);
                if (pathResult.hasErr()) {
                    file.destroy(alloc);
                    return Error(SourceTreeError::OutOfMemory);
                }

                new (&file.absolutePath_) StringUnmanaged(std::move(pathResult.takeValue()));
            }
            { // relative path
                auto copyPath = entryPath;
                copyPath.remove_filename();
                std::string u8relative = copyPath.u8string();
                StringSlice absoluteSlice(u8relative.c_str(), u8relative.size());
                // don't include leading slash
                StringSlice relativeSlice(absoluteSlice.data() + dir.len() + 1, absoluteSlice.len() - dir.len() - 1);
                auto pathResult = StringUnmanaged::copyConstructSlice(relativeSlice, alloc);
                if (pathResult.hasErr()) {
                    file.destroy(alloc);
                    return Error(SourceTreeError::OutOfMemory);
                }

                new (&file.relativePath_) StringUnmanaged(std::move(pathResult.takeValue()));
            }
            { // file name
                const auto stem = entryPath.stem();
                std::string u8stem = stem.u8string();
                auto fileNameResult =
                    StringUnmanaged::copyConstructSlice(StringSlice(u8stem.c_str(), u8stem.size()), alloc);
                if (fileNameResult.hasErr()) {
                    file.destroy(alloc);
                    return Error(SourceTreeError::OutOfMemory);
                }

                new (&file.fileName_) StringUnmanaged(std::move(fileNameResult.takeValue()));
            }
            { // file contents
                std::ifstream sourceFile(u8str);
                if (!sourceFile.is_open()) {
                    return Error(SourceTreeError::ErrorOpeningSourceFile);
                }

                sourceFile.seekg(0, std::ios::end); // go to end
                size_t fileSize = sourceFile.tellg();
                auto contentResult = StringUnmanaged::fillConstruct(alloc, fileSize, '\0');
                if (contentResult.hasErr()) {
                    file.destroy(alloc);
                    return Error(SourceTreeError::OutOfMemory);
                }

                new (&file.fileContents_) StringUnmanaged(std::move(contentResult.takeValue()));
                sourceFile.seekg(0, std::ios::beg);                   // back to beginning to read from
                sourceFile.read(file.fileContents_.data(), fileSize); // directly into string

                auto pushResult = self.files_.push(std::move(file), alloc);
                if (pushResult.hasErr()) {
                    file.destroy(alloc);
                    return Error(SourceTreeError::OutOfMemory);
                }
            }
        }

    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    } catch (const std::bad_alloc& e) {
        (void)e;
        file.destroy(alloc); // no leak
        return Error(SourceTreeError::OutOfMemory);
    }

    for (size_t i = 0; i < self.files_.len(); i++) {
        std::cout << "File: " << self.files_[i].absolutePath_.cstr() << std::endl;
        std::cout << "Relative: " << self.files_[i].relativePath_.cstr() << std::endl;
        std::cout << "Name: " << self.files_[i].fileName_.cstr() << std::endl;
        std::cout << "Contents:\n" << self.files_[i].fileContents_.cstr() << '\n' << std::endl;
    }

    return self;
}

#if SYNC_LIB_WITH_TESTS

#include "../doctest.h"

TEST_CASE("guh") {
    auto res =
        SourceTree::fromDirectory(Allocator(), "C:\\Users\\Admin\\Documents\\sync\\lib\\test\\source_tree_stuff");
    CHECK(res.hasValue());
}

#endif // SYNC_LIB_WITH_TESTS