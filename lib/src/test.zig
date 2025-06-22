const std = @import("std");

extern fn test_backtrace_stuff() void;

test {
    try std.testing.expect(1 == 1);
    test_backtrace_stuff();
}
