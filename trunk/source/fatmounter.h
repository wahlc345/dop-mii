#ifndef _FATMOUNTER_H_
#define _FATMOUNTER_H_

extern int   fatSdMount;
extern sec_t fatSdSec;
extern int   fatUsbMount;
extern sec_t fatUsbSec;

/* Mounts a USB Device */
int UsbDeviceOpen();

/* Unounts a USB Device */
void UsbDeviceClose();

/* Checks to see if the USB Devices is connected */
int UsbIsInserted(const char *path);

/* Mounts a Normal SD Card at this time. SDHC may come later */
int SdCardOpen();

/* Unmounts the SD Card */
void SdCardClose();

#endif
