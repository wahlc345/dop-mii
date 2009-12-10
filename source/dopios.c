/*-------------------------------------------------------------

dopios.c

Dop-IOS MOD - A modification of Dop-IOS by Arikado, giantpune, SifJar, and PhoenixTank

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

#include "wiibasics.h"
#include "patchmii_core.h"

#include "title.h"
#include "title_install.h"
#include "tools.h"

#include "nand.h"

#include "network.h"

#include "IOSPatcher.h"

#define PROTECTED	0
#define NORMAL		1
#define STUB_NOW	2
#define LATEST		3
#define MAX_REGION  3
#define MAX_IOS		36
#define MAX_SYSTEMMENU 6
#define MAX_CHANNEL 7


#define BLACK	0
#define RED		1
#define GREEN	2
#define YELLOW	3
#define BLUE	4
#define MAGENTA	5
#define CYAN	6
#define WHITE	7

#define CIOS_VERSION 60

#define round_up(x,n)	(-(-(x) & -(n)))

#define PAD_CHAN_0 0

struct region {
    u32 idnumber;
    char * name;
};

const struct region regions[] = {
    {0, "North America (U)"},
    {1, "Europe (E)"},
    {2, "Japan (J)"},
    {3, "Korea (K)"}
};


struct systemmenu {
    char* name;
};

const struct systemmenu systemmenus[] = {
//VERSION#1, VERSION#2, VERSION#3 NAME
    {"System Menu 3.2"},
    {"System Menu 3.3"},
    {"System Menu 3.4"},
    {"System Menu 3.5 (Korea Only)"},
    {"System Menu 4.0"},
    {"System Menu 4.1"},
    {"System Menu 4.2"}
};

struct channel {

    char* name;
};

const struct channel channels[] ={
    {"Shop Channel"},
    {"Photo Channel 1.0"},
    {"Photo Channel 1.1"},
    {"Mii Channel"},
    {"Nintendo Channel"},
    {"Internet Channel"},
    {"News Channel"},
    {"Weather Channel"}

};


struct ios {
    u32 major;
    u32 minor;
    u32 newest;
    u8 type;
    char* desc;
};

/*//i just used this list to let me install stub IOSes to test removal.  it is otherwise useless.
const struct ios ioses[]={
// IOS# OLDVERSION# NEWVERSION#, ???, Description -- Arikado
    {4,65280,65280,NORMAL,"Stub, useless now."},
    {9,520,778,NORMAL,"Used by launch titles (Zelda: Twilight Princess) and System Menu 1.0."},
    {10,768,768,NORMAL,"Stub, useless now."},
    {11,10,256,NORMAL,"Used only by System Menu 2.0 and 2.1."},
    {12,6,269,NORMAL,""},
    {13,10,273,NORMAL,""},
    {14,262,520,NORMAL,""},
    {15,257,523,NORMAL,""},
    {16,512,512,NORMAL,"Piracy prevention stub, useless."},
    {17,512,775,NORMAL,""},
    {20,12,0,NORMAL,"Used only by System Menu 2.2."},
    {21,514,782,NORMAL,"Used by old third-party titles (No More Heroes)."},
    {22,777,1037,NORMAL,""},
    {28,1292,1550,NORMAL,""},
    {30,1040,2816,NORMAL,"Used only by System Menu 3.0, 3.1, 3.2 and 3.3."},
    {31,1040,3349,NORMAL,""},
    {33,1040,3091,NORMAL,""},
    {34,1039,3348,NORMAL,""},
    {35,1040,3349,NORMAL,"Used by: Super Mario Galaxy."},
    {36,1042,3351,NORMAL,"Used by: Smash Bros. Brawl, Mario Kart Wii. Can be ES_Identify patched."},
    {37,2070,3869,NORMAL,"Used mostly by music games (Guitar Hero)."},
    {38,3610,3867,NORMAL,"Used by some modern titles (Animal Crossing)."},
    {50,4889,5120,NORMAL,"Used only by System Menu 3.4."},
    {51,4633,4864,NORMAL,"Used only by Shop Channel 3.4."},
    {53,4113,5406,NORMAL,"Used by some modern games and channels."},
    {55,4633,5406,NORMAL,"Used by some modern games and channels."},
    {56,4890,5405,NORMAL,"Used only by Wii Speak Channel 2.0."},
    {57,5404,5661,NORMAL,"Unknown yet."},
    {60,6174,6400,NORMAL,"Used by System Menu 4.0 and 4.1."},
    {61,4890,5405,NORMAL,"Used by Shop Channel 4.x."},
    {70,6687,6687,LATEST,"Used by System Menu 4.2."},
    {222,65280,65280,NORMAL,"Piracy prevention stub, useless."},
    {223,65280,65280,NORMAL,"Piracy prevention stub, useless."},
    {249,65280,65280,NORMAL,"Piracy prevention stub, useless."},
    {250,65280,65280,NORMAL,"Piracy prevention stub, useless."},
    {254,2,260,NORMAL,"PatchMii prevention stub, useless."}
};*/

