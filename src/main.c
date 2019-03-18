/*
 * main.c
 *
 *  Created on: Mar 16, 2019
 *      Author: aterrazas
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "DS2480.h"

char sttyConfig[] = "stty -F %s 9600 -icanon min 1 time 1 -parenb -cstopb cs8 -crtscts cread clocal -ixon -ixoff -ixany -echo -echoe -isig -opost";

int main(void)
{
	char ttyCMD[255];
	unsigned char ROMaddress[8];
	unsigned char padData[SCRATCH_PAD_SIZE];
	scratchPad *dataPad;

	char *portname = "/dev/ttyUSB0";

	sprintf(ttyCMD,sttyConfig,portname);
	if(system(ttyCMD))
	{
        printf("error %d configuring stty %s: %s\n\n", errno, portname, strerror (errno));
        return -1;
	}

	int fd = open (portname, O_RDWR | O_NOCTTY ); //O_NDELAY);  //
	if (fd < 0){
	        printf("error %d opening %s: %s\n\n", errno, portname, strerror (errno));
	        return -1;
	}


	readConfig(fd);

	if(readROMcmd(fd,ROMaddress))
		return -1;

	unsigned int count = 2;
	while(count){
		startTempConv(fd,ROMaddress);
		if(readScratchPAD(fd, ROMaddress, padData))
			return -1;

		dataPad = (scratchPad*)padData;
		printf("Temperature LSB = %02x, MSB = %02x, Celsius = %.3f\n",dataPad->tempLSB,dataPad->tempMSB,calculateTemp(dataPad));
		count--;
	}


	close(fd);

	return 0;
}
