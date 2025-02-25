/*
* smbusb_sbsreport
* Smart Battery Specification Report tool
*
* Copyright (c) 2016 Viktor <github@karosium.e4ward.com>
*
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <getopt.h>
#include <sys/types.h>

#include "libsmbusb.h"

typedef struct {
    unsigned char unknown_0[4];
    unsigned short cell_voltage[4];
    unsigned char unknown_1[2];
} lenovo_data_t __attribute__ ((aligned (1)));

static void printUsage()
{
    printf("smbusb_sbsreport\n"
           "  options:\n"
           "    --device=<device_uri>               = use given device for SMBus access: fx2lp, i2cdev\n"
           "      <device_uri> contain:\n"
           "        <schema>://<device_definition>\n"
           "      Example:\n"
           "        i2cdev:///dev/i2c-7\n"
           "        i2c:///dev/i2c-7\n"
           "        fx2lp://vid=0x04b4,pid=0x8613\n");
}

int main(int argc, char*argv[])
{
	int status;
	int i;
	unsigned char *block = malloc(256);
    int size = 0;

	unsigned char *tempStr = malloc(256);
	unsigned int tempWord;

    int c;
    const char *device_uri = "fx2lp://vid=0x04b4,pid=0x8613";
    while (1) {
        static struct option long_options[] = {
            {"device", required_argument, NULL, 50},
            {"help", no_argument, NULL, 'h'},
            {0}
        };

        c = getopt_long(argc, argv, "hd:", long_options, NULL);

        if (c == -1)
            break;

        switch (c) {
        case 'h':
            printUsage();
            exit(0);
        case 50:
            device_uri = optarg;
            break;
        }
    }

    if ((status = SMBOpenDevice(device_uri)) >= 0) {
		printf("SMBusb Firmware Version: %d.%d.%d\n",status&0xFF,(status >>8)&0xFF,(status >>16)&0xFF);
	} else {
		printf("Error Opening SMBusb: libusb error %d\n",status);
		exit(0);
	}

	SMBEnablePEC(1);

	printf("-------------------------------------------------\n");


	memset(tempStr,0,256);
	if (SMBReadBlock(0x16,0x20,tempStr) <=0) strcpy(tempStr,"ERROR");

	printf("Manufacturer Name:          %s\n",tempStr);

	memset(tempStr,0,256);
	if (SMBReadBlock(0x16,0x21,tempStr) <=0) strcpy(tempStr,"ERROR");
	printf("Device Name:                %s\n",tempStr);

	memset(tempStr,0,256);
	if (SMBReadBlock(0x16,0x22,tempStr) <=0) strcpy(tempStr,"ERROR");
	printf("Device Chemistry:           %s\n",tempStr);

	printf("Serial Number:              %u\n",SMBReadWord(0x16,0x1c));

	tempWord = SMBReadWord(0x16,0x1b);
	printf("Manufacture Date:	    %u.%02u.%02u\n",1980+(tempWord>>9),
						     	tempWord>>5&0xF,
							tempWord&0x1F);

	printf("\n");
	printf("Manufacturer Access:        %04x\n",SMBReadWord(0x16,0x00));

	printf("Remaining Capacity Alarm:   %u mAh(/10mWh)\n",SMBReadWord(0x16,0x01));

	printf("Remaining Time Alarm:       %u min\n",SMBReadWord(0x16,0x02));

	printf("Battery Mode:               %04x\n",SMBReadWord(0x16,0x03));


	printf("At Rate:                    %d mAh(/10mWh)\n",SMBReadWord(0x16,0x04));

	printf("At Rate Time To Full:       %u min\n",SMBReadWord(0x16,0x05));
	printf("At Rate Time To Empty:      %u min\n",SMBReadWord(0x16,0x06));

	printf("At Rate OK:                 %u\n",SMBReadWord(0x16,0x07));

	printf("Temperature:                %02.02f degC\n",(float)((SMBReadWord(0x16,0x08)*0.1)-273.15)); // unit: 0.1Kelvin

	printf("Voltage:                    %u mV\n",SMBReadWord(0x16,0x09));
	printf("Current:                    %d mA\n", (int16_t)SMBReadWord(0x16, 0x0a));
	printf("Average Current:            %d mA\n", (int16_t)SMBReadWord(0x16, 0x0b));
	printf("Max Error:                  %u %%\n",SMBReadWord(0x16,0x0c));
	printf("Relative State Of Charge    %u %%\n",SMBReadWord(0x16,0x0d));
	printf("Absolute State Of Charge    %u %%\n",SMBReadWord(0x16,0x0e));
	printf("Remaining Capacity:         %u mAh(/10mWh)\n",SMBReadWord(0x16,0x0f));
	printf("Full Charge Capacity:       %u mAh(/10mWh)\n",SMBReadWord(0x16,0x10));
	printf("Run Time To Empty:          %u min\n",SMBReadWord(0x16,0x11));
	printf("Average Time To Empty:      %u min\n",SMBReadWord(0x16,0x12));
	printf("Average Time To Full:       %u min\n",SMBReadWord(0x16,0x13));
	printf("Charging Current:           %u mA\n",SMBReadWord(0x16,0x14));
	printf("Charging Voltage:           %u mV\n",SMBReadWord(0x16,0x15));
	printf("Cycle Count:                %u\n",SMBReadWord(0x16,0x17));

	memset(block, 0, 256);
    size = SMBReadBlock(0x16, 0x23, block);
	if (size < sizeof(lenovo_data_t)) {
        printf("Manufacturer Data: ");
        for (i = 0; i < size; i++) {
            printf("%02x ", block[i]);
        }
        printf("\n");
    } else {
        lenovo_data_t *lenovo_data = (lenovo_data_t*)block;
        for (i = 0; i < 4; i++) {
            printf("Cell %d voltage:             %u mV\n", i,
                    lenovo_data->cell_voltage[3-i]);
        }
    }
}
