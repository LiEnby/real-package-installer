#ifndef RIF_H
#define RIF_H 1

int is_debug_license(const char* rif);
int is_npdrm_free_license(const char* rif);
int make_psp_fake_rif(const char* rif, const char* content_id);

#endif