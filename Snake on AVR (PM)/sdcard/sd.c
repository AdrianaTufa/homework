#include <avr/io.h>

#include "sd.h"

#define SPCR0   _SFR_IO8(0x2C)
#define SPR00   0
#define SPR10   1
#define CPHA0   2
#define CPOL0   3
#define MSTR0   4
#define DORD0   5
#define SPE0    6
#define SPIE0   7

#define SPSR0   _SFR_IO8(0x2D)
#define SPI2X0  0
#define WCOL0   6
#define SPIF0   7

#define SPDR0   _SFR_IO8(0x2E)

/*************************************************************************\
 * SPI interace                                                          *
\*************************************************************************/

void SPI_init(void)
{
	// Set spi direction: MOSI and SCK as output, MISO as input
	SPI_DDR |= (1 << SPI_MOSI) | (1 << SPI_SCK);
	SPI_DDR &= ~(1 << SPI_MISO);
	// CS pin as output
	SD_DDR |= (1 << SD_CS);
    
	// Initialize SPI port, SPI enabled as master
	SPCR0 = (1 << SPE0) | (1 << MSTR0);
	// Set SPI at double speed
	SPSR0 |= (1 << SPI2X0);
}

BYTE SPI_receive(void)
{
	SPDR0 = 0xFF;
	while(!(SPSR0 & (1 << SPIF0)));
	return SPDR0;
}

void SPI_transmit(BYTE data)
{
	SPDR0 = data;
	while(!(SPSR0 & (1 << SPIF0)));
	(void)SPDR0;
}

static
void SPI_release(void)
{
	SD_DESELECT();
	SPI_receive();
}

/*************************************************************************\
 * Functii private modulului                                             *
\*************************************************************************/

/* Trimite o comanda catre SD
 * 
 * @param cmd - commanda
 * @param arg - argument 
 */
static
BYTE send_command(BYTE cmd, DWORD arg)
{
	BYTE n, res;

	if (cmd & 0x80) {	/* ACMD<n> is the command sequense of CMD55-CMD<n> */
		cmd &= 0x7F;
		res = send_command(CMD55, 0);
		if (res > 1) return res;
	}

	/* Select the card */
	SD_DESELECT();
	SPI_receive();
	SD_SELECT();
	SPI_receive();

	/* Send a command packet */
	SPI_transmit(cmd);				/* Start + Command index */
	SPI_transmit((BYTE)(arg >> 24));		/* Argument[31..24] */
	SPI_transmit((BYTE)(arg >> 16));		/* Argument[23..16] */
	SPI_transmit((BYTE)(arg >> 8));			/* Argument[15..8] */
	SPI_transmit((BYTE)arg);			/* Argument[7..0] */
	n = 0x01;					/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	SPI_transmit(n);

	/* Receive a command response */
	n = 10;						/* Wait for a valid response in timeout of 10 attempts */
	do {
		res = SPI_receive();
	} while ((res & 0x80) && --n);

	return res;					/* Return with the response value */
}

/*************************************************************************\
 * Functii publice	                                                 *
\*************************************************************************/

/* Initializeaza disk-ul */
static BYTE CardType;
DSTATUS disk_initialize(void)
{
	BYTE n, cmd, ty, ocr[4];
	WORD tmr;

	SD_DESELECT();	 
	
	SPI_DELAY();

	SPI_init();

	if(SD_SELECTED()) 
		disk_writep(0, 0);		/* Finalize write process if it is in progress */
	for(n = 100; n; --n) 
		SPI_receive();			/* Dummy clocks */

	ty = 0;
	if(send_command(CMD0, 0) == 1) 
	{			/* Enter Idle state */
		if(send_command(CMD8, 0x1AA) == 1) 
		{	/* SDv2 */
			for(n = 0; n < 4; ++n) 
				ocr[n] = SPI_receive();		/* Get trailing return value of R7 resp */
			
			if(ocr[2] == 0x01 && ocr[3] == 0xAA) 
			{				/* The card can work at vdd range of 2.7-3.6V */
				for(tmr = 12000; tmr && send_command(ACMD41, 1UL << 30); tmr--);	/* Wait for leaving idle state (ACMD41 with HCS bit) */
				
				if(tmr && send_command(CMD58, 0) == 0) 
				{		/* Check CCS bit in the OCR */
					for(n = 0; n < 4; n++) 
						ocr[n] = SPI_receive();
					
					ty = (ocr[0] & 0x40)? CT_SD2 | CT_BLOCK: CT_SD2;	/* SDv2 (HC or SC) */
				}
			}
		} else {							/* SDv1 or MMCv3 */
			if(send_command(ACMD41, 0) <= 1) 	
			{
				ty = CT_SD1; cmd = ACMD41;	/* SDv1 */
			} else {
				ty = CT_MMC; cmd = CMD1;	/* MMCv3 */
			}

			for(tmr = 25000; tmr && send_command(cmd, 0); tmr--);	/* Wait for leaving idle state */

			if(!tmr || send_command(CMD16, 512) != 0)			/* Set R/W block length to 512 */
				ty = 0;
		}
	}

	CardType = ty;
	SPI_release();

	return ty? 0: STA_NOINIT;
}

