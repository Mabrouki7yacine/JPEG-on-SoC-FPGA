#ifndef JPEG_CORE_H
#define JPEG_CORE_H

#include "xaxidma.h"
#include "xil_types.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xparameters.h"

#include <arm_neon.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

#define BLOCK_SIZE 384 // 4 * 8 * 8 + 2 * 8 * 8
#define TIMEOUT_LIMIT 10000000

#define Coeff_Y_R   0.299f
#define Coeff_Y_G   0.587f
#define Coeff_Y_B   0.114f

#define Coeff_Cb_R -0.168736f
#define Coeff_Cb_G -0.331264f
#define Coeff_Cb_B  0.500000f

#define Coeff_Cr_R  0.500000f
#define Coeff_Cr_G -0.418688f
#define Coeff_Cr_B -0.081312f

#define ZRL_VALUE 0xF0
#define EOB_VALUE 0x00

typedef struct __attribute__((packed)) {
    uint8_t R;
    uint8_t G;
    uint8_t B;
} RGB;

typedef struct __attribute__((packed)) {
    uint8_t Y;
    uint8_t Cb;
    uint8_t Cr;
} YCbCr;

typedef struct __attribute__((packed)) {
    uint8_t Y0[64];
    uint8_t Y1[64];
    uint8_t Y2[64];
    uint8_t Y3[64];
    uint8_t Cb[64];
    uint8_t Cr[64];
} uMCU_block_t;

typedef struct __attribute__((packed)) {
    int8_t Y0[64];
    int8_t Y1[64];
    int8_t Y2[64];
    int8_t Y3[64];
    int8_t Cb[64];
    int8_t Cr[64];
} sMCU_block_t;

typedef enum {
    PADDED_ALREADY,
    REQUIRE_H_PADDING,
    REQUIRE_W_PADDING,
    REQUIRE_F_PADDING,
} Padding_t;

typedef enum {
    Luminance = 16,
    Chroma = 16,
} ChType;

typedef struct {
    uint8_t symbol;   // (RUN << 4) | SIZE
    int8_t  value;    // actual AC coefficient
} RLE_Entry_t;

typedef struct {
    int16_t DC;
    RLE_Entry_t  RLE_Entry[64];
    uint8_t  RLE_Entry_Count;
} HuffmanBlock_t;

typedef struct {
    uint16_t code;
    uint8_t  length;
} HuffmanCode_t;

static const uint8_t JPEG_ZIGZAG[64] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

static const uint8_t CATEGORY_LUT[256] = {
    0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,

    /* 128 = -128 */
    8,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    5,4,4,4,4,4,4,4,4,3,3,3,3,2,2,1
};

static const uint8_t DC_CATEGORY_LUT[511] = {
    /* -255 .. -128 */
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,

    /* -127 .. -64 */
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,

    /* -63 .. -32 */
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,

    /* -31 .. -16 */
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,

    /* -15 .. -8 */
    4,4,4,4,4,4,4,4,

    /* -7 .. -4 */
    3,3,3,3,

    /* -3 .. -2 */
    2,2,

    /* -1 */
    1,

    /* 0 */
    0,

    /* 1 */
    1,

    /* 2 .. 3 */
    2,2,

    /* 4 .. 7 */
    3,3,3,3,

    /* 8 .. 15 */
    4,4,4,4,4,4,4,4,

    /* 16 .. 31 */
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,

    /* 32 .. 63 */
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,

    /* 64 .. 127 */
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,

    /* 128 .. 255 */
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
};

static const HuffmanCode_t DC_LUMA_HUFFMAN[12] = {
    {0b00,        2},   // category 0
    {0b010,       3},   // category 1
    {0b011,       3},   // category 2
    {0b100,       3},   // category 3
    {0b101,       3},   // category 4
    {0b110,       3},   // category 5
    {0b1110,      4},   // category 6
    {0b11110,     5},   // category 7
    {0b111110,    6},   // category 8
    {0b1111110,   7},   // category 9
    {0b11111110,  8},   // category 10
    {0b111111110, 9}    // category 11
};

static const HuffmanCode_t DC_CHROMA_HUFFMAN[12] = {
    {0b00,           2},   // category 0
    {0b01,           2},   // category 1
    {0b10,           2},   // category 2
    {0b110,          3},   // category 3
    {0b1110,         4},   // category 4
    {0b11110,        5},   // category 5
    {0b111110,       6},   // category 6
    {0b1111110,      7},   // category 7
    {0b11111110,     8},   // category 8
    {0b111111110,    9},   // category 9
    {0b1111111110,  10},   // category 10
    {0b11111111110, 11}    // category 11
};

