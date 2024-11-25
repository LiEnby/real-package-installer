#ifndef PROMOTE_H

int need_promote(int content_type);
const char* find_promote_location(int content_type);
int promote(const char *path, void (*progress_callback)(const char*, uint64_t, uint64_t));
int is_app_installed(const char* title_id);

#endif