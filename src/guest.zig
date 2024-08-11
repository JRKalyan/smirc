const std = @import("std");
const stf = @import("stf.zig");

const Callable = struct {
    code: std.ArrayList(u8),
};

/// Deserialize and validate an STF syntax tree
// TODO
pub fn parse(sequence: []const u8) void {
    _ = stf.parse(sequence);
}

// bind statement
// function body expression
// structure body expression
// match expression
// call expression (built in functions include addition, multiplication, ...)
// accessor expressions into a struct
// integer literal (binary, dec, hex)
// float literal (binary, dec, hex)

// MAP FROM OPEN BRACES TO LANGUAGE NODES:
const stfa_open_to_guest_node = [stf.OPEN_COUNT] {

}


// TODO return meaningful errors. Doesn't need to use zig's error system.
pub fn compile(ast: stf.Tree) ?Callable {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    var allocator = gpa.allocator();

    var callable = Callable{
        .code = std.ArrayList(u8).init(allocator),
    };
    
    // ...

    return callable;
    // TODO save re-usable compile information and return it.
    // this can be used to speed up subsequent compilation if it's passed as an arg.
    // Will need to diff the input and previous version to intelligently recompile what's needed only.
}

// TODO could ensure type safety in the return type if Callable of X is the actual type
// (using zig generics)
pub fn call() void {
    // TODO inline assembly
    return;
}
