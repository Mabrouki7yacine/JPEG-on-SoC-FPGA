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
} Padding;

typedef enum {
    Luminance = 16,
    Chroma = 8,
} ChType;

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

uint32_t RGB2YCbCr(const RGB* RGB_stream, const uint16_t block_size, uint8_t *Y_Channel, uint8_t *Cb_Channel, uint8_t *Cr_Channel);

uint32_t DownSampling(uint8_t *Cb_Channel, uint8_t *Cr_Channel, uint16_t width, uint16_t height, uint8_t *Cb_Out, uint8_t *Cr_Out);

Padding PaddImage(const uint8_t *Src_Channel, uint16_t width, uint16_t height, ChType type, uint8_t *Out_Channel );

uint32_t BuildMCU420(
uint8_t *Y_Channel, 
uint8_t *Cb_Channel, 
uint8_t *Cr_Channel, 
uint16_t width, 
uint16_t height, 
uMCU_block_t** MCU_block);

int32_t SendBlockToPL(XAxiDma* AxiDma, uMCU_block_t* InMCU_block, sMCU_block_t* OutMCU_block);

// uint32_t ReceiveQuantizedBlocks(XAxiDma* AxiDma, MCU_block_t MCU_block);

int32_t wait_dma_done(XAxiDma* AxiDma, int32_t direction);

void DCDifferenceEncoding(sMCU_block_t* QuantBlock);

void ZigZagScan(sMCU_block_t* QuantBlock, sMCU_block_t* ZigzagBlock);

uint32_t RunLengthEncoding(uint8_t *Coeff);


#endif