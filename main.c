#include <stdio.h>
#include <sys/io.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "pci.h"
#include "pci_const.h"

int setOutput(int argc, char *argv[]);
void closeOutput();
void processDevice(uint16 bus, uint16 device, uint16 function);
uint32 readRegister(uint16 bus, uint16 device, uint16 function, uint16 reg);
char *getVendorName(uint16 vendorID);
char *getDeviceName(uint16 vendorID, uint16 deviceID);

FILE *out;

int main(int argc, char *argv[])
{
    if (setOutput(argc, argv) == E_INVALID_OPTION) {
        puts("error: invalid option");
        return E_INVALID_OPTION;
    }

    if (iopl(3)) {
        printf("I/O Privilege level change error: %s\nTry running under ROOT user\n",
                (char *) strerror(errno));
        return E_NO_ROOT_PRIVELEGES;
    }

    puts("Processing...");

    fputs("-------------------------\n", out);

    for (uint16 busid = 0; busid < BUS_LAST; busid++) {
        for (uint16 devid = 0; devid < DEVICE_LAST; devid++) {
            for (uint16 funcid = 0; funcid < FUNCTION_LAST; funcid++) {
                processDevice(busid, devid, funcid);
            }
        }
    }

    puts("Done. Press any key to continue...");
    getchar();

    closeOutput();
    return 0;
}

int setOutput(int argc, char *argv[])
{
    if (argc > 1) {
        if (argc == 3 && !strcmp(argv[1], FILE_OUTPUT_OPTION)) {
            if (out = fopen(argv[2], "w")) {
                printf("Output file is: %s\n", argv[2]);
                return 0;
            }
        }
        return E_INVALID_OPTION;
    }
    out = stdout;
    return 0;
}

void closeOutput()
{
    if (out != stdout) {
        fclose(out);
    }
}

void processDevice(uint16 bus, uint16 device, uint16 function)
{
    uint32 idRegData = readRegister(bus, device, function, ID_REGISTER);

    if (idRegData != NO_DEVICE) {
        uint16 deviceID = idRegData >> DEVICEID_SHIFT;
        uint16 vendorID = idRegData & 0xFFFF;
        fprintf(out, "%X:%X:%X\n", bus, device, function);
        char *vendorName = getVendorName(vendorID);
        fprintf(out, "Vendor ID: %04X, %s\n", vendorID, vendorName == NULL ? "Unknown vendor" : vendorName);
        char *deviceName = getDeviceName(vendorID, deviceID);
        fprintf(out, "Device ID: %04X, %s\n", deviceID, deviceName == NULL ? "Unknown device" : deviceName);

        fputs("-------------------------\n", out);
    }
}

uint32 readRegister(uint16 bus, uint16 device, uint16 function, uint16 reg)
{
    uint32 configRegAddress = (1 << 31) | (bus << BUS_SHIFT) | (device << DEVICE_SHIFT) |
            (function << FUNCTION_SHIFT) | (reg << REGISTER_SHIFT);
    outl(configRegAddress, CONTROL_PORT);
    return inl(DATA_PORT);
}

char *getVendorName(uint16 vendorID)
{
    for (int i = 0; i < PCI_VENTABLE_LEN; i++) {
        if (PciVenTable[i].VendorId == vendorID) {
            return PciVenTable[i].VendorName;
        }
    }
    return NULL;
}

char *getDeviceName(uint16 vendorID, uint16 deviceID)
{
    for (int i = 0; i < PCI_DEVTABLE_LEN; i++) {
        if (PciDevTable[i].VendorId == vendorID && PciDevTable[i].DeviceId == deviceID) {
            return PciDevTable[i].DeviceName;
        }
    }
    return NULL;
}
