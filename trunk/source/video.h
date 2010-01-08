#ifndef _VIDEO_H_
#define _VIDEO_H_

//#include <pngu/pngu.h>

int ConsoleRows;
int ConsoleCols;

/* Prototypes */
void Con_Init(u32, u32, u32, u32);
void ClearScreen();
void ClearLine();
void Con_FgColor(u32, u8);
void Con_BgColor(u32, u8);
void Con_FillRow(u32, u32, u8);
void ConSetPosition(u8 row, u8 column);

//void Video_DrawPng(IMGCTX, PNGUPROP, u16, u16);
void VideoInit();

#endif

