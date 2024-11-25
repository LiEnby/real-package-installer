#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef PKG_H 
#define PKG_H 1

#define PACKED __attribute__( ( __packed__ ) )
#define SCE_NPDRM_PACKAGE_CHECK_SIZE (0x8000)

#define IS_PSP_OR_VITA_KEY(x) ( ((x >> 0x1C)&7) == 1)
#define IS_PSP_CONTENT_TYPE(x) (x == PKG_TYPE_PSX || x == PKG_TYPE_PSP || x == PKG_TYPE_PSP_GO || x == PKG_TYPE_PSP_MINIS || x == PKG_TYPE_PSP_NEO_GEO)


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
    uint32_t meta_offset;
    uint32_t meta_count;
    uint32_t meta_size;
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

typedef struct PKG_METADATA_ENTRY {
	uint32_t type;
	uint32_t size;
} PKG_METADATA_ENTRY;

typedef struct PKG_METADATA {
    uint32_t drm_type;           //Record type 0x1 (for trial-enabled packages, drm is either 0x3 or 0xD)
    uint32_t content_type;       //Record type 0x2
    uint32_t package_flags;      //Record type 0x3
    uint32_t item_table_offset; //Record type 0xD, offset 0x0
    uint32_t item_table_size;   //Record type 0xD, offset 0x4
    uint32_t sfo_offset;         //Plaintext SFO copy, record type 0xE, offset 0x0
    uint32_t sfo_size;           //Record type 0xE, offset 0x4
} PKG_METADATA;

typedef enum PKG_TYPE {
	PKG_TYPE_PS3 = 0x1,
	PKG_TYPE_PSV = 0x2
} PKG_TYPE;

typedef enum PKG_METADATA_TYPE {
	PKG_META_DRM_TYPE = 0x1,
	PKG_META_CONTENT_TYPE = 0x2,
	PKG_META_PACKAGE_FLAGS = 0x3,
	PKG_META_PACKAGE_SIZE = 0x4,
	PKG_META_MAKE_PKG_EXE_VERSION = 0x5,
	PKG_META_APP_VERSION = 0x6,
	PKG_META_QA_DIGEST = 0x7,
	PKG_META_REQUIRED_SYS_VERSION = 0x8,
	PKG_META_UNK_9 = 0x9,
	PKG_META_INSTALL_DIRECTORY = 0xA,
	PKG_META_UNK_B = 0xB,
	PKG_META_UNK_C = 0xC,
	PKG_META_FILE_ITEM_INFO = 0xD,
	PKG_META_SFO = 0xE,
	PKG_META_UNK_F = 0xF,
	PKG_META_ENTIRETY_INFO = 0x10,
	PKG_META_PUBLISHINGTOOLS_VERISION = 0x11,
	PKG_META_SELF_INFO = 0x12
} PKG_METADATA_TYPE;

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

typedef enum PKG_CONTENT_TYPE {
	PKG_TYPE_UNK_PS3 = 0x1,
	PKG_TYPE_UNK_2 = 0x2,
	PKG_TYPE_UNK_3 = 0x3,
	PKG_TYPE_PS3_GAMEDATA = 0x4,
	PKG_TYPE_PS3_GAMEEXEC = 0x5,
	PKG_TYPE_PSX = 0x6,
	PKG_TYPE_PSP = 0x7,
	PKG_TYPE_UNK_8 = 0x8,
	PKG_TYPE_PS3_THEME = 0x9,
	PKG_TYPE_PS3_WIDGET = 0xA,
	PKG_TYPE_PS3_LICENSE = 0xB,
	PKG_TYPE_PS3_VSH_MODULE = 0xC,
	PKG_TYPE_PS3_PSN_AVATAR = 0xD,
	PKG_TYPE_PSP_GO = 0xE,
	PKG_TYPE_PSP_MINIS = 0xF,
	PKG_TYPE_PSP_NEO_GEO = 0x10,
	PKG_TYPE_PS3_VMC = 0x11,
	PKG_TYPE_PS3_PS2 = 0x12,
	PKG_TYPE_UNK_13 = 0x13,
	PKG_TYPE_PS3_PSP_REMASTER = 0x14,
	PKG_TYPE_VITA_APP = 0x15,
	PKG_TYPE_VITA_DLC = 0x16,
	PKG_TYPE_VITA_LIVEAREA = 0x17,
	PKG_TYPE_PSM = 0x18,
	PKG_TYPE_WEB_TV = 0x19,
	PKG_TYPE_PSM_UNITY = 0x1D,
	PKG_TYPE_VITA_THEME = 0x1F
} PKG_CONTENT_TYPE;

typedef enum PKG_REVISION {
	PKG_REVISION_DEBUG = 0x00,
	PKG_REVISION_FINALIZED = 0x80
} PKG_REVISION;

typedef enum PKG_PACKAGE_FLAGS { 
	PKG_FLAG_EBOOT_PBP = 0x2,
	PKG_FLAG_LICENSE_REQUIRED = 0x4,
	PKG_FLAG_MEMORY_CARD_INSTALL = 0x8,
	PKG_FLAG_PATCH_FILE = 0x10
} PKG_PACKAGE_FLAGS;

typedef struct PKG_STATE {
	SceUID fd;
	uint64_t offset;
	PKG_FILE_HEADER pkgHeader;
	PKG_EXT_HEADER pkgExtHeader;
	PKG_METADATA pkgMetadata;
	PKG_ITEM_RECORD pkgItem;
} PKG_STATE;

int get_tail_bin(PKG_STATE* state, const char* outfile);
int get_head_bin(PKG_STATE* state, const char* outfile);
int get_work_bin(PKG_STATE* state, const char* pkg_folder, const char* outfile);

int package_revision(const char* pkg_file);
int package_content_type(const char* pkg_file);
int package_meta_flags(const char* pkg_file) ;
void package_content_id(const char* pkg_file, char* content_id, size_t len);
PKG_STATE package_state(const char* pkg_file);

int expand_package(const char* pkg_file, const char* out_folder, void (*progress_callback)(const char*, uint64_t, uint64_t));

int open_pkg(PKG_STATE* state, const char* pkg_file);
int close_pkg(PKG_STATE* state);
int read_pkg(PKG_STATE* state, void* buffer, size_t bufferSize);
uint64_t seek_pkg(PKG_STATE* state, uint64_t whence, int mode);
int decrypt_pkg(int offset, void* buffer, size_t bufferSize);
int read_pkg_offset(PKG_STATE* state, uint32_t offset, void* buffer, size_t bufferSize);

#endif