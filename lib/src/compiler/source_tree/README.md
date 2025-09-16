# Source Tree

It is important that the source tree is able to change it's root directory. The sync CLI may need to run a file that imports a file in a directory above it. As such, dynamically changing the root directory (not root file) is an extremely important piece of functionality.

This also is convenient for testing modules as the programmer can separate a `src/` and `test/` directory if they so wish.

As such, we cannot know for certain that ONLY the files within the root directory will be part of the module. This also means that knowing which files to import is part of the parsing stage, as relative file imports are per source code file.

The solution to this is making the entire source tree, when reading from disk, be the entire path to the files. For instance, on windows this could look like a tree containing the following nodes in layer order:

- C:
- Users
- [AccountName]
- Documents
- [ProjectFolder]
- src
- subfolder
- example.sync

*Note: On Windows you cannot just `cd ..` to go from, say, C: to D:. On Linux/MacOS, the root directory of `/` will be used.*

This alleviates dynamic root issue that relative pathing may result in.
