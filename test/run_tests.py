import os
from pathlib import Path
import subprocess
from typing import List
import platform
import time

def run_test_sequentially(test_name: str, test_command: str):
    print(f"Running: {test_name}...")
    start_time = time.perf_counter()

    result = subprocess.run(
        test_command,
        shell=True,
        capture_output=True,
        text=True
    )

    elapsed_time = time.perf_counter() - start_time

    if result.returncode == 0:
        print(f"OK: {test_name} ({elapsed_time:.2f}s)")
    else:
        print(f"FAILED: {test_name} ({elapsed_time:.2f}s)")
        if result.stdout:
            print("STDOUT:")
            print(result.stdout)
        if result.stderr:
            print("STDERR:")
            print(result.stderr)
        print(f"Return code: {result.returncode}")


filePath = Path(__file__)
os.chdir(filePath.parent.parent) # project root, no matter where this was called from

# init docker
try:
    INSTALL_CROSS_PLATFORM_EMULATOR = "docker run --privileged --rm tonistiigi/binfmt --install all"
    CHECK_MULTIARCH_EXISTS = "docker buildx inspect multiarch"
    CREATE_MULTIARCH_BUILDX = "docker buildx create --name multiarch --use"
    USE_MULTIARCH_BUILDX = "docker buildx use multiarch"
    BOOTSTRAP = "docker buildx inspect --bootstrap"

    install_result = subprocess.run(INSTALL_CROSS_PLATFORM_EMULATOR, shell=True, check=True, capture_output=True)
    if(install_result.returncode != 0):
        if install_result.stdout:
            print("STDOUT:")
            print(install_result.stdout)
        if install_result.stderr:
            print("STDERR:")
            print(install_result.stderr)

    result = subprocess.run(CHECK_MULTIARCH_EXISTS, shell=True, capture_output=True)
    multiarch_result = None
    if result.returncode != 0:
        multiarch_result = subprocess.run(CREATE_MULTIARCH_BUILDX, shell=True, check=True, capture_output=True)
    else:
        multiarch_result = subprocess.run(USE_MULTIARCH_BUILDX, shell=True, check=True, capture_output=True)

    if(multiarch_result.returncode != 0):
        if multiarch_result.stdout:
            print("STDOUT:")
            print(multiarch_result.stdout)
        if multiarch_result.stderr:
            print("STDERR:")
            print(multiarch_result.stderr)

    bootstrap_result = subprocess.run(BOOTSTRAP, shell=True, check=True)
    if(bootstrap_result.returncode != 0):
        if bootstrap_result.stdout:
            print("STDOUT:")
            print(bootstrap_result.stdout)
        if bootstrap_result.stderr:
            print("STDERR:")
            print(bootstrap_result.stderr)
except subprocess.CalledProcessError as e:
    print(f"Command failed with exit code {e.returncode}")
    print(f"Error output: {e.stderr}")
    exit(-1)

ARCHITECTURE = platform.machine()
IS_X64 = ARCHITECTURE == 'AMD64' or ARCHITECTURE == 'x86_64'
IS_ARM64 = ARCHITECTURE == 'arm64' or ARCHITECTURE == 'aarch64'
OPERATING_SYSTEM = platform.system()

if OPERATING_SYSTEM == "Windows" and IS_X64:
    print("Windows x64 detected. Running the following test suite:")
    print(" - Windows x64 without ASan")
    print(" - Ubuntu x64 with docker")
    print(" - Ubuntu arm64 with docker")
    print(" - Ubuntu riscv64 with docker")
    print(" - WASM with Ubuntu docker + emscripten")
    print()

    run_test_sequentially(
        "Windows x64",
        'cmake -B build-x64 -DCMAKE_BUILD_TYPE=Debug -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_LIB_ASAN=OFF && cmake --build build-x64 --config Debug --parallel && ctest --test-dir build-x64 -C Debug --output-on-failure'
    )

    # run_test_sequentially(
    #     "Windows arm64",
    #     'cmake -G "Visual Studio 17 2022" -B build-arm64 -A ARM64 -DCMAKE_BUILD_TYPE=Debug -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_LIB_ASAN=OFF && cmake --build build-arm64 --config Debug --parallel && ctest --test-dir build-arm64 -C Debug --output-on-failure'
    # )

if OPERATING_SYSTEM == "Darwin" and IS_ARM64:
    print("MacOS arm64 detected. Running the following test suite:")
    print(" - MacOS arm64")
    print(" - MacOS x64 with Rosetta")
    print(" - Ubuntu x64 with docker")
    print(" - Ubuntu arm64 with docker")
    print(" - Ubuntu riscv64 with docker")
    print(" - WASM with Ubuntu docker + emscripten")

    run_test_sequentially(
        "MacOS arm64",
        'cmake -B build-x64 -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_BUILD_TYPE=Debug -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_LIB_ASAN=OFF && cmake --build build-x64 --config Debug --parallel && ctest --test-dir build-x64 -C Debug --output-on-failure'
    )

    run_test_sequentially(
        "MacOS x64",
        'cmake -B build-x64 -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_BUILD_TYPE=Debug -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_LIB_ASAN=OFF && cmake --build build-x64 --config Debug --parallel && ctest --test-dir build-x64 -C Debug --output-on-failure'
    )


run_test_sequentially(
    "Docker Ubuntu x64",
    'docker buildx build --platform linux/amd64 --load -t sync:ubuntu-x64 -f test/docker/ubuntu-x64.Dockerfile .'
)

run_test_sequentially(
    "Docker Ubuntu arm64",
    'docker buildx build --platform linux/arm64 --load -t sync:ubuntu-arm64 -f test/docker/ubuntu-arm64.Dockerfile .'
)

run_test_sequentially(
    "Docker Ubuntu riscv64",
    'docker buildx build --platform linux/riscv64 --load -t sync:ubuntu-riscv64 -f test/docker/ubuntu-riscv64.Dockerfile .'
)

run_test_sequentially(
    "Docker Ubuntu WASM",
    'docker build -t sync:wasm -f test/docker/wasm.Dockerfile .'
)
