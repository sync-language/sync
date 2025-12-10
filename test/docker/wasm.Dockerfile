FROM --platform=linux/amd64 ubuntu:24.04

RUN apt-get update && \
    apt-get install -y build-essential cmake git-all python3 nodejs && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /sync
COPY . . 

RUN git clone https://github.com/emscripten-core/emsdk.git && \
    cd emsdk && \
    ./emsdk install latest && \
    ./emsdk activate latest

ENV PATH="/sync/emsdk:/sync/emsdk/upstream/emscripten:${PATH}"
ENV EMSDK="/sync/emsdk"
ENV EM_CONFIG="/sync/emsdk/.emscripten"

RUN cd /sync && \
    emcmake cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_LIB_ASAN=OFF && \
    cmake --build build --parallel && \
    node build/test/SyncTestCore_RwLockTwoThreadShared.js
    #ctest --test-dir build 