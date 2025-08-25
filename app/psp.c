#include <vitasdk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "sha256.h"
#include "psp.h"
#include "log.h"
#include "io.h"

int custom_promote_psp(const char* pkg_path, const char* promote_path) {	
	char eboot_pbp_path[MAX_PATH];
	char license_path[MAX_PATH];
	
	// move folders over to here
	char src_game_folder[MAX_PATH];
	char dst_game_folder[MAX_PATH];
	char src_license_file[MAX_PATH];
	char dst_license_file[MAX_PATH];
	
	char disc_id[0x10]
	char content_id[0x20];
	
	uint8_t pbp_hash[0x20];
	uint8_t eboot_signature[0x200];
	
	snprintf(eboot_pbp_path, sizeof(eboot_pbp_path), "%s/USRDIR/CONTENT/EBOOT.PBP", pkg_path);
	gen_sce_ebootpbp(eboot_pbp_path, pkg_path);
	
	snprintf(src_game_folder, sizeof(src_game_folder), "%s/USRDIR/CONTENT", pkg_path);
	snprintf(dst_game_folder, sizeof(dst_game_folder), "%s/PSP/GAME/%s", promote_location, disc_id);

	snprintf(src_license_file, sizeof(src_license_file), "%s/sce_sys/package/work.bin", pkg_path);
	snprintf(dst_license_file, sizeof(dst_license_file), "%s/PSP/LICENSE/%s.rif", promote_location, content_id);
	
	
	
	return 0;
}

int read_content_id_data_psp(SceUID pbp_fd, char* content_id) {
  DataPspHeader data_psp_header;

  // read data.psp header
  int read_sz = sceIoRead(pbp_fd, &data_psp_header, sizeof(DataPspHeader));
  if(read_sz < sizeof(DataPspHeader)) return 0;

  // copy the content id from data.psp header to this content_id buffer
  strncpy(content_id, data_psp_header.content_id, 0x30);
  
  return (strlen(content_id) == 36);
}

int read_content_id_npumdimg(SceUID pbp_fd, char* content_id) {
  NpUmdImgHeader npumdimg_header;
  
  // read npumd header
  int read_sz = sceIoRead(pbp_fd, &npumdimg_header, sizeof(NpUmdImgHeader));
  if(read_sz < sizeof(NpUmdImgHeader)) return 0;
  
  // copy the content id from npumdimg_header to this content_id buffer
  strncpy(content_id, npumdimg_header.content_id, 0x30);
  
  return (strlen(content_id) == 36);
}

int determine_pbp_type(SceUID pbp_fd, PbpHeader* pbp_header) {
  char data_psar_magic[0x8];
  
  int read_sz = sceIoRead(pbp_fd, pbp_header, sizeof(PbpHeader));
  if(read_sz < sizeof(PbpHeader)) return 0;
  
  // seek to data.psar
  sceIoLseek(pbp_fd, pbp_header->data_psar_ptr, SCE_SEEK_SET);
  
  // read magic value to determine pbp type
  read_sz = sceIoRead(pbp_fd, data_psar_magic, sizeof(data_psar_magic));
  if(read_sz < sizeof(data_psar_magic)) return 0;
  
  if(memcmp(data_psar_magic,      "NPUMDIMG", 0x8) == 0) { // psp
    return PBP_TYPE_NPUMDIMG;
  }
  else if(memcmp(data_psar_magic, "PSISOIMG", 0x8) == 0) { // ps1 single disc
    return PBP_TYPE_PSISOIMG;
  }
  else if(memcmp(data_psar_magic, "PSTITLEI", 0x8) == 0) { // ps1 multi disc
    return PBP_TYPE_PSTITLEIMG;
  }
  else{ // update package, homebrew, etc, 
    return PBP_TYPE_UNKNOWN; 
  }
  
  if(read_sz < sizeof(PbpHeader)) return PBP_TYPE_UNKNOWN;
}

int read_data_psar_header(SceUID pbp_fd, char* content_id) {
  PbpHeader pbp_header;
  int pbp_type = determine_pbp_type(pbp_fd, &pbp_header);
  if(pbp_type == PBP_TYPE_NPUMDIMG) {
    // seek to start of npumdimg
    sceIoLseek(pbp_fd, pbp_header.data_psar_ptr, SCE_SEEK_SET);
    
    // read content_id from npumdimg
    return read_content_id_npumdimg(pbp_fd, content_id);
  }
  else if(pbp_type == PBP_TYPE_PSISOIMG || pbp_type == PBP_TYPE_PSTITLEIMG) {
    // seek to start of data.psp
    sceIoLseek(pbp_fd, pbp_header.data_psp_ptr, SCE_SEEK_SET);

    // read content_id from data.psp
    return read_content_id_data_psp(pbp_fd, content_id);	  
  }
  else {
    return 0; 
  }  
}

int read_sfo(SceUID pbp_fd, void** param_sfo_buffer){
  PbpHeader pbp_header;
  // read pbp header
  int read_sz = sceIoRead(pbp_fd, &pbp_header, sizeof(PbpHeader));
  if(read_sz < sizeof(PbpHeader)) return 0;

  // get sfo size
  int param_sfo_size = pbp_header.icon0_png_ptr - pbp_header.param_sfo_ptr;
  if(param_sfo_size <= 0) return 0;
  
  // allocate a buffer for the param.sfo file
  *param_sfo_buffer = malloc(param_sfo_size);
  if(*param_sfo_buffer == NULL) return 0;
  
  // seek to the start of param.sfo
  sceIoLseek(pbp_fd, pbp_header.param_sfo_ptr, SCE_SEEK_SET);
  
  // read the param.sfo file
  read_sz = sceIoRead(pbp_fd, *param_sfo_buffer,  param_sfo_size);
  if(read_sz < param_sfo_size) {
     free(*param_sfo_buffer);
     *param_sfo_buffer = NULL;
     return 0;
  }
  
  return param_sfo_size;
}

