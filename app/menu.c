#include "menu.h"
#include "draw.h"
#include "ctrl.h"
#include "log.h"
#include "io.h"
#include "pkg.h"

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

int draw_main_menu(int* selected, int* window, ...) {
	start_draw();
	draw_background();

	draw_title("Real Package Installer");
	
	DEFOPT(200);

	ADDOPT(1, "Install NpDrm-Bind Package");
	ADDOPT(1, "Install NpDrm-Free Package");
	ADDOPT(1, "Launch Sony ★Package Installer (NPXS10031)");
	
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

int draw_select_file(int* selected, int* window, char* input_folder, char* folders, int total_files) {
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
		snprintf(file, sizeof(file), "%.50s", folders + (i * MAX_PATH));
		ADDOPT(1, file);
	}
	
	end_draw();
	
	RETURNOPT();		
}

void draw_package_decrypt(char* package, uint64_t done, uint64_t total) {
	start_draw();
	draw_background();
	draw_title_format("Installing package %s ...", package);
	
	draw_text_center_format(200, "Expand package %s", package);
	draw_progress_bar(230, done, total);

	end_draw();
}

int do_select_file(char* folder, char* output, char* extension, uint64_t max_size) {
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

int do_package_decrypt(char* package) {
	PRINT_STR("do_package_decrypt\n");
	sceIoMkdir("ux0:/temp", 0777);
	sceIoMkdir("ux0:/temp/game", 0777);

	int res = expand_package(package, "ux0:/temp/game", draw_package_decrypt);
	return res;
}

void do_confirm_message(char* title, char* msg) {
	draw_confirmation_message(title, msg);
	get_key();
}

int do_draw_main_menu(char* output) {
	PROCESS_MENU(draw_main_menu, 0);
	return selected;
}