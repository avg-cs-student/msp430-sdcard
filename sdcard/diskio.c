/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/




// Includes ------------------------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <msp430fr5994.h>
#include "./diskio.h"		/* FatFs lower layer API */
#include "./sd_msp430fr5994_launchpad.h"  /* defines for working with launchpad */



// Defines -------------------------------------------------------------------------------------------

// Define Functions
#define DLY_US(n)       __delay_cycles(n * (MCLK_FREQUENCY / 1000000))   //Delay n microseconds

// Definitions for MMC/SDC command 
#define CMD0    (0x40+0)    	// GO_IDLE_STATE
#define CMD1    (0x40+1)    	// SEND_OP_COND
#define CMD8    (0x40+8)    	// SEND_IF_COND
#define CMD9    (0x40+9)    	// SEND_CSD
#define CMD10    (0x40+10)    	// SEND_CID
#define CMD12    (0x40+12)    	// STOP_TRANSMISSION
#define CMD16    (0x40+16)    	// SET_BLOCKLEN
#define CMD17    (0x40+17)    	// READ_SINGLE_BLOCK
#define CMD18    (0x40+18)    	// READ_MULTIPLE_BLOCK
#define CMD23    (0x40+23)    	// SET_BLOCK_COUNT
#define CMD24    (0x40+24)    	// WRITE_BLOCK
#define CMD25    (0x40+25)    	// WRITE_MULTIPLE_BLOCK
#define CMD41    (0x40+41)    	// SEND_OP_COND (ACMD)
#define CMD55    (0x40+55)    	// APP_CMD
#define CMD58    (0x40+58)    	// READ_OCR

// Peripheral definitions for DK-TM4C123G board

// Individual pins from MSP430 connected to the SD Card are set up in the SPIpins.h folder (Platform dependent)

// "Private" Functions ------------------------------------------------------------------------------

// Asserts the CS pin to the card (Platform dependent)
static void SELECT (void)
{
	CS_OUT_p &= ~(CS_b);
}


// De-asserts (set high) the CS pin to the card (Platform dependent)
static void DESELECT (void)
{
	CS_OUT_p |= CS_b;
}


static volatile DSTATUS Stat = STA_NOINIT;    	// Disk status
static volatile BYTE Timer1, Timer2;    	// 100Hz decrement timer
static BYTE CardType;            		// b0:MMC, b1:SDC, b2:Block addressing
static BYTE PowerFlag = 0;     			// Indicates if "power" is on


// Transmit a byte to MMC via SPI  (Platform dependent)                 
static void __attribute__((section(".upper.text"))) xmit_spi(BYTE dat){
	uint16_t gie = __get_SR_register() & GIE;	// Save interrupt state
	__disable_interrupt();				// Disable interrupts

	while(!(UCxxIFG & UCTXIFG));			// Wait for TX ready
	UCxxTXBUF = dat;				// Write byte
	while(UCxxSTATW & UCBUSY);

	UCxxRXBUF;					// Read to empty RX buffer, clear any ovverrun

	__bis_SR_register(gie);				// Reload interrupt state
}


// Receive a byte from MMC via SPI  (Platform dependent)                
static BYTE __attribute__((section(".upper.text"))) rcvr_spi (void){
	uint8_t ui8RcvDat;				// Receive variable

	uint16_t gie = __get_SR_register() & GIE;	// Save interrupt state
	__disable_interrupt();

	UCxxIFG &= ~UCRXIFG;				// Ensure RXIFG clear
	while(!(UCxxIFG & UCTXIFG));			// Wait for TX ready
	UCxxTXBUF = 0xFF;				// Send dummy byte
	while(!(UCxxIFG & UCRXIFG));			// Wait for RX buffer

	ui8RcvDat = UCxxRXBUF;				// Read RX buffer

	return (BYTE)ui8RcvDat;
}


static void __attribute__((section(".upper.text"))) rcvr_spi_m (BYTE *dst){
	*dst = rcvr_spi();
}


// Wait for card ready 
static BYTE __attribute__((section(".upper.text"))) wait_ready (void){
	BYTE res;

	Timer2 = 50;    				/* Wait for ready in timeout of 500ms */
	rcvr_spi();
	do
		res = rcvr_spi();
	while ((res != 0xFF) && Timer2);

	return res;
} // TODO: Enable timeout?


