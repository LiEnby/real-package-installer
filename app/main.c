#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <taihen.h>
#include <vitasdk.h>
#include <vita2d.h>

#include "kernel.h"
#include "draw.h"
#include "menu.h"
#include "bgm.h"
#include "log.h"
#include "fpkg.h"
#include "io.h"
#include "ime.h"
#include "rif.h"
#include "pkg.h"
#include "main.h"

static app_settings G_APP_SETTINGS = { DEFAULT_RIF_LOCATION, DEFAULT_PKG_LOCATION };

void save_settings() {
	write_file(SETTINGS_SAVE_LOCATION, &G_APP_SETTINGS, sizeof(G_APP_SETTINGS));
}

void load_settigs() {
	if(file_exist(SETTINGS_SAVE_LOCATION)) {
		read_file(SETTINGS_SAVE_LOCATION, &G_APP_SETTINGS, sizeof(G_APP_SETTINGS));
	}
}

void install_rif(char* rif, char* relRif) {
	char workBin[MAX_PATH];
	snprintf(workBin, sizeof(workBin), "%s/%s", PKG_EXPAND_LOCATION, "sce_sys/package/work.bin");
	
	copy_file(rif, workBin);
}


/*
* fake_package_installer
*/

void launch_pkg_installer(const char* arguments) {
	EnableDevPackages();
	SetHost0PackageDir(G_APP_SETTINGS.PKG_INSTALL_LOCATION);
	EnableFPkgInstallerQAF();

	// run pkg installer
	sceAppMgrLaunchAppByName(0x60000, "NPXS10031", arguments);
	sceKernelDelayThread(1000 * 1000 * 10);

	DisableFPkgInstallerQAF();
	UnsetHost0PackageDir();	
	DisableDevPackages();		
}


int handle_select_file(const char* folder, const char* extension, void (*next_step)(char*, char*)) {
	char output[MAX_PATH];
	int selected = -1;
	while(selected < 0) {
		selected = do_select_file(folder, output, extension, (uint64_t)-1);
		
		if(selected == OP_CANCELED) return OP_CANCELED;
		if(selected < 0) {
			do_confirm_message_format("No files found!", "There were no %s files found in %s", extension, folder);
			return -1;
		}
	}
	char full_output_path[MAX_PATH*2];
	snprintf(full_output_path, sizeof(full_output_path), "%s/%s", folder, output);
	
	next_step(full_output_path, output);
	return 0;
}

void handle_change_rif() {
	open_ime("Set Rights Information File directory", G_APP_SETTINGS.PKG_RIF_LOCATION, sizeof(G_APP_SETTINGS.PKG_RIF_LOCATION)-1);

	if(!file_exist(G_APP_SETTINGS.PKG_RIF_LOCATION)) {
		do_confirm_message_format("Directory not found!", "No folder exists at %s", G_APP_SETTINGS.PKG_RIF_LOCATION);
		strncpy(G_APP_SETTINGS.PKG_RIF_LOCATION, DEFAULT_RIF_LOCATION, sizeof(G_APP_SETTINGS.PKG_RIF_LOCATION)-1);
	}
	else {
		save_settings();
	}
}

void handle_change_pkg() {
	open_ime("Set package install location", G_APP_SETTINGS.PKG_INSTALL_LOCATION, sizeof(G_APP_SETTINGS.PKG_INSTALL_LOCATION)-1);
	
	if(!file_exist(G_APP_SETTINGS.PKG_INSTALL_LOCATION)) {
		do_confirm_message_format("Directory not found!", "No folder exists at %s", G_APP_SETTINGS.PKG_INSTALL_LOCATION);
		strncpy(G_APP_SETTINGS.PKG_INSTALL_LOCATION, "ux0:/package", sizeof(G_APP_SETTINGS.PKG_INSTALL_LOCATION)-1);
	}
	else {
		save_settings();
	}
}

int handle_scan_and_install_rifs(char* contentId) {
	char sourceRifPath[MAX_PATH];

	if(find_rif(contentId, G_APP_SETTINGS.PKG_RIF_LOCATION, sourceRifPath) >= 0) {
		install_rif(sourceRifPath, NULL);
		return 1;
	}
	else {
		return 0;
	}
	return 0;
}

