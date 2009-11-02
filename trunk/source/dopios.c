/*-------------------------------------------------------------

dopios.c - Dop-IOS v5 - install and patch any IOS
 
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
#include <wiiuse/wpad.h>

#include "wiibasics.h"
#include "patchmii_core.h"

#define PROTECTED	0
#define NORMAL		1
#define STUB_NOW	2
#define LATEST		3
#define MAX_IOS		31
#define DEFAULT_IOS	19

#define BLACK	0
#define RED		1
#define GREEN	2
#define YELLOW	3
#define BLUE	4
#define MAGENTA	5
#define CYAN	6
#define WHITE	7

struct ios{
	u32 major;
	u32 minor;
	u32 newest;
	u8 type;
	char* desc;
};


const struct ios ioses[]={
	{4,65280,65280,PROTECTED,"Stub, useless now."},
	{9,520,521,NORMAL,"Used by launch titles (Zelda: Twilight Princess) and System Menu 1.0."},
	{10,768,768,PROTECTED,"Stub, useless now."},
	{11,10,256,STUB_NOW,"Used only by System Menu 2.0 and 2.1."},
	{12,6,12,NORMAL,""},
	{13,10,16,NORMAL,""},
	{14,262,263,NORMAL,""},
	{15,257,266,NORMAL,""},
	{16,512,512,PROTECTED,"Piracy prevention stub, useless."},
	{17,512,518,NORMAL,""},
	{20,12,0,STUB_NOW,"Used only by System Menu 2.2."},
	{21,514,525,NORMAL,"Used by old third-party titles (No More Heroes)."},
	{22,777,780,NORMAL,""},
	{28,1292,1293,NORMAL,""},
	{30,1040,2816,STUB_NOW,"Used only by System Menu 3.0, 3.1, 3.2 and 3.3."},
	{31,1040,3092,NORMAL,""},
	{33,1040,2834,NORMAL,""},
	{34,1039,3091,NORMAL,""},
	{35,1040,3092,NORMAL,"Used by: Super Mario Galaxy."},
	{36,1042,3094,NORMAL,"Used by: Smash Bros. Brawl, Mario Kart Wii. Can be ES_Identify patched."},
	{37,2070,3612,NORMAL,"Used mostly by music games (Guitar Hero)."},
	{38,3610,3610,LATEST,"Used by some modern titles (Animal Crossing)."},
	{50,4889,5120,STUB_NOW,"Used only by System Menu 3.4."},
	{51,4633,4864,STUB_NOW,"Used only by Shop Channel 3.4."},
	{53,4113,5149,NORMAL,"Used by some modern games and channels."},
	{55,4633,5149,NORMAL,"Used by some modern games and channels."},
	{56,4890,4890,NORMAL,"Used only by Wii Speak Channel 2.0."},
	{57,5404,5404,LATEST,"Unknown yet."},
	{60,6174,6174,LATEST,"Used by System Menu 4.0."},
	{61,4890,4890,LATEST,"Used by Shop Channel 4.0."},
	{254,2,3,PROTECTED,"PatchMii prevention stub, useless."}
};
	
	

u32 ios_found[256];

void setConsoleFgColor(u8 color,u8 bold){
	printf("\x1b[%u;%um", color+30,bold);
	fflush(stdout);
}

void setConsoleBgColor(u8 color,u8 bold){
	printf("\x1b[%u;%um", color+40,bold);
	fflush(stdout);
}

void clearConsole(){
	printf("\x1b[2J");
}

void printMyTitle(){
    clearConsole();
	setConsoleBgColor(BLUE,0);
	setConsoleFgColor(WHITE,1);
	printf("                                                                        ");
	printf("                  Dop-IOS (download-patch-install)  v7                  ");
	printf("                                                                        ");
	setConsoleBgColor(BLACK,0);
	setConsoleFgColor(WHITE,0);
	fflush(stdout);
	printf("\n\n");
}


bool getMyIOS(){
	s32 ret;
	u32 count;
	int i;
	for(i=0;i<256;i++){
		ios_found[i]=0;
	}
    printMyTitle();
    printf("Loading installed IOS...");

	ret=ES_GetNumTitles(&count);
	if(ret){
		printf("ERROR: ES_GetNumTitles=%d\n", ret);
		return false;
	} 

	static u64 title_list[256] ATTRIBUTE_ALIGN(32);
	ret=ES_GetTitles(title_list, count);
	if(ret){
		printf("ERROR: ES_GetTitles=%d\n", ret);
		return false;
	}


	for(i=0;i<count;i++){
		u32 tmd_size;
		ret=ES_GetStoredTMDSize(title_list[i], &tmd_size);
		static u8 tmd_buf[MAX_SIGNED_TMD_SIZE] ATTRIBUTE_ALIGN(32);
		signed_blob *s_tmd=(signed_blob *)tmd_buf;
		ret=ES_GetStoredTMD(title_list[i], s_tmd, tmd_size);

		tmd *t=SIGNATURE_PAYLOAD(s_tmd);
		u32 tidh=t->title_id >> 32;
		u32 tidl=t->title_id & 0xFFFFFFFF;

		if(tidh==1 && t->title_version)
			ios_found[tidl]=t->title_version;

		if(i%2==0)
			printf(".");
	}
	return true;
}


void doparIos(u32 major, u32 minor,bool newest){
    printMyTitle();

    printf("Are you sure you want to install ");
	setConsoleFgColor(YELLOW,1);
	printf("IOS%d",major);
	printf(" v%d",minor);
	setConsoleFgColor(WHITE,0);
	printf("?\n");

	if(yes_or_no()){
		bool sig_check_patch=false;
		bool es_identify_patch=false;
		if(major>36 || newest){
			printf("\nApply Sig Hash Check patch in IOS%d?\n",major);
			sig_check_patch=yes_or_no();
			if(major==36){
				printf("\nApply ES_Identify patch in IOS%d?\n",major);
				es_identify_patch=yes_or_no();
			}
		}

		int ret = patchmii_install(1, major, minor, 1, major, minor, sig_check_patch,es_identify_patch);
		if (ret < 0) {
			printf("ERROR: Something failed. (ret: %d)\n",ret);
			printf("Press any key to continue.\n");
			wait_anyKey();
		}
		printf("Press any key to continue...");
		wait_anyKey();
		getMyIOS();
	}
}







int main(int argc, char **argv) {

    basicInit();
	WPAD_Init();

	int ourios = 0;
	int tracker = 1;
	int totalios = 5;
	
	//Basic scam warning, brick warning, and credits by Arikado
	printf("\x1b[2J");
	printf("\x1b[2;0H");
	printf("Welcome to Dop-IOS MOD - a modification of Dop-IOS!\n\n");
	printf("If you have paid for this software, you have been scammed.\n\n");
	printf("If misused, this software WILL brick your Wii.\n");
	printf("The authors of DOP-IOS MOD are not responsible if this occurs.\n\n");
	printf("Created by:\n");
	printf("SifJar\n");
	printf("PheonixTank\n");
	printf("Arikado - http://arikadosblog.blogspot.com\n\n");
	printf("Press A to continue. Press HOME to exit.");
	
	for(;;){
	
	WPAD_ScanPads();
	
	if(WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A)
	break;
	
	if(WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_HOME)
	exit(0);
	
	VIDEO_WaitVSync();
	
	}


	// This loop also created by Arikado
	for(;;){
		WPAD_ScanPads();

		if(WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_A)
		break;

		if(WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_HOME)
		exit(0);

		if(WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_RIGHT)
		tracker++;

		if(WPAD_ButtonsDown(WPAD_CHAN_0)&WPAD_BUTTON_LEFT)
		tracker--;

		if(tracker < 1)
		tracker = totalios;

		if(tracker > totalios)
		tracker = 1;
		
		if(tracker == 1)
		ourios = 35;

		if(tracker == 2)
		ourios = 36;

		if(tracker == 3)
		ourios = 60;

		if(tracker == 4)
		ourios = 61;
		
		if(tracker == 5)
		ourios = 249;

        printf("\x1b[2J");
		printf("\x1b[2;0H");
		printf("Which IOS would you like to use to install other IOSs?\n");
		printf("IOS: %d", ourios);

		VIDEO_WaitVSync();
	}

	// Issue corrected by PhoenixTank
	WPAD_Shutdown(); // We need to shut down the Wiimote(s) before reloading IOS or we get a crash. Video seems unaffected.

	int ret = 0;
	ret = IOS_ReloadIOS(ourios);

	WPAD_Init(); // Okay to start video up again.


	int selected=DEFAULT_IOS;
	getMyIOS();

	for(;;){
		printMyTitle();

		// Show menu
		printf("Select the IOS you want to dop.             Currently installed:\n\n        ");

		setConsoleFgColor(YELLOW,1);
		if(selected>0){
			printf("<<  ");
		}else{
			printf("    ");
		}
		setConsoleFgColor(WHITE,0);
		u32 major=ioses[selected].major;
		u32 minor=ioses[selected].minor;
		u32 newest=ioses[selected].newest;
		u8 type=ioses[selected].type;
		printf("IOS%d",major);
		setConsoleFgColor(YELLOW,1);
		if((selected+1)<MAX_IOS){
			printf("  >>");
		}
		setConsoleFgColor(WHITE,0);
		printf("                            ");
		u32 installed=ios_found[major];
		if(installed){
			printf("v%d",ios_found[major]);
			if((type==STUB_NOW || type==PROTECTED) && newest==installed)
				printf(" (stub)");
		}else{
			printf("(none)");
		}
		printf("\n\n");
        printf("%s",ioses[selected].desc);
		printf("\n\n\n");



		//Show options                
        printf("[LEFT/RIGHT]   Select IOS\n");
        if(type!=PROTECTED){
	        if(type!=STUB_NOW)
	        	printf("[A]            Install latest v%d of IOS%d\n",newest,major);
	        else
	        	printf("\n");

	        if(type!=LATEST)
	        	printf("[-]            Install old v%d of IOS%d\n",minor,major);
	        else
	        	printf("\n");
	        	
        }else{
	        printf("\n\n");
        }
        printf("[HOME]         Exit\n\n\n\n\n\n\n\n\n");
        printf("                                                 -- Dop-IOS by Marc");

		u32 pressed = wait_anyKey();
		if (pressed & WPAD_BUTTON_HOME){
			break;
		}else if (pressed & WPAD_BUTTON_RIGHT){
			selected++;
			if(selected==MAX_IOS)
				selected=MAX_IOS-1;
		}else if (pressed & WPAD_BUTTON_LEFT){
			selected--;
			if(selected==-1)
				selected=0;
		}else if (pressed & WPAD_BUTTON_A && (type==NORMAL || type==LATEST)){
			doparIos(major,newest,true);
		}else if (pressed & WPAD_BUTTON_MINUS && (type==NORMAL || type==STUB_NOW)){
			doparIos(major,minor,false);
		}
	}


	//STM_RebootSystem();
	printf("\nReturning to loader...");
	return 0;
}
