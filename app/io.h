#ifndef IO_H
#define IO_H 1

#define MAX_PATH (512)

typedef struct SearchFilter {
	uint64_t max_filesize;
	uint8_t file_only;
	char match_extension[MAX_PATH];
} SearchFilter;

int file_exist(char* path);
void remove_illegal_chars(char* str);


uint64_t get_file_size(const char* filepath);
uint64_t get_free_space(const char* device);
int read_first_filename(char* path, char* output, size_t out_size);

int get_files_in_folder(char* folder, char* out_filenames, int* total_folders, SearchFilter* filter, size_t max_files);

#endif