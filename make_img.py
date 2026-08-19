#!/usr/bin/env python3

import struct
from pathlib import Path


# ============================================================
# Configuration
# ============================================================

INPUT_FILE  = "BitStream.bin"
OUTPUT_FILE = "image.jpg"

WIDTH     = 60
HEIGHT    = 40
BIT_COUNT = 2271


# ============================================================
# Zigzag order
# ============================================================

ZIGZAG = [
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
]


# ============================================================
# Standard JPEG quantization tables
#
# IMPORTANT:
# These must be EXACTLY the same tables used by your PL core.
# ============================================================

LUMA_QTABLE = [
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68,109,103, 77,
    24, 35, 55, 64, 81,104,113, 92,
    49, 64, 78, 87,103,121,120,101,
    72, 92, 95, 98,112,100,103, 99
]

CHROMA_QTABLE = [
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
]


# ============================================================
# Standard JPEG Huffman tables
# ============================================================

DC_LUMA_BITS = [
    0x00, 0x01, 0x05, 0x01,
    0x01, 0x01, 0x01, 0x01,
    0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
]

DC_LUMA_VALUES = bytes([
    0, 1, 2, 3, 4, 5,
    6, 7, 8, 9, 10, 11
])


DC_CHROMA_BITS = [
    0x00, 0x03, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00
]

DC_CHROMA_VALUES = bytes([
    0, 1, 2, 3, 4, 5,
    6, 7, 8, 9, 10, 11
])


AC_LUMA_BITS = [
    0x00, 0x02, 0x01, 0x03,
    0x03, 0x02, 0x04, 0x03,
    0x05, 0x05, 0x04, 0x04,
    0x00, 0x00, 0x01, 0x7D
]

AC_LUMA_VALUES = bytes.fromhex("""
01 02 03 00 04 11 05 12
21 31 41 06 13 51 61 07
22 71 14 32 81 91 A1 08
23 42 B1 C1 15 52 D1 F0
24 33 62 72 82 09 0A 16
17 18 19 1A 25 26 27 28
29 2A 34 35 36 37 38 39
3A 43 44 45 46 47 48 49
4A 53 54 55 56 57 58 59
5A 63 64 65 66 67 68 69
6A 73 74 75 76 77 78 79
7A 83 84 85 86 87 88 89
8A 92 93 94 95 96 97 98
99 9A A2 A3 A4 A5 A6 A7
A8 A9 AA B2 B3 B4 B5 B6
B7 B8 B9 BA C2 C3 C4 C5
C6 C7 C8 C9 CA D2 D3 D4
D5 D6 D7 D8 D9 DA E1 E2
E3 E4 E5 E6 E7 E8 E9 EA
F1 F2 F3 F4 F5 F6 F7 F8
F9 FA
""")


AC_CHROMA_BITS = [
    0x00, 0x02, 0x01, 0x02,
    0x04, 0x04, 0x03, 0x04,
    0x07, 0x05, 0x04, 0x04,
    0x00, 0x01, 0x02, 0x77
]

AC_CHROMA_VALUES = bytes.fromhex("""
00 01 02 03 11 04 05 21
31 06 12 41 51 07 61 71
13 22 32 81 08 14 42 91
A1 B1 C1 09 23 33 52 F0
15 62 72 D1 0A 16 24 34
E1 25 F1 17 18 19 1A 26
27 28 29 2A 35 36 37 38
39 3A 43 44 45 46 47 48
49 4A 53 54 55 56 57 58
59 5A 63 64 65 66 67 68
69 6A 73 74 75 76 77 78
79 7A 82 83 84 85 86 87
88 89 8A 92 93 94 95 96
97 98 99 9A A2 A3 A4 A5
A6 A7 A8 A9 AA B2 B3 B4
B5 B6 B7 B8 B9 BA C2 C3
C4 C5 C6 C7 C8 C9 CA D2
D3 D4 D5 D6 D7 D8 D9 DA
E2 E3 E4 E5 E6 E7 E8 E9
EA F2 F3 F4 F5 F6 F7 F8
F9 FA
""")


# ============================================================
# JPEG helpers
# ============================================================

def marker_segment(marker, payload):
    length = len(payload) + 2

    return (
        bytes([0xFF, marker])
        + struct.pack(">H", length)
        + payload
    )


def create_app0():
    payload = (
        b"JFIF\x00"
        + bytes([1, 1])       # version 1.01
        + bytes([0])          # density units
        + struct.pack(">H", 1)
        + struct.pack(">H", 1)
        + bytes([0, 0])       # thumbnail
    )

    return marker_segment(0xE0, payload)


def create_dqt(table_id, table):
    # JPEG stores DQT values in zigzag order
    zigzag_table = bytes(
        table[ZIGZAG[i]]
        for i in range(64)
    )

    # precision = 0 (8 bit)
    # table ID = table_id
    payload = bytes([table_id]) + zigzag_table

    return marker_segment(0xDB, payload)


def create_sof0(width, height):
    payload = bytearray()

    # 8-bit sample precision
    payload.append(8)

    # JPEG uses big endian
    payload += struct.pack(">H", height)
    payload += struct.pack(">H", width)

    # 3 components
    payload.append(3)

    # Y
    payload += bytes([
        1,      # component ID
        0x22,   # H=2, V=2 -> 4 Y blocks / MCU
        0       # quantization table 0
    ])

    # Cb
    payload += bytes([
        2,
        0x11,
        1
    ])

    # Cr
    payload += bytes([
        3,
        0x11,
        1
    ])

    return marker_segment(0xC0, payload)


