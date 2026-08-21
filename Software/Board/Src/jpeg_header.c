#include "jpeg_header.h"

uint32_t CreateJpegFile(const char* FileName, FIL* file) {
    assert(file  != NULL);
    char buffer[50 + 5];
    snprintf(buffer, sizeof(buffer), "%.50s.jpg", FileName);
    return f_open(file, buffer, FA_WRITE | FA_CREATE_ALWAYS);
}

uint32_t CreateSOI(FIL* file) {
    assert(file  != NULL);
    UINT BytesWritten;
    FRESULT Result = f_write(file, (void*) SOI_marker, 2, &BytesWritten);
    if (Result != FR_OK || BytesWritten != 2) {
        xil_printf("f_write SOI failed\r\n");
        f_close(file);
        return (uint32_t) (XST_FAILURE);
    }
    return (uint32_t) (XST_SUCCESS);
}

uint32_t CreateApp0(FIL* file) {
    assert(file  != NULL);
    UINT BytesWritten;
    uint8_t Bytes[18] = {
        0xFF, 0xE0, // APP0 marker
        0x00, 0x10, // length
        0x4A, 0x46, 0x49, 0x46, 0x00, // "JFIF\0"
        0x01, 0x01, // VERSION 1.01
        0x00,       // Density unit : no phy unit
        0x00, 0x01, // X densiy = 1
        0x00, 0x01, // Y densiy = 1
        0x00, // Thumbnail Width
        0x00  // Thumbnail Height
    };

    FRESULT Result = f_write(file, (void*) Bytes, 18, &BytesWritten);
    if (Result != FR_OK || BytesWritten != 18) {
        xil_printf("f_write APP0 failed\r\n");
        f_close(file);
        return (uint32_t) (XST_FAILURE);
    }

    return (uint32_t) (XST_SUCCESS);
}

uint32_t CreateDQT(FIL* file, const uint8_t* LumaQTable, const uint8_t* ChromaQTable) {
    assert(file  != NULL);
    UINT BytesWritten;
    uint8_t Bytes[69] = {
        0xFF, 0xDB, // DQT marker
        0x00, 0x43, // length
        0x00        // 0b0000 8 bits precision, 0b0000 Table ID 0 -> Y
    };

    for (uint8_t i = 0; i < 64; i++) {
        Bytes[5 + i] = LumaQTable[JPEG_ZIGZAG[i]];
    }
    FRESULT Result = f_write(file, (void*) Bytes, 69, &BytesWritten);
    if (Result != FR_OK || BytesWritten != 69) {
        xil_printf("f_write DQT Luma failed\r\n");
        f_close(file);
        return (uint32_t) (XST_FAILURE);
    }

    Bytes[4] = 0x01; // 0b0000 8 bits precision, 0b0000 Table ID 0 -> Y

    for (uint8_t i = 0; i < 64; i++) {
        Bytes[5 + i] = ChromaQTable[JPEG_ZIGZAG[i]];
    }
    Result = f_write(file, (void*) Bytes, 69, &BytesWritten);
    if (Result != FR_OK || BytesWritten != 69) {
        xil_printf("f_write DQT Chroma failed\r\n");
        f_close(file);
        return (uint32_t) (XST_FAILURE);
    }

    return (uint32_t) (XST_SUCCESS);
}

uint32_t CreateSOF0(FIL* file, const uint16_t height, const uint16_t width) {
    assert(file  != NULL);
    UINT BytesWritten;
    uint8_t Bytes[19] = {
        0xFF, 0xC0, // SOF0 marker
        0x00, 0x11, // length
        0x08,       // 8bits/sample
        (uint8_t) (height >> 8), (uint8_t) (height & 0xFF), // height
        (uint8_t) (width  >> 8), (uint8_t) (width  & 0xFF), // width
        0x03,             // Num of components
        0x01, 0x22, 0x00, // Y  info
        0x02, 0x11, 0x01, // Cb info
        0x03, 0x11, 0x01  // Cr info
    };

    FRESULT Result = f_write(file, (void*) Bytes, 19, &BytesWritten);
    if (Result != FR_OK || BytesWritten != 19) {
        xil_printf("f_write SOF0 failed\r\n");
        f_close(file);
        return (uint32_t) (XST_FAILURE);
    }

    return (uint32_t) (XST_SUCCESS);
}

