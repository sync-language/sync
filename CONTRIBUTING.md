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

- [Python](https://www.python.org/) for cross platform development scripting
- [Docker](https://docs.docker.com/desktop/) to run the Linux test suite
- [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) to run the WASM test suite along with either [Node.js](https://nodejs.org/en/download) or [Bun](https://bun.com/)
- [UTM](https://mac.getutm.app/) to run Windows test suite from a MacOS device

### MacOS Setup

```sh
# download XCode tools (git, clang, make, etc)
xcode-select --install

# get CMake. Latest is fine
brew install cmake

# see https://rust-lang.org/learn/get-started/ to get the up to date command for installing rust and cargo
# curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# get Zig. As zig is still in active development with breaking changes, it should be updated on every release.
brew install zig
brew upgrade zig

# get Python. Latest is fine.
brew install python

# see https://docs.docker.com/desktop/setup/install/mac-install/ to get Docker Desktop
# alternatively run the following
# brew install --cask docker

# setup emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
# PATH and other environment variables for this terminal only
source ./emsdk_env.sh

# see https://nodejs.org/en/download to get Node.js
# alternatively get bun
# alternatively run the following
# brew install node

# get UTM
brew install --cask utm
```

#### Setup Windows VM with UTM

**1.**

Get a Windows ISO.

- For Apple Silicon: [Windows 11 ARM64 ISO](https://www.microsoft.com/en-us/software-download/windows11arm64)
- For Intel Mac: A Windows 10/11 x64 ISO should be fine

**2.**

Create the Virtual Machine.

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

**3.**

Setup Windows.

I encountered some difficulty with this part. At first I only see `SHELL>`. Run `map` to see some FS devices.

The trick is to press a key when it says "Press any key to boot from CD or DVD" (duh). If you're in the UEFI shell, enter `exit` and then select `Reset`, or just restart the VM.

Once you're actually in the Windows setup, follow it as normally.

> I don't have a product key
>
> Windows 10/11 Home
>
> Let it Restart

After it restarts, to not press a key to boot into CD / DVD again. It will keep restarting, which is fine. It is making progress as can be seen by the visible percentage.

**4.**

Setup UTM guest tools with the `UTM Guest Tools Installer`. This should be loaded on first setup already.

`https://docs.getutm.app/guest-support/windows/`.
