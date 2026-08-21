#ifndef JPEG_HEADER_H
#define JPEG_HEADER_H

#include "xil_types.h"
#include "xstatus.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "ff.h"
#include "JpegTables.h"
#include "jpeg_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

const uint8_t SOI_marker[2]  = {0xFF, 0xD8}; 
const uint8_t EOI_marker[2]  = {0xFF, 0xD9}; 

uint32_t CreateJpegFile(const char* FileName, FIL* file);
uint32_t CreateSOI(FIL* file);
uint32_t CreateApp0(FIL* file);
uint32_t CreateDQT(FIL* file, const uint8_t* LumaQTable, const uint8_t* ChromaQTable);
uint32_t CreateSOF0(FIL* file, const uint16_t height, const uint16_t width);
uint32_t CreateDHT(
    FIL* file,
    uint8_t TableID,
    uint8_t TableClass
);
uint32_t CreateSOS(FIL* file);
uint32_t AddEntropy(FIL* file, const uint8_t* BitStream, uint32_t NumBytes);
uint32_t CreateEOI(FIL* file);


#endif