#include <vitasdk.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "err.h"
#include "pkg.h"

// promote directories
const char* PROMOTE_VITA_APP    =	"ux0:/temp/app";
const char* PROMOTE_VITA_DLC    =	"ux0:/temp/addcont";
const char* PROMOTE_VITA_PATCH  =	"ux0:/temp/patch";
const char* PROMOTE_VITA_THEME  =	"ux0:/temp/theme";
const char* PROMOTE_PSM 		=	"ux0:/temp/psm";
const char* PROMOTE_PSP 		=	"ux0:/pspemu/temp/game";

const char* GENERAL_PKG_OUT_LOCATION = "ux0:/package/out";

int need_promote(int content_type) {
	switch(content_type) {
		case PKG_TYPE_UNK_PS3:
		case PKG_TYPE_UNK_2:
		case PKG_TYPE_UNK_3:
		case PKG_TYPE_PS3_GAMEDATA:
		case PKG_TYPE_PS3_GAMEEXEC:
		case PKG_TYPE_PS3_THEME:
		case PKG_TYPE_PS3_WIDGET:
		case PKG_TYPE_PS3_LICENSE:
		case PKG_TYPE_PS3_VSH_MODULE:
		case PKG_TYPE_PS3_PSN_AVATAR:
		case PKG_TYPE_PS3_VMC:
		case PKG_TYPE_PS3_PS2:
		case PKG_TYPE_PS3_PSP_REMASTER:
		case PKG_TYPE_WEB_TV:
		case PKG_TYPE_UNK_8:
		case PKG_TYPE_UNK_13:
		case PKG_TYPE_VITA_LIVEAREA:
			return 0;
		case PKG_TYPE_PSX:			
		case PKG_TYPE_PSP:			
		case PKG_TYPE_PSP_GO:		
		case PKG_TYPE_PSP_MINIS:	
		case PKG_TYPE_PSP_NEO_GEO:	
		case PKG_TYPE_VITA_APP:		
		case PKG_TYPE_VITA_DLC:		
		case PKG_TYPE_VITA_THEME:	
		case PKG_TYPE_PSM:			
		case PKG_TYPE_PSM_UNITY:	
		default:					
			return 1;				
	}
}

const char* find_promote_location(int content_type) {

	switch(content_type) {
		case PKG_TYPE_UNK_PS3:
		case PKG_TYPE_UNK_2:
		case PKG_TYPE_UNK_3:
		case PKG_TYPE_PS3_GAMEDATA:
		case PKG_TYPE_PS3_GAMEEXEC:
		case PKG_TYPE_PS3_THEME:
		case PKG_TYPE_PS3_WIDGET:
		case PKG_TYPE_PS3_LICENSE:
		case PKG_TYPE_PS3_VSH_MODULE:
		case PKG_TYPE_PS3_PSN_AVATAR:
		case PKG_TYPE_PS3_VMC:
		case PKG_TYPE_PS3_PS2:
		case PKG_TYPE_PS3_PSP_REMASTER:
		case PKG_TYPE_WEB_TV:
		case PKG_TYPE_UNK_8:
		case PKG_TYPE_UNK_13:
		case PKG_TYPE_VITA_LIVEAREA: // TODO: handle livearea pkg
		default:
			return GENERAL_PKG_OUT_LOCATION;
			
		case PKG_TYPE_PSX:
		case PKG_TYPE_PSP:
		case PKG_TYPE_PSP_GO:
		case PKG_TYPE_PSP_MINIS:
		case PKG_TYPE_PSP_NEO_GEO:
			return PROMOTE_PSP;
			
		case PKG_TYPE_VITA_APP:
			return PROMOTE_VITA_APP;

		case PKG_TYPE_VITA_DLC:
			return PROMOTE_VITA_DLC;
		
		case PKG_TYPE_VITA_THEME:
			return PROMOTE_VITA_THEME;
			
		case PKG_TYPE_PSM:
		case PKG_TYPE_PSM_UNITY:
			return PROMOTE_PSM;
	}
	
	return GENERAL_PKG_OUT_LOCATION;
	
}

int load_sce_paf() {
	static uint32_t argp[] = { 0x180000, -1, -1, 1, -1, -1 };
	int res = -1;

	SceSysmoduleOpt opt;

	memset(&opt, 0xFF, sizeof(SceSysmoduleOpt));
	opt.flags = sizeof(SceSysmoduleOpt);
	opt.result = &res;

	return sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(argp), argp, &opt);
}

int unload_sce_paf() {
	SceSysmoduleOpt opt;
	memset(&opt, 0, sizeof(opt));
	return sceSysmoduleUnloadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, 0, NULL, &opt);
}

int promote_cma(const char *path, const char *titleid, int type, void (*progress_callback)(const char*, uint64_t, uint64_t)) {
  
  ScePromoterUtilityImportParams promote_args;
  memset(&promote_args,0x00,sizeof(ScePromoterUtilityImportParams));
  strncpy(promote_args.path, path, sizeof(promote_args.path)-1);
  strncpy(promote_args.titleid, titleid, sizeof(promote_args.titleid)-1);
  promote_args.type = type;
  promote_args.attribute = 0x1;
  
  uint64_t done = 0;
  uint64_t total = 7;

  CHECK_ERROR(load_sce_paf());
  progress_callback(path, done++, total);
  
  CHECK_ERROR(sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL));
  progress_callback(path, done++, total);
  
  CHECK_ERROR(scePromoterUtilityInit());
  progress_callback(path, done++, total);
  
  CHECK_ERROR(scePromoterUtilityPromoteImport(&promote_args));
  progress_callback(path, done++, total);
  
  CHECK_ERROR(scePromoterUtilityExit());
  progress_callback(path, done++, total);
  
  CHECK_ERROR(sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL));
  progress_callback(path, done++, total);
  
  CHECK_ERROR(unload_sce_paf());
  progress_callback(path, done++, total);

  return 0;
}

int promote(const char *path, void (*progress_callback)(const char*, uint64_t, uint64_t)) {
	int state = -1;
	int result = -1;

	uint64_t done = 0;
	uint64_t total = 8;
	
	CHECK_ERROR(load_sce_paf());
	progress_callback(path, done++, total);
	
	CHECK_ERROR(sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL));
	progress_callback(path, done++, total);
	
	CHECK_ERROR(scePromoterUtilityInit());
	progress_callback(path, done++, total);
	
	CHECK_ERROR(scePromoterUtilityPromotePkgWithRif(path, 0));
	progress_callback(path, done++, total);
	
	do { CHECK_ERROR(scePromoterUtilityGetState(&state)); } while(state != 0);
	progress_callback(path, done++, total);
	
	CHECK_ERROR(scePromoterUtilityGetResult(&result));
	CHECK_ERROR(result);
	progress_callback(path, done++, total);
	
	CHECK_ERROR(scePromoterUtilityExit());
	progress_callback(path, done++, total);
	
	CHECK_ERROR(sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL));
	progress_callback(path, done++, total);
	
	CHECK_ERROR(unload_sce_paf());
	progress_callback(path, done++, total);

	return 0;
}

int is_app_installed(const char* title_id) {
	int unk = -1;
	CHECK_ERROR(load_sce_paf());
	CHECK_ERROR(sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL));	
	CHECK_ERROR(scePromoterUtilityInit());
	int exist = scePromoterUtilityCheckExist(title_id, &unk);
	CHECK_ERROR(scePromoterUtilityExit());	
	CHECK_ERROR(sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL));	
	CHECK_ERROR(unload_sce_paf());
	return (exist >= 0);
}