#include "log.h"
#include "pkg.h"
#include "io.h"
#include "err.h"
#include <vitasdk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int pkg_decrypt(int offset, void* buffer, size_t bufferSize) {
	_sceNpDrmPackageDecrypt_opt decryptOption;
	decryptOption.offset = offset; 
	decryptOption.identifier = 0x100;
	
	CHECK_ERROR(_sceNpDrmPackageDecrypt(buffer, bufferSize, &decryptOption));
	return 0;
}


uint64_t pkg_seek(pkg_state* state, uint64_t whence, int mode) {
	uint64_t newLocation = sceIoLseek(state->fd, whence, mode);
	CHECK_ERROR(newLocation);
	state->offset = newLocation;
	return newLocation;
}

int pkg_read(pkg_state* state, void* buffer, size_t bufferSize) {
	int amtRead = sceIoRead(state->fd, buffer, bufferSize);
	CHECK_ERROR(amtRead);
	
	if(state->offset >= state->pkgHeader.data_offset) {
		uint32_t relOffset = (state->offset - state->pkgHeader.data_offset);
		CHECK_ERROR(pkg_decrypt(relOffset, buffer, bufferSize));
	}
	
	state->offset += amtRead;
	return amtRead;
}

int pkg_read_offset(pkg_state* state, uint32_t offset, void* buffer, size_t bufferSize) {
	
	CHECK_ERROR(pkg_seek(state, offset, SCE_SEEK_SET));
	int amtRead = pkg_read(state, buffer, bufferSize);
	CHECK_ERROR(amtRead);
	
	return amtRead;
}

int pkg_get_metadata(pkg_state *state) {	
	CHECK_ERROR(pkg_seek(state, state->pkgHeader.meta_offset, SCE_SEEK_SET));
	
	
	for(int i = 0;  i < state->pkgHeader.meta_count; i++) {
		PKG_METADATA_ENTRY metaEntry;
		CHECK_ERROR(pkg_read(state, &metaEntry, sizeof(PKG_METADATA_ENTRY)));
		
		metaEntry.type = __builtin_bswap32(metaEntry.type);
		metaEntry.size = __builtin_bswap32(metaEntry.size);
		int rd = 0;
		
		PRINT_STR("metaEntry.type = %x, metaEntry.size = %x, rd = %x\n", metaEntry.type, metaEntry.size, rd);
		
		switch(metaEntry.type) {
			case PKG_META_DRM_TYPE:
				PRINT_STR("PKG_META_DRM_TYPE\n");
				rd += pkg_read(state, &state->pkgMetadata.drm_type, sizeof(uint32_t));
				break;
			case PKG_META_CONTENT_TYPE:
				PRINT_STR("PKG_META_CONTENT_TYPE\n");
				rd += pkg_read(state, &state->pkgMetadata.content_type, sizeof(uint32_t));
				break;
			case PKG_META_PACKAGE_FLAGS:
				PRINT_STR("PKG_META_PACKAGE_FLAGS\n");
				rd += pkg_read(state, &state->pkgMetadata.package_flags, sizeof(uint32_t));
				break;
			case PKG_META_FILE_ITEM_INFO:
				PRINT_STR("PKG_META_FILE_ITEM_INFO\n");
				rd += pkg_read(state, &state->pkgMetadata.item_table_offset, sizeof(uint32_t));
				rd += pkg_read(state, &state->pkgMetadata.item_table_size, sizeof(uint32_t));
				break;
			case PKG_META_SFO:
				PRINT_STR("PKG_META_SFO\n");
				rd += pkg_read(state, &state->pkgMetadata.sfo_offset, sizeof(uint32_t));
				rd += pkg_read(state, &state->pkgMetadata.sfo_size, sizeof(uint32_t));
				break;
			default:
				PRINT_STR("Unknown meta entry type! (%x)\n", metaEntry.type);
				break;
		}
		PRINT_STR("rd = %x, (metaEntry.size-rd) = %x\n", rd, metaEntry.size - rd);
		CHECK_ERROR(rd);
		CHECK_ERROR(pkg_seek(state, metaEntry.size - rd, SCE_SEEK_CUR));
	}
	state->pkgMetadata.drm_type           = __builtin_bswap32( state->pkgMetadata.drm_type );
	state->pkgMetadata.content_type       = __builtin_bswap32( state->pkgMetadata.content_type );
	state->pkgMetadata.package_flags      = __builtin_bswap32( state->pkgMetadata.package_flags );
	state->pkgMetadata.item_table_offset  = __builtin_bswap32( state->pkgMetadata.item_table_offset);
	state->pkgMetadata.item_table_size    = __builtin_bswap32( state->pkgMetadata.item_table_size);
	state->pkgMetadata.sfo_offset         = __builtin_bswap32( state->pkgMetadata.sfo_offset );
	state->pkgMetadata.sfo_size           = __builtin_bswap32( state->pkgMetadata.sfo_size );
		
	PRINT_STR("state->pkgMetadata.drm_type            = %x\n", state->pkgMetadata.drm_type         );
	PRINT_STR("state->pkgMetadata.content_type        = %x\n", state->pkgMetadata.content_type     );
	PRINT_STR("state->pkgMetadata.package_flags       = %x\n", state->pkgMetadata.package_flags    );
	PRINT_STR("state->pkgMetadata.item_table_offset   = %x\n", state->pkgMetadata.item_table_offset);
	PRINT_STR("state->pkgMetadata.item_table_size     = %x\n", state->pkgMetadata.item_table_size  );
	PRINT_STR("state->pkgMetadata.sfo_offset          = %x\n", state->pkgMetadata.sfo_offset       );
	PRINT_STR("state->pkgMetadata.sfo_size            = %x\n", state->pkgMetadata.sfo_size         );

	return 0;
}

