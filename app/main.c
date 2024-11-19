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

void handle_main_menu_option() {
	
	int selected = -1;
	while(1) {
		selected = do_draw_main_menu();
		switch(selected) {
			case INSTALL_NPDRM_BIND_PACKAGE:
				break;
			case INSTALL_NPDRM_FREE_PACKAGE:
				break;
			case LAUNCH_FAKE_PKG_INSTALLER:
				EnableDevPackages();
				SetHost0PackageDir("ux0:/package");
				EnableFPkgInstallerQAF();
				
				// run pkg installer
				sceAppMgrLaunchAppByName(0x60000, "NPXS10031", NULL);
				sceKernelDelayThread(1000 * 10);
				
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
