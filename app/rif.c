#include <vitasdk.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "io.h"
#include "log.h"
#include "err.h"

int make_psp_fake_rif(const char* rif, const char* content_id) {
	SceNpDrmLicense licenseBuf;
	memset(&licenseBuf, 0x00, sizeof(SceNpDrmLicense));
	
	licenseBuf.account_id = 0x0123456789ABCDEFLL;
	memset(licenseBuf.ecdsa_signature, 0xFF, 0x28);
	strncpy(licenseBuf.content_id, content_id, 0x30);
	
	return (write_file(rif, &licenseBuf,  offsetof(SceNpDrmLicense, flags)) > 0);
}

int is_debug_license(const char* rif) {
	
	SceNpDrmLicense licenseBuf;
	memset(&licenseBuf, 0, sizeof(SceNpDrmLicense));
	
	if(read_file(rif, &licenseBuf, sizeof(SceNpDrmLicense)) < 0) return 0;
	for(int i = 0; i < sizeof(SceNpDrmLicense); i++) if(((char*)(&licenseBuf))[i] != 0) return 0;
	
	return 1;

}

int is_npdrm_free_license(const char* rif) {
	SceNpDrmLicense licenseBuf;
	memset(&licenseBuf, 0, sizeof(SceNpDrmLicense));
	
	if(read_file(rif, &licenseBuf, sizeof(SceNpDrmLicense)) != sizeof(SceNpDrmLicense)) return 0;
	if(licenseBuf.version == 0xFFFF && licenseBuf.account_id == 0) return 1;
	
	return 0;
}
