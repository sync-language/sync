# docker build --no-cache -t sync:wasm -f test/docker/wasm.Dockerfile .
# docker run --name sync-wasm-tmp sync:wasm true
# docker cp sync-wasm-tmp:/sync/reports ./reports
# docker rm sync-wasm-tmp

# cached
FROM ubuntu:24.04 AS sync-wasm-base

RUN apt-get update && \
    apt-get install -y build-essential cmake git-all python3 nodejs \
        perl perl-modules libjson-perl libcapture-tiny-perl libdatetime-perl libtimedate-perl wget && \
    rm -rf /var/lib/apt/lists/*

# lcov is just perl scripts.
RUN wget https://github.com/linux-test-project/lcov/releases/download/v2.4/lcov-2.4.tar.gz && \
    tar xf lcov-2.4.tar.gz && \
    make -C lcov-2.4 install && \
    rm -rf lcov-2.4 lcov-2.4.tar.gz

WORKDIR /sync

RUN git clone https://github.com/emscripten-core/emsdk.git && \
    cd emsdk && \
    ./emsdk install latest && \
    ./emsdk activate latest

ENV PATH="/sync/emsdk:/sync/emsdk/upstream/emscripten:${PATH}"
ENV EMSDK="/sync/emsdk"
ENV EM_CONFIG="/sync/emsdk/.emscripten"
ENV LLVM_BIN="/sync/emsdk/upstream/bin"

# not cached
FROM sync-wasm-base AS wasm-build

WORKDIR /sync
COPY . .

RUN emcmake cmake -S . -B build_dbg -DCMAKE_BUILD_TYPE=Debug -DSYNC_LIB_WITH_TESTS=ON -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_LIB_ASAN=OFF -DSYNC_LIB_TSAN=OFF -DSYNC_SKIP_TSAN_TESTS=ON -DSYNC_LIB_CODE_COVERAGE=ON
RUN cmake --build build_dbg -j4

RUN emcmake cmake -S . -B build_rel -DCMAKE_BUILD_TYPE=Release -DSYNC_LIB_WITH_TESTS=ON -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_LIB_ASAN=OFF -DSYNC_LIB_TSAN=OFF -DSYNC_SKIP_TSAN_TESTS=ON -DSYNC_LIB_CODE_COVERAGE=ON
RUN cmake --build build_rel -j4

ENV ASAN_OPTIONS="detect_leaks=0:abort_on_error=1"
RUN --network=none ctest --test-dir build_dbg --output-on-failure --verbose
RUN --network=none ctest --test-dir build_rel --output-on-failure --verbose

RUN mkdir -p reports/wasm_docker

# llvm-cov from emsdk must be used — version must match the compiler that produced the profraw files.
# CMake places test binaries in test/, so the glob is predictable — no find needed.
# set -- expands the glob into positional params: $1 is the required positional arg for llvm-cov,
# the rest become -object flags via printf.

RUN $LLVM_BIN/llvm-profdata merge -sparse build_dbg/test/*.profraw -o build_dbg/coverage.profdata
# Crazy globbing thing, sure why not.
RUN set -- build_dbg/test/*.wasm && FIRST=$1 && shift && \
    $LLVM_BIN/llvm-cov export $FIRST $(printf ' -object %s' "$@") \
        -format=lcov \
        -instr-profile=build_dbg/coverage.profdata \
        -ignore-filename-regex='.*doctest\.h' \
        > reports/wasm_docker/coverage_dbg.info

RUN $LLVM_BIN/llvm-profdata merge -sparse build_rel/test/*.profraw -o build_rel/coverage.profdata
RUN set -- build_rel/test/*.wasm && FIRST=$1 && shift && \
    $LLVM_BIN/llvm-cov export $FIRST $(printf ' -object %s' "$@") \
        -format=lcov \
        -instr-profile=build_rel/coverage.profdata \
        -ignore-filename-regex='.*doctest\.h' \
        > reports/wasm_docker/coverage_rel.info

RUN lcov -a reports/wasm_docker/coverage_dbg.info \
        -a reports/wasm_docker/coverage_rel.info \
        --output-file reports/wasm_docker/coverage.info \
        --branch-coverage \
        --ignore-errors inconsistent,inconsistent,corrupt,corrupt
RUN genhtml reports/wasm_docker/coverage.info --branch-coverage --output-directory reports/wasm_docker/html \
        --ignore-errors inconsistent,inconsistent,corrupt,corrupt
