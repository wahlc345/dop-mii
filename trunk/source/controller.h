#ifndef BUTTONS_H
#define BUTTONS_H

void ScanPads(u32 *button);
void WaitAnyKey();
u32 WaitKey(u32 button);

#endif