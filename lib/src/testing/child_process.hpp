#pragma once
#ifndef SY_TESTING_CHILD_PROCESS_HPP_
#define SY_TESTING_CHILD_PROCESS_HPP_

#include "../mem/allocator.hpp"
#include "../types/result/result.hpp"
#include "../types/string/string.hpp"

#ifndef SYNC_CHILD_PROCESS_ARGV_1_NAME
#define SYNC_CHILD_PROCESS_ARGV_1_NAME "SYNC_LANG_CHILD_PROCESS"
#endif

namespace sy {
Result<StringUnmanaged, AllocErr> getCurrentExecutablePath(sy::Allocator& alloc);
}

#endif // SY_TESTING_CHILD_PROCESS_HPP_