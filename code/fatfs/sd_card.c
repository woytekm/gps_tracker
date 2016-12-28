#include "sd_card.h"

bool mount_sd(void)
{
	uint8_t temp;
	for(int i=0; i<5; i++)
	{
		temp = f_mount(&FatFs, "", 1);     /* Give a work area to the default drive */
		if(temp == 0)   break;
	}
	sd_spi_uninit();
	return temp == 0;
}

bool write_sd(void * data, uint16_t data_len)
{
	sd_spi_init();
    UINT bw;
	if (f_open(&Fil, "data.txt", FA_WRITE | FA_OPEN_APPEND) == FR_OK)
	{

		/* Create a file */
//		f_lseek(&Fil, f_size(&Fil));
		f_write(&Fil, data, data_len, &bw);	/* Write data to the file */

		f_close(&Fil);								/* Close the file */
//		if (bw != data_len)
//		{
//			return false;
//		}
	}
	sd_spi_uninit();
	return true;
}
