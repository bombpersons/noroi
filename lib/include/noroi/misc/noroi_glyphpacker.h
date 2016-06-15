#ifndef NOROI_GLYPHPACKER_INCLUDED
#define NOROI_GLYPHPACKER_INCLUDED

#include <stdbool.h>

// Glyph type.
typedef struct {
  unsigned int codepoint;
  unsigned int x;
  unsigned int y;
  unsigned int width;
  unsigned int height;
  unsigned int page;

  int bearingX;
  int bearingY;
  unsigned int advance;
} NR_GlyphPacker_Glyph;

// Handle
typedef void* NR_GlyphPacker;

NR_GlyphPacker* NR_GlyphPacker_New(int width, int height, int maxPage);
void NR_GlyphPacker_Delete(NR_GlyphPacker* packer);

bool NR_GlyphPacker_Add(NR_GlyphPacker* packer, const NR_GlyphPacker_Glyph* glyph);
bool NR_GlyphPacker_Find(NR_GlyphPacker* packer, unsigned int codepoint, NR_GlyphPacker_Glyph* glyph);

#endif
