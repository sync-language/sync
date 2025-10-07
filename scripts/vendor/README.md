# Vendor Dependencies

Sync itself does not depend on any third party libraries. For testing however, sync does rely on a few things

- [doctest](https://github.com/doctest/doctest)
- [ios-cmake](https://github.com/leetal/ios-cmake)

For better usability with CTest, some doctest scripts are added.

- [doctest.cmake](https://github.com/doctest/doctest/blob/master/scripts/cmake/doctest.cmake)
  - Gives access to `doctest_discover_tests()`
- [doctestAddTests.cmake](https://github.com/doctest/doctest/blob/master/scripts/cmake/doctestAddTests.cmake)
  - `doctest.cmake` relies on this

For simplifying the iOS toolchain setup for testing, the cmake script is added.

- [ios.toolchain.cmake](https://github.com/leetal/ios-cmake/blob/master/ios.toolchain.cmake)

Unfortunately due to CMake policy CMP0054, using `FetchContent_Declare()` doesn't seem to work.
