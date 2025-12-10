import os
from pathlib import Path
import subprocess
from typing import List
import platform
import time

filePath = Path(__file__)
os.chdir(filePath.parent.parent) # project root, no matter where this was called from

# init docker
try:
    INSTALL_CROSS_PLATFORM_EMULATOR = "docker run --privileged --rm tonistiigi/binfmt --install all"
    CHECK_MULTIARCH_EXISTS = "docker buildx inspect multiarch"
    CREATE_MULTIARCH_BUILDX = "docker buildx create --name multiarch --use"
    USE_MULTIARCH_BUILDX = "docker buildx use multiarch"
    BOOTSTRAP = "docker buildx inspect --bootstrap"

    subprocess.run(INSTALL_CROSS_PLATFORM_EMULATOR, shell=True, check=True)

    result = subprocess.run(CHECK_MULTIARCH_EXISTS, shell=True, capture_output=True)
    if result.returncode != 0:
        subprocess.run(CREATE_MULTIARCH_BUILDX, shell=True, check=True)
    else:
        subprocess.run(USE_MULTIARCH_BUILDX, shell=True, check=True)

    subprocess.run(BOOTSTRAP, shell=True, check=True)
except subprocess.CalledProcessError as e:
    print(f"Command failed with exit code {e.returncode}")
    print(f"Error output: {e.stderr}")
    exit(-1)

ARCHITECTURE = platform.machine()
IS_X64 = ARCHITECTURE == 'AMD64' or ARCHITECTURE == 'x86_64'
IS_ARM64 = ARCHITECTURE == 'arm64' or ARCHITECTURE == 'aarch64'
OPERATING_SYSTEM = platform.system()

HOST_CMAKE_CONFIGURE = "cmake -B build-host -DCMAKE_BUILD_TYPE=Debug -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_LIB_ASAN=OFF"

class NamedProcess:
    def __init__(self, name: str, process):
        self.name = name
        self.process = process
        self.start_time = time.perf_counter()

processes = []

if OPERATING_SYSTEM == "Windows" and IS_X64:
    print("Windows x64 detected. Running the following test suite:")
    print(" - Windows x64 without ASan") # stupid asan dll
    #print(" - Windows arm64 with emulation without ASan")
    print(" - Ubuntu x64 with docker")
    print(" - Ubuntu arm64 with docker")
    print(" - Ubuntu riscv64 with docker")
    print(" - WASM with Ubuntu docker + emscripten")

    try:
        host_process = subprocess.Popen('cmake -B build-x64 -DCMAKE_BUILD_TYPE=Debug -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_LIB_ASAN=OFF && cmake --build build-x64 --config Debug --parallel && ctest --test-dir build-x64 -C Debug --output-on-failure', shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        processes.append(NamedProcess("Windows x64", host_process))

        # arm64_process = subprocess.Popen('cmake -G \"Visual Studio 17 2022\" -B build-arm64 -A ARM64 -DCMAKE_BUILD_TYPE=Debug -DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF -DSYNC_LIB_ASAN=OFF && cmake --build build-arm64 --config Debug --parallel && ctest --test-dir build-arm64 -C Debug --output-on-failure', shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        # processes.append(NamedProcess("Windows arm64", arm64_process))
    except:
        pass

docker_ubuntu_x64_process = subprocess.Popen('docker buildx build --platform linux/amd64 --load --no-cache -t sync:ubuntu-x64 -f test/docker/ubuntu-x64.Dockerfile .', shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
processes.append(NamedProcess("Docker Ubuntu x64", docker_ubuntu_x64_process))

# docker_ubuntu_arm64_process = subprocess.Popen('docker buildx build --platform linux/arm64 --load --no-cache -t sync:ubuntu-arm64 -f test/docker/ubuntu-arm64.Dockerfile .', shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
# processes.append(NamedProcess("Docker Ubuntu arm64", docker_ubuntu_arm64_process))

# docker_ubuntu_riscv64_process = subprocess.Popen('docker buildx build --platform linux/riscv64 --load --no-cache -t sync:ubuntu-riscv64 -f test/docker/ubuntu-riscv64.Dockerfile .', shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
# processes.append(NamedProcess("Docker Ubuntu riscv64", docker_ubuntu_riscv64_process))

docker_ubuntu_wasm_process = subprocess.Popen('docker build --no-cache -t sync:wasm -f test/docker/wasm.Dockerfile .', shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
processes.append(NamedProcess("Docker Ubuntu WASM", docker_ubuntu_wasm_process))

for process in processes:
    stdout, stderr = process.process.communicate()
    returncode = process.process.returncode
    elapsed_time = time.perf_counter() - process.start_time # this doesn't work good cause communicate() waits

    if returncode == 0:
        print(f"OK: {process.name}")
        continue

    if stdout:
        print("STDOUT:")
        print(stdout)
    if stderr:
        print("STDERR:")
        print(stderr)

    print(f"Tests for {process.name} exited with return code: {returncode}")
