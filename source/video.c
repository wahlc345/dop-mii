#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ogcsys.h>

#include "sys.h"
#include "video.h"
#include "gecko.h"

/* Video variables */
static unsigned int *frontBuffer = NULL;
static GXRModeObj *vmode = NULL;
int ConsoleRows;
int ConsoleCols;
int ScreenWidth;
int ScreenHeight;

void ClearScreen() 
{
    /* Clear console */
    printf("\x1b[2J");
	fflush(stdout);
}

void ClearLine() 
{
    s32 cols, rows;
    u32 cnt;

    printf("\r");
    fflush(stdout);

    /* Get console metrics */
    CON_GetMetrics(&cols, &rows);

    /* Erase line */
    for (cnt = 1; cnt < cols; cnt++) {
        printf(" ");
        fflush(stdout);
    }

    printf("\r");
    fflush(stdout);
}

void Con_FgColor(u32 color, u8 bold) {
    /* Set foreground color */
    printf("\x1b[%u;%um", color + 30, bold);
    fflush(stdout);
}

void Con_BgColor(u32 color, u8 bold) {
    /* Set background color */
    printf("\x1b[%u;%um", color + 40, bold);
    fflush(stdout);
}

void Con_FillRow(u32 row, u32 color, u8 bold) {
    s32 cols, rows;
    u32 cnt;

    /* Set color */
    printf("\x1b[%u;%um", color + 40, bold);
    fflush(stdout);

    /* Get console metrics */
    CON_GetMetrics(&cols, &rows);

    /* Save current row and col */
    printf("\x1b[s");
    fflush(stdout);

    /* Move to specified row */
    printf("\x1b[%u;0H", row);
    fflush(stdout);

    /* Fill row */
    for (cnt = 0; cnt < cols; cnt++) 
	{
        printf(" ");
        fflush(stdout);
    }

    /* Load saved row and col */
    printf("\x1b[u");
    fflush(stdout);

    /* Set default color */
    Con_BgColor(0, 0);
    Con_FgColor(7, 1);
}

//void Video_DrawPng(IMGCTX ctx, PNGUPROP imgProp, u16 x, u16 y) {
//    PNGU_DECODE_TO_COORDS_YCbYCr(ctx, x, y, imgProp.imgWidth, imgProp.imgHeight, vmode->fbWidth, vmode->xfbHeight, framebuffer);
//}

void ConSetPosition(u8 row, u8 column)
{
    // The console understands VT terminal escape codes
    // This positions the cursor on row 2, column 0
    // we can use variables for this with format codes too
    // e.g. printf ("\x1b[%d;%dH", row, column );
    printf("\x1b[%u;%uH", row, column);
}

void VideoInit()
{
    // Initialise the video system
    VIDEO_Init();

    // Obtain the preferred video mode from the system
    // This will correspond to the settings in the Wii menu
    vmode = VIDEO_GetPreferredMode(NULL);

	GX_AdjustForOverscan(vmode, vmode, 0, 20);	

	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9)
	{
		vmode->viWidth += 16;
		vmode->fbWidth = vmode->viWidth;
		vmode->viXOrigin = ((VI_MAX_WIDTH_NTSC - vmode->viWidth) / 2);		
	}
	
    // Set up the video registers with the chosen mode
    VIDEO_Configure(vmode);

	ScreenWidth = vmode->viWidth;
	ScreenHeight = vmode->viHeight;

    // Allocate memory for the display in the uncached region
    frontBuffer = (u32 *) MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));	
	
	VIDEO_ClearFrameBuffer(vmode, frontBuffer, COLOR_BLACK);

	// Tell the video hardware where our display memory is	
    VIDEO_SetNextFramebuffer(frontBuffer);

    // Make the display visible
    VIDEO_SetBlack(FALSE);

    // Flush the video register changes to the hardware
    VIDEO_Flush();

    // Wait for Video setup to complete
	VIDEO_WaitVSync();
    if (vmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	else while (VIDEO_GetNextField()) VIDEO_WaitVSync();

	// Initialise the console, required for printf
	CON_InitEx(vmode, 0, 0, ScreenWidth, ScreenHeight);
	CON_GetMetrics(&ConsoleCols, &ConsoleRows);
}
