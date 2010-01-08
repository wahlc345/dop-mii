#include <gccore.h>
#include <stdio.h>
#include <string.h>

#ifndef NO_DEBUG
#include <stdarg.h>

bool geckoinit = false;

//using the gprintf from crediar because it is smaller than mine
void gprintf(const char *fmt, ...)
{
	if (!(geckoinit)) return;
	char astr[4096];
	va_list ap;
	va_start(ap,fmt);
	vsprintf( astr, fmt, ap );
	va_end(ap);
	usb_sendbuffer_safe( 1, astr, strlen(astr) );
} 

void gcprintf(const char *fmt, ...)
{
	char astr[4096];
	va_list ap;
	va_start(ap,fmt);
	vsprintf(astr, fmt, ap);
	va_end(ap);

	gprintf(astr);	
	printf(astr);	
}

void InitGecko()
{
	if (usb_isgeckoalive(EXI_CHANNEL_1))
	{
		usb_flush(EXI_CHANNEL_1);
		geckoinit = true;
	}
}

#endif /* NO_DEBUG */
