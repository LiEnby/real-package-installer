#include <stdint.h>
#ifndef MENU_H
#define MENU_H 1

#define txt_format(format, ...) \
	char txt[0x1028]; \
	snprintf(txt, sizeof(txt), format, __VA_ARGS__)
	
	
#define do_confirm_message_format(title, msg, ...) \
{ \
	txt_format(msg, __VA_ARGS__); \
	do_confirm_message(title, txt); \
};

#define OP_CANCELED (-9530)

int do_select_file(const char* folder, char* output, const char* extension, uint64_t max_size);
void do_ime();
void do_confirm_message(char* title, char* msg);
int do_main_menu(char* packageDir);
int do_run_fake_package_installer_method(char* packageDir);

int do_package_extract(const char* package, const char* expand_location);
int do_package_install(const char* expand_location, const char* package);
int do_package_rif(const char* package, char* rif_folder);

enum select_rif_options {
	SELECT_RIF_FILE,
	SELECT_RIF_DIRECTORY,
	SCAN_DIRECTORY,
	GENERATE_NOPSPEMU_RIF,
};

enum run_pkg_installer_options {
	RUN_FAKE_PACKAGE_INSTALLER,
	RUN_WITH_PACKAGE_SPECIFIED
};

enum main_menu_options {
	INSTALL_NPDRM_PACKAGE,
	EXPAND_NPDRM_PACKAGE,
	CHANGE_PKG_DIRECTORY,
	CHANGE_RIF_DIRECTORY,
	LAUNCH_FAKE_PKG_INSTALLER
};

#endif