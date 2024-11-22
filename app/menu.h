#include <stdint.h>
#ifndef MENU_H
#define MENU_H 1

#define txt_format(format, ...) \
	char txt[256]; \
	snprintf(txt, sizeof(txt), format, __VA_ARGS__)
	
	
#define do_confirm_message_format(title, msg, ...) \
{ \
	txt_format(msg, __VA_ARGS__); \
	do_confirm_message(title, txt); \
};


int do_select_file(char* folder, char* output, char* extension, uint64_t max_size);
void do_ime();
void do_confirm_message(char* title, char* msg);
int do_package_decrypt(char* package);
int do_draw_main_menu();

enum main_menu_options {
	INSTALL_NPDRM_BIND_PACKAGE,
	INSTALL_NPDRM_FREE_PACKAGE,
	LAUNCH_FAKE_PKG_INSTALLER
};

#endif