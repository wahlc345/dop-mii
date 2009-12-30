#include "ticket_dat.h"
#include "tmd_dat.h"

#define roundTo32(x) (-(-(x) & -(32)))
#define makeTitleId(x,y) (((u64)(x) << 32) | (y))

int CheckUsb2Module();
int CheckFlashAccess();
int CheckBoot2Access();
int CheckEsIdentify();
char* CheckRegion();
int sortCallback(const void * first, const void * second);
int GetCert();
int ScanIos();
int writebackLog();

static u8 certs_sys[0xA00] ATTRIBUTE_ALIGN(32);