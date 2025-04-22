const std = @import("std");

extern fn test_add(a: c_int, b: c_int) callconv(.C) c_int;

test {
    try std.testing.expect(test_add(1, 2) == 3);
}
