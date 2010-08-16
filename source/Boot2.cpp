#include <stdio.h>
#include <cstdlib>
#include <gccore.h>
#include <malloc.h>
#include <string.h>

#include "Error.h"
#include "Gecko.h"
#include "Boot2.h"
#include "Boot2v4_wad.h"
#include "Tools.h"
#include "Wad.h"
#include "rijndael.h"
#include "sha1.h"
#include "Title.h"

#define ALIGN(a,b) ((((a)+(b)-1)/(b))*(b))

int Boot2::Install(u16 version)
{
	switch (version)
	{
	case 4: return Install(Boot2v4_wad, Boot2v4_wad_size);
	default: 
		gcprintf("Unknown Boot2 Version (%u)\n", version);
		return -1;
	}

	return 0;
}

int Boot2::Install(const u8 *wad, u32 wadLen)
{	
	int ret = 0;
	signed_blob *certs = NULL;
	signed_blob *xsCert = NULL;
	signed_blob *cpCert = NULL;
	signed_blob *caCert = NULL;

	signed_blob *stmd = NULL;
	signed_blob *tmdCert = NULL;
	u32 tmdCertSize = 0;
	tmd *ptmd = NULL;	

	signed_blob *ticket = NULL;
	signed_blob *ticketCert = NULL;
	u32 ticketCertSize = 0;

	u8 *contentEnc = NULL;
	u8 *contentDec = NULL;
	u32 contentSize = 0;
	u8 key[16];
	u8 iv[16];
	sha1 hash;

	tmd_content tmdContent;

	WadHeader *header = (WadHeader*)wad;

	if (header->HeaderSize != sizeof(WadHeader))
	{
		gcprintf("Invalid Boot2 Header Length\n");
		return -1;
	}
	const u8 *wadp = wad + ROUND_UP(header->HeaderSize, 64);

	certs = (signed_blob*)Tools::AllocateMemory(header->CertsSize);
	if (!certs)
	{
		gcprintf("\n>> ERROR! Out Of Memory\n");
		ret = -1;
		goto end;
	}
	memcpy(certs, wadp, header->CertsSize);
	wadp += ROUND_UP(header->CertsSize, 64);

	// Skip CRL if included
	wadp += ROUND_UP(header->CrlSize, 64);

	ticket = (signed_blob*)Tools::AllocateMemory(header->TicketSize);
	if (!ticket)
	{
		gcprintf("\n>> ERROR! Out Of Memory\n");
		ret = -1;
		goto end;
	}
	memcpy(ticket, wadp, header->TicketSize);
	wadp += ROUND_UP(header->TicketSize, 64);

	if (!IS_VALID_SIGNATURE(ticket))
	{
		gcprintf("\n>> ERROR! Invalid Ticket\n");
		ret = -1;
		goto end;
	}

	stmd = (signed_blob*)Tools::AllocateMemory(header->TmdSize);
	if (!stmd)
	{
		gcprintf("\n>> ERROR! Out Of Memory\n");
		ret = -1;
		goto end;
	}
	memcpy(stmd, wadp, header->TmdSize);
	wadp += ROUND_UP(header->TmdSize, 64);

	xsCert = FindCert(certs, header->CertsSize, "XS");
	if (!xsCert) 
	{
		gcprintf("\n>> ERROR! No XS Cert\n");
		ret = -1;
		goto end;
	}

	cpCert = FindCert(certs, header->CertsSize, "CP");
	if (!cpCert)
	{
		gcprintf("\n>> ERROR! No CP Cert\n");
		ret = -1;
		goto end;
	}

	caCert = FindCert(certs, header->CertsSize, "CA");
	if (!caCert)
	{
		gcprintf("\n>> ERROR! No CA Cert\n");
		ret = -1;
		goto end;
	}

	ticketCertSize = SIGNED_CERT_SIZE(xsCert) + SIGNED_CERT_SIZE(caCert);
	ticketCert = (signed_blob*)Tools::AllocateMemory(ticketCertSize);
	if (!ticketCert)
	{
		gcprintf("\n>> ERROR! Out Of Memory\n");
		ret = -1; goto end;
	}
	memcpy(ticketCert, xsCert, SIGNED_CERT_SIZE(xsCert));
	memcpy(((u8*)ticketCert) + SIGNED_CERT_SIZE(xsCert), caCert, SIGNED_CERT_SIZE(caCert));

	tmdCertSize = SIGNED_CERT_SIZE(cpCert) + SIGNED_CERT_SIZE(caCert);
	tmdCert = (signed_blob*)Tools::AllocateMemory(tmdCertSize);
	if (!tmdCert)
	{
		gcprintf("\n>> ERROR! Out Of Memory\n");
		ret = -1; goto end;
	}
	memcpy(tmdCert, cpCert, SIGNED_CERT_SIZE(cpCert));
	memcpy(((u8*)tmdCert) + SIGNED_CERT_SIZE(cpCert), caCert, SIGNED_CERT_SIZE(caCert));

	ptmd = (tmd*)SIGNATURE_PAYLOAD(stmd);

	contentSize = ALIGN(ptmd->contents[0].size, 16);
	contentEnc = (u8*)Tools::AllocateMemory(contentSize);
	contentDec = (u8*)Tools::AllocateMemory(contentSize);
	if (!contentEnc || !contentDec)
	{
		gcprintf("\n>> ERROR! Out Of Memory\n");
		ret = -1; goto end;
	}	
	memcpy(contentEnc, wadp, contentSize);

	/* Decrypt Content and Check Hashes */
	Title::GetTitleKey(ticket, key);
	aes_set_key(key);
	
	memset(iv, 0, 16);
	aes_decrypt(iv, contentEnc, contentDec, contentSize);	

	tmdContent = TMD_CONTENTS(ptmd)[0];

	SHA1(contentDec, tmdContent.size, hash);
	if (memcmp(tmdContent.hash, hash, sizeof(hash)) != 0)
	{
		gprintf("Invalid Content Hash\n");
		return -1;
	}	

	/* Now the scary part. Install Boot2 */
	ret = ES_ImportBoot(ticket, header->TicketSize, ticketCert, ticketCertSize,
		  stmd, header->TmdSize, tmdCert, tmdCertSize, contentEnc, contentSize);
	
	gprintf("ES_ImportBoot = %s\n", EsError::ToString(ret));

end:
	memset(hash, 0, sizeof(hash));
	delete contentDec; contentDec = NULL;
	delete contentEnc; contentEnc = NULL;
	delete stmd; stmd = NULL;
	delete tmdCert; tmdCert = NULL;
	delete ticket; ticket = NULL;
	delete ticketCert; ticketCert = NULL;
	delete certs; certs = NULL;
	return ret;
}

signed_blob* Boot2::FindCert(signed_blob *certs, u32 certsSize, const char *match)
{
	signed_blob *cp = certs;
	while ((u32)cp < ((u32)certs + certsSize))
	{
		cert_rsa2048 *rc = (cert_rsa2048*)SIGNATURE_PAYLOAD(cp);
		if (!strncmp(rc->cert_name, match, strlen(match))) return cp;
		cp = (signed_blob*)(((u32)cp) + SIGNED_CERT_SIZE(cp));
	}

	return NULL;
}