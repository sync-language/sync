#include "assert_handler.hpp"
#include <atomic>
#include <iostream>

static_assert(sizeof(sy::AssertHandler) == sizeof(void*));
static_assert(sizeof(sy::AssertHandler) == sizeof(std::atomic<sy::AssertHandler>));

namespace sy {
namespace detail {

AssertHandler defaultHandler = [](const char* message) {
    try {
        std::cerr << message << std::endl;
    } catch (...) {
    }
};

std::atomic<AssertHandler>& actualHandler() {
    static std::atomic<AssertHandler> handler = defaultHandler;
    return handler;
}
} // namespace detail

AssertHandler getAssertHandler() noexcept { return detail::actualHandler().load(); }

void setAssertHandler(AssertHandler handler) noexcept { detail::actualHandler().store(handler); }

} // namespace sy