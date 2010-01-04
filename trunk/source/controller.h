#ifndef BUTTONS_H
#define BUTTONS_H

#ifdef __cplusplus
extern "C" {
#endif

void ScanPads(u32 *button);
void WaitAnyKey();
u32 WaitKey(u32 button);

#ifdef __cplusplus
}
#endif

#endif