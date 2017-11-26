typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

#define FILE_OUTPUT_OPTION "--file"
enum Error {E_INVALID_OPTION = 1, E_NO_ROOT_PRIVELEGES};

#define BUS_LAST 256
#define DEVICE_LAST 32
#define FUNCTION_LAST 8

#define CONTROL_PORT 0x0CF8
#define DATA_PORT 0x0CFC

#define BUS_SHIFT 16
#define DEVICE_SHIFT 11
#define FUNCTION_SHIFT 8
#define REGISTER_SHIFT 2
#define DEVICEID_SHIFT 16

#define ID_REGISTER 0x00

#define NO_DEVICE 0xFFFFFFFF