int get_pbp_sfo(const char* pbp_file, void** param_sfo_buffer) {
  PbpHeader pbp_header;
  
  if(param_sfo_buffer == NULL) return NULL;
  *param_sfo_buffer = NULL;
  
  int res = 0;
  
  if(pbp_file != NULL) {
    SceUID pbp_fd = sceIoOpen(pbp_file, SCE_O_RDONLY, 0777);
    if(pbp_fd < 0) return NULL;
    
    // read param.sfo from pbp
    res = read_sfo(pbp_fd, param_sfo_buffer);
    
    sceIoClose(pbp_fd);	
  }
  
  return res;
}

int get_pbp_content_id(const char* pbp_file, char* content_id) {  
  int res = 0;
  
  if(pbp_file != NULL && content_id != NULL) {
    SceUID pbp_fd = sceIoOpen(pbp_file, SCE_O_RDONLY, 0777);
    if(pbp_fd < 0) return 0;
    res = read_data_psar_header(pbp_fd, content_id);
    
    // check the content id is valid
    if(res) {
      int content_id_len = strnlen(content_id, 0x30);
      if(content_id_len != 0x24) res = 0;
    }
    
    sceIoClose(pbp_fd);	
  }
  
  return res;
  
}

int hash_pbp(SceUID pbp_fd, char* out_hash) {
  char wbuf[0x7c0]; 
  
  // seek to the start of the eboot.pbp
  sceIoLseek(pbp_fd, 0x00, SCE_SEEK_SET);
  
  // inital read
  int read_sz = sceIoRead(pbp_fd, wbuf, sizeof(wbuf));
  if(read_sz < sizeof(PbpHeader)) return read_sz;
  
  // calculate data hash size
  size_t hash_sz = (((PbpHeader*)wbuf)->data_psar_ptr + 0x1C0000);

  // initalize hash
  SHA256_CTX ctx;
  sha256_init(&ctx);
  
  // first hash
  sha256_update(&ctx, wbuf, read_sz);
  size_t total_hashed = read_sz;  
  
  do {
    read_sz = sceIoRead(pbp_fd, wbuf, sizeof(wbuf));
    
    if((total_hashed + read_sz) > hash_sz)
      read_sz = (hash_sz - total_hashed); // calculate remaining 
    
    sha256_update(&ctx, wbuf, read_sz);
    total_hashed += read_sz;
    
    if(read_sz < sizeof(wbuf)) // treat EOF as complete
      total_hashed = hash_sz;
    
  } while(total_hashed < hash_sz);
  
  sha256_final(&ctx, out_hash);
  
  return 1;
}

int gen_sce_ebootpbp(const char* pbp_file, const char* psp_game_folder){
  int res = 0;
  
  char pbp_hash[0x20];
  char sce_ebootpbp[0x200];
  int sw_version = 0;
  PbpHeader pbp_header;
  
  static uint8_t sony_sce_discinfo[0x20000];
  int sony_sce_discinfo_sz = read_file("vs0:app/NPXS10028/__sce_discinfo", sony_sce_discinfo, sizeof(sony_sce_discinfo));  
  
  char sce_ebootpbp_path[MAX_PATH];
  char sce_discinfo_path[MAX_PATH];
  
  snprintf(sce_ebootpbp_path, sizeof(sce_ebootpbp_path), "%s/__sce_ebootpbp", psp_game_folder); 
  snprintf(sce_discinfo_path, sizeof(sce_discinfo_path), "%s/__sce_discinfo", psp_game_folder); 

  memset(pbp_hash, 0x00, sizeof(pbp_hash));
  memset(sce_ebootpbp, 0x00, sizeof(pbp_hash));
  PRINT_STR("pbp_file: %s, psp_game_folder: %s\n", pbp_file, psp_game_folder);
  if(pbp_file != NULL) {
    SceUID pbp_fd = sceIoOpen(pbp_file, SCE_O_RDONLY, 0777);
    if(pbp_fd < 0) return pbp_fd;
    
    int pbp_type = determine_pbp_type(pbp_fd, &pbp_header); // determine pbp header
    PRINT_STR("pbp_type: %x\n", pbp_type);
    if(pbp_type == PBP_TYPE_UNKNOWN) return res;
    
    res = hash_pbp(pbp_fd, pbp_hash); // hash eboot.pbp
    
    sceIoClose(pbp_fd);
    
     // actually generate the __sce_ebootpbp or __sce_discinfo 
     if(pbp_type == PBP_TYPE_NPUMDIMG)
       res = _vshNpDrmEbootSigGenPsp(pbp_file, pbp_hash, sce_ebootpbp, &sw_version);
    else if(pbp_type == PBP_TYPE_PSISOIMG)                              
      res = _vshNpDrmEbootSigGenPs1(pbp_file, pbp_hash, sce_ebootpbp, &sw_version);
    else if(pbp_type == PBP_TYPE_PSTITLEIMG)
       res = _vshNpDrmEbootSigGenMultiDisc(pbp_file, sony_sce_discinfo, sce_ebootpbp, &sw_version);

    if(res >= 0) { // write __sce_ebootpbp
      if(pbp_type == PBP_TYPE_NPUMDIMG || pbp_type == PBP_TYPE_PSISOIMG)
        res = write_file(sce_ebootpbp_path, sce_ebootpbp, 0x200);
      else if(pbp_type == PBP_TYPE_PSTITLEIMG)
        res = write_file(sce_discinfo_path, sce_ebootpbp, 0x100);
    }
  }
  
  return res;
}