#!/usr/bin/python3

# modified from https://github.com/zeux/meshoptimizer/blob/v0.18/tools/wasmpack.py

import sys

table = []

palette = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:;";

def encode(buffer):
    result = ''

    for ch in buffer.read():
        if ch in table:
            index = table.index(ch)
            result += palette[index]
        else:
            result += palette[60 + ch // 64]
            result += palette[ch % 64]

    return result

def stats(buffer):
    hist = [0] * 256
    for ch in buffer.read():
        hist[ch] += 1

    result = [i for i in range(256)]
    result.sort(key=lambda i: hist[i], reverse=True)

    return result

# if sys.argv[-2] == 'generate':
#     theStats = stats(sys.stdin.buffer)[:60]
#     print(theStats)
#     with open(sys.argv[-1], 'wb') as fd:
#         fd.write(bytearray(theStats))

# else:
#     with open(sys.argv[-1], 'rb') as fd:
#         table = fd.read()

#     print(encode(sys.stdin.buffer))

with open(sys.argv[-1], 'rb') as fd:
    table = stats(fd)[:60]

encoded = ""

with open(sys.argv[-1], 'rb') as fd:
    encoded = encode(fd)

export_prefix = "export const "

if sys.argv[-3] == 'cjs':
    export_prefix = "module.exports."

print("""const encoded = "%s";

const table = new Uint8Array(%s);

function unpack(data) {
    const result = new Uint8Array(data.length);
    let i = 0, ch = 0, write = 0;
    for (; i < data.length; ++i) {
        ch = data.charCodeAt(i);
        result[i] = ch > 96 ? ch - 97 : ch > 64 ? ch - 39 : ch + 4;
    }
    for (i = 0; i < data.length; ++i) {
        result[write++] = (result[i] < 60) ? table[result[i]] : (result[i] - 60) * 64 + result[++i];
    }
    return result.buffer.slice(0, write);
}

%s%s = unpack(encoded);""" % (encoded, table, export_prefix, sys.argv[-2]))
