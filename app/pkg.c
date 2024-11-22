#include "log.h"
#include "pkg.h"
#include "io.h"
#include <vitasdk.h>

#define ERROR_CHECKED(x) \
	do \
	{ \
		int res = x;\
		if(res < 0) { \
			PRINT_STR("%s = 0x%X\n", #x, res);\
			return res; \
		} \
	} while(0);

typedef struct pkg_state {
	SceUID fd;
	PKG_FILE_HEADER pkgHeader;
	PKG_ITEM_RECORD pkgItem;
} pkg_state;

int pkg_decrypt(int offset, void* buffer, size_t bufferSize) {
	_sceNpDrmPackageDecrypt_opt decryptOption;
	decryptOption.offset = offset; 
	decryptOption.identifier = 0x100;
	
	ERROR_CHECKED(_sceNpDrmPackageDecrypt(buffer, bufferSize, &decryptOption));
	return 0;
}

int pkg_read(pkg_state* state, uint32_t offset, void* buffer, size_t bufferSize) {
	PRINT_STR("read offset: 0x%X\n", offset);
	ERROR_CHECKED(sceIoLseek(state->fd, state->pkgHeader.data_offset + offset, SCE_SEEK_SET));
	ERROR_CHECKED(sceIoRead(state->fd, buffer, bufferSize));
	ERROR_CHECKED(pkg_decrypt(offset, buffer, bufferSize));
	return 0;
}

int pkg_read_item(pkg_state* state, int item_index) {
	pkg_read(state, sizeof(PKG_ITEM_RECORD) * item_index, &state->pkgItem, sizeof(PKG_ITEM_RECORD));
	
	state->pkgItem.flags            = __builtin_bswap32(state->pkgItem.flags);
	state->pkgItem.filename_offset  = __builtin_bswap32(state->pkgItem.filename_offset);
	state->pkgItem.filename_size    = __builtin_bswap32(state->pkgItem.filename_size);
	state->pkgItem.data_offset      = __builtin_bswap64(state->pkgItem.data_offset);
	state->pkgItem.data_size        = __builtin_bswap64(state->pkgItem.data_size);		
	return 0;
}

int extract_file(pkg_state* state, char* outfile) {
	
	ERROR_CHECKED(sceIoLseek(state->fd, state->pkgHeader.data_offset + state->pkgItem.data_offset, SCE_SEEK_SET));	
	SceUID wfd = sceIoOpen(outfile, SCE_O_WRONLY | SCE_O_CREAT, 0777);
	ERROR_CHECKED(wfd);
	
	static char buffer[0x8000];
	
	int totalRead = 0;
	do { 
		// get amount of data to read
		int readSize = (sizeof(buffer) < (state->pkgItem.data_size - totalRead)) ? sizeof(buffer) : (state->pkgItem.data_size - totalRead);
		// read the data
		int amtRead = sceIoRead(state->fd, buffer, readSize);
		ERROR_CHECKED(amtRead);
		if(amtRead != readSize) { ERROR_CHECKED(-1); };
		
		// decrypt the data
		ERROR_CHECKED(pkg_decrypt(state->pkgItem.data_offset + totalRead, buffer, readSize));
		
		// write decrypted data
		int amtWritten = sceIoWrite(wfd, buffer, amtRead);
		ERROR_CHECKED(amtWritten);
		if(amtWritten != amtRead) { ERROR_CHECKED(-2); };
		
		totalRead += amtWritten;

	} while(totalRead < state->pkgItem.data_size);
	
	sceIoClose(wfd);
	
	return 0;
}

int close_pkg(pkg_state* state) {
	ERROR_CHECKED(sceIoClose(state->fd));
	memset(state, 0, sizeof(pkg_state));
	return 0;
}

int open_pkg(pkg_state* state, char* pkg_file) {
	char pkgBuf[0x8000];
	memset(pkgBuf, 0, sizeof(pkgBuf));
	memset(state, 0, sizeof(pkg_state));
	
	state->fd = sceIoOpen(pkg_file, SCE_O_RDONLY, 0);
	ERROR_CHECKED(state->fd);
	
	PRINT_STR("sizeof PKG_FILE_HEADER = 0x%X\n", sizeof(PKG_FILE_HEADER));
	PRINT_STR("sizeof PKG_EXT_HEADER = 0x%X\n", sizeof(PKG_EXT_HEADER));
	PRINT_STR("sizeof PKG_METADATA = 0x%X\n", sizeof(PKG_METADATA));
	PRINT_STR("sizeof PKG_ITEM_RECORD = 0x%X\n", sizeof(PKG_ITEM_RECORD));

	ERROR_CHECKED(sceIoRead(state->fd, pkgBuf, sizeof(pkgBuf)));
	ERROR_CHECKED(_sceNpDrmPackageCheck(pkgBuf, sizeof(pkgBuf), 0, 0x100));
	memcpy(&state->pkgHeader, pkgBuf, sizeof(PKG_FILE_HEADER));

	state->pkgHeader.item_count = __builtin_bswap32(state->pkgHeader.item_count);
	state->pkgHeader.data_offset = __builtin_bswap64(state->pkgHeader.data_offset);
	state->pkgHeader.type = __builtin_bswap64(state->pkgHeader.type);
	
	return 0;
}

int pkg_get_head_bin(pkg_state *state, char* head_bin_location) {
	size_t length = state->pkgHeader.data_offset - state->pkgHeader.info_offset;
    char* headBin = alloca(length);
	
	ERROR_CHECKED(sceIoLseek(state->fd, state->pkgHeader.info_offset, SCE_SEEK_SET));


}

int expand_package(char* pkg_file, char* out_folder, void (*progress_callback)(char*, uint64_t, uint64_t)) {
	
	char relFilename[MAX_PATH];
	char outfile[MAX_PATH*2];
	
	pkg_state state;
	ERROR_CHECKED(open_pkg(&state, pkg_file));
	
	
	
	for(int current_item = 0; current_item < state.pkgHeader.item_count; current_item++) 
	{
		memset(relFilename, 0, sizeof(relFilename));
		memset(outfile, 0, sizeof(outfile));
		
		// update progress in GUI
		if(progress_callback != NULL) progress_callback(pkg_file, (uint64_t)current_item, (uint64_t)state.pkgHeader.item_count);

		// read and decrypt item record entry.
		ERROR_CHECKED(pkg_read_item(&state, current_item));


		// read and decrypt item filename
		if(state.pkgItem.filename_size > sizeof(relFilename)) ERROR_CHECKED(-3);
		pkg_read(&state, state.pkgItem.filename_offset, relFilename, state.pkgItem.filename_size);
		
		
		snprintf(outfile, sizeof(outfile), "%s/%s", out_folder, relFilename);
		PRINT_STR("outfile: %s\n", outfile);
		

		switch(state.pkgItem.flags & 0x1F){
			case PKG_TYPE_FILE:
			case PKG_TYPE_SELF:
			case PKG_TYPE_PFS_KEYSTONE:
			case PKG_TYPE_PFS_FILE:
			case PKG_TYPE_PFS_TEMP_BIN:
			case PKG_TYPE_PFS_CLEARSIGN:
			case PKG_TYPE_SCESYS_RIGHT_SUPRX:
			case PKG_TYPE_SCESYS_CERT_BIN:
			case PKG_TYPE_SCESYS_DIGS_BIN:
			default:
				ERROR_CHECKED(extract_file(&state, outfile));
				break;
			case PKG_TYPE_DIR:
			case PKG_TYPE_PFS_DIR:
				sceIoMkdir(outfile, 0777);
				
				break;
		}

		PRINT_STR("flags 0x%08X (%02X) offset 0x%08llX length 0x%08llX %s\n", state.pkgItem.flags, (state.pkgItem.flags & 0x1F), state.pkgItem.data_offset, state.pkgItem.data_size, relFilename);
	}
	
	
	close_pkg(&state);
	return 0;
}
