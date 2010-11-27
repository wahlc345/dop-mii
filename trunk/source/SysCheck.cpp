#include <stdio.h>
#include <cstdlib>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <gccore.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <wiiuse/wpad.h>
#include <stdexcept>

#include "Error.h"
#include "SU_ticket_dat.h"
#include "SysCheck.h"
#include "Tools.h"
#include "Nand.h"
#include "Gecko.h"
#include "Global.h"
#include "System.h"

using namespace IO;

bool SysCheck::CheckFlashAccess()
{
	gprintf("CheckFlashAccess::Nand::OpenRead(/dev/flash) = ");
	int ret = Nand::OpenRead("/dev/flash");
	gprintf("%s\n", NandError::ToString(ret));
	if (ret < 0) return false;
	Nand::Close(ret);
	return true;
}

bool SysCheck::CheckFakeSign()
{
	// We are expecting an error here, but depending on the error it will mean
	// that it is valid for fakesign. If we get -2011 it is definately not patched with fakesign.
	// Lower Revisions will return -106 if it can fakesign	

	gprintf("CheckFakeSign::ES_AddTicket = ");
	int ret = ES_AddTicket((signed_blob*)SU_ticket_dat, SU_ticket_dat_size, System::Cert, System::Cert.Size, NULL, 0);
	gprintf("%s\n", EsError::ToString(ret));
	if (ret > -1) RemoveBogusTicket();
	return (ret > -1 || ret == -1028 || ret == -106);
}

bool SysCheck::CheckEsIdentify()
{
	gprintf("CheckESIdentify::ES_Identify = ");	
	int ret = Identify::AsSuperUser();
	gprintf("%s\n", EsError::ToString(ret)); 
	return (ret > -1);
}

bool SysCheck::CheckNandPermissions()
{
	gprintf("CheckNandPermissions::Nand::OpenReadWrite(/title/00000001/00000002/content/title.tmd) = ");
	int ret = Nand::OpenReadWrite("/title/00000001/00000002/content/title.tmd");
	gprintf("%s\n", NandError::ToString(ret));
	Nand::Close(ret);
	if (ret < 0) return false;
	return true;
}

int SysCheck::RemoveBogusTicket()
{
	int ret = 0;
	u64 titleId = 0x100000000ULL;
	u32 views;
	tikview *viewdata = NULL;
	char filename[30] = "/ticket/00000001/00000000.tik";

	// We going to try the nice way first
	ret = ES_GetNumTicketViews(titleId, &views);
	if (ret < 0) 
	{
		gprintf("\n>> ERROR! ES_GetNumTicketViews: %s\n", EsError::ToString(ret));
		return ret;
	}

	if (!views) { gprintf("Bogus Ticket Not Found\n"); return 0; }

	viewdata = (tikview*)Tools::AllocateMemory(sizeof(tikview) * views);
	if (!viewdata) 
	{
		gcprintf("\n>>ERROR! Out Of Memory\n");
		viewdata = NULL; 
		ret = -1;
		goto end;
	}
	ret = ES_GetTicketViews(titleId, viewdata, views);
	if (ret < 0) 
	{
		gprintf("\n>> ERROR! ES_GetTicketViews: %s\n", EsError::ToString(ret));
		goto end;
	}

	for (u32 i = 0; i < views; i++) 
	{
		tikview v = viewdata[i];
		if (viewdata[i].titleid == titleId) 
		{
			ret = ES_DeleteTicket(&viewdata[i]);
			break;
		}
	}	

end:
	// Ticket Not Found
	if (ret == -106) ret = 0;

	// If nice way fails lets try forcing it
	if (ret < 0) 
	{
		ret = Nand::Delete(filename);
		if (ret < 0) gprintf("\n>> ERROR! Nand::Delete: %s\n", NandError::ToString(ret));
	}

	delete viewdata; viewdata = NULL;
	return ret;
}

int SysCheck::RemoveBogusTitle()
{
	int ret = 0;
	u64 titleId = 0x100000000ULL;

	// Lets try the nice way first
	ret = ES_DeleteTitleContent(titleId);
	if (ret > -1) ret = ES_DeleteTitle(titleId);

	// Title Not Found
	if (ret == -106) ret = 0;

	// We asked the nice way. Let's try doing it manually
	if (ret < 0)
	{
		ret = Nand::Delete("/title/00000001/00000000");
		if (ret < 0) gprintf("\n>> ERROR! Nand::Delete Failed: %s\n", EsError::ToString(ret));
	}

	return ret;
}

