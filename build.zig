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
        if (@import("builtin").os.tag == .windows) {
            lib.linkSystemLibrary("dbghelp");
        }

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
        if (@import("builtin").os.tag == .windows) {
            libUnitTests.linkSystemLibrary("dbghelp");
        }

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
    "lib/src/core/core.c",
    "lib/src/core/builtin_functions/sort.cpp",
    "lib/src/core/builtin_traits/iterator.cpp",
    "lib/src/util/panic.cpp",
    "lib/src/util/os_callstack.cpp",
    "lib/src/util/simd.cpp",
    "lib/src/mem/os_mem.cpp",
    "lib/src/mem/allocator.cpp",
    "lib/src/mem/protected_allocator.cpp",
    "lib/src/threading/sync_queue.cpp",
    "lib/src/threading/sync_obj_val.cpp",
    "lib/src/types/type_info.cpp",
    "lib/src/types/function/function.cpp",
    "lib/src/types/string/string_slice.cpp",
    "lib/src/types/string/string.cpp",
    "lib/src/types/array/dynamic_array.cpp",
    "lib/src/types/array/slice.cpp",
    "lib/src/types/sync_obj/sync_obj.cpp",
    "lib/src/types/hash/groups.cpp",
    "lib/src/types/hash/map.cpp",
    "lib/src/types/option/option.cpp",
    "lib/src/types/result/result.cpp",
    "lib/src/types/task/task.cpp",
    "lib/src/types/box/box.cpp",
    "lib/src/interpreter/stack/frame.cpp",
    "lib/src/interpreter/stack/node.cpp",
    "lib/src/interpreter/stack/stack.cpp",
    "lib/src/interpreter/bytecode.cpp",
    "lib/src/interpreter/interpreter.cpp",
    "lib/src/interpreter/function_builder.cpp",
    "lib/src/compiler/compiler.cpp",
    "lib/src/compiler/tokenizer/token.cpp",
    "lib/src/compiler/tokenizer/tokenizer.cpp",
    "lib/src/compiler/graph/scope.cpp",
    "lib/src/compiler/graph/module_dependency_graph.cpp",
    "lib/src/compiler/tokenizer/file_literals.cpp",
    "lib/src/compiler/source_tree/source_tree.cpp",
    "lib/src/compiler/parser/base_nodes.cpp",
    "lib/src/compiler/parser/parser.cpp",
    "lib/src/compiler/parser/stack_variables.cpp",
    "lib/src/compiler/parser/type_resolution.cpp",
    "lib/src/compiler/parser/expression.cpp",
    "lib/src/compiler/parser/ast/function_definition.cpp",
    "lib/src/compiler/parser/ast/return.cpp",
    "lib/src/compiler/parser/ast.cpp",
    "lib/src/program/program.cpp",
    "lib/src/program/program_error.cpp",
    "lib/src/testing/child_process.cpp",
    "lib/src/testing/assert_handler.cpp",

    "lib/test/test_runner.cpp",
};
