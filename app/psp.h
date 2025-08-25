#ifndef PSP_H

typedef enum PbpType{
  PBP_TYPE_NPUMDIMG   = 0,
  PBP_TYPE_PSISOIMG   = 1,
  PBP_TYPE_PSTITLEIMG = 2,
  PBP_TYPE_UNKNOWN    = 3
} PbpType;

// for PSISOIMG and PSTITLEIMG contents

typedef struct DataPspHeader{
  char magic[0x4];
  char unk[0x7C];
  char unk2[0x32];
  char unk3[0xE];
  char hash[0x14];
  char reserved[0x58];
  char unk4[0x434];
  char content_id[0x30];
} DataPspHeader;

// for NPUMDIMG content
typedef struct NpUmdImgBody {
  uint16_t sector_size;   // 0x0800
  uint16_t unk_2;      // 0xE000
  uint32_t unk_4;
  uint32_t unk_8;
  uint32_t unk_12;
  uint32_t unk_16;
  uint32_t lba_start;
  uint32_t unk_24;
  uint32_t nsectors;
  uint32_t unk_32;
  uint32_t lba_end;
  uint32_t unk_40;
  uint32_t block_entry_offset;
  char disc_id[0x10];
  uint32_t header_start_offset;
  uint32_t unk_68;
  uint8_t unk_72;
  uint8_t bbmac_param;
  uint8_t unk_74;
  uint8_t unk_75;
  uint32_t unk_76;
  uint32_t unk_80;
  uint32_t unk_84;
  uint32_t unk_88;
  uint32_t unk_92;
} NpUmdImgBody;

typedef struct NpUmdImgHeader{
  uint8_t magic[0x08];  // "NPUMDIMG"
  uint32_t key_index; // usually 2, or 3.
  uint32_t block_basis;
  uint8_t content_id[0x30];
  NpUmdImgBody body;  
  uint8_t header_key[0x10];
  uint8_t data_key[0x10];
  uint8_t header_hash[0x10];
  uint8_t padding[0x8];
  uint8_t ecdsa_sig[0x28];
}  NpUmdImgHeader;

// generic eboot.pbp header

typedef struct PbpHeader
{
  char magic[0x4];
  uint32_t version;
  uint32_t param_sfo_ptr;  
  uint32_t icon0_png_ptr;  
  uint32_t icon1_pmf_ptr;
  uint32_t pic0_png_ptr;
  uint32_t pic1_png_ptr;  
  uint32_t snd0_at3_ptr;  
  uint32_t data_psp_ptr;  
  uint32_t data_psar_ptr;
} PbpHeader;

int get_pbp_sfo(const char* pbp_file, void** param_sfo_buffer);
int get_pbp_content_id(const char* pbp_file, char* content_id);
int gen_sce_ebootpbp(const char* pbp_file, const char* psp_game_folder);
int hash_pbp(const char* eboot_file, uint8_t* out_hash);

int custom_promote_psp(const char* path);

#endif