/*-------------------------------------------------------------

dopios.c

Dop-IOS MOD - A modification of Dop-IOS by Arikado, giantpune, Lunatik, SifJar, and PhoenixTank

Dop-IOS - install and patch any IOS by marc

Based on tona's shop installer (C) 2008 tona

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gccore.h>
#include <malloc.h>
#include <wiiuse/wpad.h>

#include "controller.h"
#include "Error.h"
#include "FakeSignInstaller.h"
#include "gecko.h"
#include "IOSPatcher.h"
#include "nand.h"
#include "network.h"
#include "patchmii_core.h"
#include "signcheckfunctions.h"
#include "svnrev.h"
#include "sys.h"
#include "title.h"
#include "title_install.h"
#include "tools.h"
#include "video.h"
#include "wiibasics.h"

#include "dopios.h"

typedef enum _ios_state_
{
	IOSSTATE_PROTECTED,
	IOSSTATE_NORMAL,
	IOSSTATE_STUB_NOW,
	IOSSTATE_LATEST
} IosState, *PIosState;

/*#define PROTECTED	0
#define NORMAL		1
#define STUB_NOW	2
#define LATEST		3*/

#define REGIONS_LEN		(sizeof(Regions) / sizeof(Region))
#define REGIONS_LO		0
#define REGIONS_HI		REGIONS_LEN - 1

#define SYSTEMMENUS_LEN (sizeof(SystemMenus) / sizeof(SystemMenu))
#define SYSTEMMENUS_LO	0
#define SYSTEMMENUS_HI	SYSTEMMENUS_LEN - 1

#define CHANNELS_LEN	(sizeof(Channels) / sizeof(Channel))
#define CHANNELS_LO		0
#define CHANNELS_HI		CHANNELS_LEN - 1

#define IOSINFO_LEN		(sizeof(IosInfoList) / sizeof(IosInfo))
#define IOSINFO_LO		0
#define IOSINFO_HI		IOSINFO_LEN - 1

#define CIOS_VERSION 36

#define round_up(x,n)	(-(-(x) & -(n)))

// Variables
bool ExitMainThread = false;
u32 ios_found[256];
char logBuffer[1024*1024];//For signcheck
u32 iosTable[256];//For signcheck

// Prevent IOS36 loading at startup
s32 __IOS_LoadStartupIOS()
{
	return 0;
}

s32 __u8Cmp(const void *a, const void *b)
{
	return *(u8 *)a-*(u8 *)b;
}


typedef struct Region
{
    u32 id;
    const char Name[30];
} ATTRIBUTE_PACKED Region;

const struct Region Regions[] = 
{
    {0, "North America (U)"},
    {1, "Europe (E)"},
    {2, "Japan (J)"},
    {3, "Korea (K)"}
};

typedef struct SystemMenu
{
    const char Name[30];
} ATTRIBUTE_PACKED SystemMenu;

const struct SystemMenu SystemMenus[] = 
{
	//VERSION#1, VERSION#2, VERSION#3 NAME
    {"System Menu 3.2"},
    {"System Menu 3.3"},
    {"System Menu 3.4"},
    {"System Menu 3.5 (Korea Only)"},
    {"System Menu 4.0"},
    {"System Menu 4.1"},
    {"System Menu 4.2"}
};

typedef struct Channel 
{
    const char Name [30];
} ATTRIBUTE_PACKED Channel;

const struct Channel Channels[] =
{
    {"Shop Channel"},
    {"Photo Channel 1.0"},
    {"Photo Channel 1.1"},
    {"Mii Channel"},
    {"Internet Channel"},
    {"News Channel"},
    {"Weather Channel"}
};

typedef struct IosInfo
{
    u32 Version;
    u32 LowRevision;
    u32 HighRevision;
    IosState State;
    const char Description[512];
} ATTRIBUTE_PACKED IosInfo;

const struct IosInfo IosInfoList[] =
{
	// IOS# OLDVERSION# NEWVERSION#, ???, Description -- Arikado
    {4,65280,65280,IOSSTATE_PROTECTED,"Stub, useless now."},
	{9,520,778,IOSSTATE_NORMAL,"Used by launch titles (IE: Zelda: Twilight Princess, Wii Sports)\nand System Menu 1.0."},
    {10,768,768,IOSSTATE_PROTECTED,"Stub, useless now."},
    {11,10,256,IOSSTATE_STUB_NOW,"Used only by System Menu 2.0 and 2.1."},
    {12,6,269,IOSSTATE_NORMAL,""},
    {13,10,273,IOSSTATE_NORMAL,""},
    {14,262,520,IOSSTATE_NORMAL,""},
    {15,257,523,IOSSTATE_NORMAL,""},
    {16,512,512,IOSSTATE_PROTECTED,"Piracy prevention stub, useless."},
    {17,512,775,IOSSTATE_NORMAL,""},
    {20,12,256,IOSSTATE_STUB_NOW,"Used only by System Menu 2.2."},
    {21,514,782,IOSSTATE_NORMAL,"Used by old third-party titles (No More Heroes)."},
    {22,777,1037,IOSSTATE_NORMAL,""},
    {28,1292,1550,IOSSTATE_NORMAL,""},
    {30,1040,2816,IOSSTATE_STUB_NOW,"Used only by System Menu 3.0, 3.1, 3.2 and 3.3."},
    {31,1040,3349,IOSSTATE_NORMAL,""},
    {33,1040,3091,IOSSTATE_NORMAL,""},
    {34,1039,3348,IOSSTATE_NORMAL,""},
    {35,1040,3349,IOSSTATE_NORMAL,"Used by various games including Super Mario Galaxy, Call Of Duty, Wii Music."},
    {36,1042,3351,IOSSTATE_NORMAL,"Used by: Smash Bros. Brawl, Mario Kart Wii.\nCan be ES_Identify patched."},
    {37,2070,3869,IOSSTATE_NORMAL,"Used mostly by music games like Guitar/Band Hero and Rock Band."},
    {38,3610,3867,IOSSTATE_NORMAL,"Used by some modern titles (Animal Crossing)."},
    {50,4889,5120,IOSSTATE_STUB_NOW,"Used only by System Menu 3.4."},
    {51,4633,4864,IOSSTATE_STUB_NOW,"Used only by Shop Channel 3.4."},
    {53,4113,5406,IOSSTATE_NORMAL,"Used by some modern games and channels."},
    {55,4633,5406,IOSSTATE_NORMAL,"Used by some modern games and channels."},
    {56,4890,5405,IOSSTATE_NORMAL,"Used by Wii Speak Channel and some newer WiiWare"},
    {57,5404,5661,IOSSTATE_NORMAL,"Unknown yet."},
    {60,6174,6400,IOSSTATE_STUB_NOW,"Used by System Menu 4.0 and 4.1."},
    {61,4890,5405,IOSSTATE_NORMAL,"Used by Shop Channel 4.x."},
    {70,6687,6687,IOSSTATE_LATEST,"Used by System Menu 4.2."},
    {222,65280,65280,IOSSTATE_PROTECTED,"Piracy prevention stub, useless."},
    //{222,65280,65280,NORMAL,"Piracy prevention stub, useless."},
    {223,65280,65280,IOSSTATE_PROTECTED,"Piracy prevention stub, useless."},
    {249,65280,65280,IOSSTATE_PROTECTED,"Piracy prevention stub, useless."},
    {250,65280,65280,IOSSTATE_PROTECTED,"Piracy prevention stub, useless."},
    {254,2,260,IOSSTATE_PROTECTED,"PatchMii prevention stub, useless."}
};

