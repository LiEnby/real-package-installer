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

#define PKG_INSTALL_LOCATION ("ux0:/package")


void handle_install_package(char* package) {

	
	PRINT_STR("install package: %s\n", package);
	EnableDevPackages();
	disable_power_off();
	lock_shell();
	
	int res = do_package_decrypt(package);

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

void handle_select_npdrmfree_package() {
	char output[MAX_PATH];
	int selected = -1;
	while(selected < 0) {
		selected = do_select_file(PKG_INSTALL_LOCATION, output, ".pkg", (uint64_t)-1);
		
		if(selected < 0) {
			do_confirm_message_format("No files found!", "There were no PKG files found in %s", PKG_INSTALL_LOCATION)
			return;
		}
	}
	char full_output_path[MAX_PATH*2];
	snprintf(full_output_path, sizeof(full_output_path), "%s/%s", PKG_INSTALL_LOCATION, output);
	
	handle_install_package(full_output_path);	
	
}

void handle_main_menu_option() {
	
	int selected = -1;
	while(1) {
		selected = do_draw_main_menu();
		switch(selected) {
			case INSTALL_NPDRM_BIND_PACKAGE:
				break;
			case INSTALL_NPDRM_FREE_PACKAGE:
				handle_select_npdrmfree_package();
				break;
			case LAUNCH_FAKE_PKG_INSTALLER:
				EnableDevPackages();
				SetHost0PackageDir(PKG_INSTALL_LOCATION);
				EnableFPkgInstallerQAF();
				
				// run pkg installer
				sceAppMgrLaunchAppByName(0x60000, "NPXS10031", NULL);
				sceKernelDelayThread(1000 * 1000 * 10);
				
				DisableFPkgInstallerQAF();
				UnsetHost0PackageDir();	
				DisableDevPackages();				
				break;
			default:
				break;
		};

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
