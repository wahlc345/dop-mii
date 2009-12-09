#ifndef _TITLE_INSTALL_H_
#define _TITLE_INSTALL_H_

/* Prototypes */
s32 Title_Download(u64, u16, signed_blob **, signed_blob **);
s32 Title_ExtractWAD(u8 *, signed_blob **, signed_blob **);
s32 Title_Install(signed_blob *, signed_blob *);
s32 Title_Clean(signed_blob *);

s32 Uninstall_FromTitle(const u64 tid);
//void uninstall_checked(u32 kind, u32 title);

#endif

