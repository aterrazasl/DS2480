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


// Write a bit - actually returns the bit read back in case you care.
//
unsigned char write_bit(int fd, unsigned char v)
{
	unsigned char val;
	//commandMode();
	if (v == 1) writeData(fd, 0x91); //port.write(0x91); //write a single "on" bit to onewire
	else writeData(fd, 0x81); //port.write(0x81); //write a single "off" bit to onewire
	//if (!waitForReply()) return 0;
	val = readData(fd);
	return val & 1;
}

//
// Read a bit - short hand for writing a 1 and seeing what we get back.
//
unsigned char read_bit(int fd)
{
	unsigned char r = write_bit(fd,1);
	return r;
}

//Find the ROM addresses of 1-Wire devices
//based from https://github.com/collin80/DS2480B/blob/master/DS2480B.cpp
int findDevices(int fd)
{
	char LastDeviceFlag = 0;
	char LastDiscrepancy=0;
	char LastFamilyDiscrepancy=0;
	unsigned char id_bit_number;
	unsigned char last_zero, rom_byte_number, search_result;
	unsigned char id_bit, cmp_id_bit;
	unsigned char ROM_NO[100];

	unsigned char newAddr[100];

	unsigned char rom_byte_mask, search_direction;

	// initialize for search
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	search_result = 0;

	// if the last call was not the last one
	if (!LastDeviceFlag)
	{
		// 1-Wire reset
		unsigned char res = sendCommand(fd,RESET);
		printf("Reset = 0x%02x\n",res);
		if (res != 0xcd)
		{
			// reset the search
			LastDiscrepancy = 0;
			LastDeviceFlag = FALSE;
			LastFamilyDiscrepancy = 0;
			return FALSE;
		}

		// issue the search command
		writeData(fd, 0xF0);//write(0xF0); //send search command to DS18B20 units

		// loop to do the search
		do
		{
			// read a bit and its complement
			id_bit = read_bit(fd);
			cmp_id_bit = read_bit(fd);

			printf("bitA = 0x%02x, bitB = 0x%02x\n",id_bit, cmp_id_bit);

			// check for no devices on 1-wire
			if ((id_bit == 1) && (cmp_id_bit == 1))
				break;
			else
			{
				// all devices coupled have 0 or 1
				if (id_bit != cmp_id_bit)
					search_direction = id_bit;  // bit write value for search
				else
				{
					// if this discrepancy if before the Last Discrepancy
					// on a previous next then pick the same as last time
					if (id_bit_number < LastDiscrepancy)
						search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
					else
						// if equal to last pick 1, if not then pick 0
						search_direction = (id_bit_number == LastDiscrepancy);

					// if 0 was picked then record its position in LastZero
					if (search_direction == 0)
					{
						last_zero = id_bit_number;

						// check for Last discrepancy in family
						if (last_zero < 9)
							LastFamilyDiscrepancy = last_zero;
					}
				}

				// set or clear the bit in the ROM byte rom_byte_number
				// with mask rom_byte_mask
				if (search_direction == 1)
					ROM_NO[rom_byte_number] |= rom_byte_mask;
				else
					ROM_NO[rom_byte_number] &= ~rom_byte_mask;

				// serial number search direction write bit
				write_bit(fd,search_direction);

				// increment the byte counter id_bit_number
				// and shift the mask rom_byte_mask
				id_bit_number++;
				rom_byte_mask <<= 1;

				// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
				if (rom_byte_mask == 0)
				{
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		}
		while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

		// if the search was successful then
		if (!(id_bit_number < 65))
		{
			// search successful so set LastDiscrepancy,LastDeviceFlag,search_result
			LastDiscrepancy = last_zero;

			// check for last device
			if (LastDiscrepancy == 0)
				LastDeviceFlag = TRUE;

			search_result = TRUE;
		}
	}

	// if no device found then reset counters so next 'search' will be like a first
	if (!search_result || !ROM_NO[0])
	{
		LastDiscrepancy = 0;
		LastDeviceFlag = FALSE;
		LastFamilyDiscrepancy = 0;
		search_result = FALSE;
	}

	printf("%s","new address = 0x");
	for (int i = 0; i < 8; i++)
	{
		newAddr[i] = ROM_NO[i];
		printf("%02x",newAddr[i]);
	}
	printf("%s\n","");
	return search_result;
}

