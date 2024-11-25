#include <vitasdk.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "io.h"
#include "log.h"
#include "err.h"
#include "pkg.h"
#include "promote.h"

int make_psp_fake_rif(const char* rif, const char* content_id) {
	SceNpDrmLicense licenseBuf;
	memset(&licenseBuf, 0x00, sizeof(SceNpDrmLicense));
	
	licenseBuf.account_id = 0x0123456789ABCDEFLL;
	memset(licenseBuf.ecdsa_signature, 0xFF, 0x28);
	strncpy(licenseBuf.content_id, content_id, 0x30);
	
	return (write_file(rif, &licenseBuf, sizeof(SceNpDrmLicense)) > 0);
}

int is_zeroed_license(const char* rif) {
	
	SceNpDrmLicense licenseBuf;
	memset(&licenseBuf, 0, sizeof(SceNpDrmLicense));
	
	if(read_file(rif, &licenseBuf, sizeof(SceNpDrmLicense)) < 0) return 0;
	for(int i = 0; i < sizeof(SceNpDrmLicense); i++) if(((char*)(&licenseBuf))[i] != 0) return 0;
	
	return 1;

}

int is_npdrm_free_license(const char* rif) {
	SceNpDrmLicense licenseBuf;
	memset(&licenseBuf, 0, sizeof(SceNpDrmLicense));
	
	if(read_file(rif, &licenseBuf, sizeof(SceNpDrmLicense)) < offsetof(SceNpDrmLicense, flags)) return 0;
	if(licenseBuf.version == -1 && licenseBuf.account_id == 0L) return 1;
	
	return 0;
}

int is_rif_required(const char* package, const char* app_directory) {
	char workBin[MAX_PATH];
	char tempBin[MAX_PATH];
	
	memset(workBin, 0, sizeof(workBin));
	memset(tempBin, 0, sizeof(tempBin));
	
	snprintf(workBin, sizeof(workBin), "%s/%s", app_directory, "sce_sys/package/work.bin");
	snprintf(tempBin, sizeof(tempBin), "%s/%s", app_directory, "sce_sys/package/temp.bin");
	
	
	PKG_STATE packageState = package_state(package);
	int contentType = packageState.pkgMetadata.content_type;
	int packageRevision = packageState.pkgHeader.revision;
	int packageFlags = packageState.pkgMetadata.package_flags;
	
	
	PRINT_STR("TempBin = %s\nWorkBin = %s\npackageRevision = 0x%X\npackageFlags = 0x%X\n", tempBin, workBin, packageRevision, packageFlags);
	
	
	// a license is not required to install a package if ....
	if(file_exist(workBin)) return 0; // if a license already exists (eg included in package, npdrmfree and such)
	if(file_exist(tempBin) && (packageRevision == PKG_REVISION_DEBUG && is_zeroed_license(tempBin))) return 0; // if its a debug package
	if(file_exist(tempBin) && is_npdrm_free_license(tempBin)) return 0; // if there is a npdrmfree license ..
	if( (contentType == PKG_TYPE_VITA_APP || IS_PSP_CONTENT_TYPE(contentType)) && ((packageFlags & PKG_FLAG_PATCH_FILE) != 0) ) return 0; // or, if the package is a Vita/Psp TitleUpdate
	
	return 1;
}