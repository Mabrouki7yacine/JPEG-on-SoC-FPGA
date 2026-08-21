#include "bmp_decode.h"

uint32_t DecodeBmpHeader(const char* FileName, FIL* file) {
    UINT BytesRead;
    FRESULT Result;

    Result = f_open(file, FileName, FA_READ);
    if (Result != FR_OK) {
        xil_printf("f_open BMP failed\r\n");
        return 0U;
    }
    uint8_t FileHeader[14];
    Result = f_read(file, (void*)FileHeader, 14, &BytesRead);
    if (Result != FR_OK || BytesRead != 14) {
        xil_printf("f_read Bmp Header failed\r\n");
        f_close(file);
        return 0U;
    }

    if (FileHeader[0] != 'B' || FileHeader[1] != 'M') {
        xil_printf("Wrong BM Signature : %c%c \r\n", (char) FileHeader[0], (char) FileHeader[1] );
        f_close(file);
        return 0U;
    }

    uint32_t FileSize = 0;
    FileSize |= ((uint32_t)FileHeader[2]);
    FileSize |= ((uint32_t)FileHeader[3] << 8);
    FileSize |= ((uint32_t)FileHeader[4] << 16);
    FileSize |= ((uint32_t)FileHeader[5] << 24);

    if (FileHeader[6] != 0x00 || FileHeader[7] != 0x00) {
        xil_printf("Wrong Reserved 1 : %u %u\r\n", FileHeader[6], FileHeader[7]);
        f_close(file);
        return 0U;
    }

    if (FileHeader[8] != 0x00 || FileHeader[9] != 0x00) {
        xil_printf("Wrong Reserved 2 : %u %u\r\n", FileHeader[8], FileHeader[9]);
        f_close(file);
        return 0U;
    }

    uint32_t Offset = 0;
    Offset |= ((uint32_t)FileHeader[10]);
    Offset |= ((uint32_t)FileHeader[11] << 8);
    Offset |= ((uint32_t)FileHeader[12] << 16);
    Offset |= ((uint32_t)FileHeader[13] << 24);

    if (Offset != 0x00000036) {
        xil_printf("Wrong Offset : %u\r\n", Offset);
        f_close(file);
        return 0U;
    }

    return FileSize;
}

