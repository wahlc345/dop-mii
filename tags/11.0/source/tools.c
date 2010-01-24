#include <malloc.h>
#include <gccore.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>

#include "gecko.h"
#include "tools.h"
#include "network.h"
#include "video.h"
#include "controller.h"
#include "sys.h"

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
	LWP_CreateThread(&spinnerThread, spinner, NULL, NULL, 0, 80);
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
	gprintf("\nReturning to Loader");
	Console_SetPosition(ConsoleRows-1, 0);	
	ClearLine();
	printf("Returning To Loader");
	fflush(stdout);
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

/*
This will shutdown the controllers, SD & USB then reload the IOS.
*/

int __reloadIos(int version, bool initWPAD)
{
	gprintf("Reloading IOS%d...", version);
	int ret;
	// The following needs to be shutdown before reload	
	System_Deinit();

	ret = IOS_ReloadIOS(version);
	if (initWPAD) WPAD_Init();
	gprintf("Done\n");
	return ret;
}

bool PromptYesNo()
{
    printf("      [A] Yes        [B] NO    [HOME|START] Exit\n");

	u32 button;
	for (button = 0;;ScanPads(&button))
	{
		if (button&WPAD_BUTTON_A) return true;
		if (button&WPAD_BUTTON_B) return false;
		if (button&WPAD_BUTTON_HOME) ReturnToLoader();
	}
}

bool PromptContinue() 
{
    printf("Are you sure you want to continue?\n");
    printf("      [A] Yes        [B] NO    [HOME|START] Exit\n");	

	u32 button;
	for (button = 0;;ScanPads(&button))
	{
		if (button&WPAD_BUTTON_A) return true;
		if (button&WPAD_BUTTON_B) return false;
		if (button&WPAD_BUTTON_HOME) ReturnToLoader();
	}
}
