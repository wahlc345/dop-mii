#ifndef _TOOLS_H_
#define _TOOLS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/unistd.h>
#include <wiiuse/wpad.h>

#define DEBUG_BUILD

void *AllocateMemory(u32 size);
void Close_SD();
void Close_USB();
void Init_Console();
bool Init_SD();
s32 Init_USB();
void Reboot();

void set_highlight(bool highlight);
bool FolderCreateTree(const char *fullpath);

s32 ReadFile(char *filepath, u8 **buffer, u32 *filesize);

void ReturnToLoader();
void SpinnerStart();
void SpinnerStop();

#define ReloadIosNoInit(version) __reloadIos(version, false);
#define ReloadIos(version) __reloadIos(version, true);
/*
This will shutdown the controllers, SD & USB then reload the IOS.
*/
int __reloadIos(int version, bool initWPAD);

#ifdef __cplusplus
}
#endif

#endif