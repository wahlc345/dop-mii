typedef struct
{
    signed_blob *certs;
    signed_blob *ticket;
    signed_blob *tmd;
    signed_blob *crl;

    u32 certs_size;
    u32 ticket_size;
    u32 tmd_size;
    u32 crl_size;

    u8 **encrypted_buffer;
    u8 **decrypted_buffer;
    u32 *buffer_size;

    u32 content_count;
} ATTRIBUTE_PACKED IOS;

void decrypt_IOS(IOS *ios);
s32 Init_IOS(IOS **ios);
void free_IOS(IOS **ios);
s32 set_content_count(IOS *ios, u32 count);
s32 IosInstall(u32 iosVersion, u32 iosRevision, bool esTruchaPatch, bool esIdentifyPatch, bool nandPatch); //, bool nand_patch, u32 location, u32 newrevision);
s32 IosInstallUnpatched(u32 iosversion, u32 revision);
s32 IosDowngrade(u32 iosVersion, u32 highRevision, u32 lowRevision);
int patch_hash_check(u8 *buf, u32 size);
int patch_identify_check(u8 *buf, u32 size);
