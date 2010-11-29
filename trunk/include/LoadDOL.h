// Copyright 2010 Joseph Jordan <joe.ftpii@psychlaw.com.au>
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
#ifndef _LOADDOL_H
#define _LOADDOL_H

#include <gccore.h>
class LoadDOL
{
  public:
    static u32 load_dol_image(void *dolstart, struct __argv *argv);
    static int automain(char *filename);
};
#endif /* _LOADDOL_H */
