#include <noroi/misc/noroi_font_retriever.h>

#ifdef __linux__

unsigned int NR_FontRetrieval_GetFontDataSize(const char* fontName) {
  return 0;
}

bool NR_FontRetrieval_GetFontData(const char* fontName, char* buff, unsigned int buffSize) {
  return false;
}

#endif
