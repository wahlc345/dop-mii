#ifndef _VIDEO_H_
#define _VIDEO_H_

//#include <pngu/pngu.h>

#define BLACK	0
#define RED		1
#define GREEN	2
#define YELLOW	3
#define BLUE	4
#define MAGENTA	5
#define CYAN	6
#define WHITE	7

#define BOLD_NONE	0
#define BOLD_NORMAL	1
#define BOLD_FAINT	2

int ConsoleRows;
int ConsoleCols;

void ClearScreen();
void ClearLine();
void Console_SetPosition(u8 row, u8 column);
void Console_SetFgColor(u8 color, u8 bold);
void Console_SetBgColor(u8 color, u8 bold);
void Console_SetColors(u8 bgColor, u8 bgBold, u8 fgColor, u8 fgBold);
int Console_GetCurrentRow();

void PrintCenter(char *text, int width);
void Video_Init();

//void Video_DrawPng(IMGCTX, PNGUPROP, u16, u16);

#endif

