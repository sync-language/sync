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

Result<SourceTree, SourceTreeError> sy::SourceTree::allFilesInDirectoryRecursive(Allocator alloc,
                                                                                 StringSlice dir) noexcept {
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
                StringSlice noFileNameSlice(u8relative.c_str(), u8relative.size());

                StringSlice relativeSlice = [&noFileNameSlice, &dir]() {
                    // dont include leading slash
                    StringSlice temp(noFileNameSlice.data() + dir.len() + 1, noFileNameSlice.len() - dir.len() - 1);
                    if (temp.len() == 0)
                        return temp;
                    // dont include ending slash
                    return StringSlice(temp.data(), temp.len() - 1);
                }();

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
        file.destroy(alloc); // no leak
        return Error(SourceTreeError::UnknownError);
    } catch (const std::bad_alloc& e) {
        (void)e;
        file.destroy(alloc); // no leak
        return Error(SourceTreeError::OutOfMemory);
    }

    return self;
}

#if SYNC_LIB_WITH_TESTS

#include "../doctest.h"

TEST_CASE("guh") {
    auto res = SourceTree::allFilesInDirectoryRecursive(
        Allocator(), "C:\\Users\\Admin\\Documents\\sync\\lib\\test\\source_tree_stuff");
    CHECK(res.hasValue());

    SourceTree tree = res.takeValue();

    for (size_t i = 0; i < tree.files().len(); i++) {
        std::cout << "File: " << tree.files()[i].absolutePath().data() << std::endl;
        std::cout << "Relative: " << tree.files()[i].relativePath().data() << std::endl;
        std::cout << "Name: " << tree.files()[i].fileName().data() << std::endl;
        std::cout << "Contents:\n" << tree.files()[i].contents().data() << '\n' << std::endl;
    }
}

#endif // SYNC_LIB_WITH_TESTS