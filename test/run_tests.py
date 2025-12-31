import os
from pathlib import Path
import subprocess
from typing import List
import platform
import time
import datetime
import argparse

ARCHITECTURE = platform.machine()
IS_X64 = ARCHITECTURE == 'AMD64' or ARCHITECTURE == 'x86_64'
IS_ARM64 = ARCHITECTURE == 'arm64' or ARCHITECTURE == 'aarch64'
OPERATING_SYSTEM = platform.system()

def _os_pretty_name() -> str:
    if OPERATING_SYSTEM == "Darwin":
        return "MacOS"
    return OPERATING_SYSTEM

PRETTY_OS_NAME = _os_pretty_name()

filePath = Path(__file__)
os.chdir(filePath.parent.parent) # project root, no matter where this was called from

def current_time_str() -> str:
    return datetime.datetime.now().strftime("%H:%M:%S")


def parse_args():
    parser = argparse.ArgumentParser(description="Run Sync test suite")

    parser.add_argument("--asan", action="store_true", help="Enable AddressSanitizer on tests")
    parser.add_argument("--tsan", action="store_true", help="Enable ThreadSanitizer tests")
    parser.add_argument("--sandbox", action="store_true", help="Run host tests in sandbox environment")
    parser.add_argument("--no-docker", action="store_true", help="Skip Docker tests")
    parser.add_argument("--no-vm", action="store_true", help="Skip Virtual Machine tests (UTM on Mac)")

    parser.add_argument(
        "--utm",
        action="append",
        nargs=3,
        metavar=("\"VM_NAME\"", "USER", "PASS"),
        help="Run UTM VM tests. Specify VM name, username, and password. MAC ONLY."
    )

    return parser.parse_args()

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

def run_docker_tests():
    # init docker
    try:
        INSTALL_CROSS_PLATFORM_EMULATOR = "docker run --privileged --rm tonistiigi/binfmt --install all"
        CHECK_MULTIARCH_EXISTS = "docker buildx inspect multiarch"
        CREATE_MULTIARCH_BUILDX = "docker buildx create --name multiarch --use"
        USE_MULTIARCH_BUILDX = "docker buildx use multiarch"
        BOOTSTRAP = "docker buildx inspect --bootstrap"

        install_result = subprocess.run(INSTALL_CROSS_PLATFORM_EMULATOR, shell=True, check=True, capture_output=True, text=True)
        if(install_result.returncode != 0):
            if install_result.stdout:
                print("STDOUT:")
                print(install_result.stdout)
            if install_result.stderr:
                print("STDERR:")
                print(install_result.stderr)

        result = subprocess.run(CHECK_MULTIARCH_EXISTS, shell=True, capture_output=True, text=True)
        multiarch_result = None
        if result.returncode != 0:
            multiarch_result = subprocess.run(CREATE_MULTIARCH_BUILDX, shell=True, check=True, capture_output=True, text=True)
        else:
            multiarch_result = subprocess.run(USE_MULTIARCH_BUILDX, shell=True, check=True, capture_output=True, text=True)

        if(multiarch_result.returncode != 0):
            if multiarch_result.stdout:
                print("STDOUT:")
                print(multiarch_result.stdout)
            if multiarch_result.stderr:
                print("STDERR:")
                print(multiarch_result.stderr)

        bootstrap_result = subprocess.run(BOOTSTRAP, shell=True, check=True, capture_output=True, text=True)
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

def cmake_build_command(build_dir: str, extra_args: str, asan: bool, tsan: bool) -> str:
    asan_flag = "ON" if asan else "OFF"
    skip_tsan_tests_flag = "OFF" if tsan else "ON"
    # TSan would be a separate flag if you have it
    return (
        f"cmake -B {build_dir} {extra_args} "
        f"-DCMAKE_BUILD_TYPE=Debug "
        f"-DSYNC_LIB_DOCTEST_ADD_CTESTS=OFF "
        f"-DSYNC_LIB_ASAN={asan_flag} -DSYNC_SKIP_TSAN_TESTS={skip_tsan_tests_flag} && "
        f"cmake --build {build_dir} --config Debug --parallel"
    )