void PrintBanner() 
{	
	ClearScreen();
	Console_SetColors(RED, 0, WHITE, 0);
	
	printf("%*s", ConsoleCols, " ");	
	
	//Console_BlinkSlow(WHITE);
	char text[ConsoleCols];
	sprintf(text, "DOP-IOS MOD v11 (SVN r%s)", SVN_REV_STR);
	PrintCenter(text, ConsoleCols);

	printf("Cooooolllll");
	ClearLine();

    printf("%*s", ConsoleCols, " ");
	
	Console_SetColors(BLACK, 0, WHITE, 0);
	printf("\n");
}

void addLogEntry(int iosNumber, int iosVersion, int trucha, int flash, int boot2, int usb2)
{
	char tmp[1024];
	sprintf(tmp, "\"IOS%i (ver %i)\", %s, %s, %s, %s\n"
		, iosNumber
		, iosVersion
		, ((trucha) ? "Enabled" : "Disabled")
		, ((flash) ? "Enabled" : "Disabled")
		, ((boot2) ? "Enabled" : "Disabled")
		, ((usb2) ? "Enabled" : "Disabled")
	);
 
	strcat(logBuffer, tmp);
}

int ScanIos()
{
	int ret;
	u32 titlesCount, tmdSize, iosFound;
	u64 *titles;
	tmd *pTMD;
	u8 *tmdBuffer;
 
	ES_GetNumTitles(&titlesCount);
	titles = memalign(32, titlesCount * sizeof(u64));
	ES_GetTitles(titles, titlesCount);	
 
	iosFound = 0;
 
	u32 i;
	for (i = 0; i < titlesCount; i++)
	{
		if (titles[i] >> 32 != 1) continue; // skip non-system titles
		
		// skip system menu, BC, MIOS and possible other non-IOS titles
		u32 titleId = titles[i] & 0xFFFFFFFF;
		if (titleId == 2 || titleId > 0xFF) continue; // skipping System Menu, BC and possible other non-IOS Titles

		// check if this is just a stub
		ret = ES_GetStoredTMDSize(1LL<<32 | titleId, &tmdSize);
		if (ret < 0) continue;

		tmdBuffer = memalign(32, tmdSize);
		ret = ES_GetStoredTMD(1LL<<32 | titleId, (signed_blob *)tmdBuffer, tmdSize);
		if (ret < 0) continue;
 
		pTMD = SIGNATURE_PAYLOAD((signed_blob *)tmdBuffer);
		u16 numContents = pTMD->num_contents;
		free(tmdBuffer);
		
		if (numContents == 1) continue; // Is A Stub
		if (numContents == 3) continue; // May be a stub so we are going to skip

		iosTable[iosFound] = titleId;
		iosFound++;
	}
	
	// Adding IOS15 to the list as it is safe to test
	iosTable[iosFound] = 0xF;	
	iosFound++;
 
	qsort (iosTable, iosFound, sizeof(u32), sortCallback);
 
	return iosFound;
}

bool getMyIOS() 
{
    s32 ret;
    u32 count, i;
    for (i=0;i<256;i++) ios_found[i]=0;
    
    PrintBanner();
    printf("Loading installed IOS...");

    ret=ES_GetNumTitles(&count);
    if (ret) 
	{
        gcprintf("ERROR: ES_GetNumTitles = %s\n", EsErrorCodeString(ret));
        return false;
    }

    static u64 title_list[256] ATTRIBUTE_ALIGN(32);
    ret=ES_GetTitles(title_list, count);
    if (ret) 
	{
        gcprintf("ERROR: ES_GetTitles = %s\n", EsErrorCodeString(ret));
        return false;
    }

    for (i=0;i<count;i++) 
	{
        u32 tmd_size;
        ret=ES_GetStoredTMDSize(title_list[i], &tmd_size);
        static u8 tmd_buf[MAX_SIGNED_TMD_SIZE] ATTRIBUTE_ALIGN(32);
        signed_blob *s_tmd=(signed_blob *)tmd_buf;
        ret=ES_GetStoredTMD(title_list[i], s_tmd, tmd_size);

        tmd *t=SIGNATURE_PAYLOAD(s_tmd);
        u32 tidh=t->title_id >> 32;
        u32 tidl=t->title_id & 0xFFFFFFFF;

        if (tidh==1 && t->title_version) ios_found[tidl]=t->title_version;

		if (i%2==0) printf(".");
    }
    return true;
}

