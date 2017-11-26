#include <stdio.h>
#include <sys/io.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "pci.h"
#include "pci_const.h"

int setOutput(int argc, char *argv[]);
void closeOutput();
void processDevice(uint16 bus, uint16 device, uint16 function);
uint32 readRegister(uint16 bus, uint16 device, uint16 function, uint16 reg);
inline bool isBridge(uint16 bus, uint16 device, uint16 function);
uint8 getHeaderType(uint16 bus, uint16 device, uint16 function);
void outputGeneralData(uint16 bus, uint16 device, uint16 function, uint32 regData);
inline void outputVendorData(uint16 vendorID);
inline void outputDeviceData(uint16 vendorID, uint16 deviceID);
void outputClassCodeData(uint32 regData);
void outputIORegisters(uint32 regData);
void outputInterruptPinData(uint32 regData);
void outputInterruptLineData(uint32 regData);
inline void outputFullBusNumberData(uint32 regData);
inline void outputBusNumberData(char *infomsg, uint8 shift, uint32 regData);
char *getVendorName(uint16 vendorID);
char *getDeviceName(uint16 vendorID, uint16 deviceID);
char *getBaseClassData(uint8 baseClass);
char *getSubclassData(uint8 subclass);
char *getSRLProgrammingInterfaceData(uint8 SRLProgrammingInterface);

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

    for (uint16 busid = 0; busid < BUS_QUANTITY; busid++) {
        for (uint16 devid = 0; devid < DEVICE_QUANTITY; devid++) {
            for (uint16 funcid = 0; funcid < FUNCTION_QUANTITY; funcid++) {
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
        outputGeneralData(bus, device, function, idRegData);

        if (isBridge(bus, device, function)) {
            fprintf(out, "Is bridge: yes\n");
            outputClassCodeData(readRegister(bus, device, function, CLASS_CODE_REGISTER));
            outputFullBusNumberData(readRegister(bus, device, function, BUS_NUMBER_REGISTER));
            outputIORegisters(readRegister(bus, device, function, IO_DATA_REGISTER));
        } else {
            fprintf(out, "Is bridge: no\n");
            outputClassCodeData(readRegister(bus, device, function, CLASS_CODE_REGISTER));
        }
        outputInterruptPinData(readRegister(bus, device, function, INTERRUPT_PIN_REGISTER));
        outputInterruptLineData(readRegister(bus, device, function, INTERRUPT_LINE_REGISTER));

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

void outputGeneralData(uint16 bus, uint16 device, uint16 function, uint32 regData)
{
    fprintf(out, "%X:%X:%X\n", bus, device, function);

    uint16 deviceID = regData >> DEVICEID_SHIFT;
    uint16 vendorID = regData & 0xFFFF;
    outputVendorData(vendorID);
    outputDeviceData(vendorID, deviceID);
}

void outputVendorData(uint16 vendorID)
{
    char *vendorName = getVendorName(vendorID);
    fprintf(out, "Vendor ID: %04X, %s\n", vendorID, vendorName ? vendorName : "unknown vendor");
}

void outputDeviceData(uint16 vendorID, uint16 deviceID)
{
    char *deviceName = getDeviceName(vendorID, deviceID);
    fprintf(out, "Device ID: %04X, %s\n", deviceID, deviceName ? deviceName : "unknown device");
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

bool isBridge(uint16 bus, uint16 device, uint16 function)
{
    return getHeaderType(bus, device, function) & 1;
}

uint8 getHeaderType(uint16 bus, uint16 device, uint16 function)
{
    uint32 htypeRegData = readRegister(bus, device, function, HEADER_TYPE_REGISTER);
    return (htypeRegData >> HEADER_TYPE_SHIFT) & 0xFF;
}

void outputClassCodeData(uint32 regData)
{
    uint32 classCode = (regData >> CLASS_CODE_SHIFT) & 0xFFFFFF;
    uint8 baseClass = (classCode >> BASE_CLASS_SHIFT) & 0xFF;
    uint8 subclass = (classCode >> SUBCLASS_SHIFT) & 0xFF;
    uint8 srlProgrammingInterface = classCode & 0xFF;

    fprintf(out, "Class code: %#X\n", classCode);
    fprintf(out, "Base class: %#X %s\n", baseClass, getBaseClassData(baseClass));
    fprintf(out, "Subclass: %#X %s\n", subclass, getSubclassData(subclass));
    fprintf(out, "Specific register level programming interface: %#X %s\n",
            srlProgrammingInterface, getSRLProgrammingInterfaceData(srlProgrammingInterface));
}

void outputIORegisters(uint32 regData)
{
    uint8 IOBase = regData & 0xFF;
    uint8 IOLimit = (regData >> IO_LIMIT_SHIFT) & 0xFF;
    printf("I/O Base: %#X\n", IOBase);
    printf("I/O Limit: %#X\n", IOLimit);
}

char *getBaseClassData(uint8 baseClass)
{
    for (int i = 0; i < PCI_CLASSCODETABLE_LEN; i++) {
        if (PciClassCodeTable[i].BaseClass == baseClass) {
            return PciClassCodeTable[i].BaseDesc;
        }
    }
    return NULL;
}

char *getSubclassData(uint8 subclass)
{
    for (int i = 0; i < PCI_CLASSCODETABLE_LEN; i++) {
        if (PciClassCodeTable[i].BaseClass == subclass) {
            return PciClassCodeTable[i].SubDesc;
        }
    }
    return NULL;
}

char *getSRLProgrammingInterfaceData(uint8 srlProgrammingInterface)
{
    for (int i = 0; i < PCI_CLASSCODETABLE_LEN; i++) {
        if (PciClassCodeTable[i].ProgIf == srlProgrammingInterface) {
            return PciClassCodeTable[i].ProgDesc;
        }
    }
    return NULL;
}

void outputInterruptPinData(uint32 regData)
{
    uint8 interruptPin = (regData >> INTERRUPT_PIN_SHIFT) & 0xFF;
    char *interruptPinData;

    switch (interruptPin) {
        case 0:
            interruptPinData = "not used";
            break;
        case 1:
            interruptPinData = "INTA#";
            break;
        case 2:
            interruptPinData = "INTB#";
            break;
        case 3:
            interruptPinData = "INTC#";
            break;
        case 4:
            interruptPinData = "INTD#";
            break;
        default:
            interruptPinData = "invalid pin number";
            break;
    }

    fprintf(out, "Interrupt pin: %s\n", interruptPinData);
}

void outputInterruptLineData(uint32 regData)
{
    uint8 interruptLine = regData & 0xFF;

    fprintf(out, "Interrupt line: ");
    if (interruptLine == 0xFF) {
        fprintf(out, "unused/unknown input\n");
    } else if (interruptLine < INTERRUPT_LINES_NUMBER) {
        fprintf(out, "%s%d\n", "IRQ", interruptLine);
    } else {
        fprintf(out, "invalid line number\n");
    }
}

void outputFullBusNumberData(uint32 regData)
{
    outputBusNumberData(PRIMARY_BUS_NUMBER, PRIMARY_BUS_NUMBER_SHIFT, regData);
    outputBusNumberData(SECONDARY_BUS_NUMBER, SECONDARY_BUS_NUMBER_SHIFT, regData);
    outputBusNumberData(SUBORDINATE_BUS_NUMBER, SUBORDINATE_BUS_NUMBER_SHIFT, regData);
}

void outputBusNumberData(char *infomsg, uint8 shift, uint32 regData)
{
    uint8 busNumber = (regData >> shift) & 0xFF;
    fprintf(out, infomsg, busNumber);
}
