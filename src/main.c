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


int main(void)
{
	char ttyCMD[255];
	unsigned char *ROMaddress = malloc(MAX_DEVICES * ROM_ADDRESS_SIZE);

	char *portname = "/dev/ttyUSB0";
	char sttyConfig[] = "stty -F %s 9600 -icanon min 1 time 1 -parenb -cstopb cs8 -crtscts cread clocal -ixon -ixoff -ixany -echo -echoe -isig -opost";

	sprintf(ttyCMD ,sttyConfig, portname);
	if( system( ttyCMD ) )
	{
		printf("error %d configuring stty %s: %s\n\n", errno, portname, strerror (errno));
		return -1;
	}

	int fd = open (portname, O_RDWR | O_NOCTTY );
	if (fd < 0){
		printf("error %d opening %s: %s\n\n", errno, portname, strerror (errno));
		return -1;
	}

	sendReset(fd);
	sendReset(fd);
	sendReset(fd);

	reset_search();
	unsigned char ROMaddressTemp[8];
	while(findDevices(fd, ROMaddressTemp))
	{
		memcpy(ROMaddress + (devicesFound * ROM_ADDRESS_SIZE), ROMaddressTemp, ROM_ADDRESS_SIZE);
		devicesFound++;
	}
	double *temperatures = malloc( sizeof(double) * devicesFound );

	while(1){
		printMultipleTemp(fd, ROMaddress, devicesFound, temperatures);
		logTemperatures(temperatures, devicesFound);
		sleep(30);
	}

	close(fd);
	free(ROMaddress);
	return 0;
}