void doparIos(struct IosInfo ios, bool newest) 
{
    PrintBanner();

    printf("Are you sure you want to install ");
	Console_SetFgColor(YELLOW, 1);
    printf("IOS%u", ios.Version);
	printf(" v%u", (newest ? ios.HighRevision : ios.LowRevision));
	Console_SetFgColor(WHITE, 0);
    printf("?\n");
	
    if (PromptContinue()) 
	{
        bool sigCheckPatch = false;
        bool esIdentifyPatch = false;
		bool nandPatch = false;
        if (ios.Version > 36 || newest) 
		{
            printf("\nApply FakeSign patch to IOS%u?\n", ios.Version);
            sigCheckPatch = PromptYesNo();
            if (ios.Version == 36) 
			{				
                printf("\nApply ES_Identify patch in IOS%u?\n", ios.Version);
                esIdentifyPatch = PromptYesNo();
            }

			if (ios.Version == 36 || ios.Version == 37) 
			{
				printf("\nApply NAND Permissions Patch?\n");
				nandPatch = PromptYesNo();
			}
        }

		int ret = 0;
		if (newest) ret = IosInstall(ios.Version, ios.HighRevision, sigCheckPatch, esIdentifyPatch, nandPatch);
		else ret = IosDowngrade(ios.Version, ios.HighRevision, ios.LowRevision);
        
        if (ret < 0) 
		{
            printf("ERROR: Something failed. (ret: %d)\n",ret);
            printf("Continue?\n");
			if (!PromptContinue()) ReturnToLoader();
        }

        getMyIOS();
    }
}

s32 __Menu_IsGreater(const void *p1, const void *p2) 
{

    u32 n1 = *(u32 *)p1;
    u32 n2 = *(u32 *)p2;

    /* Equal */
    if (n1 == n2) return 0;

    return (n1 > n2) ? 1 : -1;
}

s32 Title_GetIOSVersions(u8 **outbuf, u32 *outlen) 
{

    u8 *buffer = NULL;
    u64 *list = NULL;

    u32 count, cnt, idx;
    s32 ret;

    /* Get title list */
    ret = Title_GetList(&list, &count);
    if (ret < 0) return ret;

    /* Count IOS */
    for (cnt = idx = 0; idx < count; idx++) 
	{
        u32 tidh = (list[idx] >> 32);
        u32 tidl = (list[idx] & 0xFFFFFFFF);

        /* Title is IOS */
        if ((tidh == 0x1) && (tidl >= 3) && (tidl <= 255)) cnt++;
    }

    /* Allocate memory */
    buffer = (u8 *)memalign(32, cnt);
    if (!buffer) 
	{
        ret = -1;
        goto out;
    }

    /* Copy IOS */
    for (cnt = idx = 0; idx < count; idx++) 
	{
        u32 tidh = (list[idx] >> 32);
        u32 tidl = (list[idx] & 0xFFFFFFFF);

        /* Title is IOS */
        if ((tidh == 0x1) && (tidl >= 3) && (tidl <= 255))
            buffer[cnt++] = (u8)(tidl & 0xFF);
    }

    /* Set values */
    *outbuf = buffer;
    *outlen = cnt;

out:
    /* Free memory */
    if (list) free(list);
    return ret;
}


void InstallTheChosenSystemMenu(int region, int menu) 
{
    s32 ret = 0;

    u64 sysTid = 0x100000002ULL;
    u16 sysVersion = 0;
    static signed_blob *sysTik = NULL, *sysTmd = NULL;

	//Initialize Network
    Network_Init();

	//Initialize NAND now instead of earlier -- Thanks WiiPower
    fflush(stdout);
    s32 nandret;
    nandret = Nand_Init();

    //North America
    if (region == 0) 
	{        
        if (menu == 0) sysVersion = 289; //3.2
        if (menu == 1) sysVersion = 353; //3.3        
        if (menu == 2) sysVersion = 385; //3.4        
        if (menu == 4) sysVersion = 417; //4.0        
        if (menu == 5) sysVersion = 449; //4.1
        if (menu == 6) sysVersion = 481; //4.2
    }

    //Europe
    if (region == 1) 
	{
        if (menu == 0) sysVersion = 290; //3.2        
        if (menu == 1) sysVersion = 354; //3.3        
        if (menu == 2) sysVersion = 386; //3.4        
        if (menu == 4) sysVersion = 418; //4.0        
        if (menu == 5) sysVersion = 450; //4.1
        if (menu == 6) sysVersion = 482; //4.2
    }

    //Japan
    if (region == 2) 
	{        
        if (menu == 0) sysVersion = 288; //3.2        
        if (menu == 1) sysVersion = 352; //3.3
        if (menu == 2) sysVersion = 384; //3.4
        if (menu == 4) sysVersion = 416; //4.0        
        if (menu == 5) sysVersion = 448; //4.1        
        if (menu == 6) sysVersion = 480; //4.2
    }

    //Korea
    if (region == 3) 
	{
        //Incorrect Version Numbers        
        if (menu == 1) sysVersion = 326; //3.3V        
        if (menu == 3) sysVersion = 390; //3,5        
        if (menu == 5) sysVersion = 454; //4.1        
        if (menu == 6) sysVersion = 486; //4.2
    }

    //Download the System Menu
	printf("\n");
    printf("Downloading System Menu. This may take awhile...\n");
    ret = Title_Download(sysTid, sysVersion, &sysTik, &sysTmd);
    if (ret < 0) 
	{
        printf("\nError Downloading System Menu. Ret: %d\n", ret);
		printf("\nPress any button to continue.");
		WaitAnyKey();
    }

    //Install the System Menu
    printf("Installing System Menu. Do NOT turn the power off. This may take awhile...\n");
    ret = Title_Install(sysTik, sysTmd);
    if (ret < 0) 
	{
        printf("\nError Installing System Menu. Ret: %d\n", ret);
		printf("\nPress any button to continue.");
		WaitAnyKey();
    }

    //Close the NAND
    fflush(stdout);
    ISFS_Deinitialize();
}