static const HuffmanCode_t AC_LUMA_HUFFMAN[256] = {
    [0x00] = {0xA, 4},       // EOB
    [0x01] = {0x0, 2},
    [0x02] = {0x1, 2},
    [0x03] = {0x4, 3},
    [0x04] = {0xB, 4},
    [0x05] = {0x1A, 5},
    [0x06] = {0x78, 7},
    [0x07] = {0xF8, 8},
    [0x08] = {0x3F6, 10},
    [0x09] = {0xFF82, 16},
    [0x0A] = {0xFF83, 16},

    [0x11] = {0xC, 4},
    [0x12] = {0x1B, 5},
    [0x13] = {0x79, 7},
    [0x14] = {0x1F6, 9},
    [0x15] = {0x7F6, 11},
    [0x16] = {0xFF84, 16},
    [0x17] = {0xFF85, 16},
    [0x18] = {0xFF86, 16},
    [0x19] = {0xFF87, 16},
    [0x1A] = {0xFF88, 16},

    [0x21] = {0x1C, 5},
    [0x22] = {0xF9, 8},
    [0x23] = {0x3F7, 10},
    [0x24] = {0xFF4, 12},
    [0x25] = {0xFF89, 16},
    [0x26] = {0xFF8A, 16},
    [0x27] = {0xFF8B, 16},
    [0x28] = {0xFF8C, 16},
    [0x29] = {0xFF8D, 16},
    [0x2A] = {0xFF8E, 16},

    [0x31] = {0x3A, 6},
    [0x32] = {0x1F7, 9},
    [0x33] = {0xFF5, 12},
    [0x34] = {0xFF8F, 16},
    [0x35] = {0xFF90, 16},
    [0x36] = {0xFF91, 16},
    [0x37] = {0xFF92, 16},
    [0x38] = {0xFF93, 16},
    [0x39] = {0xFF94, 16},
    [0x3A] = {0xFF95, 16},

    [0x41] = {0x3B, 6},
    [0x42] = {0x3F8, 10},
    [0x43] = {0xFF96, 16},
    [0x44] = {0xFF97, 16},
    [0x45] = {0xFF98, 16},
    [0x46] = {0xFF99, 16},
    [0x47] = {0xFF9A, 16},
    [0x48] = {0xFF9B, 16},
    [0x49] = {0xFF9C, 16},
    [0x4A] = {0xFF9D, 16},

    [0x51] = {0x7A, 7},
    [0x52] = {0x7F7, 11},
    [0x53] = {0xFF9E, 16},
    [0x54] = {0xFF9F, 16},
    [0x55] = {0xFFA0, 16},
    [0x56] = {0xFFA1, 16},
    [0x57] = {0xFFA2, 16},
    [0x58] = {0xFFA3, 16},
    [0x59] = {0xFFA4, 16},
    [0x5A] = {0xFFA5, 16},

    [0x61] = {0x7B, 7},
    [0x62] = {0xFF6, 12},
    [0x63] = {0xFFA6, 16},
    [0x64] = {0xFFA7, 16},
    [0x65] = {0xFFA8, 16},
    [0x66] = {0xFFA9, 16},
    [0x67] = {0xFFAA, 16},
    [0x68] = {0xFFAB, 16},
    [0x69] = {0xFFAC, 16},
    [0x6A] = {0xFFAD, 16},

    [0x71] = {0xFA, 8},
    [0x72] = {0xFF7, 12},
    [0x73] = {0xFFAE, 16},
    [0x74] = {0xFFAF, 16},
    [0x75] = {0xFFB0, 16},
    [0x76] = {0xFFB1, 16},
    [0x77] = {0xFFB2, 16},
    [0x78] = {0xFFB3, 16},
    [0x79] = {0xFFB4, 16},
    [0x7A] = {0xFFB5, 16},

    [0x81] = {0x1F8, 9},
    [0x82] = {0x7FC0, 15},
    [0x83] = {0xFFB6, 16},
    [0x84] = {0xFFB7, 16},
    [0x85] = {0xFFB8, 16},
    [0x86] = {0xFFB9, 16},
    [0x87] = {0xFFBA, 16},
    [0x88] = {0xFFBB, 16},
    [0x89] = {0xFFBC, 16},
    [0x8A] = {0xFFBD, 16},

    [0x91] = {0x1F9, 9},
    [0x92] = {0xFFBE, 16},
    [0x93] = {0xFFBF, 16},
    [0x94] = {0xFFC0, 16},
    [0x95] = {0xFFC1, 16},
    [0x96] = {0xFFC2, 16},
    [0x97] = {0xFFC3, 16},
    [0x98] = {0xFFC4, 16},
    [0x99] = {0xFFC5, 16},
    [0x9A] = {0xFFC6, 16},

    [0xA1] = {0x1FA, 9},
    [0xA2] = {0xFFC7, 16},
    [0xA3] = {0xFFC8, 16},
    [0xA4] = {0xFFC9, 16},
    [0xA5] = {0xFFCA, 16},
    [0xA6] = {0xFFCB, 16},
    [0xA7] = {0xFFCC, 16},
    [0xA8] = {0xFFCD, 16},
    [0xA9] = {0xFFCE, 16},
    [0xAA] = {0xFFCF, 16},

    [0xB1] = {0x3F9, 10},
    [0xB2] = {0xFFD0, 16},
    [0xB3] = {0xFFD1, 16},
    [0xB4] = {0xFFD2, 16},
    [0xB5] = {0xFFD3, 16},
    [0xB6] = {0xFFD4, 16},
    [0xB7] = {0xFFD5, 16},
    [0xB8] = {0xFFD6, 16},
    [0xB9] = {0xFFD7, 16},
    [0xBA] = {0xFFD8, 16},

    [0xC1] = {0x3FA, 10},
    [0xC2] = {0xFFD9, 16},
    [0xC3] = {0xFFDA, 16},
    [0xC4] = {0xFFDB, 16},
    [0xC5] = {0xFFDC, 16},
    [0xC6] = {0xFFDD, 16},
    [0xC7] = {0xFFDE, 16},
    [0xC8] = {0xFFDF, 16},
    [0xC9] = {0xFFE0, 16},
    [0xCA] = {0xFFE1, 16},

    [0xD1] = {0x7F8, 11},
    [0xD2] = {0xFFE2, 16},
    [0xD3] = {0xFFE3, 16},
    [0xD4] = {0xFFE4, 16},
    [0xD5] = {0xFFE5, 16},
    [0xD6] = {0xFFE6, 16},
    [0xD7] = {0xFFE7, 16},
    [0xD8] = {0xFFE8, 16},
    [0xD9] = {0xFFE9, 16},
    [0xDA] = {0xFFEA, 16},

    [0xE1] = {0xFFEB, 16},
    [0xE2] = {0xFFEC, 16},
    [0xE3] = {0xFFED, 16},
    [0xE4] = {0xFFEE, 16},
    [0xE5] = {0xFFEF, 16},
    [0xE6] = {0xFFF0, 16},
    [0xE7] = {0xFFF1, 16},
    [0xE8] = {0xFFF2, 16},
    [0xE9] = {0xFFF3, 16},
    [0xEA] = {0xFFF4, 16},

    [0xF0] = {0x7F9, 11},    // ZRL
    [0xF1] = {0xFFF5, 16},
    [0xF2] = {0xFFF6, 16},
    [0xF3] = {0xFFF7, 16},
    [0xF4] = {0xFFF8, 16},
    [0xF5] = {0xFFF9, 16},
    [0xF6] = {0xFFFA, 16},
    [0xF7] = {0xFFFB, 16},
    [0xF8] = {0xFFFC, 16},
    [0xF9] = {0xFFFD, 16},
    [0xFA] = {0xFFFE, 16}
};