def create_dht(table_class, table_id, bits, values):

    if len(bits) != 16:
        raise ValueError("Huffman table must contain 16 length counts")

    if sum(bits) != len(values):
        raise ValueError(
            f"Huffman table mismatch: "
            f"counts={sum(bits)}, symbols={len(values)}"
        )

    table_info = (table_class << 4) | table_id

    payload = (
        bytes([table_info])
        + bytes(bits)
        + values
    )

    return marker_segment(0xC4, payload)


def create_sos():
    payload = bytearray()

    payload.append(3)

    # Y -> DC table 0, AC table 0
    payload += bytes([1, 0x00])

    # Cb -> DC table 1, AC table 1
    payload += bytes([2, 0x11])

    # Cr -> DC table 1, AC table 1
    payload += bytes([3, 0x11])

    # Baseline sequential JPEG
    payload += bytes([
        0x00,   # Ss
        0x3F,   # Se
        0x00    # Ah / Al
    ])

    return marker_segment(0xDA, payload)


# ============================================================
# Process raw PS entropy stream
# ============================================================

def prepare_entropy(data, bit_count):

    expected_bytes = (bit_count + 7) // 8

    if len(data) < expected_bytes:
        raise ValueError(
            f"Bitstream too short: "
            f"need {expected_bytes} bytes, "
            f"got {len(data)}"
        )

    # Ignore anything after the valid entropy bytes
    data = bytearray(data[:expected_bytes])

    remaining_bits = bit_count % 8

    # JPEG requires unused bits of the final byte
    # to be padded with 1s.
    #
    # Your HuffmanEncoding() writes MSB first.
    if remaining_bits != 0:

        unused_bits = 8 - remaining_bits

        keep_mask = (
            0xFF << unused_bits
        ) & 0xFF

        padding_mask = (
            1 << unused_bits
        ) - 1

        data[-1] = (
            data[-1] & keep_mask
        ) | padding_mask

    # --------------------------------------------------------
    # JPEG byte stuffing:
    #
    # entropy byte FF
    #
    # becomes
    #
    # FF 00
    # --------------------------------------------------------

    stuffed = bytearray()

    for value in data:

        stuffed.append(value)

        if value == 0xFF:
            stuffed.append(0x00)

    return bytes(stuffed)


# ============================================================
# Main
# ============================================================

def main():

    input_path = Path(INPUT_FILE)

    if not input_path.exists():
        raise SystemExit(
            f"ERROR: {INPUT_FILE} not found"
        )

    raw = input_path.read_bytes()

    expected_bytes = (BIT_COUNT + 7) // 8

    print(f"Input file     : {INPUT_FILE}")
    print(f"Input bytes    : {len(raw)}")
    print(f"BitCount       : {BIT_COUNT}")
    print(f"Expected bytes : {expected_bytes}")
    print(f"Image          : {WIDTH} x {HEIGHT}")

    if len(raw) != expected_bytes:
        print(
            "WARNING: input file size does not exactly "
            "match BitCount"
        )

    entropy = prepare_entropy(
        raw,
        BIT_COUNT
    )

    jpeg = bytearray()

    # --------------------------------------------------------
    # SOI
    # --------------------------------------------------------

    jpeg += bytes([
        0xFF, 0xD8
    ])

    # --------------------------------------------------------
    # APP0 / JFIF
    # --------------------------------------------------------

    jpeg += create_app0()

    # --------------------------------------------------------
    # DQT
    # --------------------------------------------------------

    jpeg += create_dqt(
        0,
        LUMA_QTABLE
    )

    jpeg += create_dqt(
        1,
        CHROMA_QTABLE
    )

    # --------------------------------------------------------
    # SOF0
    #
    # Real image dimensions:
    # 60 x 40
    #
    # NOT padded 64 x 48
    # --------------------------------------------------------

    jpeg += create_sof0(
        WIDTH,
        HEIGHT
    )

    # --------------------------------------------------------
    # DHT
    # --------------------------------------------------------

    # DC Luminance
    jpeg += create_dht(
        0,
        0,
        DC_LUMA_BITS,
        DC_LUMA_VALUES
    )

    # AC Luminance
    jpeg += create_dht(
        1,
        0,
        AC_LUMA_BITS,
        AC_LUMA_VALUES
    )

    # DC Chrominance
    jpeg += create_dht(
        0,
        1,
        DC_CHROMA_BITS,
        DC_CHROMA_VALUES
    )

    # AC Chrominance
    jpeg += create_dht(
        1,
        1,
        AC_CHROMA_BITS,
        AC_CHROMA_VALUES
    )

    # --------------------------------------------------------
    # SOS
    # --------------------------------------------------------

    jpeg += create_sos()

    # --------------------------------------------------------
    # Entropy generated by your Zynq
    # --------------------------------------------------------

    jpeg += entropy

    # --------------------------------------------------------
    # EOI
    # --------------------------------------------------------

    jpeg += bytes([
        0xFF, 0xD9
    ])

    Path(OUTPUT_FILE).write_bytes(jpeg)

    print()
    print(f"Raw entropy    : {expected_bytes} bytes")
    print(f"Stuffed entropy: {len(entropy)} bytes")
    print(f"JPEG size      : {len(jpeg)} bytes")
    print(f"Created        : {OUTPUT_FILE}")


if __name__ == "__main__":
    main()
