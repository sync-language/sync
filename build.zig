const std = @import("std");
const Build = std.Build;

const moduleName = "sync_lang";
const SY_CUSTOM_DEFAULT_ALLOCATOR = "SY_CUSTOM_DEFAULT_ALLOCATOR";

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const syncModule = b.addModule(
        moduleName,
        .{ .root_source_file = .{ .cwd_relative = "lib/src/root.zig" } },
    );
    { // Module

        syncModule.link_libc = true;
        syncModule.link_libcpp = true;

        syncModule.addCMacro(SY_CUSTOM_DEFAULT_ALLOCATOR, "1");

        const c_flags = [_][]const u8{};
        syncModule.addCSourceFiles(.{
            .files = &sync_lang_c_sources,
            .flags = &c_flags,
        });
    }
    { // Static Library
        const lib = b.addStaticLibrary(.{
            .name = "SyncLib",
            .root_source_file = .{ .cwd_relative = "lib/src/lib.zig" },
            .target = target,
            .optimize = optimize,
        });
        lib.root_module.addImport(moduleName, syncModule);
        b.installArtifact(lib);
    }
    { // Tests
        const libUnitTests = b.addTest(.{
            .root_source_file = .{ .cwd_relative = "lib/src/test.zig" },
            .target = target,
            .optimize = optimize,
        });
        libUnitTests.addIncludePath(b.path("lib/src"));
        libUnitTests.linkLibC();
        libUnitTests.linkLibCpp();

        const c_flags = [_][]const u8{};
        libUnitTests.addCSourceFiles(.{
            .files = &sync_lang_c_sources,
            .flags = &c_flags,
        });

        // Always create test artifacts
        {
            const options = Build.Step.InstallArtifact.Options{ .dest_dir = .{
                .override = .{ .custom = "test" },
            } };

            b.getInstallStep().dependOn(&b.addInstallArtifact(libUnitTests, options).step);
        }
        // On running "zig build test" on the command line, it will build and run the tests
        const run_lib_unit_tests = b.addRunArtifact(libUnitTests);
        const test_step = b.step("test", "Run unit tests");
        test_step.dependOn(&run_lib_unit_tests.step);
    }
}

const sync_lang_c_sources = [_][]const u8{
    "lib/src/util/panic.cpp",
    "lib/src/mem/os_mem.cpp",
    "lib/src/mem/allocator.cpp",
    "lib/src/types/type_info.cpp",
    "lib/src/types/function/function.cpp",
    "lib/src/interpreter/stack.cpp",
    "lib/src/interpreter/bytecode.cpp",
    "lib/src/interpreter/interpreter.cpp",
    "lib/src/interpreter/primitive_tag.cpp",
    "lib/src/program/program.cpp",
};
