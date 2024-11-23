#define ERROR(x) { ret = x; goto error; }

#define CHECK_ERROR(x) \
	do { \
		int ret = x;\
		if(ret < 0) { \
			PRINT_STR("%s = 0x%X\n", #x, ret);\
			return ret; \
		} \
	} while(0);
