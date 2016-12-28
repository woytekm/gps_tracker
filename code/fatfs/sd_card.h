#include <stdbool.h>
#include "string.h"
#include "ff.h"
#include "diskio.h"

FATFS FatFs;        /* FatFs work area needed for each volume */
FIL Fil;            /* File object needed for each open file */

bool mount_sd(void);
bool write_sd(void * data, uint16_t data_len);
