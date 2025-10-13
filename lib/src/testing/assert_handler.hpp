//! API
#ifndef SY_TESTING_ASSERT_HANDLER_HPP_
#define SY_TESTING_ASSERT_HANDLER_HPP_

namespace sy {
using AssertHandler = void (*)(const char* message);

AssertHandler getAssertHandler() noexcept;
void setAssertHandler(AssertHandler handler) noexcept;
} // namespace sy

#endif // SY_TESTING_ASSERT_HANDLER_HPP_