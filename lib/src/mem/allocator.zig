const std = @import("std");

pub const Allocator = extern struct {
    const Self = @This();
    const Error = std.mem.Allocator.Error;

    ptr: ?*anyopaque,
    vtable: *const VTable,

    pub const VTable = extern struct {
        allocFn: *const fn (self: *anyopaque, len: usize, alignment: usize) ?*anyopaque,
        freeFn: *const fn (self: *anyopaque, buf: *anyopaque, len: usize, alignment: usize) void,

        pub fn comptimeZigAllocatorToVTable(comptime vtable: std.mem.Allocator.VTable) *const VTable {
            const Generated = struct {
                fn alloc(ctx: *anyopaque, len: usize, alignment: usize) callconv(.C) ?*anyopaque {
                    const logAlign: u6 = @intCast(std.math.log2(alignment));
                    const mem = vtable.alloc(ctx, len, logAlign, @returnAddress());
                    return @ptrCast(mem);
                }

                fn free(ctx: *anyopaque, buf: *anyopaque, len: usize, alignment: usize) void {
                    const logAlign: u6 = @intCast(std.math.log2(alignment));
                    const mem: [*]u8 = @ptrCast(buf);
                    vtable.free(ctx, mem[0..len], logAlign, @returnAddress());
                }

                const syVTable: *const VTable = &.{ .allocFn = &@This().alloc, .freeFn = &@This().free };
            };

            return Generated.syVTable;
        }
    };

    pub fn comptimeInit(comptime zigAllocator: std.mem.Allocator) Self {
        return Self{ ._allocator = .{
            .ptr = zigAllocator.ptr,
            .vtable = VTable.comptimeZigAllocatorToVTable(zigAllocator.vtable),
        } };
    }

    pub fn init(zigAllocator: std.mem.Allocator) Error!Self {
        const allocatorPtr = try zigAllocator.create(std.mem.Allocator);

        return Self{ ._allocator = .{
            .ptr = @ptrCast(allocatorPtr),
            .vtable = DynamicVTable.vtable,
        } };
    }

    // TODO revise all these allocation functions.
    // Should support sentinels?

    pub fn create(self: Allocator, comptime T: type) Error!*T {
        return self.createAligned(T, @alignOf(T));
    }

    pub fn destroy(self: Allocator, ptr: anytype) void {
        // Used from Zig Allocator.zig
        const info = @typeInfo(@TypeOf(ptr)).Pointer;
        if (info.size != .One) @compileError("ptr must be a single item pointer");
        const T = info.child;
        if (@sizeOf(T) == 0 or @alignOf(T) == 0) return;
        const non_const_ptr = @as([*]u8, @ptrCast(@constCast(ptr)));
        var s = self;
        sy_allocator_free(&s._allocator, non_const_ptr, @sizeOf(T), @intCast(info.alignment));
    }

    pub fn alloc(self: Allocator, comptime T: type, n: usize) Error![]T {
        return self.allocAligned(T, n, @alignOf(T));
    }

    pub fn free(self: Allocator, memory: anytype) void {
        // Used from Zig Allocator.zig
        const Slice = @typeInfo(@TypeOf(memory)).Pointer;
        const bytes = std.mem.sliceAsBytes(memory);
        const bytes_len = bytes.len + if (Slice.sentinel != null) @sizeOf(Slice.child) else 0;
        if (bytes_len == 0) return;
        const non_const_ptr = @constCast(bytes.ptr);
        var s = self;
        sy_allocator_free(&s._allocator, non_const_ptr, bytes_len, @intCast(Slice.alignment));
    }

    pub fn createAligned(self: Allocator, comptime T: type, comptime alignment: ?usize) Error!*align(alignment orelse @alignOf(T)) T {
        var s = self;
        // Used from Zig Allocator.zig
        if (@sizeOf(T) == 0 or @alignOf(T) == 0) return @as(*T, @ptrFromInt(std.math.maxInt(usize)));
        const actualAlign = if (alignment) alignment.? else @alignOf(T);
        const mem = sy_allocator_alloc(&s._allocator, @sizeOf(T), actualAlign);
        if (mem) |m| {
            return @ptrCast(@alignCast(m));
        }
        return Error.OutOfMemory;
    }

    pub fn allocAligned(self: Allocator, comptime T: type, n: usize, comptime alignment: ?usize) Error![]align(alignment orelse @alignOf(T)) T {
        var s = self;
        // Used from Zig Allocator.zig
        if (@sizeOf(T) == 0 or @alignOf(T) == 0) return @as(*T, @ptrFromInt(std.math.maxInt(usize)));
        const actualAlign = if (alignment) alignment.? else @alignOf(T);
        const mem = sy_allocator_alloc(&s._allocator, @sizeOf(T) * n, actualAlign);
        if (mem) |m| {
            const slice: *align(alignment orelse @alignOf(T)) T = @ptrCast(@alignCast(m));
            return slice[0..n];
        }
        return Error.OutOfMemory;
    }

    const DynamicVTable = struct {
        fn alloc(a: *std.mem.Allocator, len: usize, alignment: usize) callconv(.C) ?*anyopaque {
            const logAlign: u6 = @intCast(std.math.log2(alignment));
            const mem = a.rawAlloc(len, logAlign, @returnAddress());
            return @ptrCast(mem);
        }

        fn free(a: *std.mem.Allocator, buf: *anyopaque, len: usize, alignment: usize) void {
            const logAlign: u6 = @intCast(std.math.log2(alignment));
            const mem: [*]u8 = @ptrCast(buf);
            a.rawFree(mem[0..len], logAlign, @returnAddress());
        }

        fn deinit(a: *std.mem.Allocator) void {
            const allocator: std.mem.Allocator = a.*;
            allocator.destroy(a);
        }

        const vtable: *const Allocator.VTable = &.{
            .allocFn = @ptrCast(&DynamicVTable.alloc),
            .freeFn = @ptrCast(&DynamicVTable.free),
            .destructorFn = @ptrCast(&DynamicVTable.deinit),
        };
    };
};

var gpa = std.heap.GeneralPurposeAllocator(.{}){};
const globalAllocator: std.mem.Allocator = blk: {
    if (@import("builtin").is_test)
        break :blk std.testing.allocator
    else {
        if (std.debug.runtime_safety)
            break :blk gpa.allocator()
        else
            break :blk std.heap.c_allocator;
    }
};

var defaultAllocator: Allocator = .{
    .ptr = null,
    .vtable = Allocator.VTable.comptimeZigAllocatorToVTable(globalAllocator),
};
pub export var sy_defaultAllocator: *Allocator = &defaultAllocator;

pub extern fn sy_allocator_alloc(self: *Allocator, len: usize, alignment: usize) callconv(.C) ?*anyopaque;
pub extern fn sy_allocator_free(self: *Allocator, buf: *anyopaque, len: usize, alignment: usize) callconv(.C) void;
