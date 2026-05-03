# docker build --no-cache -t sync:ubuntu -f test/docker/ubuntu.Dockerfile .
# docker run --name sync-ubuntu-tmp sync:ubuntu true
# docker cp sync-ubuntu-tmp:/sync/reports ./reports
# docker rm sync-ubuntu-tmp

# cached
FROM ubuntu:24.04 AS sync-ubuntu-base

RUN apt-get update && \
    apt-get install -y build-essential cmake perl perl-modules libjson-perl libcapture-tiny-perl libdatetime-perl libtimedate-perl wget && \
    rm -rf /var/lib/apt/lists/*

# lcov is just perl scripts.
RUN wget https://github.com/linux-test-project/lcov/releases/download/v2.4/lcov-2.4.tar.gz && \
    tar xf lcov-2.4.tar.gz && \
    make -C lcov-2.4 install && \
    rm -rf lcov-2.4 lcov-2.4.tar.gz

# not cached
FROM sync-ubuntu-base AS ubuntu-x64-build

WORKDIR /sync
COPY . .

RUN cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DSYNC_LIB_WITH_TESTS=ON -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_SKIP_TSAN_TESTS=ON -DSYNC_LIB_ASAN=ON -DSYNC_LIB_TSAN=OFF -DSYNC_LIB_CODE_COVERAGE=ON
RUN cmake --build build_asan -j4

RUN cmake -B build_rel -DCMAKE_BUILD_TYPE=Release -DSYNC_LIB_WITH_TESTS=ON -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_SKIP_TSAN_TESTS=ON -DSYNC_LIB_ASAN=OFF -DSYNC_LIB_TSAN=OFF -DSYNC_LIB_CODE_COVERAGE=ON
RUN cmake --build build_rel -j4

# LSan doesn't work in Docker/qemu
ENV ASAN_OPTIONS="detect_leaks=0:abort_on_error=1"
RUN --network=none ctest --test-dir build_asan --output-on-failure
RUN --network=none ctest --test-dir build_rel --output-on-failure

RUN mkdir -p reports/ubuntu_docker
RUN lcov --capture --directory build_asan --output-file reports/ubuntu_docker/coverage_asan.info \
        --branch-coverage \
        --ignore-errors negative,negative,inconsistent,inconsistent,mismatch,mismatch,gcov,gcov \
        --exclude '*/doctest.h'
RUN lcov --capture --directory build_rel --output-file reports/ubuntu_docker/coverage_rel.info \
        --branch-coverage \
        --ignore-errors negative,negative,inconsistent,inconsistent,mismatch,mismatch,gcov,gcov \
        --exclude '*/doctest.h'
RUN lcov -a reports/ubuntu_docker/coverage_asan.info \
         -a reports/ubuntu_docker/coverage_rel.info \
         --output-file reports/ubuntu_docker/coverage.info \
         --branch-coverage \
         --ignore-errors inconsistent,inconsistent,corrupt,corrupt,mismatch,mismatch
RUN lcov --summary reports/ubuntu_docker/coverage.info --branch-coverage \
        --ignore-errors inconsistent,inconsistent,corrupt,corrupt
RUN genhtml reports/ubuntu_docker/coverage.info --branch-coverage --output-directory reports/ubuntu_docker/html \
        --ignore-errors inconsistent,inconsistent,corrupt,corrupt
