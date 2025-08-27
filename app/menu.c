#include "menu.h"
#include "draw.h"
#include "ctrl.h"
#include "log.h"
#include "io.h"
#include "pkg.h"
#include "promote.h"
#include "err.h"
#include "kernel.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vitasdk.h>
#include <stdint.h>

static uint8_t options[0x1000];

#define WINDOW_SIZE (20)

#define DEFOPT(y) int option = 0;\
				  int opt_y = y; \
				  int increment_y = 20; \
				  int total = 0; \
				  memset(options, 0x00, sizeof(options))
#define ADDOPT(cond,x) if(cond) { \
					draw_option(opt_y, x, option == *selected); \
					opt_y += increment_y; \
					total++; \
					options[option] = 1; \
					PRINT_STR("options[%i] = 1\n", option); \
				   } else { \
					options[option] = 0; \
					PRINT_STR("options[%i] = 0\n", option); \
				   } \
				   option++ 

#define RETURNOPT() return option
#define CALC_FIRST_OPTION() for(first_option = 0; (options[first_option] != 1 && first_option < sizeof(options)); first_option++)
#define CALC_LAST_OPTION() for(last_option = sizeof(options); (options[last_option] != 1 && last_option > 0); last_option--)
#define PROCESS_MENU(func, ...) \
					  int window = 0; \
					  int selected = 0; \
					  memset(options, 0x00, sizeof(options));\
					  int total_options = func(&selected, &window, __VA_ARGS__); \
					  int first_option = 0;\
					  int last_option = 0;\
					  CALC_FIRST_OPTION(); \
					  CALC_LAST_OPTION(); \
					  selected = first_option;\
					  \
					while (1) { \
					  total_options = func(&selected, &window, __VA_ARGS__); \
					  CALC_FIRST_OPTION(); \
					  CALC_LAST_OPTION(); \
					  int ctrl = get_key(); \
					  \
					  switch(ctrl) { \
						 case SCE_CTRL_UP: \
							 do{ \
								selected--; \
							 } while(selected > 0 && options[selected] == 0); \
							 break; \
						 case SCE_CTRL_DOWN: \
							 do{ \
								selected++; \
							 } while(selected < sizeof(options) && options[selected] == 0); \
							 break; \
						 case SCE_CTRL_CIRCLE: \
							 return OP_CANCELED; \
						 case SCE_CTRL_CROSS: \
							 break; \
					  } \
					  if(ctrl == SCE_CTRL_CROSS) break; \
					  if(selected > last_option)  { \
						selected = last_option; \
						\
						if(total_options > WINDOW_SIZE) \
							window++; \
					  } \
					  if(selected < first_option) { \
						selected = first_option; \
						\
						if(window != first_option) \
							window--; \
					  } \
					  PRINT_STR("selected: %x\n", selected); \
					  PRINT_STR("window: %x\n", window); \
				  }





#define draw_text_center_format(y, format, ...) \
do{ \
	txt_format(format, __VA_ARGS__); \
	draw_text_center(y, txt); \
}while(0);

#define draw_title_format(title, ...) \
do{ \
	txt_format(title, __VA_ARGS__); \
	draw_title(txt); \
}while(0);

void draw_ime() {
	start_draw();
	draw_background();
	
	draw_title("Starting IME Dialog ...");

	draw_text_center(200, "IME Dialog is opening...");
	
	end_draw();
}

int draw_run_fake_package_installer_method(int* selected, int* window, char* packageDir) {
	start_draw();
	draw_background();

	draw_title("★Package Installer options...");
	
	draw_text_center(110, "This will run the offical \"fake_package_installer\" (NPXS10031) application.");
	draw_text_center(130, "it's an application built into every PlayStation Vita, but the icon is hidden.");
	draw_text_center(150, "it can ONLY install NpDrm Free and Dev Packages, from CMA or host0.");
	draw_text_center(170, "if your packages require a license this will not work to install them.");

	draw_text_center_format(210, "NOTE: host0:/package has been redirected to %.60s", packageDir);
	
	
	DEFOPT(250);

	ADDOPT(1, "★Run directly");
	ADDOPT(1, "★Run specifying a package.");
	
	end_draw();
	
	RETURNOPT();
}



int draw_package_rif(int* selected, int* window, int content_type, char* rif_folder) {
	start_draw();
	draw_background();
	
	draw_title("Rights information file Required!");
	

	draw_text_center(110, "The package file selected, requires a License aka \"Rights information file\" (RIF).");
	draw_text_center(130, "you can manually select one to use, but if it is incorrect, the package will fail to install.");
	draw_text_center(150, "if using a NoNpDrm or NoPsmDrm, please ensure the respective plugin(s) are installed.");

	draw_text_center_format(220, "Current directory for licenses %.60s", rif_folder);
		
	DEFOPT(270);
	
	ADDOPT(1, "★Select a Rights information file");
	ADDOPT(1, "★Change RIF directory");
	ADDOPT(1, "★Scan Directory for licenses matching this package.");
	ADDOPT( (IS_PSP_CONTENT_TYPE(content_type) && (module_is_running("NoPspEmuDrm_kern")) ), "★Generate NoPspEmuDrm License");
	
	end_draw();
	RETURNOPT();
}

