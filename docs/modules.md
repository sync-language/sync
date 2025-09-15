# Modules

All Sync code exists within a module. This module can be considered like a library or executable. It has types, functions, configuration, and dependencies.

Modules are constructed of a tree of source files/contents, functionally identical to a directory tree. Modules optionally may have a configuration file `sync.toml`.

TOML is a good configuration language, used in:

- Rust (Cargo.toml)
- Python (pyproject.toml)

## Build / Execution

Sync as a library (not cli) supports 2 distinct build types, along with manual entry. These are from the disk with a `sync.toml` file, and a serialized form for various uses such as over networks.

### From Disk

Within `sync.toml`, a root file is specified. From there, all non-dependency imports are added to that module through the root file recursively. `sync.toml` also describes any dependencies.

### Serialize

An entire Compiler object can be serialized and deserialized. This allows sending a collection modules and configurations to anywhere.