uint32_t CreateDHT(FIL* file, uint8_t TableID, uint8_t TableClass) {
    assert(file  != NULL);
    UINT BytesWritten;


    if (TableClass == 0x00) { // DC

        uint8_t Bytes[2 + 2 + 1 + 16 + 12] = {
            0xFF, 0xC4, // DHT marker
            0x00, 0x1F, // length
        };

        if (TableID == 0x00) {
            Bytes[4] = ((TableClass << 4) & 0xF0) | (TableID & 0x0F); // Table Info
            memcpy(Bytes + 5, DC_LUMA_BITS, 16 * sizeof(uint8_t));
            memcpy(Bytes + 5 + 16, DC_LUMA_VALUES, 12 * sizeof(uint8_t));
        } else if (TableID == 0x01) {
            Bytes[4] = ((TableClass << 4) & 0xF0) | (TableID & 0x0F); // Table Info
            memcpy(Bytes + 5, DC_CHROMA_BITS, 16 * sizeof(uint8_t));
            memcpy(Bytes + 5 + 16, DC_CHROMA_VALUES, 12 * sizeof(uint8_t));
        } else {
            xil_printf("Invalid TableID\r\n");
            f_close(file);
            return (uint32_t) (XST_FAILURE);
        }

        FRESULT Result = f_write(file, (void*) Bytes, 33, &BytesWritten);
        if (Result != FR_OK || BytesWritten != 33) {
            xil_printf("f_write DHT failed\r\n");
            f_close(file);
            return (uint32_t) (XST_FAILURE);
        }

    } else if (TableClass == 0x01) { // AC

        uint8_t Bytes[2 + 2 + 1 + 16 + 162] = {
            0xFF, 0xC4, // DHT marker
            0x00, 0xB5 // length
        };

        if (TableID == 0x00) {
            Bytes[4] = ((TableClass << 4) & 0xF0) | (TableID & 0x0F); // Table Info
            memcpy(Bytes + 5, AC_LUMA_BITS, 16 * sizeof(uint8_t));
            memcpy(Bytes + 5 + 16, AC_LUMA_VALUES, 162 * sizeof(uint8_t));
        } else if (TableID == 0x01) {
            Bytes[4] = ((TableClass << 4) & 0xF0) | (TableID & 0x0F); // Table Info
            memcpy(Bytes + 5, AC_CHROMA_BITS, 16 * sizeof(uint8_t));
            memcpy(Bytes + 5 + 16, AC_CHROMA_VALUES, 162 * sizeof(uint8_t));
        } else {
            xil_printf("Invalid TableID\r\n");
            f_close(file);
            return (uint32_t) (XST_FAILURE);
        }

        FRESULT Result = f_write(file, (void*) Bytes, 183, &BytesWritten);
        if (Result != FR_OK || BytesWritten != 183) {
            xil_printf("f_write DHT failed\r\n");
            f_close(file);
            return (uint32_t) (XST_FAILURE);
        }

    } else {
        xil_printf("Invalid TableClass\r\n");
        f_close(file);
        return (uint32_t) (XST_FAILURE);
    }

    return (uint32_t) (XST_SUCCESS);
}

uint32_t CreateSOS(FIL* file) {
    assert(file  != NULL);
    UINT BytesWritten;
    uint8_t Bytes[14] = {
        0xFF, 0xDA, // SOS marker
        0x00, 0x0C, // length
        0x03,       // Num of Comp
        0x01, 0x00, // Y  info : DC Table : 0, AC Table : 0
        0x02, 0x11, // Cb info : DC Table : 1, AC Table : 1
        0x03, 0x11, // Cr info : DC Table : 1, AC Table : 1
        0x00,       // Start from 00 (DC value)
        0x3F,       // End at 63 (last AC value)
        0x00        // Ah/Al
    };

    FRESULT Result = f_write(file, (void*) Bytes, 14, &BytesWritten);
    if (Result != FR_OK || BytesWritten != 14) {
        xil_printf("f_write SOS failed\r\n");
        f_close(file);
        return (uint32_t) (XST_FAILURE);
    }

    return (uint32_t) (XST_SUCCESS);
}

// uint32_t AddEntropy(FIL* file, const uint8_t* BitStream, uint32_t NumBytes) {
//     return (uint32_t) (XST_SUCCESS);
// }

uint32_t CreateEOI(FIL* file) {
    assert(file  != NULL);
    UINT BytesWritten;
    FRESULT Result = f_write(file, (void*) EOI_marker, 2, &BytesWritten);
    if (Result != FR_OK || BytesWritten != 2) {
        xil_printf("f_write EOI failed\r\n");
        f_close(file);
        return (uint32_t) (XST_FAILURE);
    }
    return (uint32_t) (XST_SUCCESS);
}