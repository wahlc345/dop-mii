#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/dir.h>
#include <gccore.h>

#include "tools.h"
#include "gecko.h"

bool FileExists(const char* path)
{

	FILE *fp = NULL;
	fp = fopen(path, "r");
	if (fp)
	{
		fclose(fp);
		fp = NULL;
		return true;
	}

	return false;
}

int FileRead(const char* path, const char* mode, u8 **outbuf, u32 *outlen)
{
	int ret;

	FILE *fp = NULL;
	fp = fopen(path, mode);
	if (fp)
	{
		gprintf("FileRead = %s\n", path);
		fseek(fp, 0, SEEK_END);
		*outlen = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		*outbuf = AllocateMemory(*outlen);

		if (*outbuf == NULL) 
		{
			gcprintf("Out of memory: Size %d\n", *outlen);
			ret = -1;
		}		

		if (fread(*outbuf, *outlen, 1, fp) != 1) ret = -2; // Failed to read the file so return an error
		else ret = 1; // File successfully loaded so return

		fclose(fp);
		fp = NULL;
		return ret;
	}
	else return -1;
}

bool FolderCreateTree(const char *fullpath) 
{    
    char dir[300];
    char *pch = NULL;
    u32 len;
    struct stat st;

    strlcpy(dir, fullpath, sizeof(dir));
    len = strlen(dir);
    if (len && len< sizeof(dir)-2 && dir[len-1] != '/');
    {
        dir[len++] = '/';
        dir[len] = '\0';
    }
    if (stat(dir, &st) != 0) // fullpath not exist?
	{ 
        while (len && dir[len-1] == '/') dir[--len] = '\0';	// remove all trailing /
        pch = strrchr(dir, '/');
        if (pch == NULL) return false;
        *pch = '\0';
        if (FolderCreateTree(dir)) 
		{
            *pch = '/';
            if (mkdir(dir, 0777) == -1) return false;
        } 
		else return false;
    }
    return true;
}