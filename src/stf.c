#include "stf.h"

char* stfa_to_ascii[256] = {
    // 0
    "0",
    // 1
    "1",
    // 2
    "2",
    // 3
    "3",
    // 4
    "4",
    // 5
    "5",
    // 6
    "6",
    // 7
    "7",
    // 8
    "8",
    // 9
    "9",
    // 10
    "(10)",
    // 11
    "(11)",
    // 12
    "(12)",
    // 13
    "(13)",
    // 14
    "(14)",
    // 15
    "(15)",
    // 16
    "(16)",
    // 17
    "(17)",
    // 18
    "(18)",
    // 19
    "(19)",
    // 20
    "(20)",
    // 21
    "(21)",
    // 22
    "(22)",
    // 23
    "(23)",
    // 24
    "(24)",
    // 25
    "(25)",
    // 26
    "(26)",
    // 27
    "(27)",
    // 28
    "(28)",
    // 29
    "(29)",
    // 30
    "(30)",
    // 31
    "(31)",
    // 32
    "a",
    // 33
    "b",
    // 34
    "c",
    // 35
    "d",
    // 36
    "e",
    // 37
    "f",
    // 38
    "g",
    // 39
    "h",
    // 40
    "i",
    // 41
    "j",
    // 42
    "k",
    // 43
    "l",
    // 44
    "m",
    // 45
    "n",
    // 46
    "o",
    // 47
    "p",
    // 48
    "q",
    // 49
    "r",
    // 50
    "s",
    // 51
    "t",
    // 52
    "u",
    // 53
    "v",
    // 54
    "w",
    // 55
    "x",
    // 56
    "y",
    // 57
    "z",
    // 58
    "A",
    // 59
    "B",
    // 60
    "C",
    // 61
    "D",
    // 62
    "E",
    // 63
    "F",
    // 64
    "G",
    // 65
    "H",
    // 66
    "I",
    // 67
    "J",
    // 68
    "K",
    // 69
    "L",
    // 70
    "M",
    // 71
    "N",
    // 72
    "O",
    // 73
    "P",
    // 74
    "Q",
    // 75
    "R",
    // 76
    "S",
    // 77
    "T",
    // 78
    "U",
    // 79
    "V",
    // 80
    "W",
    // 81
    "X",
    // 82
    "Y",
    // 83
    "Z",
    // 84
    "!",
    // 85
    "@",
    // 86
    "#",
    // 87
    "$",
    // 88
    "%",
    // 89
    "^",
    // 90
    "&",
    // 91
    "+",
    // 92
    "-",
    // 93
    "*",
    // 94
    "÷",
    // 95
    "=",
    // 96
    "_",
    // 97
    ".",
    // 98
    ",",
    // 99
    " ",
    // 100
    "|",
    // 101
    "?",
    // 102
    "/",
    // 103
    "\\",
    // 104
    "~",
    // 105
    "`",
    // 106
    ":",
    // 107
    ";",
    // 108
    "\"",
    // 109
    "'",
    // 110
    "(",
    // 111
    ")",
    // 112
    "[",
    // 113
    "]",
    // 114
    "{",
    // 115
    "}",
    // 116
    "<",
    // 117
    ">",
    // 118
    "<OUDENTHORPE>",
    // 119
    "<MONOTHORPE>",
    // 120
    "<DUOTHORPE>",
    // 121
    "<TRITHORPE>",
    // 122
    "<TETRATHORPE>",
    // 123
    "<PENTATHORPE>",
    // 124
    "<HEXATHORPE>",
    // 125
    "<HEPTATHORPE>",
    // 126
    "<O0>",
    "<O1>",
    "<O2>",
    "<O3>",
    "<O4>",
    "<O5>",
    "<O6>",
    "<O7>",
    "<O8>",
    "<O9>",
    "<O10>",
    "<O11>",
    "<O12>",
    "<O13>",
    "<O14>",
    "<O15>",
    "<O16>",
    "<O17>",
    "<O18>",
    "<O19>",
    "<O20>",
    "<O21>",
    "<O22>",
    "<O23>",
    "<O24>",
    "<O25>",
    "<O26>",
    "<O27>",
    "<O28>",
    "<O29>",
    "<O30>",
    "<O31>",
    "<O32>",
    "<O33>",
    "<O34>",
    "<O35>",
    "<O36>",
    "<O37>",
    "<O38>",
    "<O39>",
    "<O40>",
    "<O41>",
    "<O42>",
    "<O43>",
    "<O44>",
    "<O45>",
    "<O46>",
    "<O47>",
    "<O48>",
    "<O49>",
    "<O50>",
    "<O51>",
    "<O52>",
    "<O53>",
    "<O54>",
    "<O55>",
    "<O56>",
    "<O57>",
    "<O58>",
    "<O59>",
    "<O60>",
    "<O61>",
    "<O62>",
    "<O63>",
    "<O64>",
    "<O65>",
    "<O66>",
    "<O67>",
    "<O68>",
    "<O69>",
    "<O70>",
    "<O71>",
    "<O72>",
    "<O73>",
    "<O74>",
    "<O75>",
    "<O76>",
    "<O77>",
    "<O78>",
    "<O79>",
    "<O80>",
    "<O81>",
    "<O82>",
    "<O83>",
    "<O84>",
    "<O85>",
    "<O86>",
    "<O87>",
    "<O88>",
    "<O89>",
    "<O90>",
    "<O91>",
    "<O92>",
    "<O93>",
    "<O94>",
    "<O95>",
    "<O96>",
    "<O97>",
    "<O98>",
    "<O99>",
    "<O100>",
    "<O101>",
    "<O102>",
    "<O103>",
    "<O104>",
    "<O105>",
    "<O106>",
    "<O107>",
    "<O108>",
    "<O109>",
    "<O110>",
    "<O111>",
    "<O112>",
    "<O113>",
    "<O114>",
    "<O115>",
    "<O116>",
    "<O117>",
    "<O118>",
    "<O119>",
    "<O120>",
    "<O121>",
    "<O122>",
    "<O123>",
    "<O124>",
    "<O125>",
    "<O126>",
    "<O127>",
    "<O128>",
    "<C>"
};

