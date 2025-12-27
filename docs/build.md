# Building Sync

Sync can be included in your project through CMake, Cargo, or Zig modules, or alternatively from scratch yourself.

## CMake

```cmake
add_subdirectory(${PATH_TO_SYNC})
target_link_libraries(${YOUR_TARGET} PRIVATE SyncLib) # Also includes the relevant headers
```