int handle_select_rif(char* package) {
	char workBin[MAX_PATH];
	char contentId[MAX_PATH];

	snprintf(workBin, sizeof(workBin), "%s/%s", PKG_EXPAND_LOCATION, "sce_sys/package/work.bin");
	
	while(1) {
		int selected = -1;
		selected = do_package_rif(package, G_APP_SETTINGS.PKG_RIF_LOCATION);
		switch(selected) {
			case SELECT_RIF_FILE:
				if(handle_select_file(G_APP_SETTINGS.PKG_RIF_LOCATION, ".rif", install_rif) >= 0) return 0;
				break;
			case SELECT_RIF_DIRECTORY:
				handle_change_rif();
				break;
			case SCAN_DIRECTORY:				
				package_content_id(package, contentId, sizeof(contentId));
				if(!handle_scan_and_install_rifs(contentId)) {
					do_confirm_message_format("Rif not found", "Could not find a rif for game %s in %s", contentId, G_APP_SETTINGS.PKG_RIF_LOCATION);
					break;
				}
				return 0;
			case GENERATE_NOPSPEMU_RIF:
				package_content_id(package, contentId, sizeof(contentId));
				return make_psp_fake_rif(workBin, contentId);
			case OP_CANCELED:
				return OP_CANCELED;
			default:
				break;
		};
	}
	
}

void handle_expand_package(char* package, char* relPackage) {
	char outputDirectory[MAX_PATH] = "ux0:/expand";
	open_ime("Input package extract directory", outputDirectory, sizeof(outputDirectory)-1);
	
	
	EnableDevPackages();
	disable_power_off();
	lock_shell();
	
	int res = do_package_extract(package, outputDirectory);
	if(res < 0) {
		do_confirm_message_format("Expand failed!", "The package failed to expand, (error = 0x%X)\n", res);
		goto error;
	}
	
	error:
	DisableDevPackages();
	enable_power_off();
	unlock_shell();
	
	if(res < 0) delete_tree(PKG_EXPAND_LOCATION);
}

void handle_install_package(char* package, char* relPackage) {
	char contentId[MAX_PATH];
	PRINT_STR("install package: %s\n", package);
	
	EnableDevPackages();
	disable_power_off();
	lock_shell();
	
	int res = do_package_extract(package, PKG_EXPAND_LOCATION);
	if(res < 0) {
		do_confirm_message_format("Install failed!", "The package failed to expand, (error = 0x%X)\n", res);
		goto error;
	}
	else if(is_rif_required(package, PKG_EXPAND_LOCATION)) {
		package_content_id(package, contentId, sizeof(contentId));
		if(!handle_scan_and_install_rifs(contentId)) {
			if(handle_select_rif(package) == OP_CANCELED) goto error;
		}
	}
	
	res = do_package_install(PKG_EXPAND_LOCATION, package);
	
	if(res < 0) {
		do_confirm_message_format("Install failed!", "The package failed to promote, (error = 0x%X)", res);
		goto error;
	}
	else {
		do_confirm_message("Install complete!", "The package was installed.");
	}

	
error:
	DisableDevPackages();
	enable_power_off();
	unlock_shell();
	
	delete_tree(PKG_EXPAND_LOCATION);
	return;
}


void handle_run_pkginstaller_with_arg(char* package, char* relPackage) {
	char args[0x1028];
	snprintf(args, sizeof(args), "[BATCH]host0:/package/%s", relPackage);
	launch_pkg_installer(args);
}


void handle_run_pkg_installer() {
	int selected = -1;

	while(1) {
		selected = do_run_fake_package_installer_method(G_APP_SETTINGS.PKG_INSTALL_LOCATION);
		switch(selected) {
			case RUN_WITH_PACKAGE_SPECIFIED:
				handle_select_file(G_APP_SETTINGS.PKG_INSTALL_LOCATION, ".pkg", handle_run_pkginstaller_with_arg);
				break;
			case RUN_FAKE_PACKAGE_INSTALLER:
				launch_pkg_installer(NULL);
				break;
			case OP_CANCELED:
				return;
			default:
				break;
		};

	};
}


void handle_main_menu_option() {
	
	int selected = -1;
	selected = do_main_menu(G_APP_SETTINGS.PKG_INSTALL_LOCATION);
	switch(selected) {
		case INSTALL_NPDRM_PACKAGE:
			handle_select_file(G_APP_SETTINGS.PKG_INSTALL_LOCATION, ".pkg", handle_install_package);
			break;
		case EXPAND_NPDRM_PACKAGE:
			handle_select_file(G_APP_SETTINGS.PKG_INSTALL_LOCATION, ".pkg", handle_expand_package);
			break;
		case CHANGE_RIF_DIRECTORY:
			handle_change_rif();
			break;
		case CHANGE_PKG_DIRECTORY:
			handle_change_pkg();
			break;
		case LAUNCH_FAKE_PKG_INSTALLER:
			handle_run_pkg_installer();
			break;
		default:
			break;
	};

}


int main() {
	load_kernel_modules();
	load_settigs();
	
	init_vita2d();
	init_sound();
	
	while(1) {
		handle_main_menu_option();
	}
	
	term_sound();
	term_vita2d();
	return 0;
}
