#ifndef IO_H
#define IO_H 1

#define MAX_PATH (512)
#include <stddef.h>
#include <stdint.h>

typedef struct SearchFilter {
	uint64_t max_filesize;
	uint8_t file_only;
	char match_extension[MAX_PATH];
} SearchFilter;

uint64_t get_file_size(const char* filepath);
int file_exist(const char* path);
int write_file(const char* path, const void* data, size_t size);
int read_file(const char* path, void* data, size_t size);
int copy_file(const char* path, const char* new_path);
int delete_tree(const char* path);


uint64_t get_free_space(const char* device);
int read_first_filename(char* path, char* output, size_t out_size);
int get_files_in_folder(char* folder, char* out_filenames, int* total_folders, SearchFilter* filter, size_t max_files);

#endif