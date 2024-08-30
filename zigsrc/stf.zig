const std = @import("std");

const util = @import("util.zig");
const print = util.print;
const printn = util.printn;

// STFA SPEC:
// ==========
// - STF Alphabet has 125 'leaf' characters from byte layouts [0-125]
// - The first 32 byte layouts are leaf characters representing the numbers [0-31]
// - Following that are leaf characters for some printable ASCII characters:
//   beginning with a-z, followed by A-Z, followed by !@#$%^&+-*÷=_., |?/\~`:;\'()[]{}<>
// - Byte layouts [118-125] are for 8 SFTA unique characters:
//   the Oudenthorpe, Monothorpe, Duothorpe, Trithorpe, Tetrathorpe, Pentathorpe, Hexathorpe and the Heptathorpe
// - The remaining byte layouts [126-255] are 129 unique OPEN characters and the final byte is the CLOSE character

pub const stfa_to_ascii: [256][]const u8 = [_][]const u8{
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
    "<C>",
};

// TODO fix the count above.

// OUT OF DATE
pub const deprecated_ascii_to_stfa: [256]u8 = [_]u8{
    // 0
    118,
    // 1
    118,
    // 2
    118,
    // 3
    118,
    // 4
    118,
    // 5
    118,
    // 6
    118,
    // 7
    118,
    // 8
    118,
    // 9
    118,
    // 10
    118,
    // 11
    118,
    // 12
    118,
    // 13
    118,
    // 14
    118,
    // 15
    118,
    // 16
    118,
    // 17
    118,
    // 18
    118,
    // 19
    118,
    // 20
    118,
    // 21
    118,
    // 22
    118,
    // 23
    118,
    // 24
    118,
    // 25
    118,
    // 26
    118,
    // 27
    118,
    // 28
    118,
    // 29
    118,
    // 30
    118,
    // 31
    118,
    // 32
    97,
    // 33
    84,
    // 34
    108,
    // 35
    87,
    // 36
    88,
    // 37
    89,
    // 38
    91,
    // 39
    109,
    // 40
    110,
    // 41
    111,
    // 42
    94,
    // 43
    92,
    // 44
    99,
    // 45
    93,
    // 46
    98,
    // 47
    102,
    // 48
    0,
    // 49
    1,
    // 50
    2,
    // 51
    3,
    // 52
    4,
    // 53
    5,
    // 54
    6,
    // 55
    7,
    // 56
    8,
    // 57
    9,
    // 58
    106,
    // 59
    107,
    // 60
    116,
    // 61
    95,
    // 62
    117,
    // 63
    101,
    // 64
    86,
    // 65
    58,
    // 66
    59,
    // 67
    60,
    // 68
    61,
    // 69
    62,
    // 70
    63,
    // 71
    64,
    // 72
    65,
    // 73
    66,
    // 74
    67,
    // 75
    68,
    // 76
    69,
    // 77
    70,
    // 78
    71,
    // 79
    72,
    // 80
    73,
    // 81
    74,
    // 82
    75,
    // 83
    76,
    // 84
    77,
    // 85
    78,
    // 86
    79,
    // 87
    80,
    // 88
    81,
    // 89
    82,
    // 90
    83,
    // 91
    112,
    // 92
    103,
    // 93
    113,
    // 94
    90,
    // 95
    96,
    // 96
    105,
    // 97
    32,
    // 98
    33,
    // 99
    34,
    // 100
    35,
    // 101
    36,
    // 102
    37,
    // 103
    38,
    // 104
    39,
    // 105
    40,
    // 106
    41,
    // 107
    42,
    // 108
    43,
    // 109
    44,
    // 110
    45,
    // 111
    46,
    // 112
    47,
    // 113
    48,
    // 114
    49,
    // 115
    50,
    // 116
    51,
    // 117
    52,
    // 118
    53,
    // 119
    54,
    // 120
    55,
    // 121
    56,
    // 122
    57,
    // 123
    114,
    // 124
    100,
    // 125
    115,
    // 126
    104,
    // 127
    118,
    // 128
    118,
    // 129
    118,
    // 130
    118,
    // 131
    118,
    // 132
    118,
    // 133
    118,
    // 134
    118,
    // 135
    118,
    // 136
    118,
    // 137
    118,
    // 138
    118,
    // 139
    118,
    // 140
    118,
    // 141
    118,
    // 142
    118,
    // 143
    118,
    // 144
    118,
    // 145
    118,
    // 146
    118,
    // 147
    118,
    // 148
    118,
    // 149
    118,
    // 150
    118,
    // 151
    118,
    // 152
    118,
    // 153
    118,
    // 154
    118,
    // 155
    118,
    // 156
    118,
    // 157
    118,
    // 158
    118,
    // 159
    118,
    // 160
    118,
    // 161
    118,
    // 162
    118,
    // 163
    118,
    // 164
    118,
    // 165
    118,
    // 166
    118,
    // 167
    118,
    // 168
    118,
    // 169
    118,
    // 170
    118,
    // 171
    118,
    // 172
    118,
    // 173
    118,
    // 174
    118,
    // 175
    118,
    // 176
    118,
    // 177
    118,
    // 178
    118,
    // 179
    118,
    // 180
    118,
    // 181
    118,
    // 182
    118,
    // 183
    118,
    // 184
    118,
    // 185
    118,
    // 186
    118,
    // 187
    118,
    // 188
    118,
    // 189
    118,
    // 190
    118,
    // 191
    118,
    // 192
    118,
    // 193
    118,
    // 194
    118,
    // 195
    118,
    // 196
    118,
    // 197
    118,
    // 198
    118,
    // 199
    118,
    // 200
    118,
    // 201
    118,
    // 202
    118,
    // 203
    118,
    // 204
    118,
    // 205
    118,
    // 206
    118,
    // 207
    118,
    // 208
    118,
    // 209
    118,
    // 210
    118,
    // 211
    118,
    // 212
    118,
    // 213
    118,
    // 214
    118,
    // 215
    118,
    // 216
    118,
    // 217
    118,
    // 218
    118,
    // 219
    118,
    // 220
    118,
    // 221
    118,
    // 222
    118,
    // 223
    118,
    // 224
    118,
    // 225
    118,
    // 226
    118,
    // 227
    118,
    // 228
    118,
    // 229
    118,
    // 230
    118,
    // 231
    118,
    // 232
    118,
    // 233
    118,
    // 234
    118,
    // 235
    118,
    // 236
    118,
    // 237
    118,
    // 238
    118,
    // 239
    118,
    // 240
    118,
    // 241
    118,
    // 242
    118,
    // 243
    118,
    // 244
    118,
    // 245
    118,
    // 246
    118,
    // 247
    118,
    // 248
    118,
    // 249
    118,
    // 250
    118,
    // 251
    118,
    // 252
    118,
    // 253
    118,
    // 254
    118,
    // 255
    118,
};

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
