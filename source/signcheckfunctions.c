#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <gccore.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <wiiuse/wpad.h>

#include "signcheckfunctions.h"
#include "tools.h"
#include "gecko.h"

#define ES_ERROR_1028 -1028 // No ticket installed 
#define ES_ERROR_1035 -1035 // Title with a higher version is already installed 
#define ES_ERROR_2011 -2011 // Signature check failed (Needs Fakesign)

static u8 certs_sys[0xA00] ATTRIBUTE_ALIGN(32);

int CheckUsb2Module()
{
	gprintf("CheckUsb2Module::IOS_Open(/dev/usb/ehc) = ");
	int ret = IOS_Open("/dev/usb/ehc", 1);
	gprintf("%d\n", ret);
	if (ret < 0) return 0;
	IOS_Close(ret);
	return 1;
}
 
int CheckFlashAccess()
{
	gprintf("CheckFlashAccess::IOS_Open(/dev/flash) = ");
	int ret = IOS_Open("/dev/flash", 1);
	gprintf("%d\n", ret);
	if (ret < 0) return 0;
	IOS_Close(ret);
	return 1;
}
 
int CheckBoot2Access()
{
	gprintf("CheckBoot2Access::IOS_Open(/dev/boot2) = ");
	int ret = IOS_Open("/dev/boot2", 1);
	gprintf("%d\n", ret);
	if (ret < 0) return 0;
	IOS_Close(ret);
	return 1;
}
 
int CheckFakeSign()
{
	// We are expecting an error here, but depending on the error it will mean
	// that it is valid for fakesign. If we get -2011 it is definately not patched with fakesign.
	gprintf("CheckFakeSign::ES_AddTitleStart = ");
	int ret = ES_AddTitleStart((signed_blob*)tmd_dat, tmd_dat_size, (signed_blob *)certs_sys, sizeof certs_sys, NULL, 0);
	gprintf("%d\n", ret);
	if (ret >= 0) ES_AddTitleCancel();	
	if (ret == ES_ERROR_1028) return 1;
	return 0;
}

int CheckEsIdentify()
{
	int ret = -1;
	//u32 keyid;
 
	gprintf("CheckESIdentify::ES_Identify = ");
	ret = ES_Identify((signed_blob*)certs_sys, sizeof(certs_sys), 
					  (signed_blob*)tmd_dat, tmd_dat_size, 
					  (signed_blob*)ticket_dat, ticket_dat_size, NULL);
	gprintf("%d\n", ret);
 
	if (ret < 0) return 0;
	return 1;
}
 
/* Misc stuff. */
 
char* CheckRegion()
{	 
	switch (CONF_GetRegion())
	{
		case CONF_REGION_JP: return "Japan";
		case CONF_REGION_EU: return "Europe";
		case CONF_REGION_US: return "USA";
		case CONF_REGION_KR: return "Korea";
		default: return "Unknown?";
	}
}
 
int sortCallback(const void * first, const void * second)
{
  return ( *(u32*)first - *(u32*)second );
}
 
/* Deep stuff :D */
 
int GetCert()
{ 
	u32 fd = IOS_Open("/sys/cert.sys", 1);
	if (IOS_Read(fd, certs_sys, sizeof(certs_sys)) < sizeof(certs_sys)) return -1;
	IOS_Close(fd);
	return 0;
}