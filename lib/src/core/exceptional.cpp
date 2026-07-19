#include "exceptional.hpp"
#include <filesystem>
#include <ios>
#include <new>
#include <stdexcept>
#include <system_error>

namespace sy {
namespace internal {
SY_API Exceptional translateCurrentException() noexcept {
    try {
        throw;
    } catch (const std::bad_array_new_length&) { // must precede std::bad_alloc
        return Exceptional::Capacity;
    } catch (const std::bad_alloc&) {
        return Exceptional::OOM;
    } catch (const std::length_error&) {
        return Exceptional::Capacity;
    } catch (const std::out_of_range&) {
        return Exceptional::Bounds;
    } catch (const std::filesystem::filesystem_error&) { // must precede std::system_error
        return Exceptional::Io;
    } catch (const std::ios_base::failure&) { // must precede std::system_error
        return Exceptional::Io;
    } catch (const std::system_error&) {
        return Exceptional::System;
    } catch (const std::overflow_error&) {
        return Exceptional::Arithmetic;
    } catch (const std::underflow_error&) {
        return Exceptional::Arithmetic;
    } catch (const std::range_error&) {
        return Exceptional::Arithmetic;
    } catch (...) { // foreign/non-std throwables must not escape this noexcept boundary
        return Exceptional::Other;
    }
}
} // namespace internal
} // namespace sy
