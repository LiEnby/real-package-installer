#ifndef MAIN_H
#define MAIN_H

typedef struct app_settings { 
	char PKG_RIF_LOCATION[MAX_PATH];
	char PKG_INSTALL_LOCATION[MAX_PATH];
} app_settings;

#define PKG_EXPAND_LOCATION ("ux0:/temp/fakepkg/expand")
#define DEFAULT_RIF_LOCATION ("ux0:/rif")
#define DEFAULT_PKG_LOCATION ("ux0:/package")

#define SETTINGS_SAVE_LOCATION ("savedata0:/settings.bin")

#endif