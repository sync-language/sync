extern crate cc;

fn main()
{
    cc::Build::new()
        .cpp(true)
        .std("c++17")
        .file("src/util/panic.cpp")
        .file("src/util/os_callstack.cpp")
        .file("src/mem/allocator.cpp")
        .file("src/mem/os_mem.cpp")
        .file("src/threading/sync_queue.cpp")
        .file("src/threading/sync_obj_val.cpp")
        .file("src/types/type_info.cpp")
        .file("src/types/function/function.cpp")
        .file("src/types/string/string_slice.cpp")
        .file("src/types/string/string.cpp")
        .file("src/types/sync_obj/sync_obj.cpp")
        .file("src/interpreter/stack.cpp")
        .file("src/interpreter/bytecode.cpp")
        .file("src/interpreter/interpreter.cpp")
        .file("src/program/program.cpp")
        .compile("SyncLib");
}
