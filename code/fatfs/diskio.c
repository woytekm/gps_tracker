/*-----------------------------------------------------------------------*/
/* SPI controls (Platform dependent)                                     */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
#include "integer.h"

#include "tracker_platform.h"
#include "tracker_routines.h"
#include "tracker_includes.h"
#include "tracker_global.h"

#ifndef SPI0_ENABLED
#define SPI0_ENABLED 1
#endif
#if  SPI0_ENABLED
 // <q> SPI0_USE_EASY_DMA  - Use EasyDMA


#ifndef SPI0_USE_EASY_DMA
#define SPI0_USE_EASY_DMA 0
#endif

#endif //SPI0_ENABLED

static nrf_drv_spi_t 				m_spi_master = NRF_DRV_SPI_INSTANCE(0);

//#define SD_DRV_DEBUG

#define SD_CS      USD_CS
#define SD_SCK     USD_SCLK
#define SD_MOSI    USD_MISO
#define SD_MISO    USD_MOSI

#ifndef FATFS_CS_PIN		
#define FATFS_CS_PIN						SD_CS
#endif


/* MMC/SD command */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define CMD13	(13)		/* SEND_STATUS */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

static DSTATUS Stat = STA_NOINIT;	/* Disk status */

static BYTE CardType;			/* b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing */

static uint32_t count = 0;
uint8_t miso = 0;


static uint32_t spi_master_init(nrf_drv_spi_t * spi_master_instance)
{
    uint32_t err_code = NRF_SUCCESS;
    nrf_drv_spi_config_t spi_config =
    {
        .ss_pin = NRF_DRV_SPI_PIN_NOT_USED,
        .irq_priority = APP_IRQ_PRIORITY_LOW,
        .orc = 0xCC,
        .frequency = NRF_DRV_SPI_FREQ_8M,
        .mode = NRF_DRV_SPI_MODE_0,
        .bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST,
        .sck_pin = SD_SCK,
        .mosi_pin = SD_MOSI,
        .miso_pin = SD_MISO
    };
    err_code = nrf_drv_spi_init(spi_master_instance, &spi_config, NULL);
#ifdef SD_DRV_DEBUG
    SEGGER_RTT_printf(0,"MISO value is %x & %x & %x & %x\n", nrf_gpio_pin_read(8), err_code, count, miso);
#endif	//SD_DRV_DEBUG
    ++count;
    return err_code;
    
}

/* Initialize MMC interface */
static void init_spi (void) 
{
	/* Init SPI */
	nrf_gpio_cfg_output(FATFS_CS_PIN);
	nrf_gpio_pin_set(FATFS_CS_PIN);
	if(count == 0)
	{
		miso = 1;
	}
	if(count >= 1)
	{
		miso = SD_MISO;
		nrf_drv_spi_uninit(&m_spi_master);
	}

	spi_master_init(&m_spi_master);
	
	/* Set CS high */
	nrf_gpio_pin_set(FATFS_CS_PIN);
	
	/* Wait for stable */
//	nrf_delay_ms(1);

}

uint8_t spi_transfer(uint8_t data)
{
    uint8_t spi_txbuf[2];
    uint8_t spi_rxbuf[2];

    spi_txbuf[0]=data;
#ifdef SD_DRV_DEBUG
     SEGGER_RTT_printf(0, "Outgoing data is %x\t", spi_txbuf[0]);
#endif	//SD_DRV_DEBUG
    nrf_drv_spi_transfer(&m_spi_master, spi_txbuf, 1, spi_rxbuf, 1);
#ifdef SD_DRV_DEBUG
    SEGGER_RTT_printf(0, "Incoming data is %x\n", spi_rxbuf[0]);
#endif	//SD_DRV_DEBUG
    return spi_rxbuf[0];
}

void sd_spi_init(void)
{
	init_spi();
}

void sd_spi_uninit(void)
{
	nrf_drv_spi_uninit(&m_spi_master);
}

/* Receive multiple byte */
static void rcvr_mmc (
	BYTE *buff,		/* Pointer to data buffer */
	UINT btr		/* Number of bytes to receive (even number) */
)
{
	/* Read multiple bytes, send 0xFF as dummy */
	for (int i = 0; i < btr; i++) 
	{
	    buff[i] = spi_transfer(0xFF);
	}
}


/* Send multiple byte */
static void xmit_mmc (
	const BYTE *buff,	/* Pointer to the data */
	UINT btx			/* Number of bytes to send (even number) */
)
{
	/* Write multiple bytes */
	int i;
	for (i = 0; i < btx; i++) 
	{
		spi_transfer(buff[i]);
	}
}


/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

