#include <malloc.h>
#include <gccore.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <sys/dir.h>

#include "gecko.h"
#include "tools.h"
#include "network.h"
#include "video.h"

static volatile u32 tickcount = 0;

static lwp_t spinnerThread = LWP_THREAD_NULL;
static bool spinnerRunning = false;

static void * spinner(void *args) 
{	
	char *spinnerChars = (char*)"/-\\|";
	int spin  = 0;
	while (1) 
	{		
		if (!spinnerRunning) break;				
		printf("\b%c", spinnerChars[spin++]);
		if (!spinnerChars[spin]) spin = 0;
		fflush(stdout);
		if (!spinnerRunning) break;
		usleep(50000);			
	}	
	return NULL;
}

void SpinnerStart()
{
	if (spinnerThread != LWP_THREAD_NULL) return;
	spinnerRunning = true;
	LWP_CreateThread(&spinnerThread, spinner, NULL, NULL, 0, LWP_PRIO_IDLE);
}

void SpinnerStop()
{
	if (spinnerRunning) 
	{		
		spinnerRunning = false;
		LWP_JoinThread(spinnerThread, NULL);
		spinnerThread = LWP_THREAD_NULL;		
	}
}

void ReturnToLoader() 
{
	gcprintf("\n\nReturning To Loader");
	VIDEO_WaitVSync();
	exit(0);
}

void Reboot() 
{
    if (*(u32*)0x80001800) exit(0);
    SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}

void *AllocateMemory(u32 size)
{
	return memalign(32, (size+31)&(~31));
}

void Close_SD() 
{	
	//closing all open Files write back the cache and then shutdown em!
	fatUnmount("SD:/");
    __io_wiisd.shutdown();
}

bool Init_SD() 
{
	Close_SD();
	//right now mounts first FAT-partition
	return fatMountSimple("sd", &__io_wiisd);
}

void Close_USB() {
	fatUnmount("usb:/");
    __io_usbstorage.shutdown();
}

s32 Init_USB() 
{
	Close_USB();

    if (!fatMountSimple("usb", &__io_usbstorage)) return 0;
    return 1;
}

void set_highlight(bool highlight) 
{
    if (highlight) {
        printf("\x1b[%u;%um", 47, false);
        printf("\x1b[%u;%um", 30, false);
    } else {
        printf("\x1b[%u;%um", 37, false);
        printf("\x1b[%u;%um", 40, false);
    }
}

bool FolderCreateTree(const char *fullpath) 
{    
    char dir[300];
    char *pch = NULL;
    u32 len;
    struct stat st;

    strlcpy(dir, fullpath, sizeof(dir));
    len = strlen(dir);
    if (len && len< sizeof(dir)-2 && dir[len-1] != '/');
    {
        dir[len++] = '/';
        dir[len] = '\0';
    }
    if (stat(dir, &st) != 0) // fullpath not exist?
	{ 
        while (len && dir[len-1] == '/') dir[--len] = '\0';	// remove all trailing /
        pch = strrchr(dir, '/');
        if (pch == NULL) return false;
        *pch = '\0';
        if (FolderCreateTree(dir)) 
		{
            *pch = '/';
            if (mkdir(dir, 0777) == -1) return false;
        } 
		else return false;
    }
    return true;
}

/*
This will shutdown the controllers, SD & USB then reload the IOS.
*/

int __reloadIos(int version, bool initWPAD)
{
	gprintf("Reloading IOS%d...", version);
	int ret;
	// The following needs to be shutdown before reload
	Close_SD(); 
	Close_USB();
	WPAD_Shutdown();
	NetworkShutdown();

	ret = IOS_ReloadIOS(version);
	if (initWPAD) WPAD_Init();
	gprintf("Done\n");
	return ret;
}
