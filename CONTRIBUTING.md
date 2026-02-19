# Contributing to Sync

Thank you for wanting to contribute!

Continue reading if you want to contribute directly to the core Sync implementation within this repo. For anything outside of this repo, please contact the team directly for now.

## Environment Setup

Use whichever code editor you want. So long as you have a terminal, you're probably good to go. Ideally, use one with good CMake integration.

### Hard Requirements

These are all mandatory to develop for Sync, as all APIs must be kept up to date with any changes.

- [git](https://git-scm.com/)
- [CMake](https://cmake.org/)
- [Rust and Cargo](https://rust-lang.org/learn/get-started/)
- [Zig](https://ziglang.org/learn/getting-started/)

### Optional

- [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) to run the WASM test suite along with either [Node.js](https://nodejs.org/en/download) or [Bun](https://bun.com/)
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

#### 7. Install and Build Emscripten

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

#### 1. Get Build Tools For Your Distro

Getting CMake, Rust, and Zig will vary depending on your Distro.

#### 2. Install and Build Emscripten

```sh
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
# PATH and other environment variables for this terminal only
source ./emsdk_env.sh
```

#### 3. Install Node.js or Bun

```sh
# see https://nodejs.org/en/download to get Node.js
# alternatively get bun
curl -fsSL https://bun.sh/install | bash
```
