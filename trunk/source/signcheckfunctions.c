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

int CheckUsb2Module()
{
int ret = IOS_Open("/dev/usb/ehc", 1);
if (ret < 0)
return 0;
IOS_Close(ret);
return 1;
}
 
 
int CheckFlashAccess()
{
int ret = IOS_Open("/dev/flash", 1);
if (ret < 0)
{
return 0;
}
IOS_Close(ret);
return 1;
}
 
int CheckBoot2Access()
{
int ret = IOS_Open("/dev/boot2", 1);
if (ret < 0)
{
return 0;
}
IOS_Close(ret);
return 1;
}
 
int CheckEsIdentify()
{
int ret = -1;
u32 keyid;
 
ret = ES_Identify((signed_blob*)certs_sys, sizeof(certs_sys), (signed_blob*)tmd_dat, tmd_dat_size, (signed_blob*)ticket_dat, ticket_dat_size, &keyid);
 
if (ret < 0)
{
return 0;
}
return 1;
}
 
/* Misc stuff. */
 
char* CheckRegion()
{
s32 region = CONF_GetRegion();
 
switch (region)
{
case CONF_REGION_JP:
return "Japan";
case CONF_REGION_EU:
return "Europe";
case CONF_REGION_US:
return "Usa";
case CONF_REGION_KR:
return "Korea";
default:
return "Unknown?";
}
}
 
int sortCallback(const void * first, const void * second)
{
  return ( *(u32*)first - *(u32*)second );
}
 
/* Deep stuff :D */
 
int GetCert()
{
u32 fd;
 
fd = IOS_Open("/sys/cert.sys", 1);
if (IOS_Read(fd, certs_sys, sizeof(certs_sys)) < sizeof(certs_sys))
return -1;
IOS_Close(fd);
return 0;
}