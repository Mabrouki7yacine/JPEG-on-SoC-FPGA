#ifndef SD_CARD_H
#define SD_CARD_H

#include "xparameters.h"
#include "xil_printf.h"
#include "ff.h"
#include "xdevcfg.h"

int platform_init_fs(FATFS* fatfs);
int sd_mount(FATFS* fatfs);
int sd_write_data(char *file_name,u32 src_addr,u32 byte_len);
int sd_read_data(char *file_name,u32 src_addr,u32 byte_len);

// int main()
// {
//     int status,len;
//     char dest_str[30] = "";

//     status = sd_mount();
//     if (status != XST_SUCCESS){
// 		xil_printf("Failed to open SD card!\n");
// 		return -1;
//     } else {
//         xil_printf("Success to open SD card!\n");
//     }

//     len = strlen(src_str);
//     sd_write_data(FILE_NAME,(u32)src_str,len);
//     sd_read_data(FILE_NAME,(u32)dest_str,len);

//     if (strcmp(src_str, dest_str) == 0)
//     	xil_printf("src_str is equal to dest_str,SD card test success!\n");
//     else
//     	xil_printf("src_str is not equal to dest_str,SD card test failed!\n");

//     return 0;
// }

#endif