/*



// NOTE: ideally we don't use this so it's implemented in a silly way. This
// could be sped up by unrolling every possible byte into a comptime memoized
// array for lookup (inverse of stfa_to_ascii)
pub fn ascii_to_stfa(ascii_char: u8) u8 {
    for (stfa_to_ascii) |sequence, i| {
        if (sequence.len == 1 and sequence[0] == ascii_char) {
            return @intCast(u8, i);
        }
    }

    return 118; // Return Oudenthorpe for characters with no STFA equivalent
}

// STF SPEC
// ========
// STF = Simple Tree Format
// A valid STF sequence of STFA characters abides by the following grammar:
// TODO make this grammar nice even though I won't be using it for parsing.
// 1) root -> contents
// 2) contents -> EMPTY | LEAF contents | node contents
// 3) node -> OPEN contents CLOSE

// - LEAF is any leaf character (below 126)
// - EMPTY is empty string
// - OPEN is any open character and CLOSE is the unique close character

// Adjacent LEAF characters coalesce into the contents of a  single node when an STF tree is parsed.

// Naive tree design
// (optimized would be re-use the input buffer for lexemes)

pub const TreeNode = struct {
    children: std.ArrayList(u32),
    brace_type: u8,
};

pub const LeafNode = struct {
    lexeme: std.ArrayList(u8),
};

pub const Tree = union(enum) {
    leaf: LeafNode,
    tree: TreeNode,
};

// TODO memory leaking since deinit not being called for Tree types
// TODO customizable parser to implement layers on top of STF for non context sensitive
//      requirements: e.g. handle braces differently: encounter one language's "if" OPEN char,
//      then custom code can run on open and on close to validate a node and stop early. (e.g. verify
//      that every encountered leaf character is numeric for number literals, or below 16 for hex literals etc.
//      or for the 'if' example we can verify that we only encounter subnodes of specific types and of the right count.
pub fn parse(characters: []u8) std.ArrayList(Tree) {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    var allocator = gpa.allocator();
    var node_pool = std.ArrayList(Tree).init(allocator);
    var lexeme = std.ArrayList(u8).init(allocator);
    var tree_nodes = std.ArrayList(u32).init(allocator);
    // Always add the meta node:
    node_pool.append(Tree{ .tree = TreeNode{ .children = std.ArrayList(u32).init(allocator), .brace_type = OPEN_ZERO } }) catch @panic("OOM");
    tree_nodes.append(0) catch @panic("OOM");
    var next_node_index: u32 = 1;
    for (characters) |character| {
        print("PROCESSING: ");
        printn(stfa_to_ascii[character]);
        if (character < OPEN_ZERO) {
            // Leaf
            lexeme.append(character) catch @panic("OOM");
            printn("LEXEME CHUG");
        } else {
            if (lexeme.items.len > 0) {
                printn("ENDING LEXEME");
                // We had a sequence of leaf characters, make it a node
                node_pool.items[tree_nodes.items[tree_nodes.items.len - 1]].tree.children.append(next_node_index) catch @panic("OOM");
                node_pool.append(Tree{ .leaf = LeafNode{ .lexeme = lexeme } }) catch @panic("OOM");
                next_node_index += 1;
                lexeme = std.ArrayList(u8).init(allocator);
            }
            if (character < 255) {
                printn("OPENING");
                // Open
                node_pool.items[tree_nodes.items[tree_nodes.items.len - 1]].tree.children.append(next_node_index) catch @panic("OOM");
                node_pool.append(Tree{ .tree = TreeNode{ .children = std.ArrayList(u32).init(allocator), .brace_type = character } }) catch @panic("OOM");
                tree_nodes.append(next_node_index) catch @panic("OOM");
                next_node_index += 1;
            } else {
                printn("CLOSING");
                // Close
                if (tree_nodes.items.len == 1) {
                    @panic("EXTRA CLOSE CHARACTER");
                } else {
                    _ = tree_nodes.pop();
                }
            }
        }
    }

    if (tree_nodes.items.len != 1) {
        @panic("UNENDED OPENING BRACE");
    }

    return node_pool;
}

// TODO change these if the encoding changes. Could also think about generating these using comptime code that
// parses the stfa array
pub const OPEN_ZERO: comptime_int = 126;
pub const CLOSE: comptime_int = 255;
pub const OPEN_COUNT: comptime_int = CLOSE - OPEN_ZERO;
pub const OPEN_MAX: comptime_int = OPEN_COUNT - 1;
*/
