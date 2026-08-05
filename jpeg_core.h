#ifndef JPEG_CORE_H
#define JPEG_CORE_H

#include <arm_neon.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

#define BLOCK_SIZE 384 // 4 * 8 * 8 + 2 * 8 * 8

#define Coeff_Y_R   0.299f
#define Coeff_Y_G   0.587f
#define Coeff_Y_B   0.114f

#define Coeff_Cb_R -0.168736f
#define Coeff_Cb_G -0.331264f
#define Coeff_Cb_B  0.500000f

#define Coeff_Cr_R  0.500000f
#define Coeff_Cr_G -0.418688f
#define Coeff_Cr_B -0.081312f


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
    uint8_t* Y0;
    uint8_t* Y1;
    uint8_t* Y2;
    uint8_t* Y3;
    uint8_t* Cb;
    uint8_t* Cr;
} MCU_block_t;

typedef enum {
    PADDED_ALREADY,
    HEIGHT_PADDED,
    WIDTH_PADDED,
    REQUIRE_PADDING,
} Padding;

typedef enum {
    Luminance = 16,
    Color = 8,
} ChType;

uint32_t RGB2YCbCr(const RGB* RGB_stream, const uint16_t block_size, uint8_t *Y_Channel, uint8_t *Cb_Channel, uint8_t *Cr_Channel);

uint32_t DownSampling(uint8_t *Cb_Channel, uint8_t *Cr_Channel, uint16_t width, uint16_t height, uint8_t *Cb_Out, uint8_t *Cr_Out);

Padding PaddImage(uint8_t *Src_Channel, uint16_t width, uint16_t height, ChType type, uint8_t *Out_Channel);

uint32_t BuildMCU420(MCU_block_t MCU_block);

uint32_t SendBlocksToPL(MCU_block_t MCU_block);

uint32_t ReceiveQuantizedBlocks(MCU_block_t MCU_block);

uint32_t ZigZagScan(uint8_t *Quant_coeff, uint8_t *Zigzag_coeff);

uint32_t DCDifferenceEncoding(uint8_t *Quant_coeff);

uint32_t RunLengthEncoding(uint8_t *Coeff);



#endif