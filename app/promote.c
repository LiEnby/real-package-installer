#include <vitasdk.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "err.h"

int loadScePaf() {
	static uint32_t argp[] = { 0x180000, -1, -1, 1, -1, -1 };
	int res = -1;

	SceSysmoduleOpt opt;

	memset(&opt, 0xFF, sizeof(SceSysmoduleOpt));
	opt.flags = sizeof(SceSysmoduleOpt);
	opt.result = &res;

	return sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(argp), argp, &opt);
}

int unloadScePaf() {
	SceSysmoduleOpt opt;
	memset(&opt, 0, sizeof(opt));
	return sceSysmoduleUnloadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, 0, NULL, &opt);
}

int promote(const char *path, void (*progress_callback)(const char*, uint64_t, uint64_t)) {
	uint64_t done = 0;
	uint64_t total = 6;
	
	CHECK_ERROR(loadScePaf());
	progress_callback(path, done++, total);
	
	CHECK_ERROR(sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL));
	progress_callback(path, done++, total);
	
	CHECK_ERROR(scePromoterUtilityInit());
	progress_callback(path, done++, total);
	
	CHECK_ERROR(scePromoterUtilityPromotePkgWithRif(path, 1));
	progress_callback(path, done++, total);
	
	CHECK_ERROR(scePromoterUtilityExit());
	progress_callback(path, done++, total);
	
	CHECK_ERROR(sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL));
	progress_callback(path, done++, total);
	
	CHECK_ERROR(unloadScePaf());
	progress_callback(path, done++, total);

	return 0;
}