const struct ios ioses[]={
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
    {20,12,0,STUB_NOW,"Used only by System Menu 2.2."},
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
    {56,4890,5405,NORMAL,"Used only by Wii Speak Channel 2.0."},
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

void setConsoleFgColor(u8 color,u8 bold) {
    printf("\x1b[%u;%um", color+30,bold);
    fflush(stdout);
}

void setConsoleBgColor(u8 color,u8 bold) {
    printf("\x1b[%u;%um", color+40,bold);
    fflush(stdout);
}

void clearConsole() {
    printf("\x1b[2J");
}

void printMyTitle() {
    clearConsole();
    setConsoleBgColor(RED,0);
    setConsoleFgColor(WHITE,0);
    printf("                                                                        ");
    printf("                           Dop-IOS MOD v10                              ");
    printf("                                                                        ");
    setConsoleBgColor(BLACK,0);
    setConsoleFgColor(WHITE,0);
    fflush(stdout);
    printf("\n\n");
}


bool getMyIOS() {
    s32 ret;
    u32 count;
    int i;
    for (i=0;i<256;i++) {
        ios_found[i]=0;
    }
    printMyTitle();
    printf("Loading installed IOS...");

    ret=ES_GetNumTitles(&count);
    if (ret) {
        printf("ERROR: ES_GetNumTitles=%d\n", ret);
        return false;
    }

    static u64 title_list[256] ATTRIBUTE_ALIGN(32);
    ret=ES_GetTitles(title_list, count);
    if (ret) {
        printf("ERROR: ES_GetTitles=%d\n", ret);
        return false;
    }


    for (i=0;i<count;i++) {
        u32 tmd_size;
        ret=ES_GetStoredTMDSize(title_list[i], &tmd_size);
        static u8 tmd_buf[MAX_SIGNED_TMD_SIZE] ATTRIBUTE_ALIGN(32);
        signed_blob *s_tmd=(signed_blob *)tmd_buf;
        ret=ES_GetStoredTMD(title_list[i], s_tmd, tmd_size);

        tmd *t=SIGNATURE_PAYLOAD(s_tmd);
        u32 tidh=t->title_id >> 32;
        u32 tidl=t->title_id & 0xFFFFFFFF;

        if (tidh==1 && t->title_version)
            ios_found[tidl]=t->title_version;

        if (i%2==0)
            printf(".");
    }
    return true;
}


void doparIos(u32 major, u32 minor,bool newest) {
    printMyTitle();

    printf("Are you sure you want to install ");
    setConsoleFgColor(YELLOW,1);
    printf("IOS%d",major);
    printf(" v%d",minor);
    setConsoleFgColor(WHITE,0);
    printf("?\n");

    if (yes_or_no()) {
        bool sig_check_patch = false;
        bool es_identify_patch = false;
        if (major > 36 || newest) {
            printf("\nApply Sig Hash Check patch in IOS%d?\n",major);
            sig_check_patch = yes_or_no();
            if (major == 36) {
                printf("\nApply ES_Identify patch in IOS%d?\n",major);
                es_identify_patch = yes_or_no();
            }
        }

        int ret = patchmii_install(1, major, minor, 1, major, minor, sig_check_patch, es_identify_patch);
        if (ret < 0) {
            printf("ERROR: Something failed. (ret: %d)\n",ret);
            printf("Continue?\n");
            if (yes_or_no() == false)
                exit(0);
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
    for (cnt = idx = 0; idx < count; idx++) {
        u32 tidh = (list[idx] >> 32);
        u32 tidl = (list[idx] & 0xFFFFFFFF);

        /* Title is IOS */
        if ((tidh == 0x1) && (tidl >= 3) && (tidl <= 255))
            cnt++;
    }

    /* Allocate memory */
    buffer = (u8 *)memalign(32, cnt);
    if (!buffer) {
        ret = -1;
        goto out;
    }

    /* Copy IOS */
    for (cnt = idx = 0; idx < count; idx++) {
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
    if (list)
        free(list);

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
    if (region == 0) {

        //3.2
        if (menu == 0)
            sysVersion = 289;

        //3.3
        if (menu == 1)
            sysVersion = 353;

        //3.4
        if (menu == 2)
            sysVersion = 385;

        //4.0
        if (menu == 4)
            sysVersion = 417;

        //4.1
        if (menu == 5)
            sysVersion = 449;

        //4.2
        if (menu == 6)
            sysVersion = 481;

    }

    //Europe
    if (region == 1) {

        //3.2
        if (menu == 0)
            sysVersion = 290;

        //3.3
        if (menu == 1)
            sysVersion = 354;

        //3.4
        if (menu == 2)
            sysVersion = 386;

        //4.0
        if (menu == 4)
            sysVersion = 418;

        //4.1
        if (menu == 5)
            sysVersion = 450;

        //4.2
        if (menu == 6)
            sysVersion = 482;

    }

    //Japan
    if (region == 2) {

        //3.2
        if (menu == 0)
            sysVersion = 288;

        //3.3
        if (menu == 1)
            sysVersion = 352;

        //3.4
        if (menu == 2)
            sysVersion = 384;

        //4.0
        if (menu == 4)
            sysVersion = 416;

        //4.1
        if (menu == 5)
            sysVersion = 448;

        //4.2
        if (menu == 6)
            sysVersion = 480;

    }

    //Korea
    if (region == 3) {
        //Incorrect Version Numbers

        //3.3V
        if (menu == 1)
            sysVersion = 326;

        //3,5
        if (menu == 3)
            sysVersion = 390;

        //4.1
        if (menu == 5)
            sysVersion = 454;

        //4.2
        if (menu == 6)
            sysVersion = 486;

    }

    //Download the System Menu
    printf("Downloading System Menu. This may take awhile...\n");
    ret = Title_Download(sysTid, sysVersion, &sysTik, &sysTmd);
    if (ret < 0) {
        printf("Error Downloading System Menu. Ret: %d\n", ret);
        printf("Exiting...");
        VIDEO_WaitVSync();
        VIDEO_WaitVSync();
        VIDEO_WaitVSync();
        exit(0);
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
        exit(0);
    }

    //Close the NAND
    fflush(stdout);
    ISFS_Deinitialize();

}

void InstallTheChosenChannel(int region, int channel) {

    s32 ret = 0;

//Shop Channel
    if (channel == 0) {
        printf("\n\nInstalling the Shop Channel...");
        ret = patchmii_install(0x10002, 0x48414241, 18, 0x10002, 0x48414241, 18, false, false);
        if (ret < 0) {
            printf("\nError: %d", ret);
        } else
            printf("\nShop Channel successfully installed!");
    }

//Photo Channel 1.0
    if (channel == 1) {
        printf("\n\nInstalling the Photo Channel 1.0...");
        ret = patchmii_install(0x10002, 0x48414141 , 0, 0x10002, 0x48414141 , 0, false, false);
        if (ret < 0) {
            printf("\nError: %d", ret);
        } else
            printf("\nPhoto Channel 1.0 successfully installed!");
    }

//Photo Channel 1.1
    if (channel == 2) {
        printf("\n\nInstalling the Photo Channel 1.1...");
        ret = patchmii_install(0x10002, 0x48415941, 0, 0x10002, 0x48415941, 0, false, false);
        if (ret < 0) {
            printf("\nError: %d", ret);
        } else
            printf("\nPhoto Channel 1.1 successfully installed!");
    }

//Mii Channel
    if (channel == 3) {
        printf("\n\nInstalling the Mii Channel...");
        ret = patchmii_install(0x10002, 0x48414341, 0, 0x10002, 0x48414341, 0, false, false);
        if (ret < 0) {
            printf("\nError: %d", ret);
        } else
            printf("\nMii Channel successfully installed!");
    }

//Nintendo Channel
    if (channel == 4) {
        printf("\n\nInstalling the Nintendo Channel...");
        ret = patchmii_install(0x10001, 0x48415441, 0, 0x10001, 0x48415441, 0, false, false);
        if (region == 0)
            ret = patchmii_install(0x10001, 0x48415445, 0, 0x10001, 0x48415445, 0, false, false);
        if (region == 1)
            ret = patchmii_install(0x10001, 0x48415450, 0, 0x10001, 0x48415450, 0, false, false);
        if (region == 2)
            ret = patchmii_install(0x10001, 0x4841544a, 0, 0x10001, 0x4841544a, 0, false, false);
        if (region == 3)
            ret = patchmii_install(0x10001, 0x4841544b, 0, 0x10001, 0x4841544b, 0, false, false);
        if (ret < 0) {
            printf("\nError: %d", ret);
        } else
            printf("\nNintendo Channel successfully installed!");
    }

//Internet Channel
    if (channel == 5) {
        printf("\n\nInstalling the Internet Channel...");
        ret = patchmii_install(0x10001, 0x48414441, 0, 0x10001, 0x48414441, 0, false, false);
        if (region == 0)
            ret = patchmii_install(0x10001, 0x48414445, 0, 0x10001, 0x48414445, 0, false, false);
        if (region == 1)
            ret = patchmii_install(0x10001, 0x48414450, 0, 0x10001, 0x48414450, 0, false, false);
        if (region == 2)
            ret = patchmii_install(0x10001, 0x4841444a, 0, 0x10001, 0x4841444a, 0, false, false);
        if (region == 3)
            ret = patchmii_install(0x10001, 0x4841444b, 0, 0x10001, 0x4841444b, 0, false, false);
        if (ret < 0) {
            printf("\nError: %d", ret);
        } else
            printf("\nInternet Channel successfully installed!");
    }

//News Channel
    if (channel == 6) {
        printf("\n\nInstalling the News Channel...");
        ret = patchmii_install(0x10002, 0x48414741, 0, 0x10002, 0x48414741, 0, false, false);
        if (region == 0)
            ret = patchmii_install(0x10002,0x48414745,0,0x10002,0x48414745,0,false, false);
        if (region == 1)
            ret = patchmii_install(0x10002,0x48414750,0,0x10002,0x48414750,0,false, false);
        if (region == 2)
            ret = patchmii_install(0x10002,0x4841474a,0,0x10002,0x4841474a,0,false, false);
        if (region == 3)
            ret = patchmii_install(0x10002,0x4841474b,0,0x10002,0x4841474b,0,false, false);
        if (ret < 0) {
            printf("\nError: %d", ret);
        } else
            printf("\nNews Channel successfully installed!");
    }

//Weather Channel
    if (channel == 7) {
        printf("\n\nInstalling the Weather Channel...");
        ret = patchmii_install(0x10002, 0x48414641, 0, 0x10002, 0x48414641, 0, false, false);
        if (region == 0)
            ret = patchmii_install(0x10002,0x48414645,0,0x10002,0x48414645,0,false, false);
        if (region == 1)
            ret = patchmii_install(0x10002,0x48414650,0,0x10002,0x48414650,0,false, false);
        if (region == 2)
            ret = patchmii_install(0x10002,0x4841464a,0,0x10002,0x4841464a,0,false, false);
        if (region == 3)
            ret = patchmii_install(0x10002,0x4841464b,0,0x10002,0x4841464b,0,false, false);
        if (ret < 0) {
            printf("\nError: %d", ret);
        } else
            printf("\nWeather Channel successfully installed!");
    }


//After Installations are done:
    printf("\n\nPress A to continue!");

    while (true) {
        PAD_ScanPads();
        WPAD_ScanPads();
        if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_A) \
                || (PAD_ButtonsDown(0)&PAD_BUTTON_A))
            break;
    }


}


extern int useSd;
u8 fatIsInit = 0;
int checkAndRemoveStubs()// this can be made a whole lot smaller by using different functions with args.  but i never meant to release it so i was ok with having it be big.
{
	int ret=0;
    int rr=Title_GetVersionNObuf(0x00000001000000dell);
	Nand_Init();
	printf("\x1b[2J");
	gprintf("\nTitle_GetVersion 222 = %d",rr);
	if (rr==65280)
	{
		printf("\nOK to Remove stub IOS 222?\n\n");
		if (yes_or_no())
		{
			gprintf("\ndelete 222");
			Uninstall_FromTitle(0x00000001000000dell);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x00000001000000dfll);
	printf("\x1b[2J");
	gprintf("\nTitle_GetVersion 223 = %d",rr);
	if (rr==65280)
	{
		printf("\nOK to Remove stub IOS 223?\n\n");
		if (yes_or_no())
		{
			gprintf("\ndelete 223");
			Uninstall_FromTitle(0x00000001000000dfll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x00000001000000f9ll);
	printf("\x1b[2J");
	gprintf("\nTitle_GetVersion 249 = %d",rr);
	if (rr==65280)
	{
		printf("\nOK to Remove stub IOS 249?\n\n");
		if (yes_or_no())
		{
			gprintf("\ndelete 249");
			Uninstall_FromTitle(0x00000001000000f9ll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x00000001000000fall);
	printf("\x1b[2J");
	gprintf("\nTitle_GetVersion 250 = %d",rr);
	if (rr==65280)
	{
		printf("\nOK to Remove stub IOS 250?\n\n");
		if (yes_or_no())
		{
			gprintf("\ndelete 250");
			Uninstall_FromTitle(0x00000001000000fall);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x0000000100000004ll);
	printf("\x1b[2J");
	gprintf("\nTitle_GetVersion 4 = %d",rr);
	if (rr==65280)
	{
		printf("\nOK to Remove stub IOS 4?\n\n");
		if (yes_or_no())
		{
			gprintf("\ndelete 4");
			Uninstall_FromTitle(0x0000000100000004ll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x000000010000000all);
	printf("\x1b[2J");
	gprintf("\nTitle_GetVersion 10 = %d",rr);
	if (rr==768)
	{
		printf("\nOK to Remove stub IOS 10?\n\n");
		if (yes_or_no())
		{
			gprintf("\ndelete 10");
			Uninstall_FromTitle(0x000000010000000all);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x000000010000000bll);
	printf("\x1b[2J");
	gprintf("\nTitle_GetVersion 11 = %d",rr);
	if (rr==256)
	{
		printf("\nOK to Remove stub IOS 11?\n\n");
		if (yes_or_no())
		{
			gprintf("\ndelete 11");
			Uninstall_FromTitle(0x000000010000000bll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x0000000100000010ll);
	printf("\x1b[2J");
	gprintf("\nTitle_GetVersion 16 = %d",rr);
	if (rr==512)
	{
		printf("\nOK to Remove stub IOS 16?\n\n");
		if (yes_or_no())
		{
			gprintf("\ndelete 16");
			Uninstall_FromTitle(0x0000000100000010ll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x0000000100000014ll);
	printf("\x1b[2J");
	gprintf("\nTitle_GetVersion 20 = %d",rr);
	if (rr==0)
	{
		printf("\nOK to Remove stub IOS 20?\n\n");
		if (yes_or_no())
		{
			gprintf("\ndelete 20");
			Uninstall_FromTitle(0x0000000100000014ll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x000000010000001ell);
	printf("\x1b[2J");
	gprintf("\nTitle_GetVersion 30 = %d",rr);
	if (rr==2816)
	{
		printf("\nOK to Remove stub IOS 30?\n\n");
		if (yes_or_no())
		{
			gprintf("\ndelete 30");
			Uninstall_FromTitle(0x000000010000001ell);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x0000000100000032ll);
	printf("\x1b[2J");
	gprintf("\nTitle_GetVersion 50 = %d",rr);
	if (rr==5120)
	{
		printf("\nOK to Remove stub IOS 50?\n\n");
		if (yes_or_no())
		{
			gprintf("\ndelete 50");
			Uninstall_FromTitle(0x0000000100000032ll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x0000000100000033ll);
	printf("\x1b[2J");
	gprintf("\nTitle_GetVersion 51 = %d",rr);
	if (rr==4864)
	{
		printf("\nOK to Remove stub IOS 51?\n\n");
		if (yes_or_no())
		{
			gprintf("\ndelete 51");
			Uninstall_FromTitle(0x0000000100000033ll);
		}
		ret++;
	}
	rr=Title_GetVersionNObuf(0x000000010000003cll);
	printf("\x1b[2J");
	gprintf("\nTitle_GetVersion 60 = %d",rr);
	if (rr==6400)
	{
		printf("\nOK to Remove stub IOS 60?\n\n");
		if (yes_or_no())
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
		if (yes_or_no())
		{
			gprintf("\ndelete 254");
			Uninstall_FromTitle(0x00000001000000fell);
		}
		ret++;
	}
	return ret;
	
}

int main(int argc, char **argv) {

    basicInit();

    PAD_Init();
    WPAD_Init();

    fatInitDefault();


    int ret = 0;

    //Basic scam warning, brick warning, and credits by Arikado
    printf("\x1b[2J");
    printMyTitle();
    printf("\x1b[2;0H");
    printf("\n\nWelcome to Dop-IOS MOD - a modification of Dop-IOS!\n\n");
    printf("If you have paid for this software, you have been scammed.\n\n");
    printf("If misused, this software WILL brick your Wii.\n");
    printf("The authors of DOP-IOS MOD are not responsible if your Wii is bricked.\n\n");
    printf("Created by:\n");
    printf("SifJar\n");
    printf("PheonixTank\n");
	printf("giantpune\n");
    printf("Arikado - http://arikadosblog.blogspot.com\n\n");
    printf("Press A to continue. Press HOME to exit.");

    for (;;) {

        WPAD_ScanPads();
        PAD_ScanPads();

        if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_A) \
                || (PAD_ButtonsDown(0)&PAD_BUTTON_A))
            break;

        if (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_HOME)
            exit(0);

        VIDEO_WaitVSync();

    }

    WPAD_Shutdown();//Phoenix's bugfix

    u8 *iosVersion = NULL;
    u32 iosCnt;

    u32 cnt;
    s32 retios, selectedios = 0;

    /* Get IOS versions */
    retios = Title_GetIOSVersions(&iosVersion, &iosCnt);

    /* Sort list */
    qsort(iosVersion, iosCnt, sizeof(u8), __Menu_IsGreater);

    /* Set default version */
    for (cnt = 0; cnt < iosCnt; cnt++) {
        u8 version = iosVersion[cnt];


        /* Custom IOS available */
        if (version == CIOS_VERSION) {
            selectedios = cnt;
            break;
        }

        /* Current IOS */
        if (version == IOS_GetVersion())
            selectedios = cnt;
    }

    WPAD_Init();

    int iosreloadcount = 0;//Forces the Wii to exit(0); if IOS_ReloadIOS() is called 11 times
    int firstselection = 0;

    for (;;) {

        for (;;) {

            WPAD_ScanPads();
            PAD_ScanPads();

            printMyTitle();
            printf("\x1b[2;0H");
            printf("\n\n\nWhich IOS would you like to use to install other IOSs?\n");

            if (firstselection == 0) {
                printf("--> IOS: %d\n", iosVersion[selectedios]);
                printf("    Install an IOS that accepts fakesigning\n\n");
            }

            if (firstselection == 1) {
                printf("    IOS: %d\n", iosVersion[selectedios]);
                printf("--> Install an IOS that accepts fakesigning\n\n");
            }

            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_UP) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_UP) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_UP))
                firstselection--;

            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_DOWN) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_DOWN) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_DOWN))
                firstselection++;

            if (firstselection > 1)
                firstselection = 0;
            if (firstselection < 0)
                firstselection = 1;

            /* LEFT/RIGHT buttons */
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_LEFT) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_LEFT) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_LEFT)) {
                if ((--selectedios) <= -1)
                    selectedios = (iosCnt - 1);
            }
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_RIGHT) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_RIGHT) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_RIGHT)) {
                if ((++selectedios) >= iosCnt)
                    selectedios = 0;
            }

            /* HOME button */
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_HOME) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_HOME) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_Y))
                exit(0);

            /* A button */
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_A) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_A))
                break;

            VIDEO_WaitVSync();
        }

        //Install an IOS that accepts fakesigning
        if (firstselection == 1) {
            printf("Installing an IOS that accepts fakesigning...\n");
            if (!yes_or_no())
               exit(0);
            printf("Downgrading IOS 15...\n");
            ret = Downgrade_IOS(15, 523, 257);
            if (ret < 0) {
                printf("Downgrade failed. Exiting...");
                exit(0);
            }
			
			/*FILE* ff = fopen("sd:/42.dol","r");
			if (ff)
			{
				gprintf("\n\tfat works here1");
				fclose(ff);
			
			}*/
			
            Close_SD();
			Close_USB();
            WPAD_Shutdown();
            iosreloadcount++;
            IOS_ReloadIOS(15);
            WPAD_Init();
			
			fatInitDefault();
			/*ff = fopen("sd:/42.dol","r");
			if (ff)
			{
				gprintf("\n\tfat works here2");
				fclose(ff);
			
			}*/
            printf("IOS 15 successfully downgraded!\n");
            printf("Continue to install your fakesign accepting IOS 36?\n");
            if (!yes_or_no())
                exit(0);
            ret = Install_patched_IOS(36, 3351, true, true, true, 36, 3351);//had to add true for fspermissions to delete stubs.  since we are patching a IOS anyways, i didnt think one more would hurt.
           /* ff = fopen("sd:/42.dol","r");
			if (ff)
			{
				gprintf("\n\tfat works here3");
				fclose(ff);
			
			}*/
			
			if (ret < 0) {
                printf("Installing fakesign accepting IOS failed. Exiting...");
                exit(0);
            }
            printf("IOS 36 installed as an IOS that can accept fakesigning!\n");
            printf("Now restore IOS 15 back to normal...\n");
            if (!yes_or_no())
                exit(0);
            Close_SD();
			Close_USB();
            WPAD_Shutdown();
            iosreloadcount++;
            IOS_ReloadIOS(36);
            WPAD_Init();
			fatInitDefault();
			/*ff = fopen("sd:/42.dol","r");
			if (ff)
			{
				gprintf("\n\tfat works here4");
				fclose(ff);
			
			}*/
            ret = install_unpatched_IOS(15, 523);
            if (ret < 0) {
                printf("Error restoring IOS 15! Exiting...");
                exit(0);
            }
            printf("You now have IOS 36 successfully installed as an IOS that can accept fakesigning!\n");
            printf("Continue to Dop-IOS MOD?");
            if (!yes_or_no())
                exit(0);
				
			/*ff = fopen("sd:/42.dol","r");
			if (ff)
			{
				gprintf("\n\tfat works here5");
				fclose(ff);
			
			}*/
			
        }

        iosreloadcount++;
        if (iosreloadcount == 10) {
            printf("\nERROR! Too many attempts to load IOS. Please restart the program. Exiting...");
            VIDEO_WaitVSync();
            exit(0);
        }

        else if (firstselection != 1) {

            printMyTitle();
            printf("\x1b[2;0H");
            printf("\n\nLoading selected IOS...\n");
			
			Close_SD();
			Close_USB();
            
            WPAD_Shutdown(); // We need to shut down the Wiimote(s) before reloading IOS or we get a crash. Video seems unaffected.--PhoenixTank
            ret = IOS_ReloadIOS(iosVersion[selectedios]);
            WPAD_Init(); // Okay to start wiimote up again.--PhoenixTank
			fatInitDefault();
			gprintf("\n\tios = %d",IOS_GetVersion());
            if (ret >= 0) {
                printf("\n\n\nIOS successfully loaded! Press A to continue.");
            }

        }

        while (true) {
            WPAD_ScanPads();
            PAD_ScanPads();
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_A) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_A))
                break;
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_HOME) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_HOME) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_Y))
                exit(0);
        }

        if ((ret < 0) && (ret != -1017)) {
            printf("\n\n\nERROR! Choose an IOS that accepts fakesigning! Press A to continue.");
            while (true) {
                WPAD_ScanPads();
                PAD_ScanPads();
                if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_A) || \
                        (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_A))
                    break;
                if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_HOME) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_HOME) || \
                        (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_Y))
                    exit(0);
            }
        }
        break;
    }

    /*This definines he max number of IOSs we can have to select from*/
    int selected=19;

    bool dontcheck = false;//Small fix for a small bug

    int screen = 0;//Checks what screen we're on
    int selection = 0;//IOSs or Channels?
    int orregion = 0;//Region or...?
    int systemselection = 0;//Which system menu?
    int regionselection = 0;//Which region?
    int channelselection = 0;//Which channel?

    getMyIOS();


    regionselection = CONF_GetRegion();

    switch (regionselection) {

    case CONF_REGION_JP:
        regionselection = 2;
        break;

    case CONF_REGION_EU:
        regionselection = 1;
        break;

    case CONF_REGION_US:
        regionselection = 0;
        break;

    case CONF_REGION_KR:
        regionselection = 3;
        break;

    default:
        regionselection = 0;
        break;
    }

    for (;;) {
        printMyTitle();

        PAD_ScanPads();
        WPAD_ScanPads();

        //Screen 0 -- Update Selection Screen
        if (screen == 0) {
            if (selection == 0) {
                printf("--> IOSs\n");
                printf("    Channels\n");
                printf("    System Menu\n");
				printf("    Remove stubbed IOSs");
            }
            if (selection == 1) {
                printf("    IOSs\n");
                printf("--> Channels\n");
                printf("    System Menu\n");
				printf("    Remove stubbed IOSs");
            }
            if (selection == 2) {
                printf("    IOSs\n");
                printf("    Channels\n");
                printf("--> System Menu\n");
				printf("    Remove stubbed IOSs");
            }
			if (selection == 3) {
			    printf("    IOSs\n");
				printf("    Channels\n");
				printf("    System Menu\n");
				printf("--> Remove stubbed IOSs");
			}
            printf("\n\n\n\n\n\n[UP]/[DOWN]       Change Selection\n");
            printf("[A]               Select\n");
            printf("[HOME]/GC:[Y]     Exit\n\n\n");
            if (dontcheck) {
                if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_HOME) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_HOME) || \
                        (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_Y))
                    break;//Exit
                if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_A) || \
                        (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_A)) {
                    if (selection == 0)
                        screen = 1;
                    if (selection == 1)
                        screen = 2;
                    if (selection == 2)
                        screen = 3;
					if (selection == 3)
					    screen = 4;
                    dontcheck = false;
                }
                if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_DOWN) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_DOWN) || \
                        (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_DOWN))
                    selection++;
                if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_UP) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_UP) || \
                        (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_UP))
                    selection--;
            }
            if (selection < 0)
                selection = 3;
            if (selection > 3)
                selection = 0;
        }//End Screen 0

        //Screen1 IOS Selection Screen
        if (screen == 1) {

            // Show menu
            printf("Select the IOS you want to dop.             Currently installed:\n\n        ");

            setConsoleFgColor(YELLOW,1);
            if (selected>0) {
                printf("<<  ");
            } else {
                printf("    ");
            }
            setConsoleFgColor(WHITE,0);
            u32 major=ioses[selected].major;
            u32 minor=ioses[selected].minor;
            u32 newest=ioses[selected].newest;
            u8 type=ioses[selected].type;
            printf("IOS%d",major);
            setConsoleFgColor(YELLOW,1);
            if ((selected+1)<MAX_IOS) {
                printf("  >>");
            }
            setConsoleFgColor(WHITE,0);
            printf("                            ");
            u32 installed=ios_found[major];
            if (installed) {
                printf("v%d",ios_found[major]);
                if ((type==STUB_NOW || type==PROTECTED) && newest==installed)
                    printf(" (stub)");
            } else {
                printf("(none)");
            }
            printf("\n\n");
            printf("%s",ioses[selected].desc);
            printf("\n\n\n");



            //Show options
            printf("[LEFT/RIGHT]      Select IOS\n");
            if (type!=PROTECTED) {
                if (type!=STUB_NOW)
                    printf("[A]               Install latest v%d of IOS%d\n",newest,major);
                else
                    printf("\n");

                if (type!=LATEST)
                    printf("[-]/GC:[X]        Install old v%d of IOS%d\n",minor,major);
                else
                    printf("\n");

            } else {
                printf("\n\n");
            }
            printf("[B]               Back\n");
            printf("[HOME]/GC:[Y]     Exit\n\n\n\n\n\n\n\n");
            printf("                        -- Dop-IOS MOD by Arikado, SifJar, PhoenixTank");

            u32 pressed = WPAD_ButtonsDown(WPAD_CHAN_0);

            if (dontcheck) {
                if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_B) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_B) || \
                        (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_B))
                    screen = 0;
                if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_HOME) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_HOME) || \
                        (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_Y)) {
                    break;
                } else if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_RIGHT) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_RIGHT) || \
                           (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_RIGHT)) {
                    selected++;
                    if (selected==MAX_IOS)
                        selected=MAX_IOS-1;
                } else if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_LEFT) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_LEFT) || \
                           (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_LEFT)) {
                    selected--;
                    if (selected==-1)
                        selected=0;
                } else if ((pressed & WPAD_BUTTON_A || pressed & WPAD_CLASSIC_BUTTON_A || (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_A)) && (type==NORMAL || type==LATEST)) {
                    doparIos(major,newest,true);
                } else if ((pressed & WPAD_BUTTON_MINUS || pressed & WPAD_CLASSIC_BUTTON_MINUS || (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_X)) && (type==NORMAL || type==STUB_NOW)) {
                    doparIos(major,minor,false);
                }
            }

        }//End screen1

        //Screen 2 = Channel Choice
        if (screen == 2) {
            if (orregion == 0) {
                printf("--> Install Channel: %s\n", channels[channelselection].name);
                printf("    Region:          %s\n\n\n", regions[regionselection].name);
            }
            if (orregion == 1) {
                printf("    Install Channel: %s\n", channels[channelselection].name);
                printf("--> Region:          %s\n\n\n", regions[regionselection].name);
            }
            if (dontcheck) {
                if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_A) || \
                        (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_A)) {
                    if (yes_or_no())
                        InstallTheChosenChannel(regionselection, channelselection);
                }
            }
            printf("[UP]/[DOWN] [LEFT]/[RIGHT]       Change Selection\n");
            printf("[A]                              Select\n");
            printf("[B]                              Back\n");
            printf("[HOME]/GC:[Y]                    Exit\n\n\n");
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_DOWN) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_DOWN) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_DOWN))
                orregion++;
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_UP) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_UP) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_UP))
                orregion--;
            if (orregion > 1)
                orregion = 0;
            if (orregion < 0)
                orregion = 1;
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_LEFT) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_LEFT) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_LEFT)) {
                if (orregion == 0)
                    channelselection--;
                if (orregion == 1)
                    regionselection--;
            }
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_RIGHT) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_RIGHT) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_RIGHT)) {
                if (orregion == 0)
                    channelselection++;
                if (orregion == 1)
                    regionselection++;
            }
            if (channelselection < 0)
                channelselection = MAX_CHANNEL;
            if (channelselection > MAX_CHANNEL)
                channelselection = 0;
            if (regionselection < 0)
                regionselection = MAX_REGION;
            if (regionselection > MAX_REGION)
                regionselection = 0;
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_B) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_B) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_B))
                screen = 0;
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_HOME) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_HOME) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_Y))
                break;
        }

        //Screen 3 = System Menu Choice
        if (screen == 3) {
            //Quick Fix
            if (regionselection == MAX_REGION && systemselection == 0)
                systemselection = 1;
            if (orregion == 0) {
                printf("--> Install System Menu: %s\n", systemmenus[systemselection].name);
                printf("    Region:              %s\n\n\n", regions[regionselection].name);
            }
            if (orregion == 1) {
                printf("    Install System Menu: %s\n", systemmenus[systemselection].name);
                printf("--> Region:              %s\n\n\n", regions[regionselection].name);
            }
            if (dontcheck) {
                if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_A) || \
                        (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_A)) {
                    if (yes_or_no())
                        InstallTheChosenSystemMenu(regionselection, systemselection);
                }
            }
            printf("[UP]/[DOWN] [LEFT]/[RIGHT]       Change Selection\n");
            printf("[A]                              Select\n");
            printf("[B]                              Back\n");
            printf("[HOME]/GC:[Y]                    Exit\n\n\n");
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_DOWN) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_DOWN) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_DOWN))
                orregion++;
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_UP) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_UP) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_UP))
                orregion--;
            if (orregion > 1)
                orregion = 0;
            if (orregion < 0)
                orregion = 1;
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_LEFT) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_LEFT) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_LEFT)) {
                if (orregion == 0)
                    systemselection--;
                if (orregion == 1)
                    regionselection--;
                //Only let 3.5 appear if Korea is the selected region
                if (systemselection == 3 && regionselection != MAX_REGION)
                    systemselection--;
                //Get rid of certain menus from Korea selection
                if (regionselection == MAX_REGION && (systemselection == 0 || systemselection == 2 || systemselection == 4))
                    systemselection--;
            }
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_RIGHT) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_RIGHT) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_RIGHT)) {
                if (orregion == 0)
                    systemselection++;
                if (orregion == 1)
                    regionselection++;
                //Only let 3.5 appear if Korea is the selected region
                if (systemselection == 3 && regionselection != MAX_REGION)
                    systemselection++;
                //Get rid of certain menus from Korea selection
                if (regionselection == MAX_REGION && (systemselection == 0 || systemselection == 2 || systemselection == 4))
                    systemselection++;
            }
            if (systemselection < 0)
                systemselection = MAX_SYSTEMMENU;
            if (systemselection > MAX_SYSTEMMENU)
                systemselection = 0;
            if (regionselection < 0)
                regionselection = MAX_REGION;
            if (regionselection > MAX_REGION)
                regionselection = 0;
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_B) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_B) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_B))
                screen = 0;
            if ((WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_HOME) || (WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_CLASSIC_BUTTON_HOME) || \
                    (PAD_ButtonsDown(PAD_CHAN_0)&PAD_BUTTON_Y))
                break;
        }//End Screen 3
		
		if(screen == 4){
			printf("\n\nAre you sure you want to check for stub IOSs and delete them to\nfree up the 2 precious blocks they take up on that little nand?\n\n");
	       if (yes_or_no()) {
		       if (!checkAndRemoveStubs()) {
			printf("\n\nNo stubs found!");
			sleep(3);
		}
		 }
		  }

        VIDEO_WaitVSync();
        VIDEO_WaitVSync();

        if (!dontcheck)
            dontcheck = true;
    }


    //STM_RebootSystem();
    printf("\nReturning to loader...");
    return 0;
}
