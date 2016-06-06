#include <noroi/glfw/noroi_glfw_font.h>

#ifdef NOROI_USE_GLFW

#include <noroi/misc/noroi_glyphpacker.h>

// Freetype.
#include <ft2build.h>
#include FT_FREETYPE_H

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

static const uint8_t utf8d[] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
  8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
  0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
  0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
  0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
  1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
  1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
};

static uint32_t inline
decode(uint32_t* state, uint32_t* codep, uint32_t byte) {
  uint32_t type = utf8d[byte];

  *codep = (*state != UTF8_ACCEPT) ?
    (byte & 0x3fu) | (*codep << 6) :
    (0xff >> type) & (byte);

  *state = utf8d[256 + *state*16 + type];
  return *state;
}

FT_Library g_freetypeLibrary;
bool g_initialized = false;
bool NR_Font_Init() {
  // Only initialize once.
  if (!g_initialized) {
    // Intialize freetype.
    if (FT_Init_FreeType(&g_freetypeLibrary) != 0) {
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
  NR_GlyphPacker* glyphpacker;
} HandleType;

NR_Font* NR_Font_Load(const char* path) {
  // Attempt to load the font.
  FT_Face face;
  FT_Error err = FT_New_Face(g_freetypeLibrary, path, 0, &face);
  if (err != 0)
    return (void*)0;

  // Make sure we're using unicode mappings.
  FT_Select_Charmap(face, FT_ENCODING_UNICODE);

  // Allocate some memory for our handle.
  HandleType* hnd = malloc(sizeof(HandleType));
  memset(hnd, 0, sizeof(HandleType));
  hnd->face = face;

  // Create a glyphpacker
  hnd->glyphpacker = NR_GlyphPacker_New(2048, 2048, 10);

  // Set a default size.
  NR_Font_SetSize((void*)hnd, 0, 20);

  return (void*)hnd;
}

void NR_Font_Delete(NR_Font* font) {
  // Delete the font.
  HandleType* hnd = (HandleType*)font;

  // Delete the face.
  FT_Done_Face(hnd->face);

  // Delete our glyphpacker
  NR_GlyphPacker_Delete(hnd->glyphpacker);

  // De-allocate our handle.
  free(hnd);
}

void NR_Font_SetSize(NR_Font* font, int width, int height) {
  HandleType* hnd = (HandleType*)font;
  FT_Set_Pixel_Sizes(hnd->face, width, height);
}

bool NR_Font_Draw(NR_Font* font, const char* string, int width, int height) {
  HandleType* hnd = (HandleType*)font;

  // Decode the string into utf-8 codepoints.
  uint32_t codepoint;
  uint32_t state = 0;
  uint8_t* data = (uint8_t*)string;

  for (; *data; ++data) {
    if (!decode(&state, &codepoint, *data)) {
      // Check if we can find this codepoint.
      NR_GlyphPacker_Glyph glyph;
      glyph.codepoint = codepoint;
      bool found = NR_GlyphPacker_Find(hnd->glyphpacker, codepoint, &glyph);
      if (!found) {
        // We need to generate a glyph using freetype
        // And add it to our glypmap.
        unsigned long c = FT_Get_Char_Index(hnd->face, codepoint);
        FT_Error err = FT_Load_Glyph(hnd->face, c, FT_LOAD_RENDER);
        if (err != 0) return false;

        // Add it to our glyphmap.
        glyph.width = hnd->face->glyph->bitmap.width;
        glyph.height = hnd->face->glyph->bitmap.rows;
        NR_GlyphPacker_Add(hnd->glyphpacker, &glyph);
        found = NR_GlyphPacker_Find(hnd->glyphpacker, codepoint, &glyph);
      }

      // Draw it.
      if (found) {
        printf("%i: (%u, %u, %u, %u)\n", codepoint, glyph.x, glyph.y, glyph.width, glyph.height);
      }
    }
  }

  if (state != UTF8_ACCEPT) {
    printf("%s:%i - Malformed utf-8 string!", __FUNCTION__, __LINE__);
    return false;
  }

  return true;
}

#endif
