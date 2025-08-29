# Vendor Dependencies

Sync itself does not depend on any third party libraries. For testing however, sync does rely on [doctest](https://github.com/doctest/doctest). For better usability with CTest, some doctest scripts are added.

- [doctest.cmake](https://github.com/doctest/doctest/blob/master/scripts/cmake/doctest.cmake)
  - Gives access to `doctest_discover_tests()`
- [doctestAddTests.cmake](https://github.com/doctest/doctest/blob/master/scripts/cmake/doctestAddTests.cmake)
  - `doctest.cmake` relies on this

Unfortunately due to CMake policy CMP0054, using `FetchContent_Declare()` doesn't seem to work.
