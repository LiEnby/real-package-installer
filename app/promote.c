#include <vitasdk.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "err.h"
#include "pkg.h"
#include "sfo.h"
#include "psp.h"
#include "promote.h"
#include "io.h"

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


int promote_custom_psp(const char* pkg_path, void (*progress_callback)(const char*, uint64_t, uint64_t)) {	
	
	
	// move folders over to here
	char eboot_pbp_path[MAX_PATH];
	char dst_game_folder[MAX_PATH];
	char src_license_file[MAX_PATH];
	char dst_license_file[MAX_PATH];

	char workbuf[MAX_PATH];
	
	char disc_id[0x10];
	char content_id[0x30];
	
	progress_callback(pkg_path, 0, 1);

	snprintf(eboot_pbp_path, sizeof(eboot_pbp_path), "%s/USRDIR/CONTENT/EBOOT.PBP", pkg_path);
	PRINT_STR("eboot.pbp path: %s\n", eboot_pbp_path);
	
	get_pbp_content_id(eboot_pbp_path, content_id);
	
	snprintf(src_license_file, sizeof(src_license_file), "%s/sce_sys/package/work.bin", pkg_path);
	PRINT_STR("src_license_file: %s\n", src_license_file);
	
	
	snprintf(dst_license_file, sizeof(dst_license_file), "%s/PSP/LICENSE/%s.rif", PROMOTE_PSP, content_id);
	PRINT_STR("dst_license_file: %s\n", dst_license_file);
	
	extract_dirname(dst_license_file, workbuf, sizeof(workbuf));
	make_directories(workbuf);
	
	extract_dirname(eboot_pbp_path, workbuf, sizeof(workbuf));
	make_directories(workbuf);

	CHECK_ERROR(gen_sce_ebootpbp(eboot_pbp_path, workbuf));		
	
	char* sfo_buffer = NULL;
	int sfo_size = get_pbp_sfo(eboot_pbp_path, &sfo_buffer);

	int ret = 0;
	
	if(sfo_size > 0) {
		ret = read_sfo_key_buffer("DISC_ID", disc_id, sfo_buffer, sfo_size);
		if(sfo_buffer != NULL) free(sfo_buffer);
		
		PRINT_STR("disc_id: %s\n", disc_id);
		if(ret == 0) {
			snprintf(dst_game_folder, sizeof(dst_game_folder), "%s/PSP/GAME/%s", PROMOTE_PSP, disc_id);
			PRINT_STR("dst_game_folder: %s\n", dst_game_folder);			

			extract_dirname(dst_game_folder, workbuf, sizeof(workbuf));
			make_directories(workbuf);

			extract_dirname(dst_license_file, workbuf, sizeof(workbuf));
			make_directories(workbuf);
			
			extract_dirname(eboot_pbp_path, workbuf, sizeof(workbuf));
			ret = sceIoRename(workbuf, dst_game_folder);
			PRINT_STR("sceIoRename(%s, %s) res = 0x%x\n", workbuf, dst_game_folder, ret);
			if(ret < 0) return ret;
			

			ret = sceIoRename(src_license_file, dst_license_file);
			PRINT_STR("sceIoRename(%s, %s) ret = 0x%x\n", src_license_file, dst_license_file, ret);
			if(ret < 0) return ret;
			

			ret = promote_cma(dst_game_folder, disc_id, SCE_PKG_TYPE_PSP, progress_callback);
			return ret;
		}
	}
	else {
		if(sfo_buffer != NULL) free(sfo_buffer);
		return -99;
	}

	return ret;
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