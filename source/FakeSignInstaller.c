#include <stdio.h>
#include <gccore.h>

#include "FakeSignInstaller.h"
#include "../build/cert_sys.h"
#include "controller.h"
#include "tools.h"
#include "video.h"
#include "signcheckfunctions.h"
#include "IOSPatcher.h"
#include "wiibasics.h"
#include "gecko.h"

int FakeSignInstall()
{	
	int ret;

	// Let's see if we are already on IOS36, if not then reload to IOS36
	if (IOS_GetVersion() != 36) 
	{
		gcprintf("Current IOS is not 36. Loading IOS36...\n");
		ret = ReloadIos(36);
		if (ret < 0)
		{
			gcprintf("ERROR: Failed to load IOS36 (%d)", ret);
			return ret;
		}
	}

	gprintf("Getting certs.sys from the NAND to test for FakeSign...");
	ret = GetCert();
	if (ret < -1) 
	{
		gcprintf("Error (%d)\n", ret);
		return ret;
	}		
	gprintf("\n");
	
	// Lets see if version is already fakesigned
	gprintf("Checking FakeSign on IOS36\n");
	if (CheckFakeSign())
	{
		gcprintf("\n\nIOS36 already has FakeSign applied.\n");
		return -1;
	}
	gprintf("CheckFakeSign = %d", ret);	

	// Downgrade IOS 15
	gcprintf("*** Downgrading IOS15 to r257 ***\n");
	ret = IosDowngrade(15, IOS15Version, 257);
	if (ret < 0)
	{
		gcprintf("ERROR: Downgrade IOS15 Failed (%d)\n", ret);
		return ret;
	}

	// Install IOS36 but ask if they want ES_Identity and NAND Patches installed.
	gcprintf("\n*** Installing IOS36 (r%d) w/FakeSign ***\n", IOS36Version);
	// Load IOS 15
	gcprintf("Loading IOS15 (r257)...");	
	SpinnerStart();
	ret = ReloadIos(15);
	SpinnerStop();
	if (ret < 0)
	{
		gcprintf("ERROR: Failed to Load IOS15 (%d)", ret);
		return ret;
	}
	gcprintf("\b.Done\n");

	printf("\nApply ES_Identify Patch to IOS36?\n");
	bool esIdentifyPatch = PromptYesNo();

	printf("\nApply NAND Permission Patch to IOS36?\n");
	bool nandPatch = PromptYesNo();

	gcprintf("\n");
	ret = IosInstall(36, IOS36Version, true, esIdentifyPatch, nandPatch);
	if (ret < 0)
	{
		gcprintf("Failed to install IOS36 (%d)", ret);
		return ret;
	}

	// Load IOS 36 to restore IOS 15
	gcprintf("\n*** Restoring IOS15 (r%d) ***\n", IOS15Version);
	gcprintf("Loading IOS36 (r%d)...", IOS36Version);
	SpinnerStart();
	ret = ReloadIos(36);
	SpinnerStop();
	if (ret < 0)
	{
		gcprintf("ERROR: Failed to Load IOS36 (%d)", ret);
		return ret;
	}
	gcprintf("\b.Done\n");

	ret = IosInstallUnpatched(15, IOS15Version);
	if (ret < 0)
	{
		gcprintf("Failed to reinstall IOS15. (%d)", ret);
		return ret;
	}

	return 1;
}