#ifndef _SAVEGAME_H_
#define _SAVEGAME_H_

#include <fat.h>
#include <sdcard/gcsd.h>
#include <sdcard/wiisd_io.h>

/* Constants */
#define SAVEGAME_EXTRACT	0
#define SAVEGAME_INSTALL	1
#define ENTRIES_PER_PAGE	11
#define SAVES_DIRECTORY		"/savegames"

/* Macros */
#define NB_DEVICES		(sizeof(deviceList) / sizeof(fatDevice))

extern const DISC_INTERFACE __io_usb2storage;

/* Savegame structure */
struct savegame {
	/* Title name */
	char name[65];

	/* Title ID */
	u64 tid;
};

/* 'FAT Device' struct */
typedef struct {
	/* Device mount point */
	char *mount;

	/* Device name */
	char *name;

	/* Device interface */
	const DISC_INTERFACE *interface;
} fatDevice;

/* Device list */
static fatDevice deviceList[] = {
	{ "sd",		"Wii SD Slot",			&__io_wiisd },
	{ "usb",	"USB Mass Storage Device",	&__io_usbstorage },
	//{ "usb2",	"USB 2.0 Mass Storage Device",	&__io_usb2storage },
	{ "gcsda",	"SD Gecko (Slot A)",		&__io_gcsda },
	{ "gcsdb",	"SD Gecko (Slot B)",		&__io_gcsdb },
};

/* Prototypes */
s32 Savegame_GetNandPath(u64, char *);
s32 Savegame_GetTitleName(const char *, char *);
s32 Savegame_CheckTitle(const char *);
s32 Savegame_Manage(u64, u32, const char *);
s32 Menu_Device();
s32 __Menu_RetrieveList(struct savegame **outbuf, u32 *outlen, s32 mode, s32 device);

#endif