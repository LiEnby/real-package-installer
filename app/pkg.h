#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef PKG_H 
#define PKG_H 1

#define PACKED __attribute__( ( __packed__ ) )
#define SCE_NPDRM_PACKAGE_CHECK_SIZE (0x8000)

typedef struct PKG_ITEM_RECORD {
    uint32_t filename_offset;
    uint32_t filename_size;
    uint64_t data_offset;
    uint64_t data_size;
    uint32_t flags;
    uint32_t reserved;
} PACKED PKG_ITEM_RECORD;

typedef struct PKG_FILE_HEADER {
    uint32_t magic;
    uint16_t revision;
    uint16_t type;
    uint32_t info_offset;
    uint32_t info_count;
    uint32_t header_size;
    uint32_t item_count;
    uint64_t total_size;
    uint64_t data_offset;
    uint64_t data_size;
    char content_id[0x30];
    uint8_t digest[0x10];
    uint8_t pkg_data_iv[0x10];
    uint8_t pkg_signatures[0x40];
} PACKED PKG_FILE_HEADER;

// Extended PKG header, found in PSV packages
typedef struct PKG_EXT_HEADER {
    uint32_t magic;
    uint32_t unknown_01;
    uint32_t header_size;
    uint32_t data_size;
    uint32_t data_offset;
    uint32_t data_type;
    uint64_t pkg_data_size;

    uint32_t padding_01;
    uint32_t data_type2;
    uint32_t unknown_02;
    uint32_t padding_02;
    uint64_t padding_03;
    uint64_t padding_04;
} PACKED PKG_EXT_HEADER;

typedef struct PKG_METADATA {
    uint32_t drm_type;           //Record type 0x1 (for trial-enabled packages, drm is either 0x3 or 0xD)
    uint32_t content_type;       //Record type 0x2
    uint32_t package_flags;      //Record type 0x3
    uint32_t index_table_offset; //Record type 0xD, offset 0x0
    uint32_t index_table_size;   //Record type 0xD, offset 0x4
    uint32_t sfo_offset;         //Plaintext SFO copy, record type 0xE, offset 0x0
    uint32_t sfo_size;           //Record type 0xE, offset 0x4
} PKG_METADATA;

typedef enum PKG_FILE_TYPE {
	PKG_TYPE_FILE = 0x3,
	PKG_TYPE_DIR = 0x4,
	PKG_TYPE_SELF = 0xE,
	PKG_TYPE_PFS_KEYSTONE = 0x10,
	PKG_TYPE_PFS_FILE = 0x11,
	PKG_TYPE_PFS_DIR = 0x12,
	PKG_TYPE_PFS_TEMP_BIN = 0x13,
	PKG_TYPE_PFS_CLEARSIGN = 0x15,
	PKG_TYPE_SCESYS_RIGHT_SUPRX = 0x16,
	PKG_TYPE_SCESYS_CERT_BIN = 0x17,
	PKG_TYPE_SCESYS_DIGS_BIN = 0x18
} PKG_FILE_TYPE;

int expand_package(char* pkg_file, char* out_folder, void (*progress_callback)(char*, uint64_t, uint64_t));

#endif