// Send 80 or so clock transitions with CS and DI held high. This is required after card power up to get it into SPI mode
static void __attribute__((section(".upper.text"))) send_initial_clock_train(void){
	unsigned int i;
	// uint8_t ui8RcvDat;				// Receive variable

	// Ensure CS is held high.
	DESELECT();

	for(i = 0; i < 10; i++){
		xmit_spi(0xFF);
	}

} // TODO: Switch from xmit_spi() to manual?


// Power Control  (Platform dependent)
// When the target system does not support socket power control, there is nothing to do in these functions and chk_power always returns 1.  
static void __attribute__((section(".upper.text"))) power_on (void){
	/*
	* This doesn't really turn the power on, but initializes the
	* SSI port and pins needed to talk to the card.
	*/

	//Port initialization for SD Card operation
	SCLK_SEL0_p &= ~(SCLK_b);
	SCLK_SEL1_p |= SCLK_b;
	MISO_SEL0_p &= ~(MISO_b);
	MISO_SEL1_p |= MISO_b;
	MOSI_SEL0_p &= ~(MOSI_b);
	MOSI_SEL1_p |= MOSI_b;
	SCLK_DIR_p |= SCLK_b;
	MOSI_DIR_p |= MOSI_b;

	CS_SEL0_p &= ~(CS_b);
	CS_SEL1_p &= ~(CS_b);
	CS_OUT_p |= CS_b;
	CS_DIR_p |= CS_b;

	MISO_REN_p |= MISO_b;
	MOSI_REN_p |= MOSI_b;
	MISO_OUT_p |= MISO_b;
	MOSI_OUT_p |= MOSI_b;

	//Initialize USCI_A1 for SPI Master operation
	UCxxCTLW0 = UCSWRST;                                    //Put state machine in reset
	UCxxCTLW0 |= UCCKPL | UCMSB | UCMST | UCMODE_0 | UCSYNC;  //3-pin, 8-bit SPI master
	// //Clock polarity select - The inactive state is high
	// //MSB first

	// //this is a hack to fix the SMCLK to be 1MHZ   //left in from Amulet code incase it might be useful later...
	// CSCTL3 &= ~DIVS_3;
	// CSCTL3 |=  DIVS_3;
	// //end hack 

	//SMCLK is running at 16MHz on the bracelet!
	//f_UCxCLK = 16MHz / UCxxBRO = 16MHz / 64 = 250kHz

	UCxxCTLW0 |= UCSSEL_2;                          //Use SMCLK, keep RESET
	UCxxBR0 = 64;                                   //Initial SPI clock must be <400kHz  //Note: did end up working with this value being 24, but clock would be running at 500kHz?
	UCxxBR1 = 0;                                            
	UCxxCTLW0 &= ~UCSWRST;                          //Release USCI state machine
	UCxxIFG &= ~UCRXIFG;

	// Set DI and CS high and apply more than 74 pulses to SCLK for the card
	// to be able to accept a native command.
	send_initial_clock_train();

	PowerFlag = 1;
}


// Set the SSI speed to the max setting
static void __attribute__((section(".upper.text"))) set_max_speed(void)
{
    UCxxCTLW0 |= UCSWRST;                                    //Put state machine in reset
    UCxxBR0 = 1;                                             //f_UCxCLK = 16MHz/64 = 250kHz
    UCxxBR1 = 0;
    UCxxCTLW0 &= ~UCSWRST;                                   //Release USCI state machine
}// TODO: Check speeds

static void __attribute__((section(".upper.text"))) power_off (void)
{
	PowerFlag = 0;
}

static int __attribute__((section(".upper.text"))) chk_power(void)
{
	/* Socket power state: 0=off, 1=on */
	return PowerFlag;
}


/* Receive a data packet from MMC */
static BOOL __attribute__((section(".upper.text"))) rcvr_datablock (
    BYTE *buff,            		/* Data buffer to store received data */
    UINT btr            		/* Byte count (must be even number) */
){
	BYTE token;

	Timer1 = 100;
	do {                            	/* Wait for data packet in timeout of 100ms */
		token = rcvr_spi();
	} while ((token == 0xFF) && Timer1);

	if(token != 0xFE) return FALSE;    	/* If not valid data token, retutn with error */

	do {                            	/* Receive the data block into buffer */
		rcvr_spi_m(buff++);
		rcvr_spi_m(buff++);
	} while (btr -= 2);
	rcvr_spi();                        	/* Discard CRC */
	rcvr_spi();

	return TRUE;                    	/* Return with success */
} // TODO: Timer not implemented