def ctest_run_command(build_dir: str) -> str:
    return f"ctest --test-dir {build_dir} -C Debug --output-on-failure"

def run_host_test(sandbox: bool):
    start_time = time.perf_counter()

    print(f"[Host Test {current_time_str()}]: Build CMake")
    cmake_result = subprocess.run(f"{cmake_build_command("build-host", f"{"-DSYNC_SANDBOX_TESTS=ON" if sandbox else ""}", True, True)}", shell=True, capture_output=True, text=True)
    if cmake_result.returncode != 0:
        if cmake_result.stdout:
            print("STDOUT:")
            print(cmake_result.stdout)
        if cmake_result.stderr:
            print("STDERR:")
            print(cmake_result.stderr)
        exit(-1)

    print(f"[Host Test {current_time_str()}]: Run CTest")
    ctest_result = subprocess.run(f"{ctest_run_command("build-host")}", shell=True, capture_output=True, text=True)
    if ctest_result.returncode != 0:
        if ctest_result.stdout:
            print("STDOUT:")
            print(ctest_result.stdout)
        if ctest_result.stderr:
            print("STDERR:")
            print(ctest_result.stderr)
        exit(-1)

    elapsed_time = time.perf_counter() - start_time
    print(f"[Host Test {current_time_str()}]: Took {elapsed_time:.2f}s")

    subprocess.run(f"rm -rf ./build-host", shell=True, check=True)

