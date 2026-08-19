#include "sd_card.h"


int platform_init_fs(FATFS* fatfs)
{
	FRESULT status;
	TCHAR *Path = "0:/";
	BYTE work[FF_MAX_SS];

	status = f_mount(fatfs, Path, 1);
	if (status != FR_OK) {
		xil_printf("Volume is not FAT formated; formating FAT\r\n");
		status = f_mkfs(Path, FM_FAT32, 0, work, sizeof work);
		if (status != FR_OK) {
			xil_printf("Unable to format FATfs\r\n");
			return -1;
		}
		status = f_mount(fatfs, Path, 1);
		if (status != FR_OK) {
			xil_printf("Unable to mount FATfs\r\n");
			return -1;
		}
	}
	return 0;
}

int sd_mount(FATFS* fatfs)
{
    int status;
    status = platform_init_fs(fatfs);
    if(status){
        xil_printf("ERROR: f_mount returned %d!\n",status);
        return XST_FAILURE;
    }
    return XST_SUCCESS;
}

int sd_write_data(char *file_name,u32 src_addr,u32 byte_len)
{
    FIL fil;
    UINT bw;

    f_open( &fil, file_name, FA_CREATE_ALWAYS | FA_WRITE);
    f_lseek(&fil, 0);
    f_write(&fil, (void*) src_addr,byte_len, &bw);
    f_close(&fil);
    return 0;
}

int sd_read_data(char *file_name,u32 src_addr,u32 byte_len)
{
	FIL fil;
    UINT br;

    f_open(&fil,file_name,FA_READ);
    f_lseek(&fil,0);
    f_read(&fil,(void*)src_addr,byte_len,&br);
    f_close(&fil);
    return 0;
}
