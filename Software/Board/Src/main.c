#include "jpeg_core.h"
#include "jpeg_header.h"
#include "xaxidma.h"
#include "xil_types.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xtime_l.h"
#include "ff.h"
#include "xdevcfg.h"
// #include "image_rgb.h"
#include "JpegTables.h"
#include "bmp_decode.h"

XAxiDma AxiDma;
#define DMA_DEV_ID      XPAR_AXIDMA_0_DEVICE_ID

static FATFS fatfs;

int main() {
    XTime ReadStart, ReadEnd;
    XTime ConvertStart, ConvertEnd;
    XTime PLStart, PLEnd;
    XTime WriteStart, WriteEnd;

    uint32_t ReadTime_us;
    uint32_t ConvertTime_us;
    uint32_t PLTime_us;
    uint32_t WriteTime_us;
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

    FIL SrcFile;
    uint32_t height;
    uint32_t width;

    XTime_GetTime(&ReadStart);
    uint32_t FileSize  = DecodeBmpHeader("image.bmp", &SrcFile);
    if (FileSize == 0) {
        xil_printf( "Wrong BMP File Size = 0\r\n");
        return XST_FAILURE;
    }

    uint32_t FullImageSize = DecodeDIBHeader(&SrcFile, &width, &height);
    if (FullImageSize == 0) {
        xil_printf( "Wrong Full Image Size = 0\r\n");
        return XST_FAILURE;
    }

    if (FullImageSize + 54U != FileSize) {
        xil_printf( "Wrong BMP File Size : expected %u, got %u\r\n", FullImageSize + 54U, FileSize);
        f_close(&SrcFile);
        return XST_FAILURE;
    }

    BGR* ImageBGR = malloc(FullImageSize);
    if (ImageBGR == NULL) {
        xil_printf("ImageBGR malloc failed\r\n");
        f_close(&SrcFile);
        return XST_FAILURE;
    }

    uint32_t ImageSize = height * width;

    RetVal = ReadBGRImage(&SrcFile, (uint8_t*)ImageBGR, FullImageSize);

    if (RetVal != XST_SUCCESS) {
        xil_printf("ReadBGRImage failed\r\n");
        free(ImageBGR);
        return XST_FAILURE;
    }

    XTime_GetTime(&ReadEnd);


    XTime_GetTime(&ConvertStart);
    uint8_t* YChannel  = malloc(ImageSize);
    if (YChannel == NULL) {
        xil_printf("YChannel malloc failed\r\n");
        free(ImageBGR);
        return XST_FAILURE;
    }
    uint8_t* CbChannel = malloc(ImageSize);
    if (CbChannel == NULL) {
        free(ImageBGR);
        free(YChannel);
        xil_printf("CbChannel malloc failed\r\n");
        return XST_FAILURE;
    }
    uint8_t* CrChannel = malloc(ImageSize);
    if (CrChannel == NULL) {
        free(ImageBGR);
        free(YChannel);
        free(CbChannel);
        xil_printf("CrChannel malloc failed\r\n");
        return XST_FAILURE;
    }

    RetVal = BGR2YCbCr(ImageBGR, ImageSize, YChannel, CbChannel, CrChannel);
    if (RetVal != XST_SUCCESS) {
        xil_printf("BGR2YCbCr failed\r\n");
        free(ImageBGR);
        free(YChannel);
        free(CbChannel);
        free(CrChannel);
        return XST_FAILURE;
    }
    free(ImageBGR);

    uint32_t NumBlocks = GetNumBlocks16x16(width, height);
    uint8_t* YChannelPd  = malloc(NumBlocks * 16 * 16);
    if (YChannelPd == NULL) {
        xil_printf("YChannelPd malloc failed\r\n");
        return XST_FAILURE;
    }
    Padding_t Padding = PaddImage(YChannel, width, height, Luminance, YChannelPd);
    uint8_t* YChannelPadded =  NULL;
    switch (Padding)
    {
        case PADDED_ALREADY:
            // xil_printf("Padding: already aligned, no padding required\r\n");
            YChannelPadded = YChannel;
            free(YChannelPd);
            break;

        case REQUIRE_H_PADDING:
            // xil_printf("Padding: height padding required\r\n");
            YChannelPadded = YChannelPd;
            free(YChannel);
            break;

        case REQUIRE_W_PADDING:
            // xil_printf("Padding: width padding required\r\n");
            YChannelPadded = YChannelPd;
            free(YChannel);
            break;

        case REQUIRE_F_PADDING:
            // xil_printf("Padding: width and height padding required\r\n");
            YChannelPadded = YChannelPd;
            free(YChannel);
            break;

        default:
            // xil_printf("Padding: unknown value\r\n");
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
    Padding = PaddImage(CbChannel, width, height, Chroma, CbChannelPd);
    uint8_t* CbChannelPadded =  NULL;
    switch (Padding)
    {
        case PADDED_ALREADY:
            // xil_printf("Padding: already aligned, no padding required\r\n");
            CbChannelPadded = CbChannel;
            free(CbChannelPd);
            break;

        case REQUIRE_H_PADDING:
            // xil_printf("Padding: height padding required\r\n");
            CbChannelPadded = CbChannelPd;
            free(CbChannel);
            break;

        case REQUIRE_W_PADDING:
            // xil_printf("Padding: width padding required\r\n");
            CbChannelPadded = CbChannelPd;
            free(CbChannel);
            break;

        case REQUIRE_F_PADDING:
            // xil_printf("Padding: width and height padding required\r\n");
            CbChannelPadded = CbChannelPd;
            free(CbChannel);
            break;

        default:
            // xil_printf("Padding: unknown value\r\n");
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
    Padding = PaddImage(CrChannel, width, height, Chroma, CrChannelPd);
    uint8_t* CrChannelPadded =  NULL;
    switch (Padding)
    {
        case PADDED_ALREADY:
            // xil_printf("Padding: already aligned, no padding required\r\n");
            CrChannelPadded = CrChannel;
            free(CrChannelPd);
            break;

        case REQUIRE_H_PADDING:
            // xil_printf("Padding: height padding required\r\n");
            CrChannelPadded = CrChannelPd;
            free(CrChannel);
            break;

        case REQUIRE_W_PADDING:
            // xil_printf("Padding: width padding required\r\n");
            CrChannelPadded = CrChannelPd;
            free(CrChannel);
            break;

        case REQUIRE_F_PADDING:
            // xil_printf("Padding: width and height padding required\r\n");
            CrChannelPadded = CrChannelPd;
            free(CrChannel);
            break;

        default:
            // xil_printf("Padding: unknown value\r\n");
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
    
    uint32_t PaddedWidth  = ((width  + 15) / 16) * 16;
    uint32_t PaddedHeight = ((height + 15) / 16) * 16;
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

    XTime_GetTime(&PLStart);
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
        Xil_DCacheInvalidateRange(
            (UINTPTR)&sMCU_block[i],
            sizeof(sMCU_block_t)
        );
    }
    free(uMCU_block);
    XTime_GetTime(&PLEnd);

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

    uint32_t MaxBitStreamSize = NumBlocks * 6 * MAX_STUFFED_BYTES_PER_BLOCK;
    uint8_t* BitStream = calloc(MaxBitStreamSize, sizeof(uint8_t));
    if (BitStream == NULL) {
        xil_printf("BitStream malloc failed\r\n");
        return XST_FAILURE;
    }
    uint32_t BitCount = (uint32_t)HuffmanEncoding_ByteStuffing(HuffmanBlock, NumBlocks * 6, BitStream);
    free(HuffmanBlock);

    XTime_GetTime(&ConvertEnd);
    FIL file;
    uint32_t Result;
    XTime_GetTime(&WriteStart);

    uint32_t NumBytes = (BitCount) / 8;
    Result = CreateJpegFile("image", &file);
    if (Result != (uint32_t) FR_OK) {
        xil_printf("CreateJpegFile failed\r\n");
        return XST_FAILURE;
    }

    Result = CreateSOI(&file);
    if (Result != (uint32_t) XST_SUCCESS) {
        xil_printf("CreateSOI failed\r\n");
        return XST_FAILURE;
    }

    Result = CreateApp0(&file);
    if (Result != (uint32_t) XST_SUCCESS) {
        xil_printf("CreateApp0 failed\r\n");
        return XST_FAILURE;
    }

    Result = CreateDQT(&file, LUMA_QUANT, CHROMA_QUANT);
    if (Result != (uint32_t) XST_SUCCESS) {
        xil_printf("CreateDQT failed\r\n");
        return XST_FAILURE;
    }

    Result = CreateSOF0(&file, (const uint16_t) height,  (const uint16_t) width);
    if (Result != (uint32_t) XST_SUCCESS) {
        xil_printf("CreateSOF0 failed\r\n");
        return XST_FAILURE;
    }

    Result = CreateDHT(&file, 0, 0);
    if (Result != (uint32_t) XST_SUCCESS) {
        xil_printf("CreateDHT failed\r\n");
        return XST_FAILURE;
    }

    Result = CreateDHT(&file, 0, 1);
    if (Result != (uint32_t) XST_SUCCESS) {
        xil_printf("CreateDHT failed\r\n");
        return XST_FAILURE;
    }

    Result = CreateDHT(&file, 1, 0);
    if (Result != (uint32_t) XST_SUCCESS) {
        xil_printf("CreateDHT failed\r\n");
        return XST_FAILURE;
    }

    Result = CreateDHT(&file, 1, 1);
    if (Result != (uint32_t) XST_SUCCESS) {
        xil_printf("CreateDHT failed\r\n");
        return XST_FAILURE;
    }

    Result = CreateSOS(&file);
    if (Result != (uint32_t) XST_SUCCESS) {
        xil_printf("CreateSOS failed\r\n");
        return XST_FAILURE;
    }

    Result = AddEntropy(&file, BitStream, NumBytes);
    if (Result != (uint32_t) XST_SUCCESS) {
        xil_printf("AddEntropy failed\r\n");
        return XST_FAILURE;
    }

    Result = CreateEOI(&file);
    if (Result != (uint32_t) XST_SUCCESS) {
        xil_printf("CreateEOI failed\r\n");
        return XST_FAILURE;
    }
    free(BitStream);
    XTime_GetTime(&WriteEnd);

    xil_printf("BitCount: %u bits\r\n", BitCount);
    xil_printf("Bitstream written: %u bytes\r\n", NumBytes);

    ReadTime_us = (uint32_t)(
        ((uint64_t)(ReadEnd - ReadStart) * 1000000ULL)
        / COUNTS_PER_SECOND
    );

    ConvertTime_us = (uint32_t)(
        ((uint64_t)(ConvertEnd - ConvertStart) * 1000000ULL)
        / COUNTS_PER_SECOND
    );

    PLTime_us = (uint32_t)(
        ((uint64_t)(PLEnd - PLStart) * 1000000ULL)
        / COUNTS_PER_SECOND
    );

    WriteTime_us = (uint32_t)(
        ((uint64_t)(WriteEnd - WriteStart) * 1000000ULL)
        / COUNTS_PER_SECOND
    );

    uint32_t FullTime_us = ReadTime_us + ConvertTime_us + WriteTime_us;

    xil_printf("\r\n========== JPEG TIMING ==========\r\n");
    xil_printf("BMP Read       : %u us\r\n", ReadTime_us);
    xil_printf("BMP -> JPEG    : %u us\r\n", ConvertTime_us);
    xil_printf("PL stage       : %u us\r\n", PLTime_us);
    xil_printf("JPEG SD Write  : %u us\r\n", WriteTime_us);
    xil_printf("Full Time      : %u us\r\n", FullTime_us);
    xil_printf("=================================\r\n");
    return XST_SUCCESS;
}