void InstallTheChosenChannel(int region, int channel) {

    s32 ret = 0;

	//Shop Channel
    if (channel == 0) 
	{
        printf("\n\nInstalling the Shop Channel...");
		if (region != 3) ret = patchmii_install(0x10002, 0x48414241, 0, 0x10002, 0x48414241, 0, false, false);		
		else  ret = patchmii_install(0x10002, 0x4841424B, 0, 0x10002, 0x4841424B, 0, false, false);
        if (ret < 0) printf("\nError: %d", ret);
        else printf("\nShop Channel successfully installed!");
    }

	//Photo Channel 1.0
    if (channel == 1) 
	{
        printf("\n\nInstalling the Photo Channel 1.0...");
        ret = patchmii_install(0x10002, 0x48414141, 0, 0x10002, 0x48414141 , 0, false, false);
        if (ret < 0) printf("\nError: %d", ret);
        else printf("\nPhoto Channel 1.0 successfully installed!");
    }

	//Photo Channel 1.1
    if (channel == 2) 
	{
        printf("\n\nInstalling the Photo Channel 1.1...");
		if (region != 3) ret = patchmii_install(0x10002, 0x48415941, 0, 0x10002, 0x48415941, 0, false, false);
		else ret = patchmii_install(0x10002, 0x4841594B, 0, 0x10002, 0x4841594B, 0, false, false);
        if (ret < 0) printf("\nError: %d", ret);
        else printf("\nPhoto Channel 1.1 successfully installed!");
    }

	//Mii Channel
    if (channel == 3) 
	{
        printf("\n\nInstalling the Mii Channel...");
        ret = patchmii_install(0x10002, 0x48414341, 0, 0x10002, 0x48414341, 0, false, false);
        if (ret < 0) printf("\nError: %d", ret);
        else printf("\nMii Channel successfully installed!");
    }

	//Internet Channel
    if (channel == 4) 
	{
        printf("\n\nInstalling the Internet Channel...");
        //ret = patchmii_install(0x10001, 0x48414441, 0, 0x10001, 0x48414441, 0, false, false); <-- Supposedly causes failure
        if (region == 0) ret = patchmii_install(0x10001, 0x48414445, 0, 0x10001, 0x48414445, 0, false, false);
        if (region == 1) ret = patchmii_install(0x10001, 0x48414450, 0, 0x10001, 0x48414450, 0, false, false);
        if (region == 2) ret = patchmii_install(0x10001, 0x4841444A, 0, 0x10001, 0x4841444A, 0, false, false);
        if (region == 3) ret = patchmii_install(0x10001, 0x4841444B, 0, 0x10001, 0x4841444B, 0, false, false);
        if (ret < 0) printf("\nError: %d", ret);
        else printf("\nInternet Channel successfully installed!");
    }

	//News Channel
    if (channel == 5) 
	{
        printf("\n\nInstalling the News Channel...");
        ret = patchmii_install(0x10002, 0x48414741, 0, 0x10002, 0x48414741, 0, false, false);
        if (region == 0) ret = patchmii_install(0x10002,0x48414745,0,0x10002,0x48414745,0,false, false);
        if (region == 1) ret = patchmii_install(0x10002,0x48414750,0,0x10002,0x48414750,0,false, false);
        if (region == 2) ret = patchmii_install(0x10002,0x4841474a,0,0x10002,0x4841474a,0,false, false);
        if (region == 3) ret = patchmii_install(0x10002,0x4841474b,0,0x10002,0x4841474b,0,false, false);
        if (ret < 0) printf("\nError: %d", ret);
        else printf("\nNews Channel successfully installed!");
    }

	//Weather Channel
    if (channel == 6) 
	{
        printf("\n\nInstalling the Weather Channel...");
        ret = patchmii_install(0x10002, 0x48414641, 0, 0x10002, 0x48414641, 0, false, false);
        if (region == 0) ret = patchmii_install(0x10002,0x48414645,0,0x10002,0x48414645,0,false, false);
        if (region == 1) ret = patchmii_install(0x10002,0x48414650,0,0x10002,0x48414650,0,false, false);
        if (region == 2) ret = patchmii_install(0x10002,0x4841464a,0,0x10002,0x4841464a,0,false, false);
        if (region == 3) ret = patchmii_install(0x10002,0x4841464b,0,0x10002,0x4841464b,0,false, false);
        if (ret < 0) printf("\nError: %d", ret);
        else printf("\nWeather Channel successfully installed!");
    }

	//After Installations are done:
    printf("\n\nPress A to continue!");
	u32 button;
	while (ScanPads(&button))
	{		
		if (ExitMainThread) return;
		if (button&WPAD_BUTTON_A) break;
		if (button&WPAD_BUTTON_HOME) ReturnToLoader();
	}
}



int CheckAndRemoveStubs()
{
	int _RemoveStub_(int ios, int stubVersion, u64 titleId)
	{
		int version = Title_GetVersionNObuf(titleId);
		Nand_Init();
		gprintf("\nTitle_GetVersion %d = %d", ios, version);
		if (version == stubVersion)
		{
			PrintBanner();
			printf("OK to Remove stub IOS %d?\n", ios);
			if (PromptYesNo())
			{
				gprintf("\nDeleting Stub IOS %d", ios);
				Uninstall_FromTitle(titleId);
			}
			return 1;
		}

		return 0;
	}	

	int count = 0;	
	count += _RemoveStub_(4, 65280, 0x0000000100000004ll);
	count += _RemoveStub_(10, 768, 0x000000010000000All);
	count += _RemoveStub_(11, 256, 0x000000010000000Bll);
	count += _RemoveStub_(16, 512, 0x0000000100000010ll);
	count += _RemoveStub_(20, 256, 0x0000000100000014ll);
	count += _RemoveStub_(30, 2816, 0x000000010000001Ell);
	count += _RemoveStub_(50, 5120, 0x0000000100000032ll);
	count += _RemoveStub_(51, 4864, 0x0000000100000033ll);
	count += _RemoveStub_(60, 6400, 0x000000010000003Cll);
	count += _RemoveStub_(222, 65280, 0x00000001000000DEll);
	count += _RemoveStub_(223, 65280, 0x00000001000000DFll);
	count += _RemoveStub_(249, 65280, 0x00000001000000F9ll);
	count += _RemoveStub_(250, 65280, 0x00000001000000FAll);
	count += _RemoveStub_(254, 260, 0x00000001000000FEll);
	return count;	
}