int close_pkg(pkg_state* state) {
	CHECK_ERROR(sceIoClose(state->fd));
	memset(state, 0, sizeof(pkg_state));
	return 0;
}


int open_pkg(pkg_state* state, const char* pkg_file) {
	static char checkPkg[0x8000];
	memset(checkPkg, 0, sizeof(checkPkg));
	memset(state, 0, sizeof(pkg_state));
	
	state->fd = sceIoOpen(pkg_file, SCE_O_RDONLY, 0);
	CHECK_ERROR(state->fd);	
	
	// check pkg with npdrm ...
	CHECK_ERROR(sceIoRead(state->fd, checkPkg, sizeof(checkPkg)));
	CHECK_ERROR(_sceNpDrmPackageCheck(checkPkg, sizeof(checkPkg), 0, 0x100));
	CHECK_ERROR(sceIoLseek(state->fd, 0, SCE_SEEK_SET));
	
	
	CHECK_ERROR(sceIoRead(state->fd, &state->pkgHeader, sizeof(PKG_FILE_HEADER)));

	PRINT_STR("sizeof PKG_FILE_HEADER = 0x%X\n", sizeof(PKG_FILE_HEADER));
	PRINT_STR("sizeof PKG_EXT_HEADER = 0x%X\n", sizeof(PKG_EXT_HEADER));
	PRINT_STR("sizeof PKG_METADATA = 0x%X\n", sizeof(PKG_METADATA));
	PRINT_STR("sizeof PKG_ITEM_RECORD = 0x%X\n", sizeof(PKG_ITEM_RECORD));
	
	state->pkgHeader.magic       = __builtin_bswap32( state->pkgHeader.magic );
	state->pkgHeader.revision    = __builtin_bswap16( state->pkgHeader.revision );
	state->pkgHeader.type        = __builtin_bswap16( state->pkgHeader.type );
	state->pkgHeader.meta_offset = __builtin_bswap32( state->pkgHeader.meta_offset );
	state->pkgHeader.meta_count  = __builtin_bswap32( state->pkgHeader.meta_count );
	state->pkgHeader.meta_size   = __builtin_bswap32( state->pkgHeader.meta_size );
	state->pkgHeader.item_count  = __builtin_bswap32( state->pkgHeader.item_count );
	state->pkgHeader.total_size  = __builtin_bswap64( state->pkgHeader.total_size );
	state->pkgHeader.data_offset = __builtin_bswap64( state->pkgHeader.data_offset );
	state->pkgHeader.data_size   = __builtin_bswap64( state->pkgHeader.data_size );
	
	PRINT_STR("state->pkgHeader.magic       = %x\n", state->pkgHeader.magic       );
	PRINT_STR("state->pkgHeader.revision    = %x\n", state->pkgHeader.revision    );
	PRINT_STR("state->pkgHeader.type        = %x\n", state->pkgHeader.type        );
	PRINT_STR("state->pkgHeader.meta_offset = %x\n", state->pkgHeader.meta_offset );
	PRINT_STR("state->pkgHeader.meta_count  = %x\n", state->pkgHeader.meta_count  );
	PRINT_STR("state->pkgHeader.meta_size   = %x\n", state->pkgHeader.meta_size   );
	PRINT_STR("state->pkgHeader.item_count  = %x\n", state->pkgHeader.item_count  );
	PRINT_STR("state->pkgHeader.total_size  = %llx\n", state->pkgHeader.total_size  );
	PRINT_STR("state->pkgHeader.data_offset = %llx\n", state->pkgHeader.data_offset );
	PRINT_STR("state->pkgHeader.data_size   = %llx\n", state->pkgHeader.data_size   );
	
	
	
	if (state->pkgHeader.meta_size > sizeof(PKG_FILE_HEADER)) {
		CHECK_ERROR(sceIoRead(state->fd, &state->pkgExtHeader, sizeof(PKG_EXT_HEADER)));
		
		state->pkgExtHeader.magic         =    __builtin_bswap32( state->pkgExtHeader.magic );
        state->pkgExtHeader.unknown_01    =    __builtin_bswap32( state->pkgExtHeader.unknown_01 );
        state->pkgExtHeader.header_size   =    __builtin_bswap32( state->pkgExtHeader.header_size );
        state->pkgExtHeader.data_size     =    __builtin_bswap32( state->pkgExtHeader.data_size );
        state->pkgExtHeader.data_offset   =    __builtin_bswap32( state->pkgExtHeader.data_offset );
        state->pkgExtHeader.data_type     =    __builtin_bswap32( state->pkgExtHeader.data_type );
        state->pkgExtHeader.pkg_data_size =    __builtin_bswap64( state->pkgExtHeader.pkg_data_size );
        state->pkgExtHeader.padding_01    =    __builtin_bswap32( state->pkgExtHeader.padding_01 );
        state->pkgExtHeader.data_type2    =    __builtin_bswap32( state->pkgExtHeader.data_type2 );
        state->pkgExtHeader.unknown_02    =    __builtin_bswap32( state->pkgExtHeader.unknown_02 );
        state->pkgExtHeader.padding_02    =    __builtin_bswap32( state->pkgExtHeader.padding_02 );
        state->pkgExtHeader.padding_03    =    __builtin_bswap64( state->pkgExtHeader.padding_03 );
        state->pkgExtHeader.padding_04    =    __builtin_bswap64( state->pkgExtHeader.padding_04 );
		
		PRINT_STR("state->pkgExtHeader.magic         = %x\n", state->pkgExtHeader.magic         ); 
		PRINT_STR("state->pkgExtHeader.unknown_01    = %x\n", state->pkgExtHeader.unknown_01    ); 
		PRINT_STR("state->pkgExtHeader.header_size   = %x\n", state->pkgExtHeader.header_size   ); 
		PRINT_STR("state->pkgExtHeader.data_size     = %x\n", state->pkgExtHeader.data_size     ); 
		PRINT_STR("state->pkgExtHeader.data_offset   = %x\n", state->pkgExtHeader.data_offset   ); 
		PRINT_STR("state->pkgExtHeader.data_type     = %x\n", state->pkgExtHeader.data_type     ); 
		PRINT_STR("state->pkgExtHeader.pkg_data_size = %llx\n", state->pkgExtHeader.pkg_data_size ); 
		PRINT_STR("state->pkgExtHeader.padding_01    = %x\n", state->pkgExtHeader.padding_01    ); 
		PRINT_STR("state->pkgExtHeader.data_type2    = %x\n", state->pkgExtHeader.data_type2    ); 
		PRINT_STR("state->pkgExtHeader.unknown_02    = %x\n", state->pkgExtHeader.unknown_02    ); 
		PRINT_STR("state->pkgExtHeader.padding_02    = %x\n", state->pkgExtHeader.padding_02    ); 
		PRINT_STR("state->pkgExtHeader.padding_03    = %llx\n", state->pkgExtHeader.padding_03    ); 
		PRINT_STR("state->pkgExtHeader.padding_04    = %llx\n", state->pkgExtHeader.padding_04    ); 
		
	}
	
	CHECK_ERROR(pkg_get_metadata(state));
	
	return 0;
}

