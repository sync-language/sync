# Testing

## Test Types

Two specific types of tests are needed. These are normal unit/system/integration tests, as well as failure testing. All normal tests are required for all supported targets and languages. Failure testing is for the actual implementation of sync, as well as testing written in the sync programming language. Failure testing may not be done through exceptions, as sync does not support exceptions at the language level, and the C++ implementation should assume no reasonable exceptions occur, or that any exceptions that do occur, such as when interacting with the standard library, do not result in invalid sync program state.

Due to the testing requirements of this library, a custom tester is used and embedded within the project.

This testing library is inspired by [doctest](https://github.com/doctest/doctest) and [Google Test](https://github.com/google/googletest)

TODO support [VSCode Testing API](https://code.visualstudio.com/api/extension-guides/testing). Example at [vscode-catch2-test-adapter](https://github.com/matepek/vscode-catch2-test-adapter).

### Unit / System / Integration Testing

For practical purposes, the naming of these tests are interchangable. They should pass without failure, and may all run within the normal process. Exceptions should not be explicitly tested for apart from interacting with the C++ standard library.

### Failure Testing

These tests are expected to fail, and should fail in ways that would be considered unrecoverable, such as segmentation faulting due to writing to read-only memory. These tests will exist within a separate process for C++, and a copy of the `sy::Program` for the language level tests.

## Library Tests

All tests are compiled with the program, unless `SYNC_LIB_NO_TESTS` is defined.

### CMake

When targeting `Release`, `SYNC_LIB_NO_TESTS` will be defined, and tests will not be compiled with the by default.