static const HuffmanCode_t AC_CHROMA_HUFFMAN[256] = {
    [0x00] = {0x0, 2},       // EOB
    [0x01] = {0x1, 2},
    [0x02] = {0x4, 3},
    [0x03] = {0xA, 4},
    [0x04] = {0x18, 5},
    [0x05] = {0x19, 5},
    [0x06] = {0x38, 6},
    [0x07] = {0x78, 7},
    [0x08] = {0x1F4, 9},
    [0x09] = {0x3F6, 10},
    [0x0A] = {0xFF4, 12},

    [0x11] = {0xB, 4},
    [0x12] = {0x39, 6},
    [0x13] = {0xF6, 8},
    [0x14] = {0x1F5, 9},
    [0x15] = {0x7F6, 11},
    [0x16] = {0xFF5, 12},
    [0x17] = {0xFF88, 16},
    [0x18] = {0xFF89, 16},
    [0x19] = {0xFF8A, 16},
    [0x1A] = {0xFF8B, 16},

    [0x21] = {0x1A, 5},
    [0x22] = {0xF7, 8},
    [0x23] = {0x3F7, 10},
    [0x24] = {0xFF6, 12},
    [0x25] = {0x7FC2, 15},
    [0x26] = {0xFF8C, 16},
    [0x27] = {0xFF8D, 16},
    [0x28] = {0xFF8E, 16},
    [0x29] = {0xFF8F, 16},
    [0x2A] = {0xFF90, 16},

    [0x31] = {0x1B, 5},
    [0x32] = {0xF8, 8},
    [0x33] = {0x3F8, 10},
    [0x34] = {0xFF7, 12},
    [0x35] = {0xFF91, 16},
    [0x36] = {0xFF92, 16},
    [0x37] = {0xFF93, 16},
    [0x38] = {0xFF94, 16},
    [0x39] = {0xFF95, 16},
    [0x3A] = {0xFF96, 16},

    [0x41] = {0x3A, 6},
    [0x42] = {0x1F6, 9},
    [0x43] = {0xFF97, 16},
    [0x44] = {0xFF98, 16},
    [0x45] = {0xFF99, 16},
    [0x46] = {0xFF9A, 16},
    [0x47] = {0xFF9B, 16},
    [0x48] = {0xFF9C, 16},
    [0x49] = {0xFF9D, 16},
    [0x4A] = {0xFF9E, 16},

    [0x51] = {0x3B, 6},
    [0x52] = {0x3F9, 10},
    [0x53] = {0xFF9F, 16},
    [0x54] = {0xFFA0, 16},
    [0x55] = {0xFFA1, 16},
    [0x56] = {0xFFA2, 16},
    [0x57] = {0xFFA3, 16},
    [0x58] = {0xFFA4, 16},
    [0x59] = {0xFFA5, 16},
    [0x5A] = {0xFFA6, 16},

    [0x61] = {0x79, 7},
    [0x62] = {0x7F7, 11},
    [0x63] = {0xFFA7, 16},
    [0x64] = {0xFFA8, 16},
    [0x65] = {0xFFA9, 16},
    [0x66] = {0xFFAA, 16},
    [0x67] = {0xFFAB, 16},
    [0x68] = {0xFFAC, 16},
    [0x69] = {0xFFAD, 16},
    [0x6A] = {0xFFAE, 16},

    [0x71] = {0x7A, 7},
    [0x72] = {0x7F8, 11},
    [0x73] = {0xFFAF, 16},
    [0x74] = {0xFFB0, 16},
    [0x75] = {0xFFB1, 16},
    [0x76] = {0xFFB2, 16},
    [0x77] = {0xFFB3, 16},
    [0x78] = {0xFFB4, 16},
    [0x79] = {0xFFB5, 16},
    [0x7A] = {0xFFB6, 16},

    [0x81] = {0xF9, 8},
    [0x82] = {0xFFB7, 16},
    [0x83] = {0xFFB8, 16},
    [0x84] = {0xFFB9, 16},
    [0x85] = {0xFFBA, 16},
    [0x86] = {0xFFBB, 16},
    [0x87] = {0xFFBC, 16},
    [0x88] = {0xFFBD, 16},
    [0x89] = {0xFFBE, 16},
    [0x8A] = {0xFFBF, 16},

    [0x91] = {0x1F7, 9},
    [0x92] = {0xFFC0, 16},
    [0x93] = {0xFFC1, 16},
    [0x94] = {0xFFC2, 16},
    [0x95] = {0xFFC3, 16},
    [0x96] = {0xFFC4, 16},
    [0x97] = {0xFFC5, 16},
    [0x98] = {0xFFC6, 16},
    [0x99] = {0xFFC7, 16},
    [0x9A] = {0xFFC8, 16},

    [0xA1] = {0x1F8, 9},
    [0xA2] = {0xFFC9, 16},
    [0xA3] = {0xFFCA, 16},
    [0xA4] = {0xFFCB, 16},
    [0xA5] = {0xFFCC, 16},
    [0xA6] = {0xFFCD, 16},
    [0xA7] = {0xFFCE, 16},
    [0xA8] = {0xFFCF, 16},
    [0xA9] = {0xFFD0, 16},
    [0xAA] = {0xFFD1, 16},

    [0xB1] = {0x1F9, 9},
    [0xB2] = {0xFFD2, 16},
    [0xB3] = {0xFFD3, 16},
    [0xB4] = {0xFFD4, 16},
    [0xB5] = {0xFFD5, 16},
    [0xB6] = {0xFFD6, 16},
    [0xB7] = {0xFFD7, 16},
    [0xB8] = {0xFFD8, 16},
    [0xB9] = {0xFFD9, 16},
    [0xBA] = {0xFFDA, 16},

    [0xC1] = {0x1FA, 9},
    [0xC2] = {0xFFDB, 16},
    [0xC3] = {0xFFDC, 16},
    [0xC4] = {0xFFDD, 16},
    [0xC5] = {0xFFDE, 16},
    [0xC6] = {0xFFDF, 16},
    [0xC7] = {0xFFE0, 16},
    [0xC8] = {0xFFE1, 16},
    [0xC9] = {0xFFE2, 16},
    [0xCA] = {0xFFE3, 16},

    [0xD1] = {0x7F9, 11},
    [0xD2] = {0xFFE4, 16},
    [0xD3] = {0xFFE5, 16},
    [0xD4] = {0xFFE6, 16},
    [0xD5] = {0xFFE7, 16},
    [0xD6] = {0xFFE8, 16},
    [0xD7] = {0xFFE9, 16},
    [0xD8] = {0xFFEA, 16},
    [0xD9] = {0xFFEB, 16},
    [0xDA] = {0xFFEC, 16},

    [0xE1] = {0x3FE0, 14},
    [0xE2] = {0xFFED, 16},
    [0xE3] = {0xFFEE, 16},
    [0xE4] = {0xFFEF, 16},
    [0xE5] = {0xFFF0, 16},
    [0xE6] = {0xFFF1, 16},
    [0xE7] = {0xFFF2, 16},
    [0xE8] = {0xFFF3, 16},
    [0xE9] = {0xFFF4, 16},
    [0xEA] = {0xFFF5, 16},

    [0xF0] = {0x3FA, 10},     // ZRL
    [0xF1] = {0x7FC3, 15},
    [0xF2] = {0xFFF6, 16},
    [0xF3] = {0xFFF7, 16},
    [0xF4] = {0xFFF8, 16},
    [0xF5] = {0xFFF9, 16},
    [0xF6] = {0xFFFA, 16},
    [0xF7] = {0xFFFB, 16},
    [0xF8] = {0xFFFC, 16},
    [0xF9] = {0xFFFD, 16},
    [0xFA] = {0xFFFE, 16}
};

