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
void waitforbuttonpress(u32 *out, u16 *outGC);

void set_highlight(bool highlight);
bool FolderCreateTree(const char *fullpath);

s32 ReadFile(char *filepath, u8 **buffer, u32 *filesize);

void ReturnToLoader();
void SpinnerStart();
void SpinnerStop();

void debug_printf(const char *fmt, ...);
void gprintf(const char *fmt, ...);
