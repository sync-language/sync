# Contributing to Sync

Thank you for wanting to contribute!

Continue reading if you want to contribute directly to the core Sync implementation within this repo. For anything outside of this repo, please contact the team directly for now.

## Environment Setup

Use whichever code editor you want. So long as you have a terminal, you're probably good to go. Ideally, use one with good CMake integration.

### Hard Requirements

These are all mandatory to develop for Sync, as all APIs must be kept up to date with any changes.

- [git](https://git-scm.com/)
- [CMake](https://cmake.org/)
- [Rust with Cargo](https://rust-lang.org/learn/get-started/)
- [Zig](https://ziglang.org/learn/getting-started/)

### Optional

- [Python](https://www.python.org/) to execute cross-platform tests
- [Docker](https://docs.docker.com/desktop/) to run the Linux test suite
- [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) to run the WASM test suite along with either [Node.js](https://nodejs.org/en/download) or [Bun](https://bun.com/)
- [QEMU](https://www.qemu.org/) basis for VMs
- [quickemu](https://github.com/quickemu-project/quickemu) for Mac/Windows/Linux VMs
- [UTM](https://mac.getutm.app/) to run Windows test suite from a MacOS device
- [bwrap](https://github.com/containers/bubblewrap) for sandboxing Linux

### MacOS Setup

#### 1. Get XCode Tools

```sh
# git, clang, make, etc
xcode-select --install
```

#### 2. Install CMake

```sh
# latest is fine
brew install cmake
```

#### 3. Install Rust and Cargo

```sh
# see https://rust-lang.org/learn/get-started/ to get the up to date command for installing rust and cargo
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

#### 4. Install Zig

```sh
# as zig is still in active development with breaking changes, it should be updated on every release.
brew install zig
brew upgrade zig
```

#### 5. Install Python

```sh
# get Python. Latest is fine.
brew install python
```

#### 6. Install Docker

```sh
# see https://docs.docker.com/desktop/setup/install/mac-install/ to get Docker Desktop
# alternatively run the following
brew install --cask docker
```

#### 7. Install Emscripten

```sh
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
# PATH and other environment variables for this terminal only
source ./emsdk_env.sh
```

#### 8. Install Node.js or Bun

```sh
# see https://nodejs.org/en/download to get Node.js
# alternatively get bun
# alternatively run the following
brew install node
```

#### 9. Install VM Runners

```sh
# get QEMU, UTM, and SSH auto insert password
brew install qemu
brew install --cask utm
brew install sshpass
```

#### 10. Setup Windows VM with UTM

##### 10.1. Get a Windows ISO

**NOTE:** Running unlicensed windows is not recommended. We recommend getting a trial version of Windows Enterprise from the [Microsoft Evaluation Center](https://www.microsoft.com/en-us/evalcenter/evaluate-windows-11-iot-enterprise-ltsc).

For unlicensed Windows (not recommended), get a normal ISO.

- For Apple Silicon: [Windows 11 ARM64 ISO](https://www.microsoft.com/en-us/software-download/windows11arm64)
- For Intel Mac: A Windows 10/11 x64 ISO should be fine

##### 10.2. Create the Virtual Machine

1. Open UTM
2. Click "+"
3. Select "Virtualize" or "Emulate"
    - "Virtualize" if the CPU architectures match (apple silicon + win11 arm OR intel mac + win10/11 x64)
    - "Emulate" otherwise
4. Select "Windows"
5. Set the required specs
    - 8192MB recommended memory
    - 4 cores recommend (maybe 2 is fine?)
6. Select `Install Windows 10 or higher`
7. Select your downloaded ISO file
8. Set disk size (64+GB)
    - 128GB recommended, as Sync does not require too many resources to run the test suite, but VS tools are quite big
9. No shared directories are required
10. Name the VM. This name will be used later. Prefer the format `Windows <10 or 11> <arch>` for instance `Windows 11 arm64`

##### 10.3. Setup Windows

I encountered some difficulty with this part. At first I only see `SHELL>`. Run `map` to see some FS devices.

The trick is to press a key when it says "Press any key to boot from CD or DVD" (duh). If you're in the UEFI shell, enter `exit` and then select `Reset`, or just restart the VM.

Once you're actually in the Windows setup, follow it as normally.

> I don't have a product key
>
> Windows 10/11 Home
>
> Let it Restart

After it restarts, to not press a key to boot into CD / DVD again. It will keep restarting, which is fine. It is making progress as can be seen by the visible percentage.

##### 10.4. Setup UTM guest tools

Setup UTM guest tools with the `UTM Guest Tools Installer`. This should be loaded on first setup already.

`https://docs.getutm.app/guest-support/windows/`.

##### 10.5. Configure SSH

Configure SSH

Open Adminstrator Powershell

```sh
# This first one may take a while
Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0
Start-Service sshd
Set-Service -Name sshd -StartupType Automatic
New-NetFirewallRule -Name sshd -DisplayName 'OpenSSH Server' -Enabled True -Direction Inbound -Protocol TCP -Action Allow -LocalPort 22
# Maybe also disable the firewall
Set-NetFirewallProfile -Profile Domain,Public,Private -Enabled False
```

##### 10.6. Perform Windows Environment Setup as Normal

Install Windows Environment Tools by performing the [Windows Environment Setup](#windows-setup).

You do not need to setup Emscripten or Docker for the Virtual Machine.

##### 10.7. Useful VM Commands

```sh
# ssh into the VM
utmctl ip-address "<yourvmname>"
sshpass -p 'PASSWORD' ssh -o StrictHostKeyChecking=no USERNAME@IP
```

### Windows Setup

#### 1. Visual Studio Build Tools

Download [Visual Studio](https://visualstudio.microsoft.com/).

Run the Visual Studio Installer Installer (Microsoft what the HECK).

Select the C++ Desktop build tools, then select Install. This may take a while.

#### 2. Download CMake

Install [CMake](https://cmake.org/download/) for your architecture.

#### 3. Download Rust and Cargo

Download the [rustup-init.exe](https://rust-lang.org/learn/get-started/) for your architecture.

Go through the setup.

#### 4. Download Zig

```sh
winget install -e --id zig.zig
```

### Linux Setup

TODO

## Running Tests

The python script [test/run_tests.py](test/run_tests.py) will run a configured set of the test suite.

### Arguments

| Argument | Options | Description |
|----------|---------|-------------|
| `--asan` | N/A | Enable Address Sanitizing on main tests |
| `--tsan` | N/A | Enable Thread Sanitizer tests |
| `--sandbox` | N/A | Run host system tests in sandboxed environment |
| `--no-docker` | N/A | Disables execution of docker tests |
| `--no-vm` | N/A | Disables execution of Virtual Machine tests<br>- UTM on MacOS |
| `--utm` | "VM_NAME" USER PASS | Specifies a virtual machine for UTM to run. Multiple `--utm` options are allowed |