u8 *get_ioslist(u32 *cnt)
{
	u64 *buf = 0;
	s32 res;
	u32 i;
	u32 tcnt = 0, icnt;
	u8 *ioses = NULL;
	
	//Get stored IOS versions.
	res = ES_GetNumTitles(&tcnt);
	if(res < 0)
	{
		printf("ERROR: ES_GetNumTitles: %s\n", EsErrorCodeString(res));
		return 0;
	}
	buf = memalign(32, sizeof(u64) * tcnt);
	res = ES_GetTitles(buf, tcnt);
	if(res < 0)
	{
		printf("ERROR: ES_GetTitles: %s\n", EsErrorCodeString(res));
		if (buf) free(buf);
		return 0;
	}

	icnt = 0;
	for(i = 0; i < tcnt; i++)
	{
		if(*((u32 *)(&(buf[i]))) == 1 && (u32)buf[i] > 2 && (u32)buf[i] < 0x100)
		{
			icnt++;
			ioses = (u8 *)realloc(ioses, sizeof(u8) * icnt);
			ioses[icnt - 1] = (u8)buf[i];
		}
	}

	ioses = (u8 *)malloc(sizeof(u8) * icnt);
	icnt = 0;
	
	for(i = 0; i < tcnt; i++)
	{
		if(*((u32 *)(&(buf[i]))) == 1 && (u32)buf[i] > 2 && (u32)buf[i] < 0x100)
		{
			icnt++;
			ioses[icnt - 1] = (u8)buf[i];
		}
	}
	free(buf);
	qsort(ioses, icnt, 1, __u8Cmp);

	*cnt = icnt;
	return ioses;
}

void show_boot2_info()
{
    VIDEO_WaitVSync();
    PrintBanner();
    printf("\x1b[2;0H");
	printf("\n\n");

	printf("Retrieving boot2 version...\n");
	u32 boot2version = 0;
	int ret = ES_GetBoot2Version(&boot2version);
	if (ret < 0)
	{
		printf("ERROR: ES_GetBoot2Version: %s", EsErrorCodeString(ret));
		printf("Could not get boot2 version. It's possible your Wii is\n");
		printf("a boot2v4+ Wii, maybe not.\n");
	} else
	{
		printf("Your boot2 version is: %u\n", boot2version);
		if (boot2version < 4) printf("This means you should not have problems.\n");
	}	
	
	printf("\n");
	printf("Boot2v4 is an indicator for the 'new' Wii hardware revision that \n");
	printf("prevents the execution of some old IOS. These Wiis are often called\n");
	printf("LU64+ Wiis or 'unsoftmoddable' Wiis. You MUST NOT downgrade one of these\n");
	printf("Wiis and be EXTRA careful when messing with ANYTHING on them.\n");
	printf("The downgraded IOS15 you get with the Trucha Bug Restorer should work\n");
	printf("on these Wiis and not harm Wiis in general.\n");
	printf("\n");
	printf("If you updated your Wii via wifi to 4.2 or higher, your boot2 got\n");
	printf("updated by this and you can't use it as indicator for this.\n");
	printf("\n");
	printf("Press any button to return to the menu\n");
	WaitAnyKey();
	VIDEO_WaitVSync();
}

