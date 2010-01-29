#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>

#include "sys.h"
#include "video.h"
#include "tools.h"
#include "gecko.h"
#include "dopios.h"
#include "network.h"
#include "svnrev.h"

/* Constants */
#define CERTS_LEN 0x280

/* Variables */
static const char certs_fs[] ATTRIBUTE_ALIGN(32) = "/sys/cert.sys";
bool Shutdown = false;

void __Sys_ResetCallback() 
{
	ExitMainThread = true;
}

void __Sys_PowerCallback() 
{
    /* Poweroff console */
    Shutdown = true;
	ExitMainThread = true;
}

void System_Init() 
{
    /* Initialize video subsytem */
#ifndef NO_DEBUG
	InitGecko();
#endif
	gprintf("\n\nDOP-IOS MOD (r%s)\n", SVN_REV_STR);
	gprintf("Initializing Wii\n");
	gprintf("VideoInit\n"); Video_Init();  
	gprintf("WPAD_Init\n"); WPAD_Init();
	gprintf("PAD_Init\n"); PAD_Init();

    /* Set RESET/POWER button callback */
	WPAD_SetPowerButtonCallback(__Sys_PowerCallback);
    SYS_SetResetCallback(__Sys_ResetCallback);
    SYS_SetPowerCallback(__Sys_PowerCallback);
}

void System_Deinit()
{
	Close_SD();
	Close_USB();
	WPAD_Shutdown();
	NetworkShutdown();
}

void Sys_Reboot(void) {
    gprintf("Restart console\n");
	System_Deinit();
    STM_RebootSystem();
}

void System_Shutdown() 
{
	gprintf("\n");
	gprintf("Powering Off Console\n");
	System_Deinit();
		
	int ret;
    if (CONF_GetShutdownMode() == CONF_SHUTDOWN_IDLE) 
	{
        ret = CONF_GetIdleLedMode();		
        if (ret >= CONF_LED_OFF && ret <= CONF_LED_BRIGHT) ret = STM_SetLedMode(ret);

        ret = STM_ShutdownToIdle();
    } 
	else ret = STM_ShutdownToStandby();
}

void Sys_LoadMenu(void) 
{
    u32 *stub = (u32 *)0x80001800;

    /* Homebrew Channel stub */
    if (*stub) ReturnToLoader();

    /* Return to the Wii system menu */
    SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}

s32 Sys_GetCerts(signed_blob **certs, u32 *len) 
{
    static signed_blob certificates[CERTS_LEN] ATTRIBUTE_ALIGN(32);

    s32 fd, ret;

    /* Open certificates file */
    fd = IOS_Open(certs_fs, 1);
    if (fd < 0) return fd;

    /* Read certificates */
    ret = IOS_Read(fd, certificates, sizeof(certificates));

    /* Close file */
    IOS_Close(fd);

    /* Set values */
    if (ret > 0) 
	{
        *certs = certificates;
        *len = sizeof(certificates);
    }

    return ret;
}

