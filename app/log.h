#include <vitasdk.h>
#ifndef LOG_H
#define LOG_H 1

#define ENABLE_LOGGING 1

#ifdef ENABLE_LOGGING
#define PRINT_STR(...) sceClibPrintf(__VA_ARGS__)
#define PRINT_BUFFER_LEN(buffer, size) for(int i = 0; i < size; i++) { \
												PRINT_STR("%02X ", (unsigned char)(buffer[i]));	\
										 } \
										 PRINT_STR("\n")
#define PRINT_BUFFER(buffer) PRINT_BUFFER_LEN(buffer, sizeof(buffer))

#else
#define PRINT_STR(...) /**/
#define PRINT_BUFFER(buffer) /**/
#define PRINT_BUFFER_LEN(buffer, size) /**/
#endif

#define TO_HEX(in, insz, out, outsz) \
{ \
	unsigned char * pin = in; \
	const char * hex = "0123456789ABCDEF"; \
	char * pout = out; \
	for(; pin < in+insz; pout +=2, pin++){ \
		pout[0] = hex[(*pin>>4) & 0xF]; \
		pout[1] = hex[ *pin     & 0xF]; \
		if (pout + 2 - out > outsz){ \
			break; \
		} \
	} \
	pout[-1] = 0; \
}

#endif