int extract_file(pkg_state* state, const char* outfile) {
	
	CHECK_ERROR(pkg_seek(state, state->pkgHeader.data_offset + state->pkgItem.data_offset, SCE_SEEK_SET));	
	SceUID wfd = sceIoOpen(outfile, SCE_O_WRONLY | SCE_O_CREAT, 0777);
	CHECK_ERROR(wfd);
	
	int ret = 0;
	static char buffer[0x8000];
	int totalRead = 0;
	
	do { 
		// get amount of data to read
		int readSize = (sizeof(buffer) < (state->pkgItem.data_size - totalRead)) ? sizeof(buffer) : (state->pkgItem.data_size - totalRead);
		
		// read the data
		int amtRead = pkg_read(state, buffer, readSize);
		if(amtRead < 0) ERROR(amtRead);
		if(amtRead != readSize) ERROR(-1);

		// write decrypted data
		int amtWritten = sceIoWrite(wfd, buffer, amtRead);
		if(amtWritten < 0) ERROR(amtWritten);
		if(amtWritten != amtRead) ERROR(-1);
		
		totalRead += amtWritten;

	} while(totalRead < state->pkgItem.data_size);

error:
	sceIoClose(wfd);
	return ret;
}

int expand_package(const char* pkg_file, const char* out_folder, void (*progress_callback)(const char*, uint64_t, uint64_t)) {
	
	char relFilename[MAX_PATH];
	char outfile[MAX_PATH*2];
	
	pkg_state state;
	CHECK_ERROR(open_pkg(&state, pkg_file));
	
	for(int current_item = 0; current_item < state.pkgHeader.item_count; current_item++) 
	{
		memset(relFilename, 0, sizeof(relFilename));
		memset(outfile, 0, sizeof(outfile));
		
		// update progress in GUI
		if(progress_callback != NULL) progress_callback(pkg_file, (uint64_t)current_item, (uint64_t)state.pkgHeader.item_count);

		// read item record entry
		CHECK_ERROR(pkg_read_offset(&state, ((state.pkgHeader.data_offset + state.pkgMetadata.item_table_offset) + (current_item * sizeof(PKG_ITEM_RECORD))), &state.pkgItem, sizeof(PKG_ITEM_RECORD)));
		
		state.pkgItem.flags            = __builtin_bswap32(state.pkgItem.flags);
		state.pkgItem.filename_offset  = __builtin_bswap32(state.pkgItem.filename_offset);
		state.pkgItem.filename_size    = __builtin_bswap32(state.pkgItem.filename_size);
		state.pkgItem.data_offset      = __builtin_bswap64(state.pkgItem.data_offset);
		state.pkgItem.data_size        = __builtin_bswap64(state.pkgItem.data_size);	


		// read and item filename
		if(state.pkgItem.filename_size > sizeof(relFilename)) CHECK_ERROR(-3);
		CHECK_ERROR(pkg_read_offset(&state, (state.pkgHeader.data_offset + state.pkgItem.filename_offset), relFilename, state.pkgItem.filename_size));	
		snprintf(outfile, sizeof(outfile), "%s/%s", out_folder, relFilename);


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
				CHECK_ERROR(extract_file(&state, outfile));
				break;
			case PKG_TYPE_DIR:
			case PKG_TYPE_PFS_DIR:
				sceIoMkdir(outfile, 0777);
				
				break;
		}

		PRINT_STR("flags 0x%08X (%02X) offset 0x%08llX length 0x%08llX %s\n", state.pkgItem.flags, (state.pkgItem.flags & 0x1F), state.pkgItem.data_offset, state.pkgItem.data_size, relFilename);
	}
	
	snprintf(outfile, sizeof(outfile), "%s/%s", out_folder, "sce_sys/package/head.bin");
	get_head_bin(&state, outfile);

	snprintf(outfile, sizeof(outfile), "%s/%s", out_folder, "sce_sys/package/tail.bin");
	get_tail_bin(&state, outfile);

	
	close_pkg(&state);
	return 0;
}


