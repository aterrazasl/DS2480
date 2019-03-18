/*
 * DS2480.h
 *
 *  Created on: Mar 17, 2019
 *      Author: aterrazas
 */

#ifndef DS2480_H_
#define DS2480_H_


#define FALSE 0
#define TRUE 1

//Parameter codes
#define PDSRC 	0x03
#define PPD 	0x05
#define SPUD	0x07
#define W1LT	0x09
#define DSOW0RT	0x0b
#define LOAD	0x0d
#define RBR		0x0f

//DS2480 ROM commands
#define RESET		0xc1
#define DATA_MODE	0xe1
#define CMD_MODE	0xe3
#define DUMMY		0xff
#define READROM		0x33
#define MATCHROM	0x55
#define SEARCHROM	0xf0
#define ALARMSEARCH	0xec
#define SKIPROM		0xcc

//DS18S20 commands
#define CONV_TEMP		0x44
#define COPY_SCRATCHPAD	0X48
#define WR_SCRATCHPAD	0x4E
#define RD_SCRATCHPAD	0xbe
#define RECALL			0xb8
#define RD_POWER		0xb4

#define SCRATCH_PAD_SIZE	9

typedef struct {
	unsigned char tempLSB;
	unsigned char tempMSB;
	unsigned char TH;
	unsigned char TL;
	unsigned char Reserved1;
	unsigned char Reserved2;
	unsigned char countRemain;
	unsigned char countPerC;
	unsigned char CRC;
} scratchPad;


int readConfig(int fd);
int readROMcmd(int fd, unsigned char * data);
int startTempConv(int fd, unsigned char * address);
double calculateTemp(scratchPad *dataPad);
int readScratchPAD(int fd, unsigned char * address, unsigned char * data);
int findDevices(int fd);


#endif /* DS2480_H_ */
