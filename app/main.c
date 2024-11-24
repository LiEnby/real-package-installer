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

static char PKG_INSTALL_LOCATION[MAX_PATH] = ("ux0:/package");

void handle_install_package(char* package, char* relPackage) {

	
	PRINT_STR("install package: %s\n", package);
	EnableDevPackages();
	disable_power_off();
	lock_shell();
	
	int res = do_package_install(package);

	DisableDevPackages();
	enable_power_off();
	unlock_shell();
	
	if(res < 0) {
		do_confirm_message_format("Install failed!", "The package failed to install, (error = 0x%X)\n", res);
	}
	else {
		do_confirm_message("Install complete!", "The package was installed.");
	}
}

void handle_select_npdrm_package(void (*next_step)(char*, char*)) {
	char output[MAX_PATH];
	int selected = -1;
	while(selected < 0) {
		selected = do_select_file(PKG_INSTALL_LOCATION, output, ".pkg", (uint64_t)-1);
		
		if(selected == OP_CANCELED) return;
		if(selected < 0) {
			do_confirm_message_format("No files found!", "There were no PKG files found in %s", PKG_INSTALL_LOCATION);
			return;
		}
	}
	char full_output_path[MAX_PATH*2];
	snprintf(full_output_path, sizeof(full_output_path), "%s/%s", PKG_INSTALL_LOCATION, output);
	
	next_step(full_output_path, output);
	
}

/*
* fake_package_installer
*/

void launch_pkg_installer(const char* arguments) {
	EnableDevPackages();
	SetHost0PackageDir(PKG_INSTALL_LOCATION);
	EnableFPkgInstallerQAF();

	// run pkg installer
	sceAppMgrLaunchAppByName(0x60000, "NPXS10031", arguments);
	sceKernelDelayThread(1000 * 1000 * 10);

	DisableFPkgInstallerQAF();
	UnsetHost0PackageDir();	
	DisableDevPackages();		
}

void handle_run_pkginstaller_with_arg(char* package, char* relPackage) {
	char args[0x1028];
	snprintf(args, sizeof(args), "[BATCH]\nhost0:/package/%s", relPackage);
	launch_pkg_installer(args);
}


void handle_run_pkg_installer() {
	int selected = -1;

	while(1) {
		selected = do_run_fake_package_installer_method(PKG_INSTALL_LOCATION);
		switch(selected) {
			case RUN_WITH_PACKAGE_SPECIFIED:
				handle_select_npdrm_package(handle_run_pkginstaller_with_arg);
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
	selected = do_main_menu(PKG_INSTALL_LOCATION);
	switch(selected) {
		case INSTALL_NPDRM_PACKAGE:
			handle_select_npdrm_package(handle_install_package);
			break;
		case CHANGE_PKG_DIRECTORY:
			open_ime("Set package install location", PKG_INSTALL_LOCATION, sizeof(PKG_INSTALL_LOCATION)-1);
			
			if(!file_exist(PKG_INSTALL_LOCATION)) {
				do_confirm_message_format("Directory not found!", "No folder exists at %s", PKG_INSTALL_LOCATION);
				strncpy(PKG_INSTALL_LOCATION, "ux0:/package", sizeof(PKG_INSTALL_LOCATION)-1);
			}
			
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
	init_vita2d();
	init_sound();
	
	while(1) {
		handle_main_menu_option();
	}
	
	term_sound();
	term_vita2d();
	return 0;
}
