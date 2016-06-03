#include <noroi/glfw/noroi_glfw_font.h>

#ifdef NOROI_USE_GLFW

// Freetype.
#include <ft2build.h>
#include FT_FREETYPE_H

FT_Library g_freetypeLibrary;
bool g_initialized = false;
bool NR_Font_Init() {
  // Only initialize once.
  if (!g_initialized) {
    // Intialize freetype.
    if (!FT_Init_FreeType(&g_freetypeLibrary)) {
      return false;
    }

    g_initialized = true;
  }

  return true;
}

void NR_Font_Shutdown() {
  if (g_initialized) {
    // De-initialize freetype.
    FT_Done_FreeType(g_freetypeLibrary);

    g_initialized = false;
  }
}

// Internal representation of NR_Font
typedef struct {
  FT_Face face;
} HandleType;

NR_Font* NR_Font_Load(const char* path) {
  // Attempt to load the font.
  FT_Face face;
  if (!FT_New_Face(g_freetypeLibrary, path, 0, &face))
    return (void*)0;

  // Allocate some memory for our handle.
  HandleType* hnd = malloc(sizeof(HandleType));
  memset(hnd, 0, sizeof(HandleType));
  hnd->face = face;

  return (void*)hnd;
}

void NR_Font_Delete(NR_Font* font) {
  // Delete the font.
  HandleType* hnd = (HandleType*)font;

  // Delete the face.
  FT_Done_Face(hnd->face);

  // De-allocate our handle.
  free(hnd);
}

void NR_Font_SetSize(NR_Font* font, int width, int height) {
  HandleType* hnd = (HandleType*)font;
  FT_Set_Pixel_Sizes(hnd->face, width, height);
}

void NR_Font_Draw(NR_Font* font, NR_Glyph* glyphs, int width, int height) {

}

#endif
