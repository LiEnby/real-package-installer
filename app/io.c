#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <vitasdk.h>

#include "io.h"
#include "log.h"
#include "err.h"

int file_exist(const char* path) {
	SceIoStat stat;
	int res = sceIoGetstat(path, &stat);
	if(res >= 0) return 1;
	else return 0;
}

int get_files_in_folder(const char* folder, char* out_filenames, int* total_folders, SearchFilter* filter, size_t max_files) {
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

int delete_tree(const char* path) {
	int ret = 0;
	int dirReadRet = 0;
	
	if(!file_exist(path)) return 0;
	
	SceUID dfd = sceIoDopen(path);
	if(dfd <= 0) ERROR(dfd);
	
	char* ent = malloc(MAX_PATH);
	if(ent == NULL) ERROR(-1);
	memset(ent, 0x00, MAX_PATH);
	
	SceIoDirent* dir = malloc(sizeof(SceIoDirent));
	if(dir == NULL) ERROR(-1);
	
	do{
		memset(dir, 0x00, sizeof(SceIoDirent));
		dirReadRet = sceIoDread(dfd, dir);
		
		if(dirReadRet > 0) {
			snprintf(ent, MAX_PATH, "%s/%s", path, dir->d_name);
			
			if(SCE_S_ISDIR(dir->d_stat.st_mode)) {
				ret = delete_tree(ent);
				PRINT_STR("delete_tree(%s) = 0x%X\n", ent, ret);
				if(ret < 0) ERROR(ret);
			}
			else{
				ret = sceIoRemove(ent);
				PRINT_STR("sceIoRemove(%s) = 0x%X\n", ent, ret);
				if(ret < 0) ERROR(ret);
			}	
		}
	} while(dirReadRet > 0);
	
	
error:
	if(dfd > 0) sceIoDclose(dfd);
	if(ent != NULL) free(ent);
	if(dir != NULL) free(dir);
	
	CHECK_ERROR(sceIoRmdir(path));
	
	return ret;
}

int read_first_filename(const char* path, char* output, size_t out_size) {
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

uint64_t get_free_space(const char* device) {
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

uint64_t get_file_size(const char* filepath) {
	SceIoStat stat;
	int res = sceIoGetstat(filepath, &stat);
	if(res >= 0)
		return stat.st_size;
	return 0;
}

int read_file(const char* path, void* data, size_t size) {
	
	int ret = 0;
	SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
	if(fd > 0) {
		ret = sceIoRead(fd, data, size);
		if(ret < 0) goto error;
		if(ret != size) ERROR(-1);
	}
	
error:
	sceIoClose(fd);
	return ret;
	
}

int write_file(const char* path, const void* data, size_t size) {
	char outdir[MAX_PATH];
	int ret = 0;

	// create directory for file if it doesnt exist already.
	extract_dirname(path, outdir, sizeof(outdir));
	make_directories(outdir);
	
	SceUID wfd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT, 0777);
	if(wfd < 0) ERROR(wfd);
	
	if(wfd > 0) {
		ret = sceIoWrite(wfd, data, size);
		if(ret < 0) goto error;
		if(ret != size) ERROR(-1);
	}
	
error:
	sceIoClose(wfd);
	return ret;
	
}

int copy_file(const char* path, const char* new_path) {
	int ret = 0;
	uint64_t fileSize = get_file_size(path);
	char* data = malloc(fileSize);
	
	if(data != NULL) {
		if(read_file(path, data, fileSize) != fileSize) ERROR(fileSize);
		if(write_file(new_path, data, fileSize) != fileSize) ERROR(fileSize);
	}
error:	
	if(data != NULL) free(data);
	return ret;
}

int extract_dirname(const char* path, char* dirname, int dirname_length) {
	strncpy(dirname, path, dirname_length);
	
	
	int lastSlash = 0;
	for(int i = 0; i < strlen(path); i++) {
		if(path[i] == '/' || path[i] == '\\') lastSlash = i;
	}
	
	dirname[lastSlash] = 0;
	return lastSlash;
}

void make_directories(const char* path) {
	if(file_exist(path)) return;
	
	char dirname[MAX_PATH];
	memset(dirname, 0x00, sizeof(dirname));
	

	for(int i = 0; i < strlen(path); i++) {
		if(path[i] == '/' || path[i] == '\\') {
			memset(dirname, 0, sizeof(dirname));
			strncpy(dirname, path, i);

			if(!file_exist(dirname)){
				PRINT_STR("Creating Directory: %s\n", dirname);
				sceIoMkdir(dirname, 0777);
			}	
		}
	}
	
	sceIoMkdir(path, 0777);
	
}