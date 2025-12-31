# cached
FROM --platform=linux/amd64 ubuntu:24.04 AS ubuntu-x64-base

RUN apt-get update && \
    apt-get install -y build-essential cmake && \
    rm -rf /var/lib/apt/lists/*

# not cached
FROM ubuntu-x64-base AS ubuntu-x64-build

WORKDIR /sync
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Debug -DSYNC_LIB_WITH_TESTS=ON -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_SKIP_TSAN_TESTS=ON
RUN cmake --build build --parallel

# LSan doesn't work in Docker/qemu
ENV ASAN_OPTIONS="detect_leaks=0:abort_on_error=1"
RUN --network=none ctest --test-dir build --output-on-failure 
