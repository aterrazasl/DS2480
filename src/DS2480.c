/*
 * DS2480.c
 *
 *  Created on: Mar 17, 2019
 *      Author: aterrazas
 */

#include "DS2480.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>


//Calculate the CRC
//based on https://github.com/collin80/DS2480B/blob/master/DS2480B.cpp
int calculateCRC(unsigned char * addr, unsigned int len)
{
	unsigned char crc = 0;

	while (len--) {
		unsigned char inbyte = *addr++;
		for (unsigned char i = 8; i; i--) {
			unsigned char mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	return crc;
}

//Writes 1 byte to the file descriptor
int writeData(int fd, unsigned char data)
{
	if(write(fd, &data,1) < 0){
		printf("error %d Writing: %s\n\n", errno, strerror (errno));
	}
//	printf("sent = 0x%02x, %s\n",data,"write complete");

	usleep(10 * 1000);
	return 0;
}

//Reads 1 byte from file descriptor
//Returns the byte read
unsigned char readData(int fd)//, unsgned char * data)
{
	unsigned char data[1];
	if(read(fd, &data,1) < 0){
		printf("error %d Reading: %s\n\n", errno, strerror (errno));
	}
//	printf("-- Read = 0x%02x, %s\n", data[0],"read complete");

	return data[0];
}

//Send 1 byte command and returns the data received
unsigned char sendCommand(int fd, unsigned char data)
{
	writeData(fd, data);
	return readData(fd);
}

//not implemented yet
int writeParameter(int fd, unsigned char param, unsigned char data){

	writeData(fd, '\x63');
	readData(fd);

	return 0;
}

//Send reset
int sendReset(int fd)
{
	sendCommand(fd, RESET);
	return 0;
}

//send read rom command and return the ROM Address
int readROMcmd(int fd, unsigned char * data)
{
	int i=0;
	unsigned char crc;
	printf("%s","---- Read ROM Address ----\n");

	sendReset(fd);
	writeData(fd,DATA_MODE);	//enter in Data mode
	sendCommand(fd,READROM);	//Read ROM command

	printf("%s","ROM data = 0x");
	for(i=0; i < 8 ;i++)
	{
		data[i] = sendCommand(fd,DUMMY);	// Send dummy byte to retreive data
		printf("%02X",data[i]);
	}
	printf("%s\n","");
	crc = calculateCRC(data,8);
//	printf("CRC = 0x%02x\n",crc);

	writeData(fd,CMD_MODE);	//comes back into command mode
	sendReset(fd);

	if(crc)
		printf("%s : failed CRC\n",__func__);

	return crc;
}

//Sets the match address and leave the bus in Data mode
int matchROMcmd(int fd, unsigned char * data)
{
	int i=0;

	sendReset(fd);
	writeData(fd,DATA_MODE);	//enter in Data mode
	sendCommand(fd,MATCHROM);	//Match ROM command

	for(i=0; i < 8 ;i++)
	{
		sendCommand(fd,data[i]);	// send rom address
	}

//	writeData(fd,CMD_MODE);	//comes back into command mode
//	sendReset(fd);

	return 0;

}

//Send read scratch command to matched addess
//return the pad data in data
int readScratchPAD(int fd, unsigned char * address, unsigned char * data){
	int i;
	unsigned char crc;
	matchROMcmd(fd, address);

	sendCommand(fd,RD_SCRATCHPAD);	//Read ROM command

//	printf("%s","Scratch pad data = 0x");
	for(i=0; i < SCRATCH_PAD_SIZE ;i++)
	{
		data[i] = sendCommand(fd,DUMMY);	// Send dummy byte to retreive data
//		printf("%02X",data[i]);
	}
//	printf("%s\n","");

	crc = calculateCRC(data,9);
//	printf("CRC = 0x%02x\n",crc);

	writeData(fd,CMD_MODE);	//comes back into command mode
	sendReset(fd);

	if(crc)
		printf("%s : failed CRC\n",__func__);

	return crc;
}

//Send the start temp conversion command and waits delay
//Returns to CMD mode and send reset pulse
int startTempConv(int fd, unsigned char * address){

	printf("%s","---- Start temperature conversion ----\n");

	matchROMcmd(fd, address);

	sendCommand(fd,CONV_TEMP);

	sleep(1);
	writeData(fd,CMD_MODE);	//comes back into command mode
	sendReset(fd);

	return 0;
}

//Prints DS2480 configuration
int readConfig(int fd)
{
	printf("%s","---- Printing DS2480 config ----\n");
	sendReset(fd);
	sendCommand(fd, PDSRC);
	sendCommand(fd, PPD);
	sendCommand(fd, SPUD);
	sendCommand(fd, W1LT);
	sendCommand(fd, DSOW0RT);
	sendCommand(fd, LOAD);
	sendCommand(fd, RBR);
	sendReset(fd);

	return 0;

}

//Calculates the temperature in scratchPad structure
//Returns the value in Celsius
double calculateTemp(scratchPad *dataPad)
{
	double temp=0.0;

	unsigned short helper=0;

	helper = (unsigned short)dataPad->tempMSB;
	helper = helper << 8;
	helper = helper | dataPad->tempLSB;

	helper = helper >> 1;

	temp = (double)helper + ( (dataPad->tempLSB & 0x0001) * 0.25 );

	temp = temp - 0.25 + ( ((double)(dataPad->countPerC - dataPad->countRemain) ) / ( (double) dataPad->countPerC) );

	return temp;
}