uint32_t RGB2YCbCr(const RGB* RGB_stream, const uint16_t block_size, uint8_t *Y_Channel, uint8_t *Cb_Channel, uint8_t *Cr_Channel);

uint32_t DownSampling(uint8_t *Cb_Channel, uint8_t *Cr_Channel, uint16_t width, uint16_t height, uint8_t *Cb_Out, uint8_t *Cr_Out);

uint32_t GetNumBlocks8x8(uint32_t width, uint32_t height);

uint32_t GetNumBlocks16x16(uint32_t width, uint32_t height);

Padding_t PaddImage(const uint8_t *Src_Channel, uint16_t width, uint16_t height, ChType type, uint8_t *Out_Channel );

uint32_t BuildMCU420(uint8_t *Y_Channel,  uint8_t *Cb_Channel,  uint8_t *Cr_Channel,  uint16_t width,  uint16_t height,  uMCU_block_t* MCU_block);

int32_t SendBlockToPL(XAxiDma* AxiDma, uMCU_block_t* InMCU_block, sMCU_block_t* OutMCU_block);

// uint32_t ReceiveQuantizedBlocks(XAxiDma* AxiDma, MCU_block_t MCU_block);

int32_t wait_dma_done(XAxiDma* AxiDma, int32_t direction);

void DCDifferenceEncoding(const sMCU_block_t *QuantBlock, int16_t dc_diff_val[6], int reset);

void ZigZagScan(sMCU_block_t* QuantBlock, sMCU_block_t* ZigzagBlock);

uint32_t RunLengthEncoding(const int8_t *QuantCoeff, RLE_Entry_t* RLE_Entry);

void DCDiffEnc_ZigZag_RLE(
    const sMCU_block_t *QuantBlock,
    HuffmanBlock_t *HuffmanBlock,
    int8_t *previousY,
    int8_t *previousCb,
    int8_t *previousCr);

uint64_t HuffmanEncoding(const HuffmanBlock_t *HuffmanBlock, uint32_t NumBlocks, uint8_t* bitstream);


#endif