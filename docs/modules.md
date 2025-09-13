# Modules

All Sync code exists within a module. This module can be considered like a library or executable. It has types, functions, configuration, and dependencies.

Modules are constructed of a tree of source files/contents, functionally identical to a directory tree. Modules optionally may have a configuration file `sync.toml`. *TODO maybe `build.sy` would be better? Can write code to configure build stuff*

## Source Tree

A source tree starts from a root file. Any file that is imported by that one, and any file imported by those imports, and so on, is added into the tree structure. This resembles a directory tree. Any file in the directory structure that is not imported will not be considered part of the source tree.

## Configuration File
