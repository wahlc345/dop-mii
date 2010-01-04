/*
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdlib.h>
#include <errno.h>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <network.h>
#include <sys/errno.h>
#include <fat.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "http.h"
#include "tools.h"
#include "network.h"

extern bool networkInitialized;

int GetNusObject(u32 titleid1, u32 titleid2, u32 version, char *content, u8 **outbuf, u32 *outlen) 
{
    char buf[200] = "";
	char filename[256] = "";
    int ret = 0;	
    uint httpStatus = 200;	
	FILE *fp = NULL;
	
	snprintf(filename, sizeof(filename), "sd:/%08X/%08X/v%d/%s", titleid1, titleid2, version, content);
	
	debug_printf("\nNusGetObject:Init_SD...");
    if (Init_SD()) 
	{		
		debug_printf("Complete\n");
		fp = fopen(filename, "rb");
		if (fp) 
		{
			gprintf("\n\tLoading File = %s ", filename);
			fseek(fp, 0, SEEK_END);
			*outlen = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			*outbuf = AllocateMemory(*outlen);

			if (*outbuf == NULL) 
			{
				printf("\nOut of memory: Size %d ", *outlen);
				ret = -1;
			}		
			
			if (ret == 0) 
			{				
				if (fread(*outbuf, *outlen, 1, fp) != 1) ret = -2; /* Failed to read the file so return an error */
				else ret = 1; /* File successfully loaded so return */
			}
			fclose(fp);
			fp = NULL;				
			Close_SD();
			return ret;	
		}

		Close_SD();
		gprintf("\n***%s not found on the SD card.\n*** Trying to download it from the internet...\n",filename);
	}
    	
	debug_printf("NusGetObject:NetworkInit...");
	NetworkInit();
	debug_printf("Completed\n");
    
    snprintf(buf, 128, "http://nus.cdn.shop.wii.com/ccs/download/%08x%08x/%s", titleid1, titleid2, content);

	int retry = 5;
	while (1)
	{				
		ret = http_request(buf, (u32)(1 << 31));
		if (!ret) 
		{
			retry--;
			printf("Error making HTTP request. Trying Again. \n");
			gprintf("Request: %s Ret: %d\n", buf, ret);
			sleep(1);
			if (retry < 0) return -1;
		}
		else break;
    }
    ret = http_get_result(&httpStatus, outbuf, outlen);

    if (Init_SD()) 
	{
		char folder[300] = "";
		snprintf(folder, sizeof(folder), "sd:/%08X/%08X/v%d", titleid1, titleid2, version);
		gprintf("\n\tDEBUG: FolderCreateTree() = %s ", (FolderCreateTree(folder)?"true":"false"));
		
		fp = fopen(filename, "wb");
		if (fp) 
		{
		    fwrite(*outbuf, *outlen, 1, fp);
			fclose(fp);
		}
		else gprintf("\n\tDEBUG: Could not write file %s to the SD Card. \n", filename);
		Close_SD();
    }

    if (((int)*outbuf & 0xF0000000) == 0xF0000000) return (int) *outbuf;

    return 0;
}