uint32_t DecodeDIBHeader(FIL* file, uint32_t* width, uint32_t* height) {
    UINT BytesRead;
    FRESULT Result;

    uint8_t DIBHeader[40];
    Result = f_read(file, (void*)DIBHeader, 40, &BytesRead);
    if (Result != FR_OK || BytesRead != 40) {
        xil_printf("f_read DIB Header failed\r\n");
        f_close(file);
        return 0U;
    }

    uint32_t HeaderSize = 0;
    HeaderSize |= ((uint32_t)DIBHeader[0]);
    HeaderSize |= ((uint32_t)DIBHeader[1] << 8);
    HeaderSize |= ((uint32_t)DIBHeader[2] << 16);
    HeaderSize |= ((uint32_t)DIBHeader[3] << 24);

    if (HeaderSize != 0x00000028) {
        xil_printf("Wrong HeaderSize : %u\r\n", HeaderSize);
        f_close(file);
        return 0U;
    }

    *(width) = 0;
    *(width) |= ((uint32_t)DIBHeader[4]);
    *(width) |= ((uint32_t)DIBHeader[5] << 8);
    *(width) |= ((uint32_t)DIBHeader[6] << 16);
    *(width) |= ((uint32_t)DIBHeader[7] << 24);

    *(height) = 0;
    *(height)  = ((uint32_t)DIBHeader[8]);
    *(height) |= ((uint32_t)DIBHeader[9]  << 8);
    *(height) |= ((uint32_t)DIBHeader[10] << 16);
    *(height) |= ((uint32_t)DIBHeader[11] << 24);

    if (((*height) & 0x80000000U) == 0U) {
        xil_printf("BMP must be top-down\r\n");
        f_close(file);
        return 0U;
    }

    *(height) = (~(*height)) + 1U;

    if (DIBHeader[12] != 0x01 || DIBHeader[13] != 0x00) {
        xil_printf("Wrong Plane : %u %u\r\n", DIBHeader[12], DIBHeader[13]);
        f_close(file);
        return 0U;
    }

    if (DIBHeader[14] != 0x18 || DIBHeader[15] != 0x00) {
        xil_printf("Wrong Bits per pixel : %u %u\r\n", DIBHeader[14], DIBHeader[15]);
        f_close(file);
        return 0U;
    }

    uint32_t Compression = 0;
    Compression |= ((uint32_t)DIBHeader[16]);
    Compression |= ((uint32_t)DIBHeader[17] << 8);
    Compression |= ((uint32_t)DIBHeader[18] << 16);
    Compression |= ((uint32_t)DIBHeader[19] << 24);
    if (Compression != 0x00000000) {
        xil_printf("Wrong Compression : %u\r\n", Compression);
        f_close(file);
        return 0U;
    }

    uint32_t ImageSize = 0;
    ImageSize |= ((uint32_t)DIBHeader[20]);
    ImageSize |= ((uint32_t)DIBHeader[21] << 8);
    ImageSize |= ((uint32_t)DIBHeader[22] << 16);
    ImageSize |= ((uint32_t)DIBHeader[23] << 24);

    uint32_t RowSize = (((*width) * 3U + 3U) / 4U) * 4U;
    uint32_t ExpectedImageSize = RowSize * (*height);

    if (ExpectedImageSize != ImageSize) {
        xil_printf("Wrong Image Size : %u, %u\r\n", ExpectedImageSize, ImageSize);
        f_close(file);
        return 0U;
    }

    uint32_t XRes = 0;
    XRes |= ((uint32_t)DIBHeader[24]);
    XRes |= ((uint32_t)DIBHeader[25] << 8);
    XRes |= ((uint32_t)DIBHeader[26] << 16);
    XRes |= ((uint32_t)DIBHeader[27] << 24);

    uint32_t YRes = 0;
    YRes |= ((uint32_t)DIBHeader[28]);
    YRes |= ((uint32_t)DIBHeader[29] << 8);
    YRes |= ((uint32_t)DIBHeader[30] << 16);
    YRes |= ((uint32_t)DIBHeader[31] << 24);

    uint32_t ColorUsed = 0;
    ColorUsed |= ((uint32_t)DIBHeader[32]);
    ColorUsed |= ((uint32_t)DIBHeader[33] << 8);
    ColorUsed |= ((uint32_t)DIBHeader[34] << 16);
    ColorUsed |= ((uint32_t)DIBHeader[35] << 24);
    if (ColorUsed != 0x00000000) {
        xil_printf("Wrong Used Color : %u\r\n", ColorUsed);
        f_close(file);
        return 0U;
    }

    uint32_t ImportantColor = 0;
    ImportantColor |= ((uint32_t)DIBHeader[36]);
    ImportantColor |= ((uint32_t)DIBHeader[37] << 8);
    ImportantColor |= ((uint32_t)DIBHeader[38] << 16);
    ImportantColor |= ((uint32_t)DIBHeader[39] << 24);
    if (ImportantColor != 0x00000000) {
        xil_printf("Wrong Important Color : %u\r\n", ImportantColor);
        f_close(file);
        return 0U;
    }

    return ImageSize;
}

uint32_t ReadBGRImage(FIL* file, uint8_t* Image, uint32_t ImageSize)
{
    assert(file  != NULL);
    assert(Image != NULL);

    UINT BytesRead;
    FRESULT Result = f_read(file, Image, ImageSize, &BytesRead);

    if (Result != FR_OK || BytesRead != ImageSize) {
        xil_printf("f_read BGR Image failed : %u / %u bytes\r\n", BytesRead, ImageSize);
        f_close(file);
        return (uint32_t)XST_FAILURE;
    }

    f_close(file);
    return (uint32_t)XST_SUCCESS;
}