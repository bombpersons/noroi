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

  unsigned int bearingX;
  unsigned int bearingY;
  unsigned int advance;
} NR_GlyphInfo;

// Handle
typedef void* NR_GlyphPacker;

NR_GlyphPacker* NR_GlyphPacker_New(int width, int height, int maxPage);
void NR_GlyphPacker_Delete(NR_GlyphPacker* packer);

bool NR_GlyphPacker_AddGlyph(NR_GlyphPacker* packer, unsigned int codepoint, const NR_GlyphInfo* glyph);
void NR_GlyphPacker_DeleteGlyph(NR_GlyphPacker* packer, unsigned int codepoint);

bool NR_GlyphPacker_GetGlyph(NR_GlyphPacker* packer, unsigned int codepoint, NR_GlyphInfo* glyph);

#endif
