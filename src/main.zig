const std = @import("std");

pub fn main() anyerror!void {
    try std.io.getStdOut().writer().print("😏\n", .{});
}