int draw_main_menu(int* selected, int* window, char* packageDir) {
	start_draw();
	draw_background();

	draw_title("★RealPackage Installer");

	draw_text_center_format(170, "Package directory is: %.60s", packageDir);

	
	DEFOPT(250);

	ADDOPT(1, "★Install Package Files");
	ADDOPT(1, "★Expand Package Files");
	ADDOPT(1, "★Change Package directory");
	ADDOPT(1, "★Change License directory");
	ADDOPT(1, "★Run Sony Package Installer");
	
	end_draw();
	
	RETURNOPT();
}

void draw_confirmation_message(char* title, char* msg) {
	start_draw();
	draw_background();
	
	draw_title(title);

	draw_text_center(200, msg);
	draw_text_center(250, "Press any button to continue ...");
	
	end_draw();
}

int draw_select_file(int* selected, int* window, const char* input_folder, char* folders, int total_files) {
	start_draw();
	draw_background();
	
	draw_title_format("Select a file from: %s ...", input_folder);
	
	DEFOPT(110);

	// check if window - total_files is less than the window size.	
	// reset window to window_size if it is
	if( *window > (total_files % WINDOW_SIZE) ) {
		*window = (total_files % WINDOW_SIZE);
	}
	
	for(int i = *window; i <= *window + WINDOW_SIZE; i++) {
		if(i >= total_files) break;
		
		char file[MAX_PATH];
		snprintf(file, sizeof(file), "%.65s", (folders + (i * MAX_PATH)));
		ADDOPT(1, file);
	}
	
	end_draw();
	
	RETURNOPT();		
}

void draw_package_expand(const char* package, uint64_t done, uint64_t total) {
	start_draw();
	draw_background();
	draw_title_format("Installing package %.50s ...", package);
	
	draw_text_center_format(200, "Expand package %.50s", package);
	draw_progress_bar(230, done, total);

	end_draw();
}

void draw_package_promote(const char* package, uint64_t done, uint64_t total) {
	start_draw();
	draw_background();
	draw_title_format("Running ScePromote ... %s", package);
	
	draw_text_center(200, "Promoting Package (creating a bubble) ...");
	draw_progress_bar(230, done, total);

	end_draw();
}

int do_select_file(const char* folder, char* output, const char* extension, uint64_t max_size) {
	int total_files = 0;	
	static char files[MAX_PATH * sizeof(options)];
	
	SearchFilter filter;
	memset(&filter, 0x00, sizeof(SearchFilter));
	filter.max_filesize = max_size;
	filter.file_only = 1;
	strncpy(filter.match_extension, extension, sizeof(filter.match_extension));
	
	int res = get_files_in_folder(folder, files, &total_files, &filter, sizeof(options));
	
	PRINT_STR("get_files_in_folder = %x\n", res);
	if(res < 0) return res;
	if(total_files <= 0) return -2;
	
	PRINT_STR("total_files: %x\n", total_files);
	
	PROCESS_MENU(draw_select_file, folder, files, total_files);
	strncpy(output, files + (selected * MAX_PATH), MAX_PATH);	
	return selected;	
}

int do_package_install(const char* expand_location, const char* package) {
	PRINT_STR("%s\n", __FUNCTION__);
	char dirname[MAX_PATH];
	
	int contentType = package_content_type(package);
	const char* promote_location = find_promote_location(contentType);
	PRINT_STR("package_content_type = %x!\npromote_location = %s\n", contentType, promote_location);	
	
	// ensure directories exist.
	extract_dirname(promote_location, dirname, sizeof(dirname));
	make_directories(dirname);
	
	int res = delete_tree(promote_location);
	if(res >= 0) {
		if(need_promote(contentType) && res >= 0) {
			// workaround psp needing stat.bin
			if(IS_PSP_CONTENT_TYPE(contentType)) { 
				res = promote_custom_psp(expand_location, draw_package_promote);
			}
			else {
				res = sceIoRename(expand_location, promote_location);
				res = promote(promote_location, draw_package_promote);
			}
		}
	}
	
	if(res != 0)
	{
		//delete_tree(promote_location);
	}

	return res;
}

int do_package_extract(const char* package, const char* expand_location) {
	PRINT_STR("%s\n", __FUNCTION__);
	CHECK_ERROR(expand_package(package, expand_location, draw_package_expand));
	return 0;
}

int do_package_rif(const char* package, char* rif_folder) {
	int contentType = package_content_type(package);	

	PROCESS_MENU(draw_package_rif, contentType, rif_folder);
	return selected;
}


void do_confirm_message(char* title, char* msg) {
	draw_confirmation_message(title, msg);
	get_key();
}

int do_run_fake_package_installer_method(char* packageDir) {
	PROCESS_MENU(draw_run_fake_package_installer_method, packageDir);
	return selected;
}

int do_main_menu(char* packageDir) {
	PROCESS_MENU(draw_main_menu, packageDir);
	return selected;
}

void do_ime() {
	for(int i = 0; i < 0x5; i++)
		draw_ime();
}