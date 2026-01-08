#include "child_process.hpp"
#include "../core/core_internal.h"
#if defined(_WIN32) || defined(WIN32)
#define WIN32_LEAN_AND_MEAN
// clang-format off
#include <windows.h>
#include <malloc.h>
#include <memoryapi.h>
// clang-format on
#elif defined(__EMSCRIPTEN__)
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__GNUC__)
#include <linux/limits.h>
#include <unistd.h>
#endif

using namespace sy;

Result<StringUnmanaged, AllocErr> sy::getCurrentExecutablePath(Allocator& alloc) {
#if defined(_WIN32) || defined(WIN32)
    // https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=registry
    char buffer[MAX_PATH] = {'\0'};
    DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    StringSlice slice(buffer, length);
    return StringUnmanaged::copyConstructSlice(slice, alloc);
#elif defined(__APPLE__)
    char buffer[1024] = {'\0'};
    uint32_t length;
    auto result = _NSGetExecutablePath(buffer, &length);
    sy_assert(result != -1, "Failed to get current executable path");
    StringSlice slice(buffer, length);
    return StringUnmanaged::copyConstructSlice(slice, alloc);
#elif defined(__EMSCRIPTEN__)
    (void)alloc;
    return StringUnmanaged();
#elif defined(__GNUC__)
    char buffer[PATH_MAX] = {'\0'};
    ssize_t length = readlink("/proc/self/exe", buffer, PATH_MAX);
    sy_assert(length != -1, "Failed to get current executable path");
    StringSlice slice(buffer, length);
    return StringUnmanaged::copyConstructSlice(slice, alloc);
#endif
}

#if SYNC_LIB_WITH_TESTS

#include "../doctest.h"
#include <iostream>

// TEST_CASE("") {
//     Allocator alloc;
//     auto name = getCurrentExecutablePath(alloc).takeValue();
//     std::cout << name.cstr() << std::endl;

//     STARTUPINFO si;
//     PROCESS_INFORMATION pi;
//     ZeroMemory(&si, sizeof(si));
//     si.cb = sizeof(si);
//     ZeroMemory(&pi, sizeof(pi));

//     if (!CreateProcess(name.cstr(), SYNC_CHILD_PROCESS_ARGV_1_NAME " hi", nullptr, nullptr, false, 0, nullptr,
//     nullptr,
//                        &si, &pi)) {
//         std::cerr << "CreateProcess failed " << GetLastError() << std::endl;
//     } else {
//         WaitForSingleObject(pi.hProcess, INFINITE);
//     }

//     CloseHandle(pi.hProcess);
//     CloseHandle(pi.hThread);

//     name.destroy(alloc);
// }

#endif // SYNC_LIB_WITH_TESTS