/* Send a data packet to MMC */
#if _READONLY == 0
static BOOL xmit_datablock (
    const BYTE *buff,    		/* 512 byte data block to be transmitted */
    BYTE token            		/* Data/Stop token */
){
	BYTE resp, wc;


	if (wait_ready() != 0xFF) return FALSE;

	xmit_spi(token);                    /* Xmit data token */
	if (token != 0xFD) {    		/* Is data token */
		wc = 0;
		do {                            /* Xmit the 512 byte data block to MMC */
		    xmit_spi(*buff++);
		    xmit_spi(*buff++);
		} while (--wc);

		xmit_spi(0xFF);                 /* CRC (Dummy) */
		xmit_spi(0xFF);
		resp = rcvr_spi();              /* Reveive data response */
		if ((resp & 0x1F) != 0x05)      /* If not accepted, return with error */
		    return FALSE;
	}

	return TRUE;
}
#endif /* _READONLY */


/* Send a command packet to MMC */
static BYTE __attribute__((section(".upper.text"))) send_cmd (
    BYTE cmd,        			/* Command byte */
    DWORD arg        			/* Argument */
){
	BYTE n, res;


	if (wait_ready() != 0xFF) return 0xFF;

	/* Send command packet */
	xmit_spi(cmd);                     	/* Command */
	xmit_spi((BYTE)(arg >> 24));        /* Argument[31..24] */
	xmit_spi((BYTE)(arg >> 16));        /* Argument[23..16] */
	xmit_spi((BYTE)(arg >> 8));         /* Argument[15..8] */
	xmit_spi((BYTE)arg);                /* Argument[7..0] */
	n = 0xff;
	if (cmd == CMD0) n = 0x95;          /* CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;          /* CRC for CMD8(0x1AA) */
	xmit_spi(n);

	/* Receive command response */
	if (cmd == CMD12) rcvr_spi();       /* Skip a stuff byte when stop reading */
	n = 10;                             /* Wait for a valid response in timeout of 10 attempts */
	do
		res = rcvr_spi();
	while ((res & 0x80) && --n);

	return res;            		/* Return with the response value */
}


/*-----------------------------------------------------------------------*
 * Send the special command used to terminate a multi-sector read.
 *
 * This is the only command which can be sent while the SDCard is sending
 * data. The SDCard spec indicates that the data transfer will stop 2 bytes
 * after the 6 byte CMD12 command is sent and that the card will then send
 * 0xFF for between 2 and 6 more bytes before the R1 response byte.  This
 * response will be followed by another 0xFF byte.  In testing, however, it
 * seems that some cards don't send the 2 to 6 0xFF bytes between the end of
 * data transmission and the response code.  This function, therefore, merely
 * reads 10 bytes and, if the last one read is 0xFF, returns the value of the
 * latest non-0xFF byte as the response code.
 *
 *-----------------------------------------------------------------------*/
static BYTE __attribute__((section(".upper.text"))) send_cmd12 (void){
	BYTE n, res, val;

	/* For CMD12, we don't wait for the card to be idle before we send
	* the new command.
	*/

	/* Send command packet - the argument for CMD12 is ignored. */
	xmit_spi(CMD12);
	xmit_spi(0);
	xmit_spi(0);
	xmit_spi(0);
	xmit_spi(0);
	xmit_spi(0);

	/* Read up to 10 bytes from the card, remembering the value read if it's
	not 0xFF */
	for(n = 0; n < 10; n++){
		val = rcvr_spi();
		if(val != 0xFF){
		    res = val;
		}
	}

	return res;            /* Return with the response value */
}




// "Public" Functions -------------------------------------------------------------------------------