static int wait_ready ( void )
{
	BYTE d;

	for(int i = 5000; i; i--)
	{
		rcvr_mmc(&d, 1);
		if(d == 0xFF)	break;
//		nrf_delay_us(10);
	}

	return (d == 0xFF) ? 1 : 0;
}



/*-----------------------------------------------------------------------*/
/* Deselect card and release SPI                                         */
/*-----------------------------------------------------------------------*/

static void sd_deselect (void)
{
	BYTE d;

	nrf_gpio_pin_set(FATFS_CS_PIN);		/* Set CS# high */
	rcvr_mmc(&d, 1);	/* Dummy clock (force DO hi-z for multiple slave SPI) */
	nrf_gpio_cfg_input(miso, NRF_GPIO_PIN_NOPULL);
}



/*-----------------------------------------------------------------------*/
/* Select card and wait for ready                                        */
/*-----------------------------------------------------------------------*/

static int sd_select (void)	/* 1:OK, 0:Timeout */
{
	BYTE d;
	nrf_gpio_pin_clear(FATFS_CS_PIN);				/* Set CS# low */
	rcvr_mmc(&d, 1);	/* Dummy clock (force DO enabled) */
	if (wait_ready()) return 1;	/* Wait for card ready */

	sd_deselect();
	return 0;			/* Failed */
}



/*-----------------------------------------------------------------------*/
/* Receive a data packet from the MMC                                    */
/*-----------------------------------------------------------------------*/

static int rcvr_datablock (	/* 1:OK, 0:Error */
	BYTE *buff,			/* Data buffer */
	UINT btr			/* Data block length (byte) */
)
{
	BYTE d[2];
	UINT tmr;


	for (tmr = 100; tmr; tmr--) {	/* Wait for data packet in timeout of 100ms */
		rcvr_mmc(d, 1);
		if (d[0] != 0xFF) break;
//		nrf_delay_us(10);
	}
	if (d[0] != 0xFE) return 0;		/* If not valid data token, return with error */

	rcvr_mmc(buff, btr);			/* Receive the data block into buffer */
	rcvr_mmc(d, 2);					/* Discard CRC */

	return 1;						/* Return with success */
}



/*-----------------------------------------------------------------------*/
/* Send a data packet to the MMC                                         */
/*-----------------------------------------------------------------------*/

static int xmit_datablock (	/* 1:OK, 0:Failed */
	const BYTE *buff,	/* Ponter to 512 byte data to be sent */
	BYTE token			/* Token */
)
{
	BYTE d[2];


	if (!wait_ready()) return 0;

	d[0] = token;
	xmit_mmc(d, 1);				/* Xmit a token */
	if (token != 0xFD) {		/* Is it data token? */
		xmit_mmc(buff, 512);	/* Xmit the 512 byte data block to MMC */
		rcvr_mmc(d, 2);			/* Xmit dummy CRC (0xFF,0xFF) */
		rcvr_mmc(d, 1);			/* Receive data response */
		if ((d[0] & 0x1F) != 0x05)	/* If not accepted, return with error */
			return 0;
	}

	return 1;
}


/*-----------------------------------------------------------------------*/
/* Send a command packet to the MMC                                      */
/*-----------------------------------------------------------------------*/

static BYTE send_cmd (		/* Return value: R1 resp (bit7==1:Failed to send) */
	BYTE cmd,		/* Command index */
	DWORD arg		/* Argument */
)
{
	BYTE n, d, buf[6];


	if (cmd & 0x80) {	/* ACMD<n> is the command sequense of CMD55-CMD<n> */
		cmd &= 0x7F;
		n = send_cmd(CMD55, 0);
		if (n > 1) return n;
	}

	/* Select the card and wait for ready except to stop multiple block read */
	if (cmd != CMD12) {
		sd_deselect();
		if (!sd_select()) return 0xFF;
	}

	/* Send a command packet */
	buf[0] = 0x40 | cmd;			/* Start + Command index */
	buf[1] = (BYTE)(arg >> 24);		/* Argument[31..24] */
	buf[2] = (BYTE)(arg >> 16);		/* Argument[23..16] */
	buf[3] = (BYTE)(arg >> 8);		/* Argument[15..8] */
	buf[4] = (BYTE)arg;				/* Argument[7..0] */
	n = 0x01;						/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;		/* (valid CRC for CMD0(0)) */
	if (cmd == CMD8) n = 0x87;		/* (valid CRC for CMD8(0x1AA)) */
	buf[5] = n;
	xmit_mmc(buf, 6);

	/* Receive command response */
	if (cmd == CMD12) rcvr_mmc(&d, 1);	/* Skip a stuff byte when stop reading */
	n = 10;								/* Wait for a valid response in timeout of 10 attempts */
	do
		rcvr_mmc(&d, 1);
	while ((d & 0x80) && --n);

	return d;			/* Return with the response value */
}


