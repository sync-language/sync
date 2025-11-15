const std = @import("std");

extern fn test_backtrace_stuff() void;

test {
    try std.testing.expect(1 == 1);
    //std.debug.print("hmmm\n", .{});
    // test_backtrace_stuff();
}