def run_utm_test(vm_name: str, username: str, password: str, args):
    subprocess.run("killall UTM", shell=True)
    time.sleep(1)
    subprocess.run("open -a UTM", shell=True)
    time.sleep(3)

    def cleanup_vm():
        print(f"[UTM Test \"{vm_name}\" {current_time_str()}]: Cleaning up VM \"{test_vm_name}\"")
        subprocess.run(f"utmctl stop \"{test_vm_name}\" --force", shell=True)
        subprocess.run(f"utmctl delete \"{test_vm_name}\"", shell=True)

    test_vm_name = f"TEST - {vm_name}"
    print(f"[UTM Test \"{vm_name}\" {current_time_str()}]: Stop VM \"{vm_name}\"")
    subprocess.run(f"utmctl stop \"{vm_name}\"", shell=True)
    print(f"[UTM Test \"{vm_name}\" {current_time_str()}]: Stop VM if exists \"{test_vm_name}\"")
    subprocess.run(f"utmctl stop \"{test_vm_name}\" --force", shell=True)

    print(f"[UTM Test \"{vm_name}\" {current_time_str()}]: Deleting old test VM if exists \"{test_vm_name}\"")
    subprocess.run(f"utmctl delete \"{test_vm_name}\"", shell=True)

    try:
        print(f"[UTM Test \"{vm_name}\" {current_time_str()}]: Cloning VM \"{vm_name}\" -> \"{test_vm_name}\"")
        subprocess.run(f"utmctl clone \"{vm_name}\" --name \"{test_vm_name}\"", shell=True, check=True)
        time.sleep(3)

        print(f"[UTM Test \"{vm_name}\" {current_time_str()}]: Disabling VM networking \"{test_vm_name}\"")
        vm_path = f"{os.environ['HOME']}/Library/Containers/com.utmapp.UTM/Data/Documents/{test_vm_name}.utm/config.plist"
        subprocess.run([
            "/usr/libexec/PlistBuddy",
            "-c", "Set :Network:0:Mode Host",
            vm_path
        ], check=True)

        subprocess.run("killall UTM", shell=True)
        time.sleep(1)
        subprocess.run("open -a UTM", shell=True)
        time.sleep(3)

        print(f"[UTM Test \"{vm_name}\" {current_time_str()}]: Starting VM \"{test_vm_name}\"")
        subprocess.run(f"utmctl start \"{test_vm_name}\"", shell=True, check=True)

        # some time for network
        print(f"[UTM Test \"{vm_name}\" {current_time_str()}]: Waiting for VM ip address")
        time.sleep(30)
        print(f"[UTM Test \"{vm_name}\" {current_time_str()}]: Getting ip address \"{test_vm_name}\"")
        ip_addr_result = subprocess.run(f"utmctl ip-address \"{test_vm_name}\"", shell=True, capture_output=True, text=True, check=True)
        ip_addr_result_split = ip_addr_result.stdout.split("\n")
        ip_addr = ip_addr_result_split[0]
        time.sleep(5)

        sshpass_login = f"sshpass -p '{password}' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null {username}@{ip_addr}"

        print(f"[UTM Test \"{vm_name}\" {current_time_str()}]: Making test directory 'sync_test' \"{test_vm_name}\"")
        subprocess.run(f"{sshpass_login} \"mkdir C:\\Users\\{username}\\sync_test 2>nul\"", shell=True, check=True)

        print(f"[UTM Test \"{vm_name}\" {current_time_str()}]: Copying source to test directory 'sync_test' \"{test_vm_name}\"")
        subprocess.run(f"tar czf - --exclude='.git' --exclude='build*' --exclude='._.' --exclude='.DS_Store' . | {sshpass_login} \"cd C:\\Users\\{username}\\sync_test && tar xzf -\"", shell=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Command failed with exit code {e.returncode}")
        print(f"Error output: {e.stderr}")
        cleanup_vm()
        exit(-1)
    
    print(f"[UTM Test \"{vm_name}\" {current_time_str()}]: Build CMake \"{test_vm_name}\"")
    cmake_result = subprocess.run(f"{sshpass_login} \"cd C:\\Users\\{username}\\sync_test && {cmake_build_command("build", "", False, False)}\"", shell=True, capture_output=True, text=True)
    if cmake_result.returncode != 0:
        if cmake_result.stdout:
            print("STDOUT:")
            print(cmake_result.stdout)
        if cmake_result.stderr:
            print("STDERR:")
            print(cmake_result.stderr)
        cleanup_vm()
        exit(-1)

    print(f"[UTM Test \"{vm_name}\" {current_time_str()}]: Run CTest \"{test_vm_name}\"")
    ctest_result = subprocess.run(f"{sshpass_login} \"cd C:\\Users\\{username}\\sync_test && {ctest_run_command("build")}\"", shell=True, capture_output=True, text=True)
    if ctest_result.returncode != 0:
        if ctest_result.stdout:
            print("STDOUT:")
            print(ctest_result.stdout)
        if ctest_result.stderr:
            print("STDERR:")
            print(ctest_result.stderr)
        cleanup_vm()
        exit(-1)

    cleanup_vm()


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

if __name__ == "__main__":
    args = parse_args()

    print("Configured Test Suite:")
    print(f" - Host {PRETTY_OS_NAME} {ARCHITECTURE}")

    if not args.no_docker:
        print(" - Ubuntu x64 with docker")
        print(" - Ubuntu arm64 with docker")
        print(" - Ubuntu riscv64 with docker")
        print(" - WASM with Ubuntu docker + emscripten")

    if not args.no_vm:
        if OPERATING_SYSTEM == "Darwin":
            for vm in args.utm:
                print(f" - {vm[0]} with UTM Virtual Machine (WILL RESTART UTM IF OPEN)")

    run_host_test(args.sandbox)

    if not args.no_docker:
        run_docker_tests()

    if not args.no_vm:
        if OPERATING_SYSTEM == "Darwin":
            for vm in args.utm:
                run_utm_test(vm[0], vm[1], vm[2], args)

# utmctl stop "Windows 11 arm64"
# utmctl delete "test - Windows 11 arm64"
# utmctl clone "Windows 11 arm64" --name "test - Windows 11 arm64"
# utmctl start "test - Windows 11 arm64"
# utmctl ip-address "test - Windows 11 arm64"
# sshpass -p 'dev' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null dev@192.168.128.2 "mkdir C:\Users\dev\sync_test 2>nul"
# sshpass -p 'dev' ssh dev@192.168.64.2 "mkdir C:\Users\dev\sync_test 2>nul"
# tar czf - --exclude='.git' --exclude='build*' --exclude='._.' --exclude='.DS_Store' . | sshpass -p 'dev' ssh dev@192.168.64.2 "cd C:\Users\dev\sync_test && tar xzf -"
