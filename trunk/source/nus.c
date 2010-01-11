#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ogc/es.h>

#include "http.h"
#include "tools.h"
#include "network.h"
#include "gecko.h"
#include "FileSystem.h"

extern bool networkInitialized;

int GetNusObject(u32 titleid1, u32 titleid2, u16 *version, char *content, u8 **outbuf, u32 *outlen) 
{
	
	char buf[200] = "";
	char filename[256] = "";
    int ret = 0;	
    uint httpStatus = 200;	
	FILE *fp = NULL;

	void _extractTmdVersion()
	{
		gprintf("Getting Version from TMD File\n");

		tmd *pTMD = (tmd *)SIGNATURE_PAYLOAD((signed_blob *)*outbuf);
		*version = pTMD->title_version;
		sprintf(content, "%s.%u", content, *version);
		snprintf(filename, sizeof(filename), "sd:/%08X/%08X/v%u/%s", titleid1, titleid2, *version, content);
		gprintf("New Filename = %s\n", filename);
	}
	
	gprintf("\nTitleID1 = %08X, TitleID2 = %08X, Content = %s, Version = %u\n", titleid1, titleid2, content, *version);

	// If version is = 0, check for existence sd:/XXX/XXX/tmd
	// This is used for NUS offline mode
	if (strcmpi(content, "TMD") == 0 && *version == 0 && Init_SD())
	{
		snprintf(filename, sizeof(filename), "sd:/%08X/%08X/tmd", titleid1, titleid2);
		gprintf("Looking on SD card for (%s).\n", filename);
		if (FileExists(filename))
		{
			ret = FileReadBinary(filename, outbuf, outlen);
			_extractTmdVersion();
			if (ret > 0) return ret;
		}
		else gprintf("File (%s) not found.\n", filename);

		Close_SD();
	}
	//else gprintf("No Card Found or content != TMD or Version != 0\n");

	// If Version != 0 then we know what version of a file we want
	// so continue normal operations
	snprintf(filename, sizeof(filename), "sd:/%08X/%08X/v%u/%s", titleid1, titleid2, *version, content);

    if (*version != 0 && Init_SD()) 
	{		
		ret = FileReadBinary(filename, outbuf, outlen);
		Close_SD();
		if (ret > 0) return ret; 
		gprintf("\n* %s not found on the SD card.", filename);
		gprintf("\n* Trying to download it from the internet...\n");
	}
	//else gprintf("No Card Found or Version == 0\n");
    	
	gprintf("NusGetObject::NetworkInit...");
	NetworkInit();
	gprintf("Done\n");
    
    snprintf(buf, 128, "http://%s%s%08x%08x/%s", NusHostname, NusPath, titleid1, titleid2, content);
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
		_extractTmdVersion();
	}

    if (*version != 0 && Init_SD()) 
	{
		char folder[300] = "";
		snprintf(folder, sizeof(folder), "sd:/%08X/%08X/v%u", titleid1, titleid2, *version);
		gprintf("FolderCreateTree() = %s ", (FolderCreateTree(folder)? "True" : "False"));
		
		gprintf("\nWriting File (%s)\n", filename);

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
