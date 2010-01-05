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
#include <fat.h>
#include <wiiuse/wpad.h>

#include "FakeSignInstaller.h"
#include "wiibasics.h"
#include "patchmii_core.h"
#include "title.h"
#include "title_install.h"
#include "tools.h"
#include "nand.h"
#include "network.h"
#include "IOSPatcher.h"
#include "signcheckfunctions.h"
#include "svnrev.h"
#include "video.h"
#include "ticket_dat.h"
#include "tmd_dat.h"
#include "controller.h"

#define PROTECTED	0
#define NORMAL		1
#define STUB_NOW	2
#define LATEST		3
#define MAX_REGION  3
#define MAX_IOS		36
#define MAX_SYSTEMMENU 6
#define MAX_CHANNEL 6


#define BLACK	0
#define RED		1
#define GREEN	2
#define YELLOW	3
#define BLUE	4
#define MAGENTA	5
#define CYAN	6
#define WHITE	7

#define CIOS_VERSION 36

#define round_up(x,n)	(-(-(x) & -(n)))

#define PAD_CHAN_0 0

char logBuffer[1024*1024];//For signcheck
u32 iosTable[256];//For signcheck
extern bool IsDebugging;

// Prevent IOS36 loading at startup
s32 __IOS_LoadStartupIOS()
{
	return 0;
}

s32 __u8Cmp(const void *a, const void *b)
{
	return *(u8 *)a-*(u8 *)b;
}

struct region 
{
    u32 idnumber;
    const char *name;
};

const struct region regions[] = 
{
    {0, "North America (U)"},
    {1, "Europe (E)"},
    {2, "Japan (J)"},
    {3, "Korea (K)"}
};

struct systemmenu 
{
    const char* name;
};

const struct systemmenu systemmenus[] = 
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

struct channel 
{
    const char* name;
};

const struct channel channels[] =
{
    {"Shop Channel"},
    {"Photo Channel 1.0"},
    {"Photo Channel 1.1"},
    {"Mii Channel"},
    {"Internet Channel"},
    {"News Channel"},
    {"Weather Channel"}
};


struct ios 
{
    u32 version;
    u32 lowRevision;
    u32 highRevision;
    u8 type;
    const char *desc;
};

const struct ios ioses[]=
{
	// IOS# OLDVERSION# NEWVERSION#, ???, Description -- Arikado
    {4,65280,65280,PROTECTED,"Stub, useless now."},
    {9,520,778,NORMAL,"Used by launch titles (Zelda: Twilight Princess) and System Menu 1.0."},
    {10,768,768,PROTECTED,"Stub, useless now."},
    {11,10,256,STUB_NOW,"Used only by System Menu 2.0 and 2.1."},
    {12,6,269,NORMAL,""},
    {13,10,273,NORMAL,""},
    {14,262,520,NORMAL,""},
    {15,257,523,NORMAL,""},
    {16,512,512,PROTECTED,"Piracy prevention stub, useless."},
    {17,512,775,NORMAL,""},
    {20,12,256,STUB_NOW,"Used only by System Menu 2.2."},
    {21,514,782,NORMAL,"Used by old third-party titles (No More Heroes)."},
    {22,777,1037,NORMAL,""},
    {28,1292,1550,NORMAL,""},
    {30,1040,2816,STUB_NOW,"Used only by System Menu 3.0, 3.1, 3.2 and 3.3."},
    {31,1040,3349,NORMAL,""},
    {33,1040,3091,NORMAL,""},
    {34,1039,3348,NORMAL,""},
    {35,1040,3349,NORMAL,"Used by: Super Mario Galaxy."},
    {36,1042,3351,NORMAL,"Used by: Smash Bros. Brawl, Mario Kart Wii. Can be ES_Identify patched."},
    {37,2070,3869,NORMAL,"Used mostly by music games (Guitar Hero)."},
    {38,3610,3867,NORMAL,"Used by some modern titles (Animal Crossing)."},
    {50,4889,5120,STUB_NOW,"Used only by System Menu 3.4."},
    {51,4633,4864,STUB_NOW,"Used only by Shop Channel 3.4."},
    {53,4113,5406,NORMAL,"Used by some modern games and channels."},
    {55,4633,5406,NORMAL,"Used by some modern games and channels."},
    {56,4890,5405,NORMAL,"Used by Wii Speak Channel and some newer WiiWare"},
    {57,5404,5661,NORMAL,"Unknown yet."},
    {60,6174,6400,STUB_NOW,"Used by System Menu 4.0 and 4.1."},
    {61,4890,5405,NORMAL,"Used by Shop Channel 4.x."},
    {70,6687,6687,LATEST,"Used by System Menu 4.2."},
    {222,65280,65280,PROTECTED,"Piracy prevention stub, useless."},
    //{222,65280,65280,NORMAL,"Piracy prevention stub, useless."},
    {223,65280,65280,PROTECTED,"Piracy prevention stub, useless."},
    {249,65280,65280,PROTECTED,"Piracy prevention stub, useless."},
    {250,65280,65280,PROTECTED,"Piracy prevention stub, useless."},
    {254,2,260,PROTECTED,"PatchMii prevention stub, useless."}
};