/* Initialize Disk Drive */
// TODO: Check Timer function
DSTATUS __attribute__((section(".upper.text"))) disk_initialize (
    BYTE drv        				/* Physical drive nmuber (0) */
){
	BYTE n, ty, ocr[4];


	if (drv) return STA_NOINIT;            	/* Supports only single drive */
	if (Stat & STA_NODISK) return Stat;    	/* No card in the socket */

	power_on();                            	/* Force socket power on */
	send_initial_clock_train();            	/* Ensure the card is in SPI mode */

	SELECT();                			/* CS = L */
	ty = 0;
	if (send_cmd(CMD0, 0) == 1) {            	/* Enter Idle state */
		Timer1 = 100;                        	/* Initialization timeout of 1000 msec */
		if (send_cmd(CMD8, 0x1AA) == 1) {    	/* SDC Ver2+ */
		    for (n = 0; n < 4; n++) ocr[n] = rcvr_spi();
		    if (ocr[2] == 0x01 && ocr[3] == 0xAA) {    		/* The card can work at vdd range of 2.7-3.6V */
			do {
			    if (send_cmd(CMD55, 0) <= 1 && send_cmd(CMD41, 1UL << 30) == 0)    break;    /* ACMD41 with HCS bit */
			} while (Timer1);
			if (Timer1 && send_cmd(CMD58, 0) == 0) {   	/* Check CCS bit */
			    for (n = 0; n < 4; n++) ocr[n] = rcvr_spi();
			    ty = (ocr[0] & 0x40) ? 6 : 2;
			}
		    }
		} else {                            			/* SDC Ver1 or MMC */
		    ty = (send_cmd(CMD55, 0) <= 1 && send_cmd(CMD41, 0) <= 1) ? 2 : 1;    	/* SDC : MMC */
		    do {
			if (ty == 2) {
			    if (send_cmd(CMD55, 0) <= 1 && send_cmd(CMD41, 0) == 0) break;    	/* ACMD41 */
			} else {
			    if (send_cmd(CMD1, 0) == 0) break;                                	/* CMD1 */
			}
		    } while (Timer1);
		    if (!Timer1 || send_cmd(CMD16, 512) != 0)   			 	/* Select R/W block length */
			ty = 0;
		}
	}
	CardType = ty;
	DESELECT();            			/* CS = H */
	rcvr_spi();           			/* Idle (Release DO) */

	if (ty) {           		 	/* Initialization succeded */
		Stat &= ~STA_NOINIT;        		/* Clear STA_NOINIT */
		set_max_speed();
	} else {            			/* Initialization failed */
		power_off();
	}

	return Stat;
}


/* Get Disk Status */
DSTATUS __attribute__((section(".upper.text"))) disk_status (
    BYTE drv        			/* Physical drive nmuber (0) */
){
	if (drv) return STA_NOINIT;        	/* Supports only single drive */
	return Stat;
}



