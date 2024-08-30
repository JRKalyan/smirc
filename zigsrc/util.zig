const std = @import("std");

pub fn print(thing: anytype) void {
    const ThingType = @TypeOf(thing);
    switch (@typeInfo(ThingType)) {
        .Int => {
            std.io.getStdOut().writer().print("{d}", .{thing}) catch {};
        },
        .ComptimeInt => {
            std.io.getStdOut().writer().print("{d}", .{thing}) catch {};
        },
        .Float => {
            std.io.getStdOut().writer().print("{d:.3}", .{thing}) catch {};
        },
        else => {
            std.io.getStdOut().writer().print("{s}", .{thing}) catch {};
        },
    }
}

pub fn printn(thing: anytype) void {
    print(thing);
    print("\n");
}
