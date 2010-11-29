// This file blatantly stolen from:
/****************************************************************************
 * WiiMC (Forwarder)
 * Tantric 2009-2010
 ***************************************************************************/

#include <malloc.h>
#include <stdlib.h>
#include <ogc/machine/processor.h>
#include <sys/iosupport.h>
#include <stdio.h>
#include <string.h>

#include <gccore.h>
#include <ogcsys.h>

#include "LoadDOL.h"

//extern void __exception_closeall();
typedef void (*entrypoint) (void);
u32 load_dol_image (void *dolstart, struct __argv *argv);

static ssize_t __out_write(struct _reent *r, int fd, const char *ptr, size_t len)
{
	return -1;
}

const devoptab_t phony_out = 
{ "stdout",0,NULL,NULL,__out_write,
  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL,0,NULL,NULL,NULL,NULL,NULL };



//start shagkur
typedef struct _dolheader {
        u32 text_pos[7];
        u32 data_pos[11];
        u32 text_start[7];
        u32 data_start[11];
        u32 text_size[7];
        u32 data_size[11];
        u32 bss_start;
        u32 bss_size;
        u32 entry_point;
} dolheader;

u32 LoadDOL::load_dol_image(void *dolstart, struct __argv *argv)
{
        u32 i;
        dolheader *dolfile;

        if (!dolstart)
                return 0;

        dolfile = (dolheader *) dolstart;
        for (i = 0; i < 7; i++)
        {
                if ((!dolfile->text_size[i]) || (dolfile->text_start[i] < 0x100))
                        continue;

                memmove((void *) dolfile->text_start[i], dolstart
                        + dolfile->text_pos[i], dolfile->text_size[i]);

                DCFlushRange ((void *) dolfile->text_start[i], dolfile->text_size[i]);
                ICInvalidateRange((void *) dolfile->text_start[i], dolfile->text_size[i]);
        }

        for (i = 0; i < 11; i++)
        {
                if ((!dolfile->data_size[i]) || (dolfile->data_start[i] < 0x100))
                        continue;

                memmove((void *) dolfile->data_start[i], dolstart
                                + dolfile->data_pos[i], dolfile->data_size[i]);

                DCFlushRange((void *) dolfile->data_start[i],
                                dolfile->data_size[i]);
        }

        if (argv && argv->argvMagic == ARGV_MAGIC)
        {
                void *new_argv = (void *) (dolfile->entry_point + 8);
                memmove(new_argv, argv, sizeof(*argv));
                DCFlushRange(new_argv, sizeof(*argv));
        }
        return dolfile->entry_point;
}
// end shagkur

int LoadDOL::automain(char* filename)
{
	void *buffer = (void *)0x92000000;
	devoptab_list[STD_OUT] = &phony_out; // to keep libntfs happy
	devoptab_list[STD_ERR] = &phony_out; // to keep libntfs happy

	int i,j;

	// mount devices and look for file
	char filepath[1024] = { 0 };
	FILE *fp = NULL;

	strcpy(filepath, filename);
	fp = fopen(filepath, "rb");

	if(!fp)
	{
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
	}
found:
	fseek (fp, 0, SEEK_END);
	int len = ftell(fp);
	fseek (fp, 0, SEEK_SET);
	fread(buffer, 1, len, fp);
	fclose (fp);
	//UnmountAllDevices();
	//USB_Deinitialize();

	// load entry point
	struct __argv args;
	bzero(&args, sizeof(args));
	args.argvMagic = ARGV_MAGIC;
	args.length = strlen(filepath) + 2;
	args.commandLine = (char*)malloc(args.length);
	strcpy(args.commandLine, filepath);
	args.commandLine[args.length - 1] = '\0';
	args.argc = 1;
	args.argv = &args.commandLine;
	args.endARGV = args.argv + 1;

	entrypoint exeEntryPoint = (entrypoint)load_dol_image(buffer, &args);
	
	if(!exeEntryPoint)
	{
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
	}

	VIDEO_WaitVSync();
	VIDEO_WaitVSync();

	u32 level;
	SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
	_CPU_ISR_Disable(level);
	//__exception_closeall();
	exeEntryPoint();
	_CPU_ISR_Restore(level);
	return 0;
}
