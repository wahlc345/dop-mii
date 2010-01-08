#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ogc/es.h>

#include "http.h"
#include "tools.h"
#include "network.h"
#include "gecko.h"

extern bool networkInitialized;

int GetNusObject(u32 titleid1, u32 titleid2, u16 *version, char *content, u8 **outbuf, u32 *outlen) 
{
    char buf[200] = "";
	char filename[256] = "";
    int ret = 0;	
    uint httpStatus = 200;	
	FILE *fp = NULL;
	
	if (*version != 0) snprintf(filename, sizeof(filename), "sd:/%08X/%08X/v%u/%s", titleid1, titleid2, *version, content);
	
	gprintf("\nNusGetObject::Init_SD...");
    if (*version != 0 && Init_SD()) 
	{		
		gprintf("Done\n");
		fp = fopen(filename, "rb");
		if (fp) 
		{
			gprintf("Loading File = %s\n", filename);
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
				if (fread(*outbuf, *outlen, 1, fp) != 1) ret = -2; // Failed to read the file so return an error
				else ret = 1; // File successfully loaded so return
			}
			fclose(fp);
			fp = NULL;				
			Close_SD();
			return ret;	
		}

		Close_SD();
		gprintf("\n* %s not found on the SD card.", filename);
		gprintf("\n* Trying to download it from the internet...\n");
	}
	else gprintf("No Card Found\n");
    	
	gprintf("NusGetObject::NetworkInit...");
	NetworkInit();
	gprintf("Done\n");
    
    snprintf(buf, 128, "http://nus.cdn.shop.wii.com/ccs/download/%08x%08x/%s", titleid1, titleid2, content);
	gprintf("Attemping to download %s\n\n", buf);

	int retry = 5;
	while (1)
	{				
		ret = http_request(buf, (u32)(1 << 31));
		//gprintf("http_request = %d\n", ret);
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
	//gprintf("http_get_result = %d\n", ret);

	// if version = 0 and file is TMD file then lets extract the version and return it
	// so that the rest of the files will save to the correct folder
	if (strcmpi(content, "TMD") == 0 && *version == 0 && *outbuf != NULL && *outlen != 0)
	{
		gprintf("Getting Version from TMD File\n");

		tmd *pTMD = (tmd *)SIGNATURE_PAYLOAD((signed_blob *)*outbuf);
		*version = pTMD->title_version;
		sprintf(content, "%s.%u", content, *version);
		snprintf(filename, sizeof(filename), "sd:/%08X/%08X/v%u/%s", titleid1, titleid2, *version, content);
		gprintf("New Filename = %s\n", filename);
	}

    if (*version != 0 && Init_SD()) 
	{
		char folder[300] = "";
		snprintf(folder, sizeof(folder), "sd:/%08X/%08X/v%u", titleid1, titleid2, *version);
		gprintf("FolderCreateTree() = %s ", (FolderCreateTree(folder)? "True" : "False"));
		
		fp = fopen(filename, "wb");
		if (fp) 
		{
		    fwrite(*outbuf, *outlen, 1, fp);
			fclose(fp);
		}
		else gprintf("\nCould not write file %s to the SD Card. \n", filename);
		Close_SD();
    }

    if (((int)*outbuf & 0xF0000000) == 0xF0000000) return (int) *outbuf;

    return 0;
}