/* Citeste sector partial
 *
 * @param buf - pointer catre buffer-ul de citire 
 * 	(NULL => octetii cititi sunt trimisi catre stream)
 * @param lba - numarul de sector (LBA)
 * @param offset - offset in octeti (0..511)
 * @param cnt - numarul de octeti de citit (offset + cnt <= 512)
 */

DRESULT disk_readp(BYTE *buf, DWORD lba, WORD offset, WORD cnt)
{
	DRESULT res;
	BYTE rc;
	WORD bc;

	if(!(CardType & CT_BLOCK)) lba *= 512;		/* Convert to byte address if needed */

	res = RES_ERROR;
	if(send_command(CMD17, lba) == 0) 
	{		/* READ_SINGLE_BLOCK */
		bc = 30000;
		do 
		{							/* Wait for data packet in timeout of 100ms */
			rc = SPI_receive();
		} while(rc == 0xFF && --bc);

		if(rc == 0xFE) 
		{				/* A data packet arrived */
			bc = 514 - offset - cnt;

			/* Skip leading bytes */
			if(offset) 
			{
				do 
					SPI_receive(); 
				while(--offset);
			}

			/* Receive a part of the sector */
			if(buf) 
			{	/* Store data to the memory */
				do
					*buf++ = SPI_receive();
				while(--cnt);
			} 
			
			/* Skip trailing bytes and CRC */
			do 
				SPI_receive(); 
			while(--bc);

			res = RES_OK;
		}
	}

	SPI_release();
	return res;
}

/* Scrie un sector partial
 *
 * @param buf - pointer catre buffer-ul de scriere
 * 	(NULL => initializeaza/finalizeaza scrierea de sector)
 * @param sa - numarul de octeti de trimis, numarul de sector (LBA) sau 0
 */
DRESULT disk_writep(const BYTE *buf, DWORD sa)
{
	DRESULT res;
	WORD bc;
	static WORD wc;

	res = RES_ERROR;

	if(buf) 
	{		/* Send data bytes */
		bc = (WORD)sa;
		while(bc && wc) 
		{		/* Send data bytes to the card */
			SPI_transmit(*buf++);
			wc--; 
			bc--;
		}
		res = RES_OK;
	} else {
		if(sa) 
		{	/* Initiate sector write process */
			if(!(CardType & CT_BLOCK)) sa *= 512;	/* Convert to byte address if needed */
			if(send_command(CMD24, sa) == 0) 
			{			/* WRITE_SINGLE_BLOCK */
				SPI_transmit(0xFF); SPI_transmit(0xFE);		/* Data block header */
				wc = 512;							/* Set byte counter */
				res = RES_OK;
			}
		} else {	/* Finalize sector write process */
			bc = wc + 2;
			while(bc--) 
				SPI_transmit(0);	/* Fill left bytes and CRC with zeros */
			if((SPI_receive() & 0x1F) == 0x05) 
			{	/* Receive data resp and wait for end of write process in timeout of 300ms */
				for(bc = 65000; SPI_receive() != 0xFF && bc; bc--);	/* Wait ready */

				if(bc) 
					res = RES_OK;
			}
			SPI_release();
		}
	}

	return res;
}
