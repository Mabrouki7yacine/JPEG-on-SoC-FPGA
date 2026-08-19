#include "jpeg_core.h"
#include "xaxidma.h"
#include "xil_types.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xtime_l.h"
#include "ff.h"
#include "xdevcfg.h"
#include "image_rgb.h"

XAxiDma AxiDma;
#define DMA_DEV_ID      XPAR_AXIDMA_0_DEVICE_ID

static FATFS fatfs;

int main() {
    uint32_t RetVal = -1;
    XAxiDma_Config *CfgPtr;

    xil_printf("\r\nJpeg Core Test Started\r\n");

    RetVal = sd_mount(&fatfs);
    if (RetVal != XST_SUCCESS) {
        xil_printf("SD mount failed\r\n");
        return XST_FAILURE;
    }

    CfgPtr = XAxiDma_LookupConfig(DMA_DEV_ID);
    if (!CfgPtr) {
        xil_printf("ERROR: No DMA config found\r\n");
        return XST_FAILURE;
    }

    RetVal = (uint32_t) XAxiDma_CfgInitialize(&AxiDma, CfgPtr);
    if (RetVal != XST_SUCCESS) {
        xil_printf("ERROR: DMA initialization failed\r\n");
        return XST_FAILURE;
    }

    if (XAxiDma_HasSg(&AxiDma)) {
        xil_printf("ERROR: DMA is in Scatter-Gather mode\r\n");
        return XST_FAILURE;
    }

    uint8_t* YChannel  = malloc(IMAGE_SIZE);
    if (YChannel == NULL) {
        xil_printf("YChannel malloc failed\r\n");
        return XST_FAILURE;
    }
    uint8_t* CbChannel = malloc(IMAGE_SIZE);
    if (CbChannel == NULL) {
        xil_printf("CbChannel malloc failed\r\n");
        return XST_FAILURE;
    }
    uint8_t* CrChannel = malloc(IMAGE_SIZE);
    if (CrChannel == NULL) {
        xil_printf("CrChannel malloc failed\r\n");
        return XST_FAILURE;
    }

    RetVal = RGB2YCbCr(ImageRGB, IMAGE_SIZE, YChannel, CbChannel, CrChannel);
    if (RetVal != 0) {
        xil_printf("RGB2YCbCr failed\r\n");
        return XST_FAILURE;
    }

    uint32_t NumBlocks = GetNumBlocks16x16(IMAGE_WIDTH, IMAGE_HEIGHT);
    uint8_t* YChannelPd  = malloc(NumBlocks * 16 * 16);
    if (YChannelPd == NULL) {
        xil_printf("YChannelPd malloc failed\r\n");
        return XST_FAILURE;
    }
    Padding_t Padding = PaddImage(YChannel, IMAGE_WIDTH, IMAGE_HEIGHT, Luminance, YChannelPd);
    uint8_t* YChannelPadded =  NULL;
    switch (Padding)
    {
        case PADDED_ALREADY:
            xil_printf("Padding: already aligned, no padding required\r\n");
            YChannelPadded = YChannel;
            free(YChannelPd);
            break;

        case REQUIRE_H_PADDING:
            xil_printf("Padding: height padding required\r\n");
            YChannelPadded = YChannelPd;
            free(YChannel);
            break;

        case REQUIRE_W_PADDING:
            xil_printf("Padding: width padding required\r\n");
            YChannelPadded = YChannelPd;
            free(YChannel);
            break;

        case REQUIRE_F_PADDING:
            xil_printf("Padding: width and height padding required\r\n");
            YChannelPadded = YChannelPd;
            free(YChannel);
            break;

        default:
            xil_printf("Padding: unknown value\r\n");
            free(YChannelPd);
            free(YChannel);
            return XST_FAILURE;
            break;
    }

    uint8_t* CbChannelPd  = malloc(NumBlocks * 16 * 16);
    if (CbChannelPd == NULL) {
        xil_printf("CbChannelPd malloc failed\r\n");
        return XST_FAILURE;
    }
    Padding = PaddImage(CbChannel, IMAGE_WIDTH, IMAGE_HEIGHT, Chroma, CbChannelPd);
    uint8_t* CbChannelPadded =  NULL;
    switch (Padding)
    {
        case PADDED_ALREADY:
            xil_printf("Padding: already aligned, no padding required\r\n");
            CbChannelPadded = CbChannel;
            free(CbChannelPd);
            break;

        case REQUIRE_H_PADDING:
            xil_printf("Padding: height padding required\r\n");
            CbChannelPadded = CbChannelPd;
            free(CbChannel);
            break;

        case REQUIRE_W_PADDING:
            xil_printf("Padding: width padding required\r\n");
            CbChannelPadded = CbChannelPd;
            free(CbChannel);
            break;

        case REQUIRE_F_PADDING:
            xil_printf("Padding: width and height padding required\r\n");
            CbChannelPadded = CbChannelPd;
            free(CbChannel);
            break;

        default:
            xil_printf("Padding: unknown value\r\n");
            free(CbChannelPd);
            free(CbChannel);
            return XST_FAILURE;
            break;
    }

    uint8_t* CrChannelPd  = malloc(NumBlocks * 16 * 16);
    if (CrChannelPd == NULL) {
        xil_printf("CrChannelPd malloc failed\r\n");
        return XST_FAILURE;
    }
    Padding = PaddImage(CrChannel, IMAGE_WIDTH, IMAGE_HEIGHT, Chroma, CrChannelPd);
    uint8_t* CrChannelPadded =  NULL;
    switch (Padding)
    {
        case PADDED_ALREADY:
            xil_printf("Padding: already aligned, no padding required\r\n");
            CrChannelPadded = CrChannel;
            free(CrChannelPd);
            break;

        case REQUIRE_H_PADDING:
            xil_printf("Padding: height padding required\r\n");
            CrChannelPadded = CrChannelPd;
            free(CrChannel);
            break;

        case REQUIRE_W_PADDING:
            xil_printf("Padding: width padding required\r\n");
            CrChannelPadded = CrChannelPd;
            free(CrChannel);
            break;

        case REQUIRE_F_PADDING:
            xil_printf("Padding: width and height padding required\r\n");
            CrChannelPadded = CrChannelPd;
            free(CrChannel);
            break;

        default:
            xil_printf("Padding: unknown value\r\n");
            free(CrChannelPd);
            free(CrChannel);
            return XST_FAILURE;
            break;
    }

    uint8_t* CbChannelDS = malloc(NumBlocks * 16 * 16 / 4);
    if (CbChannelDS == NULL) {
        xil_printf("CbChannelDS malloc failed\r\n");
        return XST_FAILURE;
    }
    uint8_t* CrChannelDS = malloc(NumBlocks * 16 * 16 / 4);
    if (CrChannelDS == NULL) {
        xil_printf("CrChannelDS malloc failed\r\n");
        return XST_FAILURE;
    }
    
    uint32_t PaddedWidth  = ((IMAGE_WIDTH  + 15) / 16) * 16;
    uint32_t PaddedHeight = ((IMAGE_HEIGHT + 15) / 16) * 16;
    RetVal = DownSampling(CbChannelPadded, CrChannelPadded, PaddedWidth, PaddedHeight, CbChannelDS, CrChannelDS);
    if (RetVal != XST_SUCCESS) {
        xil_printf("DownSampling failed\r\n");
        return XST_FAILURE;
    }
    free(CbChannelPadded);
    free(CrChannelPadded);

    uMCU_block_t* uMCU_block = malloc(NumBlocks * sizeof(uMCU_block_t));
    if (uMCU_block == NULL) {
        xil_printf("uMCU_block malloc failed\r\n");
        return XST_FAILURE;
    }
    RetVal = BuildMCU420(YChannelPadded, CbChannelDS, CrChannelDS, PaddedWidth, PaddedHeight, uMCU_block);
    if (RetVal != NumBlocks) {
        xil_printf("BuildMCU420 failed\r\n");
        return XST_FAILURE;
    }
    free(YChannelPadded);
    free(CbChannelDS);
    free(CrChannelDS);

    sMCU_block_t* sMCU_block = malloc(NumBlocks * sizeof(sMCU_block_t));
    if (sMCU_block == NULL) {
        xil_printf("sMCU_block malloc failed\r\n");
        return XST_FAILURE;
    }

    for (uint16_t i = 0; i < NumBlocks; i++) {
        RetVal = (uint32_t) SendBlockToPL(&AxiDma, &(uMCU_block[i]), &(sMCU_block[i]));
        if (RetVal != XST_SUCCESS) {
            xil_printf("SendBlockToPL %d failed\r\n", i);
            return XST_FAILURE;
        }

        RetVal = (uint32_t) wait_dma_done(&AxiDma, XAXIDMA_DMA_TO_DEVICE);
        if (RetVal != XST_SUCCESS) {
            xil_printf("ERROR: MM2S did not finish\r\n");
            return XST_FAILURE;
        }

        RetVal = (uint32_t) wait_dma_done(&AxiDma, XAXIDMA_DEVICE_TO_DMA);
        if (RetVal != XST_SUCCESS) {
            xil_printf("ERROR: S2MM did not finish\r\n");
            return XST_FAILURE;
        }
    }
    free(uMCU_block);

    int8_t previousY = 0;
    int8_t previousCb = 0;
    int8_t previousCr = 0;

    HuffmanBlock_t* HuffmanBlock = malloc(NumBlocks * 6 * sizeof(HuffmanBlock_t));
    if (HuffmanBlock == NULL) {
        xil_printf("HuffmanBlock malloc failed\r\n");
        return XST_FAILURE;
    }

    for (uint16_t i = 0; i < NumBlocks; i++) {
        DCDiffEnc_ZigZag_RLE(
            &(sMCU_block[i]),
            &(HuffmanBlock[i * 6]),
            &previousY,
            &previousCb,
            &previousCr); 
    }
    free(sMCU_block);

    uint32_t MaxBitStreamSize = NumBlocks * 6 * MAX_BYTES_PER_BLOCK;
    uint8_t* BitStream = calloc(MaxBitStreamSize, sizeof(uint8_t));
    if (BitStream == NULL) {
        xil_printf("BitStream malloc failed\r\n");
        return XST_FAILURE;
    }
    uint32_t BitCount = (uint32_t)HuffmanEncoding(HuffmanBlock, NumBlocks * 6, BitStream);
    free(HuffmanBlock);

    FIL fil;
    FRESULT Result;
    UINT BytesWritten;

    uint32_t NumBytes = (BitCount + 7) / 8;
    Result = f_open( &fil, "BitStream.bin", FA_CREATE_ALWAYS | FA_WRITE);
    if (Result != FR_OK) {
        xil_printf("f_open failed: %d\r\n", Result);
        return XST_FAILURE;
    }

    Result = f_write(&fil, (void*) BitStream, NumBytes, &BytesWritten);
    if (Result != FR_OK || BytesWritten != NumBytes) {
        xil_printf("f_write failed\r\n");
        f_close(&fil);
        return XST_FAILURE;
    }

    f_close(&fil);

    xil_printf("BitCount: %u bits\r\n", BitCount);
    xil_printf("Bitstream written: %u bytes\r\n", NumBytes);

    free(BitStream);

    return XST_SUCCESS;
}