u32 ios_found[256];

void printCenter(char *text, int width)
{
	int textLen = strlen(text);
	int leftPad = (width - textLen) / 2;
	int rightPad = (width - textLen) - leftPad;
	printf("%*s%s%*s", leftPad, " ", text, rightPad, " ");
}

void setConsoleFgColor(u8 color,u8 bold) 
{
    printf("\x1b[%u;%um", color+30,bold);
    fflush(stdout);
}

void setConsoleBgColor(u8 color,u8 bold) 
{
    printf("\x1b[%u;%um", color+40,bold);
    fflush(stdout);
}

void printMyTitle() 
{
	int lineLength = 72;
	
	ClearScreen();
    setConsoleBgColor(RED,0);
    setConsoleFgColor(WHITE,0);
	
    printf("%*s", lineLength, " ");	

	char text[lineLength];
	sprintf(text, "Dop-IOS MOD v11 (SVN r%s)", SVN_REV_STR);
	printCenter(text, lineLength);

    printf("%*s", lineLength, " ");
    setConsoleBgColor(BLACK,0);
    setConsoleFgColor(WHITE,0);
    fflush(stdout);
	printf("\n\n");
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
    
    printMyTitle();
    printf("Loading installed IOS...");

    ret=ES_GetNumTitles(&count);
    if (ret) 
	{
        printf("ERROR: ES_GetNumTitles=%d\n", ret);
        return false;
    }

    static u64 title_list[256] ATTRIBUTE_ALIGN(32);
    ret=ES_GetTitles(title_list, count);
    if (ret) 
	{
        printf("ERROR: ES_GetTitles=%d\n", ret);
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


void doparIos(struct ios ios, bool newest) 
{
	uint iosVersion = ios.version;
	uint lowRevision = ios.lowRevision;
	uint highRevision = ios.highRevision;
	
    printMyTitle();

    printf("Are you sure you want to install ");
    setConsoleFgColor(YELLOW,1);
    printf("IOS%d", iosVersion);
	printf(" v%d", (newest ? highRevision : lowRevision));
    setConsoleFgColor(WHITE,0);
    printf("?\n");
	
    if (PromptContinue()) 
	{
        bool sigCheckPatch = false;
        bool esIdentifyPatch = false;
		bool nandPatch = false;
        if (iosVersion > 36 || newest) 
		{
            printf("\nApply Sig Hash Check patch in IOS%d?\n", iosVersion);
            sigCheckPatch = PromptYesNo();
            if (iosVersion == 36) 
			{				
                printf("\nApply ES_Identify patch in IOS%d?\n", iosVersion);
                esIdentifyPatch = PromptYesNo();
            }

			if (iosVersion == 36 || iosVersion == 37) 
			{
				printf("\nApply NAND Permissions Patch?\n");
				nandPatch = PromptYesNo();
			}
        }

		int ret = 0;
		if (newest) ret = IosInstall(iosVersion, highRevision, sigCheckPatch, esIdentifyPatch, nandPatch);
		else ret = IosDowngrade(iosVersion, highRevision, lowRevision);
        
        if (ret < 0) 
		{
            printf("ERROR: Something failed. (ret: %d)\n",ret);
            printf("Continue?\n");
			if (!PromptContinue()) ReturnToLoader();
        }

        getMyIOS();
    }
}

s32 __Menu_IsGreater(const void *p1, const void *p2) {

    u32 n1 = *(u32 *)p1;
    u32 n2 = *(u32 *)p2;

    /* Equal */
    if (n1 == n2)
        return 0;

    return (n1 > n2) ? 1 : -1;
}

s32 Title_GetIOSVersions(u8 **outbuf, u32 *outlen) {

    u8 *buffer = NULL;
    u64 *list = NULL;

    u32 count, cnt, idx;
    s32 ret;

    /* Get title list */
    ret = Title_GetList(&list, &count);
    if (ret < 0)
        return ret;

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

    goto out;

out:
    /* Free memory */
    if (list) free(list);
    return ret;
}


void InstallTheChosenSystemMenu(int region, int menu) {

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
    printf("Downloading System Menu. This may take awhile...\n");
    ret = Title_Download(sysTid, sysVersion, &sysTik, &sysTmd);
    if (ret < 0) 
	{
        printf("Error Downloading System Menu. Ret: %d\n", ret);
        printf("Exiting...");
        VIDEO_WaitVSync();
        VIDEO_WaitVSync();
        VIDEO_WaitVSync();
		ReturnToLoader();
    }

    //Install the System Menu
    printf("Installing System Menu. Do NOT turn the power off. This may take awhile...\n");
    ret = Title_Install(sysTik, sysTmd);
    if (ret < 0) {
        printf("Error Installing System Menu. Ret: %d\n", ret);
        printf("Exiting...");
        VIDEO_WaitVSync();
        VIDEO_WaitVSync();
        VIDEO_WaitVSync();
        ReturnToLoader();
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
        ret = patchmii_install(0x10002, 0x48414241, 18, 0x10002, 0x48414241, 18, false, false);		
        if (ret < 0) printf("\nError: %d", ret);
        else printf("\nShop Channel successfully installed!");
    }

	//Photo Channel 1.0
    if (channel == 1) 
	{
        printf("\n\nInstalling the Photo Channel 1.0...");
        ret = patchmii_install(0x10002, 0x48414141 , 0, 0x10002, 0x48414141 , 0, false, false);
        if (ret < 0) printf("\nError: %d", ret);
        else printf("\nPhoto Channel 1.0 successfully installed!");
    }

	//Photo Channel 1.1
    if (channel == 2) 
	{
        printf("\n\nInstalling the Photo Channel 1.1...");
        ret = patchmii_install(0x10002, 0x48415941, 0, 0x10002, 0x48415941, 0, false, false);
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
        if (region == 2) ret = patchmii_install(0x10001, 0x4841444a, 0, 0x10001, 0x4841444a, 0, false, false);
        if (region == 3) ret = patchmii_install(0x10001, 0x4841444b, 0, 0x10001, 0x4841444b, 0, false, false);
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
	u32 button = 0;
	for (button = 0;;ScanPads(&button))
	{
		if (button&WPAD_BUTTON_A) break;
		if (button&WPAD_BUTTON_HOME) ReturnToLoader();
	}
}

int checkAndRemoveStubs()// this can be made a whole lot smaller by using different functions with args.  but i never meant to release it so i was ok with having it be big.
{
	int ret=0;
    int rr=Title_GetVersionNObuf(0x00000001000000dell);
	Nand_Init();
	ClearScreen();
	gprintf("\nTitle_GetVersion 222 = %d",rr);
	if (rr==65280)
	{
		printf("\nOK to Remove stub IOS 222?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 222");
			Uninstall_FromTitle(0x00000001000000dell);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x00000001000000dfll);
	ClearScreen();
	gprintf("\nTitle_GetVersion 223 = %d",rr);
	if (rr==65280)
	{
		printf("\nOK to Remove stub IOS 223?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 223");
			Uninstall_FromTitle(0x00000001000000dfll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x00000001000000f9ll);
	ClearScreen();
	gprintf("\nTitle_GetVersion 249 = %d",rr);
	if (rr==65280)
	{
		printf("\nOK to Remove stub IOS 249?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 249");
			Uninstall_FromTitle(0x00000001000000f9ll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x00000001000000fall);
	ClearScreen();
	gprintf("\nTitle_GetVersion 250 = %d",rr);
	if (rr==65280)
	{
		printf("\nOK to Remove stub IOS 250?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 250");
			Uninstall_FromTitle(0x00000001000000fall);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x0000000100000004ll);
	ClearScreen();
	gprintf("\nTitle_GetVersion 4 = %d",rr);
	if (rr==65280)
	{
		printf("\nOK to Remove stub IOS 4?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 4");
			Uninstall_FromTitle(0x0000000100000004ll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x000000010000000all);
	ClearScreen();
	gprintf("\nTitle_GetVersion 10 = %d",rr);
	if (rr==768)
	{
		printf("\nOK to Remove stub IOS 10?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 10");
			Uninstall_FromTitle(0x000000010000000all);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x000000010000000bll);
	ClearScreen();
	gprintf("\nTitle_GetVersion 11 = %d",rr);
	if (rr==256)
	{
		printf("\nOK to Remove stub IOS 11?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 11");
			Uninstall_FromTitle(0x000000010000000bll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x0000000100000010ll);
	ClearScreen();
	gprintf("\nTitle_GetVersion 16 = %d",rr);
	if (rr==512)
	{
		printf("\nOK to Remove stub IOS 16?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 16");
			Uninstall_FromTitle(0x0000000100000010ll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x0000000100000014ll);
	ClearScreen();
	gprintf("\nTitle_GetVersion 20 = %d",rr);
	if (rr==256)
	{
		printf("\nOK to Remove stub IOS 20?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 20");
			Uninstall_FromTitle(0x0000000100000014ll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x000000010000001ell);
	ClearScreen();
	gprintf("\nTitle_GetVersion 30 = %d",rr);
	if (rr==2816)
	{
		printf("\nOK to Remove stub IOS 30?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 30");
			Uninstall_FromTitle(0x000000010000001ell);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x0000000100000032ll);
	ClearScreen();
	gprintf("\nTitle_GetVersion 50 = %d",rr);
	if (rr==5120)
	{
		printf("\nOK to Remove stub IOS 50?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 50");
			Uninstall_FromTitle(0x0000000100000032ll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x0000000100000033ll);
	ClearScreen();
	gprintf("\nTitle_GetVersion 51 = %d",rr);
	if (rr==4864)
	{
		printf("\nOK to Remove stub IOS 51?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 51");
			Uninstall_FromTitle(0x0000000100000033ll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x000000010000003cll);
	ClearScreen();
	gprintf("\nTitle_GetVersion 60 = %d",rr);
	if (rr==6400)
	{
		printf("\nOK to Remove stub IOS 60?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 60");
			Uninstall_FromTitle(0x000000010000003cll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x00000001000000fell);
	gprintf("\nTitle_GetVersion 254 = %d",rr);
	if (rr==260)
	{
		printf("\nOK to Remove stub IOS 254?\n\n");
		if (PromptYesNo())
		{
			gprintf("\ndelete 254");
			Uninstall_FromTitle(0x00000001000000fell);
		}
		ret++;
	}
	return ret;
	
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
		printf("ES_GetNumTitles: Error! (result = %d)\n", res);
		return 0;
	}
	buf = memalign(32, sizeof(u64) * tcnt);
	res = ES_GetTitles(buf, tcnt);
	if(res < 0)
	{
		printf("ES_GetTitles: Error! (result = %d)\n", res);
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
    ClearScreen();
    printMyTitle();
    printf("\x1b[2;0H");
	printf("\n\n");

	printf("Retrieving boot2 version...\n");
	u32 boot2version = 0;
	int ret = ES_GetBoot2Version(&boot2version);
	if (ret < 0)
	{
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
}

int ios_selectionmenu(int default_ios)
{
	u32 selection = 0;
	u32 ioscount;
	u8 *list = get_ioslist(&ioscount);
	u32 button;
	
	u32 i;
	for (i=0;i<ioscount;i++)
	{
		// Default to default_ios if found, else the loaded IOS
		if (list[i] == default_ios)
		{
			selection = i;
			break;
		}
		if (list[i] == IOS_GetVersion())
		{
			selection = i;
		}
	}	
	
	while (1)
	{
		//printf("\x1B[%d;%dH",2,0);	// move console cursor to y/x		
	    ClearScreen();
        printMyTitle();
        printf("\x1b[2;0H");
		printf("\n\n");
		printf("Select which IOS to load:       \b\b\b\b\b\b");
		
		set_highlight(true);
		printf("IOS%u\n", list[selection]);
		set_highlight(false);
		
		printf("Press B to continue without IOS Reload\n");
		
		for (button = 0;;ScanPads(&button))
		{
			if (button&WPAD_BUTTON_LEFT)
			{
				if (selection > 0) selection--;
				else selection = ioscount - 1;
			}

			if (button&WPAD_BUTTON_RIGHT)
			{
				if (selection < ioscount -1	) selection++;
				else selection = 0;
			}

			if (button&WPAD_BUTTON_B) return 0;
			if (button&WPAD_BUTTON_HOME) ReturnToLoader();
			if (button) break;
		}
		if (button&WPAD_BUTTON_A) break;
	}
	return list[selection];
}

int main(int argc, char **argv) 
{
	if (argc > 1) IsDebugging = true; // used by debug_printf()

	basicInit();
    PAD_Init();
    WPAD_Init();

	int ret = 0;
	u32 button;

    //Basic scam warning, brick warning, and credits by Arikado
    ClearScreen();
    printMyTitle();
    printf("\x1b[2;0H");
    printf("\n\nWelcome to Dop-IOS MOD - a modification of Dop-IOS!\n\n");
    printf("If you have paid for this software, you have been scammed.\n\n");
    printf("If misused, this software WILL brick your Wii.\n");
    printf("The authors of DOP-IOS MOD are not responsible if your Wii is bricked.\n\n");
    printf("Created by: SifJar, PheonixTank, giantpune, Lunatik\n");
    printf("            Arikado - http://arikadosblog.blogspot.com\n\n");
	printf("Press A to continue. Press [HOME|START] to exit.");	
		
	for (button = 0;;ScanPads(&button))
	{
		if (button&WPAD_BUTTON_A) break;
		if (button&WPAD_BUTTON_HOME) ReturnToLoader();
		VIDEO_WaitVSync();
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

	int firstSelection= 0;

    while (1) 
	{		
InitialMenu:
		while (1) 
		{
            printMyTitle();
            printf("\x1b[2;0H");
            printf("\n\n\nWhich IOS would you like to use to install other IOSs?\n");

			printf("%3s IOS: %d\n", (firstSelection== 0 ? "-->" : " "), iosVersion[selectedIos]);
			printf("%3s Install IOS36 (r%d) w/FakeSign\n", (firstSelection== 1 ? "-->" : " "), IOS36Version);
			printf("%3s Exit\n", (firstSelection== 2 ? "-->" : " "));

			button = 0;
			ScanPads(&button);

			if (button&WPAD_BUTTON_A) break;
			if (button&WPAD_BUTTON_HOME) ReturnToLoader();
			
			if (button&WPAD_BUTTON_UP) firstSelection--;
			if (button&WPAD_BUTTON_DOWN) firstSelection++;

			if (firstSelection< 0) firstSelection= 2;
			if (firstSelection> 2) firstSelection= 0;

			if (firstSelection== 0)
			{
				if (button&WPAD_BUTTON_LEFT && --selectedIos <= -1) selectedIos = (iosCnt - 1);
				if (button&WPAD_BUTTON_RIGHT && ++selectedIos >= iosCnt) selectedIos = 0;
			}

			VIDEO_WaitVSync();
        }

		if (firstSelection== 2)
		{
			printf("\n\nAre you sure you want to exit?\n");
			if (PromptYesNo()) ReturnToLoader();
			firstSelection = 0;
			goto InitialMenu;
		}

        //Install an IOS that accepts fakesigning
        if (firstSelection == 1) 
		{
			printf("\n");
			if (!PromptContinue()) goto InitialMenu;

			ClearScreen();
			printMyTitle();
			ret = FakeSignInstall();
			if (ret > 0) 
			{
				ClearScreen();
				printMyTitle();
				printf("Installation of IOS36 (r%d) w/FakeSign was completed successfully!!!\n", IOS36Version);
				printf("You may now use IOS36 to install anything else.\n");
			}
			printf("\n\nPress any key to continue.\n");
			WaitAnyKey();
			firstSelection = 0;
			goto InitialMenu;
	   }

        if (firstSelection == 0) 
		{
            printMyTitle();
            printf("\x1b[2;0H");
            printf("\n\nLoading selected IOS...\n");
			
			ret = ReloadIos(iosVersion[selectedIos]);

            if (ret >= 0) printf("\n\nIOS successfully loaded! Press A to continue.");

			for (button = 0;;ScanPads(&button))
			{
				if (button&WPAD_BUTTON_A) break;
				if (button&WPAD_BUTTON_HOME) ReturnToLoader();
			}
        }

		if (firstSelection == 0)
		{
			if ((ret < 0) && (ret != -1017)) 
			{
				printf("\n\n\nERROR! Choose an IOS that accepts fakesigning! Press A to continue.");
				for (button = 0;;ScanPads(&button))
				{
					if (button&WPAD_BUTTON_A) break;
					if (button&WPAD_BUTTON_HOME) ReturnToLoader();
				}
			}
			break;
		}
    }

    /*This definines he max number of IOSs we can have to select from*/
    int selected=19;

    int screen = 0;//Checks what screen we're on
    int selection = 0;//IOSs or Channels?
    int orregion = 0;//Region or...?
    int systemselection = 0;//Which system menu?
    int regionselection = 0;//Which region?
    int channelselection = 0;//Which channel?

	FILE *logFile;//For signcheck
	u32 iosToTest = 0;//For signcheck
    int reportResults[6];//For signcheck

    getMyIOS();

    regionselection = CONF_GetRegion();

    switch (regionselection) 
	{
		case CONF_REGION_US: regionselection = 0; break;
		case CONF_REGION_EU: regionselection = 1; break;
		case CONF_REGION_JP: regionselection = 2; break;
		case CONF_REGION_KR: regionselection = 3; break;
		default: regionselection = 0; break;
    }

    while (1) 
	{        
        //Screen 0 -- Update Selection Screen
		if (screen == 0) 
		{
			printMyTitle();
			printf("%3s IOSs\n", (selection == 0 ? "-->" : " "));
			printf("%3s Channels\n", (selection == 1 ? "-->" : " "));
			printf("%3s System Menu\n", (selection == 2 ? "-->" : " "));
			printf("%3s Remove stubbed IOSs\n", (selection == 3 ? "-->" : " "));
			printf("%3s Display boot2 information\n", (selection == 4 ? "-->" : " "));
			printf("%3s Scan the Wii's internals (signcheck)", (selection == 5 ? "-->" : " "));

            printf("\n\n\n\n\n[UP]/[DOWN]       Change Selection\n");
            printf("[A]               Select\n");
			printf("[HOME]/GC:[START] Exit\n\n\n");

			for (button = 0;;ScanPads(&button))
			{
				if (button&WPAD_BUTTON_A)
				{					
					if (selection == 0) screen = 1;
					if (selection == 1) screen = 2;
					if (selection == 2) screen = 3;
					if (selection == 3) screen = 4;
					if (selection == 4) show_boot2_info();
					if (selection == 5) screen = 5;
				}
				if (button&WPAD_BUTTON_DOWN) selection++;
				if (button&WPAD_BUTTON_UP) selection--;
				if (button&WPAD_BUTTON_HOME) ReturnToLoader();
				if (button) break;
			}

            if (selection < 0) selection = 5;
            if (selection > 5) selection = 0;

			VIDEO_WaitVSync();
        } //End Screen 0

        //Screen1 IOS Selection Screen
        if (screen == 1) 
		{
			printMyTitle();
            // Show menu
            printf("Select the IOS you want to dop.             Currently installed:\n\n        ");

            setConsoleFgColor(YELLOW,1);

            if (selected>0) printf("<<  ");
            else printf("    ");

            setConsoleFgColor(WHITE,0);
            u32 iosVersion = ioses[selected].version;
            u32 lowRevision = ioses[selected].lowRevision;
            u32 highRevision = ioses[selected].highRevision;
            u8 type = ioses[selected].type;
            printf("IOS%d",iosVersion);
            setConsoleFgColor(YELLOW,1);

            if ((selected+1)<MAX_IOS)  printf("  >>");

            setConsoleFgColor(WHITE,0);
            printf("                            ");
            u32 installed = ios_found[iosVersion];
            if (installed) 
			{
                printf("v%d",ios_found[iosVersion]);
                if ((type==STUB_NOW || type==PROTECTED) && highRevision==installed) printf(" (stub)");
            } 
			else  printf("(none)");

            printf("\n\n");
            printf("%s",ioses[selected].desc);
            printf("\n\n\n");

			//Show options
            printf("[LEFT/RIGHT]      Select IOS\n");
            if (type!=PROTECTED) 
			{
                if (type!=STUB_NOW) printf("[A]               Install latest v%d of IOS%d\n",highRevision,iosVersion);
                else printf("\n");

                if (type!=LATEST) printf("[-]/GC:[L]        Install old v%d of IOS%d\n",lowRevision,iosVersion);
                else printf("\n");

            } 
			else printf("\n\n");

            printf("[B]               Back\n");
            printf("[HOME]/GC:[START] Exit\n\n\n\n\n\n\n\n");
            printf("-- Dop-IOS MOD by Arikado, SifJar, PhoenixTank, giantpune, Lunatik");		

			for (button = 0;;ScanPads(&button))
			{
				if (button&WPAD_BUTTON_B) screen = 0;

				if (button&WPAD_BUTTON_RIGHT) 
				{
					selected++;
					if (selected == MAX_IOS) selected = MAX_IOS -1;
				}
				if (button&WPAD_BUTTON_LEFT)
				{
					selected--;
					if (selected < 0) selected = 0;
				}

				if ((button&WPAD_BUTTON_A) && (type == NORMAL || type == LATEST)) doparIos(ioses[selected], true);
				if ((button&WPAD_BUTTON_MINUS) && (type == NORMAL || type == STUB_NOW)) doparIos(ioses[selected], false);
				if (button&WPAD_BUTTON_HOME) ReturnToLoader();
				if (button) break;
            }

        }//End screen1

        //Screen 2 = Channel Choice
        if (screen == 2) 
		{
			printMyTitle();
			printf("%3s Install Channel: %s\n", (orregion == 0 ? "-->" : " "), channels[channelselection].name);
			printf("%3s Region:          %s\n\n\n", (orregion == 1 ? "-->" : " "), regions[regionselection].name);

            printf("[UP]/[DOWN] [LEFT]/[RIGHT]       Change Selection\n");
            printf("[A]                              Select\n");
            printf("[B]                              Back\n");
            printf("[HOME]/GC:[START]                Exit\n\n\n");

			for (button = 0;;ScanPads(&button))
			{
				if (button&WPAD_BUTTON_HOME) ReturnToLoader();
				if (button&WPAD_BUTTON_A && PromptContinue()) InstallTheChosenChannel(regionselection, channelselection);

				if (button&WPAD_BUTTON_DOWN) orregion++;
				if (button&WPAD_BUTTON_UP) orregion--;

				if (orregion > 1) orregion = 0;
				if (orregion < 0) orregion = 1;

				if (button&WPAD_BUTTON_LEFT)
				{
					if (orregion == 0) channelselection--;
					if (orregion == 1) regionselection--;
				}

				if (button&WPAD_BUTTON_RIGHT)
				{
					if (orregion == 0) channelselection++;
					if (orregion == 1) regionselection++;
				}

				if (channelselection < 0) channelselection = MAX_CHANNEL;
				if (channelselection > MAX_CHANNEL) channelselection = 0;
				if (regionselection < 0) regionselection = MAX_REGION;
				if (regionselection > MAX_REGION) regionselection = 0;

				if (button&WPAD_BUTTON_B) screen = 0;
				if (button) break;
			}
		}

        //Screen 3 = System Menu Choice
        if (screen == 3) 
		{
            //Quick Fix
            if (regionselection == MAX_REGION && systemselection == 0) systemselection = 1;

			printMyTitle();
			printf("%3s Install System Menu: %s\n", (orregion == 0 ? "-->" : " "), systemmenus[systemselection].name);
			printf("%3s Region:              %s\n\n\n", (orregion == 1 ? "-->" : " "), regions[regionselection].name);

            printf("[UP]/[DOWN] [LEFT]/[RIGHT]       Change Selection\n");
            printf("[A]                              Select\n");
            printf("[B]                              Back\n");
            printf("[HOME]/GC:[START]                Exit\n\n\n");

			for (button=0;;ScanPads(&button))
			{
				if (button&WPAD_BUTTON_HOME) ReturnToLoader();
				if (button&WPAD_BUTTON_A && PromptContinue()) InstallTheChosenSystemMenu(regionselection, systemselection);
				if (button&WPAD_BUTTON_DOWN) orregion++;
				if (button&WPAD_BUTTON_UP) orregion--;

				if (orregion > 1) orregion = 0;
				if (orregion < 0) orregion = 1;

				if (button&WPAD_BUTTON_LEFT)  
				{
					if (orregion == 0) systemselection--;
					if (orregion == 1) regionselection--;

					//Only let 3.5 appear if Korea is the selected region
					if (systemselection == 3 && regionselection != MAX_REGION) systemselection--;

					//Get rid of certain menus from Korea selection
					if (regionselection == MAX_REGION && (systemselection == 0 || systemselection == 2 || systemselection == 4))
						systemselection--;
				}
				if (button&WPAD_BUTTON_RIGHT) 
				{
					if (orregion == 0) systemselection++;
					if (orregion == 1) regionselection++;

					//Only let 3.5 appear if Korea is the selected region
					if (systemselection == 3 && regionselection != MAX_REGION) systemselection++;

					//Get rid of certain menus from Korea selection
					if (regionselection == MAX_REGION && (systemselection == 0 || systemselection == 2 || systemselection == 4))
						systemselection++;
				}
				if (systemselection < 0) systemselection = MAX_SYSTEMMENU;
				if (systemselection > MAX_SYSTEMMENU) systemselection = 0;
				if (regionselection < 0) regionselection = MAX_REGION;
				if (regionselection > MAX_REGION) regionselection = 0;

				if (button&WPAD_BUTTON_B) screen = 0;
				if (button) break;
			}
        }//End Screen 3
		
		//Screen 4 -- Detect and remove stubbed IOSs
		if (screen == 4)
		{
			printf("\n\nAre you sure you want to check for stub IOSs and delete them to\nfree up the 2 precious blocks they take up on that little nand?\n\n");
			if (PromptContinue() && !checkAndRemoveStubs()) 
			{
					printf("\n\nNo stubs found!");
					sleep(3);
			}
			screen = 0;
	    }//End screen 4
		
		//Screen 5 -- Signcheck
		if (screen == 5)
		{
			printMyTitle();
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
						
			printMyTitle();
			printf("\x1b[2;0H\n\n"); // Move Cursor
			setConsoleFgColor(WHITE,true);
			printf("%-13s %-10s %-11s %-10s %-10s %-10s\n", "IOS Version", "FakeSign", "ES_Identify", "Flash", "Boot2", "USB 2.0");
			setConsoleFgColor(WHITE,false);

			WPAD_Shutdown();

			int iosToTestCnt = 1;
			while (iosToTest > 0) 
			{			
				fflush(stdout);
 
				ReloadIosNoInit(iosTable[iosToTest]);
 				
				char iosString[50] = "";
				sprintf(iosString, "%d (r%d)", iosTable[iosToTest], IOS_GetRevision());
				printf("%-13s ", iosString);
				printf("%-10s ", (reportResults[1] = CheckFakeSign()) ? "Enabled" : "Disabled");
				printf("%-11s ", (reportResults[1] = CheckEsIdentify()) ? "Enabled" : "Disabled");
				printf("%-10s ", (reportResults[2] = CheckFlashAccess()) ? "Enabled" : "Disabled");
				printf("%-10s ", (reportResults[3] = CheckBoot2Access()) ? "Enabled" : "Disabled");
				printf("%-10s ", (reportResults[4] = CheckUsb2Module()) ? "Enabled" : "Disabled");
				printf("\n");
				fflush(stdout);
 
				addLogEntry(iosTable[iosToTest], IOS_GetRevision(), reportResults[1], reportResults[2], reportResults[3], reportResults[4]);
 
				if ((iosToTestCnt % 20) == 0) 
				{
					WPAD_Init();
					printf("Press [A] Continue, [HOME|START] Return Loader\n");
					for (button = 0;;ScanPads(&button))
					{
						if (button&WPAD_BUTTON_A) break;
						if (button&WPAD_BUTTON_HOME) ReturnToLoader();
					}
					WPAD_Shutdown();
				}

				iosToTest--;
				iosToTestCnt++;
			}
			
			ReloadIos(iosVersion[selectedIos]);
			
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

        VIDEO_WaitVSync();
        VIDEO_WaitVSync();
    }

    printf("\nReturning to loader...");
    return 0;
}