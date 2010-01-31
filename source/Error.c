#include <stdio.h>
#include "Error.h"

char* EsErrorCodeString(int errorCode)
{
	char* message = "";

	switch (errorCode)
	{
		case -106: message = "(-106) Invalid TMD when using ES_OpenContent or access denied."; break;
		case -1009: message = "(-1009) Read failure (short read)."; break;
		case -1010: message = "(-1010) Write failure (short write)."; break;
		case -1012: message = "(-1012) Invalid signature type."; break;
		case -1015: message = "(-1015) Invalid value for byte at 0x180 in ticket (valid:0,1,2)"; break;
		case -1017: message = "(-1017) Wrong IN or OUT size, wrong size for a part of the vector, vector alignment problems, non-existant ioctl."; break;
		case -1020: message = "(-1020) ConsoleID mismatch"; break;
		case -1022: message = "(-1022) Content did not match hash in TMD."; break;
		case -1024: message = "(-1024) Memory allocation failure."; break;
		case -1026: message = "(-1026) Incorrect access rights."; break;
		case -1028: message = "(-1028) No ticket installed."; break;
		case -1029: message = "(-1029) Installed Ticket/TMD is invalid"; break;
		case -1035: message = "(-1035) Title with a higher version is already installed."; break;
		case -1036: message = "(-1036) Required sysversion(IOS) is not installed."; break;
		case -2008: message = "(-2008) Invalid parameter(s)."; break;		
		case -2011: message = "(-2011) Signature check failed."; break;
		case -2013: message = "(-2013) Keyring is full (contains 0x20 keys)."; break;
		case -2014: message = "(-2014) Bad has length (!=20)"; break;
		case -2016: message = "(-2016) Unaligned Data."; break;
		case -4100: message = "(-4100) Wrong Ticket-, Cert size or invalid Ticket-, Cert data"; break;
		default: sprintf(message, "(%d) Unknown Error", errorCode);
	}
	
	return message;	
}

char* FsErrorCodeString(int errorCode)
{
	char* message = "";

	switch (errorCode)
	{
		case -1: message = "(-1) Permission Denied."; break;
		case -2: message = "(-2) File Exists."; break;
		case -4: message = "(-4) Invalid Argument."; break;
		case -6: message = "(-6) File not found."; break;
		case -8: message = "(-8) Resource Busy."; break;
		case -12: message = "(-12) Returned on ECC error."; break;
		case -22: message = "(-22) Alloc failed during request."; break;
		case -102: message = "(-102) Permission denied."; break;
		case -103: message = "(-103) Returned for \"corrupted\" NAND."; break;
		case -105: message = "(-105) File exists."; break;
		case -106: message = "(-106) File not found."; break;
		case -107: message = "(-107) Too many fds open."; break;
		case -108: message = "(-108) Memory is full."; break;
		case -109: message = "(-190) Too many fds open."; break;
		case -110: message = "(-110) Path Name is too long."; break;
		case -111: message = "(-111) FD is already open."; break;
		case -114: message = "(-114) Returned on ECC error."; break;
		case -115: message = "(-115) Directory not empty."; break;
		case -116: message = "(-116) Max Directory Depth Exceeded."; break;
		case -118: message = "(-118) Resource busy."; break;
		case -119: message = "(-119) Fatal Error."; break;
		default: sprintf(message, "(%d) Unknown Error", errorCode);
	}

	return message;
}