/* Read Sector(s)  */
DRESULT __attribute__((section(".upper.text"))) disk_read (
    BYTE drv,            		/* Physical drive nmuber (0) */
    BYTE *buff,            		/* Pointer to the data buffer to store read data */
    DWORD sector,       	  	/* Start sector number (LBA) */
    UINT count            		/* Sector count (1..255) */
){
	if (drv || !count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;

	if (!(CardType & 4)) sector *= 512;    	/* Convert to byte address if needed */

	SELECT();            		   	/* CS = L */

	if (count == 1) {    		   	/* Single block read */
	if ((send_cmd(CMD17, sector) == 0) 	/* READ_SINGLE_BLOCK */
	    && rcvr_datablock(buff, 512))
	    count = 0;
	}
	else {               			/* Multiple block read */
	if (send_cmd(CMD18, sector) == 0) {    	/* READ_MULTIPLE_BLOCK */
	    do {
		if (!rcvr_datablock(buff, 512)) break;
		buff += 512;
	    } while (--count);
	    send_cmd12();                	/* STOP_TRANSMISSION */
	}
	}

	DESELECT();            			/* CS = H */
	rcvr_spi();            			/* Idle (Release DO) */

	return count ? RES_ERROR : RES_OK;
}



/* Write Sector(s) */
#if _READONLY == 0
DRESULT __attribute__((section(".upper.text"))) disk_write (
    BYTE drv,            			/* Physical drive nmuber (0) */
    const BYTE *buff,    			/* Pointer to the data to be written */
    DWORD sector,       			/* Start sector number (LBA) */
    UINT count           			/* Sector count (1..255) */
){
	if (drv || !count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;
	if (Stat & STA_PROTECT) return RES_WRPRT;

	if (!(CardType & 4)) sector *= 512;    	/* Convert to byte address if needed */

	SELECT();           		 	/* CS = L */

	if (count == 1) {    			/* Single block write */
		if ((send_cmd(CMD24, sector) == 0)    	/* WRITE_BLOCK */
		    && xmit_datablock(buff, 0xFE))
		    count = 0;
	}
	else {                			/* Multiple block write */
		if (CardType & 2) {
		    send_cmd(CMD55, 0); send_cmd(CMD23, count);    /* ACMD23 */
		}
		if (send_cmd(CMD25, sector) == 0) {    	/* WRITE_MULTIPLE_BLOCK */
		    do {
			if (!xmit_datablock(buff, 0xFC)) break;
			buff += 512;
		    } while (--count);
		    if (!xmit_datablock(0, 0xFD))    	/* STOP_TRAN token */
			count = 1;
		}
	}

	DESELECT();            			/* CS = H */
	rcvr_spi();            			/* Idle (Release DO) */

	return count ? RES_ERROR : RES_OK;
}
#endif /* _READONLY */



/* Disk IO Control */
DRESULT __attribute__((section(".upper.text"))) disk_ioctl (
    BYTE drv,        				/* Physical drive nmuber (0) */
    BYTE ctrl,        				/* Control code */
    void *buff        				/* Buffer to send/receive control data */  //was void instead of Byte
){
	DRESULT res;
	BYTE n, csd[16], *ptr = (BYTE*) buff;
	WORD csize;


	if (drv) return RES_PARERR;

	res = RES_ERROR;

	if (ctrl == CTRL_POWER) {
		switch (*ptr) {
		case 0:        				/* Sub control code == 0 (POWER_OFF) */
		    if (chk_power())
			power_off();        		/* Power off */
		    res = RES_OK;
		    break;
		case 1:        				/* Sub control code == 1 (POWER_ON) */
		    power_on();                		/* Power on */
		    res = RES_OK;
		    break;
		case 2:        				/* Sub control code == 2 (POWER_GET) */
		    *(ptr+1) = (BYTE)chk_power();
		    res = RES_OK;
		    break;
		default :
		    res = RES_PARERR;
		}
	}
	else {
		if (Stat & STA_NOINIT) return RES_NOTRDY;

		SELECT();        			/* CS = L */

		switch (ctrl) {
			case GET_SECTOR_COUNT :    		/* Get number of sectors on the disk (DWORD) */
			    if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
				if ((csd[0] >> 6) == 1) {    	/* SDC ver 2.00 */
				    csize = csd[9] + ((WORD)csd[8] << 8) + 1;
				    *(DWORD*)buff = (DWORD)csize << 10;
				} else {                    	/* MMC or SDC ver 1.XX */
				    n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
				    csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
				    *(DWORD*)buff = (DWORD)csize << (n - 9);
				}
				res = RES_OK;
			    }
			    break;

			case GET_SECTOR_SIZE :    		/* Get sectors on the disk (WORD) */
			    *(WORD*)buff = 512;
			    res = RES_OK;
			    break;

			case CTRL_SYNC :    			/* Make sure that data has been written */
			    if (wait_ready() == 0xFF)
				res = RES_OK;
			    break;

			case MMC_GET_CSD :    			/* Receive CSD as a data block (16 bytes) */
			    if (send_cmd(CMD9, 0) == 0       	/* READ_CSD */
				&& rcvr_datablock(ptr, 16))
				res = RES_OK;
			    break;

			case MMC_GET_CID :    			/* Receive CID as a data block (16 bytes) */
			    if (send_cmd(CMD10, 0) == 0        	/* READ_CID */
				&& rcvr_datablock(ptr, 16))
				res = RES_OK;
			    break;

			case MMC_GET_OCR :    			/* Receive OCR as an R3 resp (4 bytes) */
			    if (send_cmd(CMD58, 0) == 0) {    	/* READ_OCR */
				for (n = 0; n < 4; n++)
				    *ptr++ = rcvr_spi();
				res = RES_OK;
			    }
				break;  //added to remove fallthrough warning on compile - NT

			//        case MMC_GET_TYPE :    /* Get card type flags (1 byte) */
			//            *ptr = CardType;
			//            res = RES_OK;
			//            break;

			default:
			    res = RES_PARERR;
		}

		DESELECT();            			/* CS = H */
		rcvr_spi();            			/* Idle (Release DO) */
	}

	return res;
}



/* Device Timer Interrupt Procedure  (Platform dependent)                */
/* This function must be called in period of 10ms                        */
// TODO: Timer function, CHECK!
void __attribute__((section(".upper.text"))) disk_timerproc (void)
{
	//    BYTE n, s;
	BYTE n;


	n = Timer1;                        /* 100Hz decrement timer */
	if (n) Timer1 = --n;
	n = Timer2;
	if (n) Timer2 = --n;
}

/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module           */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support a real time clock.          */
// TODO: Once RTC is working, change!
DWORD __attribute__((section(".upper.text"))) get_fattime (void)
{
	return    ((2007UL-1980) << 25)    // Year = 2007
	    | (6UL << 21)            // Month = June
	    | (5UL << 16)            // Day = 5
	    | (11U << 11)            // Hour = 11
	    | (38U << 5)            // Min = 38
	    | (0U >> 1)                // Sec = 0
	    ;
}
