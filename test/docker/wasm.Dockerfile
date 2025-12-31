# cached
FROM ubuntu:24.04 AS wasm-base

RUN apt-get update && \
    apt-get install -y build-essential cmake git-all python3 nodejs && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /sync

RUN git clone https://github.com/emscripten-core/emsdk.git && \
    cd emsdk && \
    ./emsdk install latest && \
    ./emsdk activate latest

ENV PATH="/sync/emsdk:/sync/emsdk/upstream/emscripten:${PATH}"
ENV EMSDK="/sync/emsdk"
ENV EM_CONFIG="/sync/emsdk/.emscripten"

# not cached
FROM wasm-base AS wasm-build

WORKDIR /sync
COPY . .

RUN emcmake cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_LIB_ASAN=OFF
RUN cmake --build build --parallel
RUN --network=none ctest --test-dir build --output-on-failure --verbose
#RUN node --experimental-wasm-threads build/test/SyncTestCore_FilesystemGetFileSizeSmall.js