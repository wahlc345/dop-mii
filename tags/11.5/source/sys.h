#ifndef _SYS_H_
#define _SYS_H_

bool Shutdown;

void System_Init();
void System_Deinit();
void System_Shutdown();

void Sys_Reboot(void);
void Sys_LoadMenu(void);
s32 Sys_GetCerts(signed_blob **, u32 *);

#endif

