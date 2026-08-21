#ifndef BMP_DECODE_H
#define BMP_DECODE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <ff.h>
#include <xstatus.h>
#include <xil_types.h>
#include <xil_cache.h>
#include <xil_printf.h>
#include <xparameters.h>

uint32_t DecodeBmpHeader(const char* FileName, FIL* file); /*Read File Size*/
uint32_t DecodeDIBHeader(FIL* file, uint32_t* width, uint32_t* height); /* Read Image size, width and height */
uint32_t ReadBGRImage(FIL* file, uint8_t* Image, uint32_t ImageSize); /* Read BGR Stream */

#endif