#include <stdint.h>
#include <stdlib.h>
#include <vitasdk.h>
#include <string.h>

#include "io.h"
#include "log.h"
#include "err.h"

int file_exist(char* path) {
	SceIoStat stat;
	int res = sceIoGetstat(path, &stat);
	if(res >= 0) return 1;
	else return 0;
}

int get_files_in_folder(char* folder, char* out_filenames, int* total_folders, SearchFilter* filter, size_t max_files) {
	int ret = 0;
	
	// get total folder count
	*total_folders = 0;
	memset(out_filenames, 0x00, MAX_PATH * max_files);
	
	// read file list 
	int dfd = sceIoDopen(folder);
	PRINT_STR("sceIoDopen dfd: %x\n", dfd);
	if(dfd < 0) ERROR(dfd);

	SceIoDirent ent;
	
	for(int i = 0; i < max_files; i++) {
		int res = sceIoDread(dfd, &ent);
		PRINT_STR("sceIoDread res: %x\n", res);
		if(res < 0) ERROR(res);
		if(res == 0) break;
		if(ent.d_name == NULL) break;
		
		if(filter != NULL) {
			// ensure file is above a certain size
			if(ent.d_stat.st_size > filter->max_filesize) {
				PRINT_STR("%s is too big\n", ent.d_name);
				continue;
			}
			
			// match only files
			if(filter->file_only && SCE_S_ISDIR(ent.d_stat.st_mode)) {
				PRINT_STR("%s is directory\n", ent.d_name);
				continue;				
			}
			
			// match only specific file extension logic
			if(filter->match_extension[0] != '*') { 
				size_t dir_name_length = strlen(ent.d_name);
				size_t extension_length = strlen(filter->match_extension);

				PRINT_STR("dir_name_length = %x\n", dir_name_length);
				PRINT_STR("extension_length = %x\n", extension_length);
				
				if( strcasecmp (ent.d_name + (dir_name_length - extension_length), filter->match_extension) != 0 ) {
					PRINT_STR("%s is not extension: %s\n", ent.d_name, filter->match_extension);
					continue;
				}
			}
			
		}
		PRINT_STR("%s passed all filter checks\n", ent.d_name);
		
		strncpy(out_filenames + (*total_folders * MAX_PATH), ent.d_name, MAX_PATH-1); 					
		PRINT_STR("ent.d_name: %s\n", ent.d_name);

		*total_folders += 1;
		PRINT_STR("total_folders: %x\n", *total_folders);
	}
	
	ret = sceIoDclose(dfd);
	PRINT_STR("sceIoDclose: %x\n", ret);
	if(ret > 0) ERROR(ret);
	
	return 0;
	error:
	PRINT_STR("Error case reached: %x\n", ret);
	if(dfd >= 0)
		sceIoDclose(dfd);
	return ret;	
}

int read_first_filename(char* path, char* output, size_t out_size) {
	int ret = 0;
	int dfd = sceIoDopen(path);
	if(dfd < 0) ERROR(dfd);
	
	SceIoDirent ent;
	int res = sceIoDread(dfd, &ent);
	if(res < 0) ERROR(res);
	
	strncpy(output, ent.d_name, out_size);
	
error:
	if(dfd >= 0)
		sceIoDclose(dfd);
	return ret;	
}

void remove_illegal_chars(char* str) {
	// remove illegal characters from file name
	int slen = strlen(str);
	for(int i = 0; i < slen; i++) {
		if(str[i] == '/' ||
		str[i] == '\\' ||
		str[i] == ':' ||
		str[i] == '?' ||
		str[i] == '*' ||
		str[i] == '"' ||
		str[i] == '|' ||
		str[i] == '>' ||
		str[i] == '\n' ||
		str[i] == '\r' ||
		str[i] == '<')
			str[i] = ' ';		
	}
}


uint64_t get_free_space(const char* device) {
	uint64_t max_size = 0;
	uint64_t free_space = 0;
	 
	// host0 will always report as 0 bytes free
	if(strcmp("host0:", device) == 0)
		return 0xFFFFFFFFFFFFFFFF;
	 
	SceIoDevInfo info;
	int res = sceIoDevctl(device, 0x3001, NULL, 0, &info, sizeof(SceIoDevInfo));
	if (res < 0) {
		free_space = 0;
	} else {
		free_space = info.free_size;
	}
	
	return free_space;
}
