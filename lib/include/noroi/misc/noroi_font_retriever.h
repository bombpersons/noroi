#ifndef NOROI_FONT_RETRIEVER
#define NOROI_FONT_RETRIEVER

#include <stdbool.h>

unsigned int NR_FontRetrieval_GetFontDataSize(const char* fontName);
bool NR_FontRetrieval_GetFontData(const char* fontName, char* buff, unsigned int buffSize);

#endif
