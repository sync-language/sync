const std = @import("std");
const Build = std.Build;

const moduleName = "sync_lang";

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    { // Module
        const syncModule = b.addModule(
            moduleName,
            .{ .root_source_file = .{ .cwd_relative = "lib/src/root.zig" } },
        );
        syncModule.link_libc = true;
        syncModule.link_libcpp = true;

        const c_flags = [_][]const u8{};
        syncModule.addCSourceFiles(.{
            .files = &sync_lang_c_sources,
            .flags = &c_flags,
        });
    }
    {
        const libUnitTests = b.addTest(.{
            .root_source_file = .{ .cwd_relative = "lib/src/tests.zig" },
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
    "lib/src/test.cpp",
};
