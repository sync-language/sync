# Web Assembly

Sync uses Emscripten for WASM.

## Building

No special build steps are required for this to work, but CMake with `emcmake` is preferred for simplicity.

### CMake

```sh
# Using the directory `build`
emcmake cmake -S . -B build -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_LIB_ASAN=OFF
cmake --build build
```

You may need to specify the generator. Ninja is tested and works.

```sh
emcmake <...> -G "Ninja"
```

You may optionally need to pass a `CMAKE_TOOLCHAIN_FILE` to emcmake, but this is typically not necessary.

```sh
emcmake <...> -DCMAKE_TOOLCHAIN_FILE="{EMSDK_DIRECTORY}\upstream\emscripten\cmake\Modules\Platform\Emscripten.cmake"
```
