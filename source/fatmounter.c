#include <string.h>
#include <ogc/lwp_watchdog.h>
#include <ogc/mutex.h>
#include <ogc/system.h>
#include <ogc/usbstorage.h>
#include <sdcard/wiisd_io.h>
#include <locale.h>
#include <fat.h>

#include "tools.h"
#include "patchmii_core.h"

//these are the only stable and speed is good
#define CACHE 32
#define SECTORS 64
#define SECTORS_SD 32

#define MOUNT_NONE 0
#define MOUNT_SD   1
#define MOUNT_SDHC 2
#define MOUNT_USB  1

/* Disc interfaces */
extern const DISC_INTERFACE __io_usbstorage_ro;
extern const DISC_INTERFACE __io_wiiums;
extern const DISC_INTERFACE __io_wiiums_ro;

sec_t _FAT_startSector;

int   fatSdMount = MOUNT_NONE;
sec_t fatSdSec = 0; // u32

int   fatUsbMount = 0;
sec_t fatUsbSec = 0;

int UsbDeviceOpen() {
	gprintf("\nUSBDevice_Init()");

	//closing all open Files write back the cache and then shutdown em!
    fatUnmount("USB:/");
    //right now mounts first FAT-partition

	//try first mount with cIOS
    if (!fatMount("USB", &__io_wiiums, 0, CACHE, SECTORS)) {
		//try now mount with libogc
		if (!fatMount("USB", &__io_usbstorage, 0, CACHE, SECTORS)) {
			gprintf(":-1");
			return -1;
		}
	}
	
	fatUsbMount = MOUNT_USB;
	fatUsbSec = _FAT_startSector;
	gprintf(":0");
	return 0;
}

void UsbDeviceClose() {
	gprintf("\nUSBDevice_deInit()");
    //closing all open Files write back the cache and then shutdown em!
    fatUnmount("USB:/");

	fatUsbMount = MOUNT_NONE;
	fatUsbSec = 0;
}

int UsbIsInserted(const char *path) {
    if (!strncmp(path, "USB:", 4)) return 1;
    return __io_wiisd.isInserted();
}

int SdCardOpen() {
	gprintf("\nSDCard_Init()");

    //closing all open Files write back the cache and then shutdown em!
    fatUnmount("SD:/");
    //right now mounts first FAT-partition
	if (fatMount("SD", &__io_wiisd, 0, CACHE, SECTORS)) 
	{
		fatSdMount = MOUNT_SD;
		fatSdSec = _FAT_startSector;
		gprintf(":1");
		return 1;
	}
	gprintf(":-1");
    return -1;
}

void SdCardClose() {
	gprintf("\nSDCard_deInit()");
    //closing all open Files write back the cache and then shutdown em!
    fatUnmount("SD:/");

	fatSdMount = MOUNT_NONE;
	fatSdSec = 0;
}