// thanks to Double_A
// Get the system menu version from TMD
u32 SysCheck::GetSysMenuVersion(void)
{
	static u64 TitleID ATTRIBUTE_ALIGN(32) = 0x0000000100000002LL;
	static u32 tmdSize ATTRIBUTE_ALIGN(32);

	// Get the stored TMD size for the system menu
	if (ES_GetStoredTMDSize(TitleID, &tmdSize) < 0) return false;

	signed_blob *TMD = (signed_blob *)memalign(32, (tmdSize+32)&(~31));
	memset(TMD, 0, tmdSize);

	// Get the stored TMD for the system menu
	if (ES_GetStoredTMD(TitleID, TMD, tmdSize) < 0) return false;

	// Get the system menu version from TMD
	tmd *rTMD = (tmd *)(TMD+(0x140/sizeof(tmd *)));
	u32 version = rTMD->title_version;

	free(TMD);

	// Return the system menu version
	return version;
}

float SysCheck::GetSysMenuFriendlyVersion(void) 
{
	float ninVersion = 0.0;
	
	switch (GetSysMenuVersion())
	{
		case 33:
			ninVersion = 1.0f;
			break;

		case 128:
		case 97:
		case 130:
			ninVersion = 2.0f;
			break;

		case 162:
			ninVersion = 2.1f;
			break;
			
		case 192:
		case 193:
		case 194:
			ninVersion = 2.2f;
			break;
			
		case 224:
		case 225:
		case 226:
			ninVersion = 3.0f;
			break;
			
		case 256:
		case 257:
		case 258:
			ninVersion = 3.1f;
			break;
			
		case 288:
		case 289:
		case 290:
			ninVersion = 3.2f;
			break;
			
		case 352:
		case 353:
		case 354:
		case 326:
			ninVersion = 3.3f;
			break;
			
		case 384:
		case 385:
		case 386:
			ninVersion = 3.4f;
			break;
			
		case 390:
			ninVersion = 3.5f;
			break;
		
		case 416:
		case 417:
		case 418:
			ninVersion = 4.0f;
			break;
		
		case 448:
		case 449:
		case 450:
		case 454:
			ninVersion = 4.1f;
			break;
			
		case 480:
		case 481:
		case 482:
		case 486:
			ninVersion = 4.2f;
			break;
			
		case 512:
		case 513:
		case 514:
		case 518:
			ninVersion = 4.3f;
			break;
    }
	
	return ninVersion;
}

// Check if this is an IOS stub (according to WiiBrew.org)
bool SysCheck::IsKnownStub(u32 noIOS, s32 noRevision)
{
	// TODO: make this use Titles.xml
	if (noIOS ==   3 && noRevision == 65280) return true;
	if (noIOS ==   4 && noRevision == 65280) return true;
	if (noIOS ==  10 && noRevision ==   768) return true;
	if (noIOS ==  11 && noRevision ==   256) return true;
	if (noIOS ==  16 && noRevision ==   512) return true;
	if (noIOS ==  20 && noRevision ==   256) return true;
	if (noIOS ==  30 && noRevision ==  2816) return true;
	if (noIOS ==  40 && noRevision ==  3072) return true;
	if (noIOS ==  50 && noRevision ==  5120) return true;
	if (noIOS ==  51 && noRevision ==  4864) return true;
	if (noIOS ==  52 && noRevision ==  5888) return true;
	if (noIOS ==  60 && noRevision ==  6400) return true;
	if (noIOS ==  70 && noRevision ==  6912) return true;
	if (noIOS == 222 && noRevision == 65280) return true;
	if (noIOS == 223 && noRevision == 65280) return true;
	if (noIOS == 249 && noRevision == 65280) return true;
	if (noIOS == 250 && noRevision == 65280) return true;
	if (noIOS == 254 && noRevision ==     2) return true;
	if (noIOS == 254 && noRevision ==     3) return true;
	if (noIOS == 254 && noRevision ==   260) return true;
	
	
	// BootMii As IOS is installed on IOS254 rev 31338
	if (noIOS == 254 && (noRevision == 31338 || noRevision == 65281)) return true;

	return false;
}
// end blatant copy-paste
