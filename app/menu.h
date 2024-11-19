#include <stdint.h>
int do_select_file(char* folder, char* output, char* extension, uint64_t max_size);
void do_ime();
void do_confirm_message(char* title, char* msg);
int do_draw_main_menu();

enum main_menu_options {
	INSTALL_NPDRM_BIND_PACKAGE,
	INSTALL_NPDRM_FREE_PACKAGE,
	LAUNCH_FAKE_PKG_INSTALLER
};