int get_head_bin(pkg_state* state, const char* outfile) {
	
	int ret = 0;
	int headSize = (state->pkgHeader.data_offset + state->pkgMetadata.item_table_size);
	char* head = malloc(headSize);

	PRINT_STR("headSize = %x, head = %p\n", headSize, head);
	
	if(head != NULL) {
		if(sceIoLseek(state->fd, 0, SCE_SEEK_SET) != 0) ERROR(-99);
		if(sceIoRead(state->fd, head, headSize) != headSize) ERROR(-99);
		if(write_file(outfile, head, headSize) != headSize) ERROR(-99);		
	}
	else {
		ERROR(0);
	}
		
error:
	pkg_seek(state, 0, SCE_SEEK_SET);
	free(head);
	return ret;
}

int get_tail_bin(pkg_state* state, const char* outfile) {
	
	int ret = 0;
	uint64_t tailOffset = state->pkgHeader.data_offset + state->pkgHeader.data_size;
	int tailSize = state->pkgHeader.total_size - tailOffset;
	char* tail = malloc(tailSize);

	PRINT_STR("tailOffset = %llx, tailSize = %x, tail = %p\n", tailOffset, tailSize, tail);
	
	if(tail != NULL) {
		if(sceIoLseek(state->fd, tailOffset, SCE_SEEK_SET) != tailOffset) ERROR(-99);
		if(sceIoRead(state->fd, tail, tailSize) != tailSize) ERROR(-99);
		if(write_file(outfile, tail, tailSize) != tailSize) ERROR(-99);
	}
	else {
		ERROR(0);
	}
	
error:
	pkg_seek(state, 0, SCE_SEEK_SET);
	free(tail);
	return ret;
}
