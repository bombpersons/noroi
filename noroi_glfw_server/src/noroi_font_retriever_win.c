#include <noroi/glfw_server/noroi_font_retriever.h>

#ifdef _WIN32
#include <Windows.h>

HFONT _createFontHandle(const char* fontName) {
  return CreateFont(0, // nHeight
                    0, // nWidth
                    0, // nEscapement
                    0, // nOrientation
                    FW_NORMAL, // fnWeight
                    false, // italic
                    false, // underline
                    false, // strikeout
                    DEFAULT_CHARSET, // charset
                    OUT_DEFAULT_PRECIS, // outputPrecision
                    CLIP_DEFAULT_PRECIS, // clipPrecision
                    DEFAULT_QUALITY, // quality
                    FF_DONTCARE, // pitchAndFamily
                    fontName); // face name.
}

void _deleteFontHandle(HFONT font) {
  DeleteObject(font);
}

unsigned int NR_FontRetrieval_GetFontDataSize(const char* fontName) {
  // Try and get the font handle.
  HFONT font = _createFontHandle(fontName);
  if (!font) return 0;

  // Get the data size.
  HDC dc = CreateCompatibleDC(NULL);
  SelectObject(dc, font);
  DWORD size = GetFontData(dc, 0, 0, NULL, 0);
  DeleteDC(dc);

  // Delete the font handle.
  _deleteFontHandle(font);

  // Return the size of the font data.
  return size;
}

bool NR_FontRetrieval_GetFontData(const char* fontName, char* buff, unsigned int buffSize) {
  bool success = false;

  // Try and get the font handle.
  HFONT font = _createFontHandle(fontName);
  if (!font) return false;

  // Get the data size.
  HDC dc = CreateCompatibleDC(NULL);
  SelectObject(dc, font);
  DWORD size = GetFontData(dc, 0, 0, NULL, 0);
  if (size > 0) {
    if (GetFontData(dc, 0, 0, buff, buffSize) != GDI_ERROR) {
      success = true;
    }
  }
  DeleteDC(dc);

  // Delete the font handle.
  _deleteFontHandle(font);

  // Whether or not we got the data.
  return success;
}

#endif