/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE drv			/* Drive number (always 0) */
)
{
	if (drv) return STA_NOINIT;

	return Stat;
}


/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE drv		/* Physical drive nmuber (0) */
)
{
	BYTE n, ty, cmd, buf[4];
	UINT tmr;
	DSTATUS s;


	if (drv) return RES_NOTRDY;

//	nrf_delay_ms(1);		/* 10ms */
	init_spi();				/* Init SPI */

	/* Add code to detect SD Using detect pin here */

	for (n = 10; n; n--) rcvr_mmc(buf, 1);	/* Apply 80 dummy clocks and the card gets ready to receive command */

	ty = 0;
	if (send_cmd(CMD0, 0) == 1) {			/* Enter Idle state */
		if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDv2? */
			rcvr_mmc(buf, 4);							/* Get trailing return value of R7 resp */
			if (buf[2] == 0x01 && buf[3] == 0xAA) {		/* The card can work at vdd range of 2.7-3.6V */
				for (tmr = 1000; tmr; tmr--) {			/* Wait for leaving idle state (ACMD41 with HCS bit) */
					if (send_cmd(ACMD41, 1UL << 30) == 0) break;
//					nrf_delay_us(10);
				}
				if (tmr && send_cmd(CMD58, 0) == 0) {	/* Check CCS bit in the OCR */
					rcvr_mmc(buf, 4);
					ty = (buf[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* SDv2 */
				}
			}
		} else {							/* SDv1 or MMCv3 */
			if (send_cmd(ACMD41, 0) <= 1) 	{
				ty = CT_SD1; cmd = ACMD41;	/* SDv1 */
			} else {
				ty = CT_MMC; cmd = CMD1;	/* MMCv3 */
			}
			for (tmr = 1000; tmr; tmr--) {			/* Wait for leaving idle state */
				if (send_cmd(cmd, 0) == 0) break;
//				nrf_delay_us(10);
			}
			if (!tmr || send_cmd(CMD16, 512) != 0)	/* Set R/W block length to 512 */
				ty = 0;
		}
	}
	CardType = ty;
	s = ty ? 0 : STA_NOINIT;
	Stat = s;

	sd_deselect();

	return s;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE drv,			/* Physical drive nmuber (0) */
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	DWORD sector,		/* Start sector number (LBA) */
	UINT count			/* Sector count (1..128) */
)
{
	BYTE cmd;


	if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert LBA to byte address if needed */

	cmd = count > 1 ? CMD18 : CMD17;			/*  READ_MULTIPLE_BLOCK : READ_SINGLE_BLOCK */
	if (send_cmd(cmd, sector) == 0) {
		do {
			if (!rcvr_datablock(buff, 512)) break;
			buff += 512;
		} while (--count);
		if (cmd == CMD18) send_cmd(CMD12, 0);	/* STOP_TRANSMISSION */
	}
	sd_deselect();

	return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0) */
	const BYTE *buff,	/* Pointer to the data to be written */
	DWORD sector,		/* Start sector number (LBA) */
	UINT count			/* Sector count (1..128) */
)
{
	if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert LBA to byte address if needed */

	if (count == 1) {	/* Single block write */
		if ((send_cmd(CMD24, sector) == 0)	/* WRITE_BLOCK */
			&& xmit_datablock(buff, 0xFE))
			count = 0;
	}
	else {				/* Multiple block write */
		if (CardType & CT_SDC) send_cmd(ACMD23, count);
		if (send_cmd(CMD25, sector) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if (!xmit_datablock(buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!xmit_datablock(0, 0xFD))	/* STOP_TRAN token */
				count = 1;
		}
	}
	sd_deselect();

	return count ? RES_ERROR : RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	BYTE n, csd[16];
	DWORD cs;


	if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;	/* Check if card is in the socket */

	res = RES_ERROR;
	switch (ctrl) {
		case CTRL_SYNC :		/* Make sure that no pending write process */
			if (sd_select()) res = RES_OK;
			break;

		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
			if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
				if ((csd[0] >> 6) == 1) {	/* SDC ver 2.00 */
					cs = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
					*(DWORD*)buff = cs << 10;
				} else {					/* SDC ver 1.XX or MMC */
					n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
					cs = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
					*(DWORD*)buff = cs << (n - 9);
				}
				res = RES_OK;
			}
			break;

		case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
			*(DWORD*)buff = 128;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
	}

	sd_deselect();

	return res;
}