void MainThread_Execute()
{	
	int ret = 0;
	u32 button;
    /*This definines he max number of IOSs we can have to select from*/
    int selected=19;

	int menuSelection = 0; // Used for menus?
	int screen = 0; //Checks what screen we're on    
    int orRegion = 0; //Region or...?
    int systemSelection = 0; //Which system menu?
    int regionSelection = 0; //Which region?
    int channelSelection = 0; //Which channel?
	bool exitMenu = false; // Used to break out of menus so goto statements do not have to be used
	bool exitSubMenu = false; // Same as exitMenu but for sub while loops

	FILE *logFile;//For signcheck
	u32 iosToTest = 0;//For signcheck
    int reportResults[6];//For signcheck

    //Basic scam warning, brick warning, and credits by Arikado
	VIDEO_WaitVSync();
    PrintBanner();
    printf("Welcome to DOP-IOS MOD - a modification of DOP-IOS!\n\n");
    printf("If you have paid for this software, you have been scammed.\n\n");
    printf("If misused, this software WILL brick your Wii.\n");
    printf("The authors of DOP-IOS MOD are not responsible if your Wii is bricked.\n\n");
    printf("Created by: SifJar, PheonixTank, giantpune, Lunatik\n");
    printf("            Arikado - http://arikadosblog.blogspot.com\n\n");
	printf("Press A to continue. Press [HOME|START] to exit.");		
	VIDEO_WaitVSync();
			
	while (ScanPads(&button))
	{
		if (ExitMainThread) return;
		if (button&WPAD_BUTTON_A) break;
		if (button&WPAD_BUTTON_HOME) ReturnToLoader();		
	}

    WPAD_Shutdown();//Phoenix's bugfix

    u8 *iosVersion = NULL;
    u32 iosCnt;

    u32 cnt;
    s32 retios;
	s32 selectedIos = 0;

    /* Get IOS versions */
    retios = Title_GetIOSVersions(&iosVersion, &iosCnt);

    /* Sort list */
    qsort(iosVersion, iosCnt, sizeof(u8), __Menu_IsGreater);

    /* Set default version */
    for (cnt = 0; cnt < iosCnt; cnt++) 
	{
        u8 version = iosVersion[cnt];

        /* Custom IOS available */
        if (version == CIOS_VERSION) 
		{
            selectedIos = cnt;
            break;
        }

        /* Current IOS */
        if (version == IOS_GetVersion()) selectedIos = cnt;
    }

    WPAD_Init();

	menuSelection = 0;

    while (!exitMenu) 
	{		
		if (ExitMainThread) return;
		exitSubMenu = false;
		while (!exitSubMenu) 
		{
			if (ExitMainThread) return;
			VIDEO_WaitVSync();
            PrintBanner();
            printf("Which IOS would you like to use to install other IOSes?\n");
			printf("%3s IOS: %d\n", (menuSelection == 0 ? "-->" : " "), iosVersion[selectedIos]);
			printf("%3s Install IOS36 (r%d) w/FakeSign\n", (menuSelection == 1 ? "-->" : " "), IOS36Version);
			printf("%3s Exit\n", (menuSelection == 2 ? "-->" : " "));	
			VIDEO_WaitVSync();

			while (ScanPads(&button))
			{
				if (ExitMainThread) return;
				if (button&WPAD_BUTTON_A) exitSubMenu = true;
				if (button&WPAD_BUTTON_HOME) ReturnToLoader();
				
				if (button&WPAD_BUTTON_UP) menuSelection--;
				if (button&WPAD_BUTTON_DOWN) menuSelection++;

				if (menuSelection < 0) menuSelection = 2;
				if (menuSelection > 2) menuSelection = 0;

				if (menuSelection == 0)
				{
					if (button&WPAD_BUTTON_LEFT && --selectedIos <= -1) selectedIos = (iosCnt - 1);
					if (button&WPAD_BUTTON_RIGHT && ++selectedIos >= iosCnt) selectedIos = 0;
				}	
				if (button) break;
			}
        }

		if (ExitMainThread) return;

		switch (menuSelection)
		{
			case 0: // For next menu
				gprintf("Loading IOS\n");
				VIDEO_WaitVSync();
				PrintBanner();				
				printf("Loading selected IOS...\n");
											
				ret = ReloadIos(iosVersion[selectedIos]);

				if (ret >= 0) printf("\n\nIOS successfully loaded! Press A to continue.");
				VIDEO_WaitVSync();
				
				while (ScanPads(&button))
				{
					if (ExitMainThread) return;
					if (button&WPAD_BUTTON_A) { exitMenu = true; break; }
					if (button&WPAD_BUTTON_HOME) ReturnToLoader();
				}								

				if ((ret < 0) && (ret != -1017)) 
				{
					printf("\n\n\nERROR! Choose an IOS that accepts fakesigning! Press A to continue.");
										
					while (ScanPads(&button))
					{
						if (ExitMainThread) return;
						if (button&WPAD_BUTTON_A) { exitMenu = false; break; }
						if (button&WPAD_BUTTON_HOME) ReturnToLoader();
					}
				}

				break;
			case 1: //Install an IOS that accepts fakesigning
				printf("\n");
				if (!PromptContinue()) break;

				VIDEO_WaitVSync();
				PrintBanner();
				ret = FakeSignInstall();
				if (ret > 0) 
				{
					ClearScreen();
					PrintBanner();
					printf("Installation of IOS36 (r%d) w/FakeSign was completed successfully!!!\n", IOS36Version);
					printf("You may now use IOS36 to install anything else.\n");
					menuSelection = 0;
				}
				printf("\n\nPress any key to continue.\n");
				VIDEO_WaitVSync();
				WaitAnyKey();
				break;
			case 2: // Exit
				printf("\n\nAre you sure you want to exit?\n");
				if (PromptYesNo()) ReturnToLoader();
				break;
		}
    }

    getMyIOS();

    switch (CONF_GetRegion()) 
	{
		case CONF_REGION_US: regionSelection = 0; break;
		case CONF_REGION_EU: regionSelection = 1; break;
		case CONF_REGION_JP: regionSelection = 2; break;
		case CONF_REGION_KR: regionSelection = 3; break;
		default: regionSelection = 0; break;
    }

	menuSelection = 0;
    while (!ExitMainThread) 
	{        
        //Screen 0 -- Update Selection Screen
		if (screen == 0) 
		{
			VIDEO_WaitVSync();
			PrintBanner();
			printf("%3s IOSs\n", (menuSelection == 0 ? "-->" : " "));
			printf("%3s Channels\n", (menuSelection == 1 ? "-->" : " "));
			printf("%3s System Menu\n", (menuSelection == 2 ? "-->" : " "));
			printf("%3s Remove stubbed IOSs\n", (menuSelection == 3 ? "-->" : " "));
			printf("%3s Display boot2 information\n", (menuSelection == 4 ? "-->" : " "));
			printf("%3s Scan the Wii's internals (signcheck)", (menuSelection == 5 ? "-->" : " "));

            printf("\n\n\n\n\n[UP]/[DOWN]       Change Selection\n");
            printf("[A]               Select\n");
			printf("[HOME]/GC:[START] Exit\n\n\n");
			VIDEO_WaitVSync();
			
			while (ScanPads(&button))
			{
				if (ExitMainThread) return;
				if (button&WPAD_BUTTON_A)
				{					
					if (menuSelection == 0) screen = 1;
					if (menuSelection == 1) screen = 2;
					if (menuSelection == 2) screen = 3;
					if (menuSelection == 3) screen = 4;
					if (menuSelection == 4) show_boot2_info();
					if (menuSelection == 5) screen = 5;
				}
				if (button&WPAD_BUTTON_DOWN) menuSelection++;
				if (button&WPAD_BUTTON_UP) menuSelection--;
				if (button&WPAD_BUTTON_HOME) ReturnToLoader();
				if (button) break;
			}

            if (menuSelection < 0) menuSelection = 5;
            if (menuSelection > 5) menuSelection = 0;
        } //End Screen 0

        //Screen1 IOS Selection Screen
        if (screen == 1) 
		{
			IosInfo ios = IosInfoList[selected];

			VIDEO_WaitVSync();
			// Show menu
			PrintBanner();            
            printf("Select the IOS you want to DOP.             Currently installed:\n\n        ");

			Console_SetFgColor(YELLOW, 1);
			printf("%-*s", 4, (selected > IOSINFO_LO) ? "<<" : "");
			Console_SetFgColor(WHITE, 0);
		
            printf("IOS%u", ios.Version);

			Console_SetFgColor(YELLOW, 1);
			printf("%*s", 4, (selected < IOSINFO_HI) ? ">>" : "");
			Console_SetFgColor(WHITE, 0);

            Console_SetPosition(Console_GetCurrentRow(), 44);

            u32 installed = ios_found[ios.Version];
            if (installed) 
			{
                printf("v%u",ios_found[ios.Version]);
                if ((ios.State == IOSSTATE_STUB_NOW || ios.State == IOSSTATE_PROTECTED) && ios.HighRevision==installed) printf(" (stub)");
            } 
			else printf("(none)");

            printf("\n\n");
			Console_SetFgColor(WHITE, BOLD_NORMAL);
			printf("Description\n");
			Console_SetFgColor(WHITE, 0);
            printf("%s",ios.Description);

			Console_SetPosition(ConsoleRows-12, 0);

			//Show options
            printf("[LEFT/RIGHT]      Select IOS\n");
            if (ios.State != IOSSTATE_PROTECTED) 
			{
                if (ios.State != IOSSTATE_STUB_NOW) printf("[A]               Install latest v%u of IOS%u\n", ios.HighRevision, ios.Version);
                else printf("\n");

                if (ios.State != IOSSTATE_LATEST) printf("[-]/GC:[L]        Install old v%d of IOS%d\n", ios.LowRevision, ios.Version);
                else printf("\n");

            } 
			else printf("\n\n");

            printf("[B]               Back\n");
            printf("[HOME]/GC:[START] Exit");

			Console_SetPosition(ConsoleRows-2, 0);
            printf("-- Dop-IOS MOD by Arikado, SifJar, PhoenixTank, giantpune, Lunatik");
			VIDEO_WaitVSync();

			while (ScanPads(&button))
			{
				if (ExitMainThread) return;
				if (button&WPAD_BUTTON_B) screen = 0;

				if (button&WPAD_BUTTON_RIGHT) 
				{
					selected++;
					if (selected >= IOSINFO_LEN) selected = IOSINFO_HI;
				}
				if (button&WPAD_BUTTON_LEFT)
				{
					selected--;
					if (selected <= IOSINFO_LO) selected = IOSINFO_LO;
				}

				if ((button&WPAD_BUTTON_A) && (ios.State == IOSSTATE_NORMAL || ios.State == IOSSTATE_LATEST)) doparIos(ios, true);
				if ((button&WPAD_BUTTON_MINUS) && (ios.State == IOSSTATE_NORMAL || ios.State == IOSSTATE_STUB_NOW)) doparIos(ios, false);
				if (button&WPAD_BUTTON_HOME) ReturnToLoader();
				if (button) break;
            }

        }//End screen1

        //Screen 2 = Channel Choice
        if (screen == 2) 
		{
			VIDEO_WaitVSync();
			PrintBanner();
			printf("%3s Install Channel: %s\n", (orRegion == 0 ? "-->" : " "), Channels[channelSelection].Name);
			printf("%3s Region:          %s\n\n\n", (orRegion == 1 ? "-->" : " "), Regions[regionSelection].Name);

            printf("[UP]/[DOWN] [LEFT]/[RIGHT]       Change Selection\n");
            printf("[A]                              Select\n");
            printf("[B]                              Back\n");
            printf("[HOME]/GC:[START]                Exit\n\n\n");
			VIDEO_WaitVSync();

			while (ScanPads(&button))
			{
				if (ExitMainThread) return;
				if (button&WPAD_BUTTON_HOME) ReturnToLoader();
				if (button&WPAD_BUTTON_A && PromptContinue()) InstallTheChosenChannel(regionSelection, channelSelection);

				if (button&WPAD_BUTTON_DOWN) orRegion++;
				if (button&WPAD_BUTTON_UP) orRegion--;

				if (orRegion > 1) orRegion = 0;
				if (orRegion < 0) orRegion = 1;

				if (button&WPAD_BUTTON_LEFT)
				{
					if (orRegion == 0) channelSelection--;
					if (orRegion == 1) regionSelection--;
				}

				if (button&WPAD_BUTTON_RIGHT)
				{
					if (orRegion == 0) channelSelection++;
					if (orRegion == 1) regionSelection++;
				}

				if (channelSelection < 0) channelSelection = CHANNELS_HI;
				if (channelSelection > CHANNELS_HI) channelSelection = CHANNELS_LO;
				if (regionSelection < 0) regionSelection = REGIONS_HI;
				if (regionSelection > REGIONS_HI) regionSelection = REGIONS_LO;

				if (button&WPAD_BUTTON_B) screen = 0;
				if (button) break;
			}
		}

        //Screen 3 = System Menu Choice
        if (screen == 3) 
		{
			VIDEO_WaitVSync();
            if (regionSelection == REGIONS_HI && systemSelection == 0) systemSelection = 1;

			PrintBanner();
			printf("%3s Install System Menu: %s\n", (orRegion == 0 ? "-->" : " "), SystemMenus[systemSelection].Name);
			printf("%3s Region:              %s\n\n\n", (orRegion == 1 ? "-->" : " "), Regions[regionSelection].Name);

            printf("[UP]/[DOWN] [LEFT]/[RIGHT]       Change Selection\n");
            printf("[A]                              Select\n");
            printf("[B]                              Back\n");
            printf("[HOME]/GC:[START]                Exit\n\n\n");
			VIDEO_WaitVSync();

			while (ScanPads(&button))
			{
				if (ExitMainThread) return;
				if (button&WPAD_BUTTON_HOME) ReturnToLoader();
				if (button&WPAD_BUTTON_A && PromptContinue()) InstallTheChosenSystemMenu(regionSelection, systemSelection);
				if (button&WPAD_BUTTON_DOWN) orRegion++;
				if (button&WPAD_BUTTON_UP) orRegion--;

				if (orRegion > 1) orRegion = 0;
				if (orRegion < 0) orRegion = 1;

				if (button&WPAD_BUTTON_LEFT)  
				{
					if (orRegion == 0) systemSelection--;
					if (orRegion == 1) regionSelection--;

					//Only let 3.5 appear if Korea is the selected region
					if (systemSelection == 3 && regionSelection != REGIONS_HI) systemSelection--;

					//Get rid of certain menus from Korea selection
					if (regionSelection == REGIONS_HI && (systemSelection == 0 || systemSelection == 2 || systemSelection == 4))
						systemSelection--;
				}
				if (button&WPAD_BUTTON_RIGHT) 
				{
					if (orRegion == 0) systemSelection++;
					if (orRegion == 1) regionSelection++;

					//Only let 3.5 appear if Korea is the selected region
					if (systemSelection == 3 && regionSelection != REGIONS_HI) systemSelection++;

					//Get rid of certain menus from Korea selection
					if (regionSelection == REGIONS_HI && (systemSelection == 0 || systemSelection == 2 || systemSelection == 4))
						systemSelection++;
				}
				if (systemSelection < SYSTEMMENUS_LO) systemSelection = SYSTEMMENUS_HI;
				if (systemSelection > SYSTEMMENUS_HI) systemSelection = SYSTEMMENUS_LO;
				if (regionSelection < REGIONS_LO) regionSelection = REGIONS_HI;
				if (regionSelection > REGIONS_HI) regionSelection = REGIONS_LO;

				if (button&WPAD_BUTTON_B) screen = 0;
				if (button) break;
			}
        }//End Screen 3
		
		//Screen 4 -- Detect and remove stubbed IOSs
		if (screen == 4)
		{
			VIDEO_WaitVSync();
			PrintBanner();
			printf("Are you sure you want to check for stub IOSs and delete them to\n");
			printf("free up the 2 precious blocks they take up on that little nand?\n\n");
			VIDEO_WaitVSync();
			if (PromptContinue() && !CheckAndRemoveStubs()) 
			{
					printf("\n\nNo stubs found!");
					sleep(3);
			}
			screen = 0;			
	    }//End screen 4
		
		//Screen 5 -- Signcheck
		if (screen == 5)
		{
			VIDEO_WaitVSync();
			PrintBanner();
			printf("\x1b[2;0H\n\n"); // Move Cursor
			printf("[*] Using IOS%i (rev %i)\n", IOS_GetVersion(), IOS_GetRevision());
			printf("[*] Region %s\n", CheckRegion());
			printf("[*] Hollywood version 0x%x\n", *(u32 *)0x80003138);
			printf("[*] Getting certs.sys from the NAND\t\t\t\t");
			printf("%s\n", (!GetCert()) ? "[DONE]" : "[FAIL]");
			printf("\n");
			iosToTest = ScanIos() - 1;			
			printf("[*] Found %i IOSes that are safe to test.\n", iosToTest);
			printf("\n");
			sleep(5);

			if (ExitMainThread) return;
 
			char tmp[1024];
            u32 deviceId;
 
		    ES_GetDeviceID(&deviceId);			
 
            sprintf(tmp, "\"Dop-IOS MOD Report\"\n");
            strcat(logBuffer, tmp);
            sprintf(tmp, "\"Wii region\", %s\n", CheckRegion());
            strcat(logBuffer, tmp);
            sprintf(tmp, "\"Wii unique device id\", %u\n", deviceId);
            strcat(logBuffer, tmp);
            sprintf(tmp, "\n");
            strcat(logBuffer, tmp);
            sprintf(tmp, "%s, %s, %s, %s, %s, %s\n", "\"IOS number\"", "\"FakeSign\"", "\"ES_Identify\"", "\"Flash access\"", "\"Boot2 access\"", "\"Usb2.0 IOS tree\"");
            strcat(logBuffer, tmp);
            sprintf(tmp, "\n");
            strcat(logBuffer, tmp);
						
			PrintBanner();
			printf("\x1b[2;0H\n\n"); // Move Cursor
			Console_SetFgColor(WHITE, 1);
			printf("%-13s %-10s %-11s %-10s %-10s %-10s\n", "IOS Version", "FakeSign", "ES_Identify", "Flash", "Boot2", "USB 2.0");
			Console_SetFgColor(WHITE, 0);

			WPAD_Shutdown();

			int iosToTestCnt = 1;
			while (iosToTest > 0) 
			{	
				if (ExitMainThread) return;
				VIDEO_WaitVSync();
				ReloadIosNoInit(iosTable[iosToTest]);
 				
				char iosString[50] = "";				
				sprintf(iosString, "%d (r%d)", iosTable[iosToTest], IOS_GetRevision());
				gprintf("\nTesting IOS %s\n", iosString);
				printf("%-13s ", iosString);
				printf("%-10s ", (reportResults[1] = CheckFakeSign()) ? "Enabled" : "Disabled");
				printf("%-11s ", (reportResults[1] = CheckEsIdentify()) ? "Enabled" : "Disabled");
				printf("%-10s ", (reportResults[2] = CheckFlashAccess()) ? "Enabled" : "Disabled");
				printf("%-10s ", (reportResults[3] = CheckBoot2Access()) ? "Enabled" : "Disabled");
				printf("%-10s ", (reportResults[4] = CheckUsb2Module()) ? "Enabled" : "Disabled");
				printf("\n");
				VIDEO_WaitVSync();
 
				addLogEntry(iosTable[iosToTest], IOS_GetRevision(), reportResults[1], reportResults[2], reportResults[3], reportResults[4]);
 
				if ((iosToTestCnt % (ConsoleRows - 7)) == 0) 
				{
					WPAD_Init();
					printf("Press [A] Continue, [HOME|START] Return Loader\n");
					while (ScanPads(&button))
					{
						if (ExitMainThread) return;
						if (button&WPAD_BUTTON_A) break;
						if (button&WPAD_BUTTON_HOME) ReturnToLoader();
					}
					WPAD_Shutdown();
				}

				iosToTest--;
				iosToTestCnt++;
			}
			
			ReloadIos(iosVersion[selectedIos]);
			
			if (ExitMainThread) return;

			printf("\n");
			printf("Creating log on SD...\n\n");
			
			if (Init_SD())
			{
				logFile = fopen("sd:/DopIosModReport.csv", "wb");
				fwrite(logBuffer, 1, strlen(logBuffer), logFile);
				fclose(logFile);
				printf("All done, you can find the report on the root of your SD Card\n\n");
				Close_SD();
			}
			else
			{ 			
				printf("Failed to create log on your SD Card!\n\n");
				
				printf("\n");
				printf("Creating log on USB...\n\n");
				
				if (Init_USB())
				{
					logFile = fopen("usb:/DopIosModReport.csv", "wb");
					fwrite(logBuffer, 1, strlen(logBuffer), logFile);
					fclose(logFile);
					printf("All done, you can find the report on the root of your USB Device\n\n");
					Close_USB();
				}				
				else printf("Failed to create log on your USB device!\n\n");				
			}
			 
			printf("Return to the main Dop-IOS MOD menu?\n");
			if(!PromptYesNo()) ReturnToLoader();
			
			iosToTest = 0;
			screen = 0;
		} //End screen 5		
    }	

	return;
}

int main(int argc, char **argv)
{	
	System_Init();
	MainThread_Execute();
	if (Shutdown) System_Shutdown();
	else 
	{
		ReturnToLoader();
		System_Deinit();
	}	
	return 0;
}
