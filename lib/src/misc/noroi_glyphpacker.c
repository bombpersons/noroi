#include <noroi/misc/noroi_glyphpacker.h>

#include <stdlib.h>
#include <string.h>

#define GLYPH_BUCKET_SIZE (256)

// Copepoint hash function ( found at https://github.com/akrinke/Font-Stash/blob/master/fontstash.c )
static unsigned int _hashCodePoint(unsigned int a) {
	a += ~(a<<15);
	a ^=  (a>>10);
	a +=  (a<<3);
	a ^=  (a>>6);
	a += ~(a<<11);
	a ^=  (a>>16);
	return a;
}

// Linked list.
typedef struct GlyphInfoLinked_s {
  NR_GlyphInfo info;
  struct GlyphInfoLinked_s* next;
} GlyphInfo;

// Handle type.
typedef struct {
  GlyphInfo* bucket[GLYPH_BUCKET_SIZE];
  unsigned int width, height, maxPage;
} HandleType;

NR_GlyphPacker* NR_GlyphPacker_New(int width, int height, int maxPage) {
  HandleType* hnd = malloc(sizeof(HandleType));
  if (!hnd)
    return (void*)0;

  memset(hnd, 0, sizeof(HandleType));

  hnd->width = width;
  hnd->height = height;
  hnd->maxPage = maxPage;
  return (void*)hnd;
}

void NR_GlyphPacker_Delete(NR_GlyphPacker* packer) {
  HandleType* hnd = (HandleType*)packer;

  // Free the linked lists in the bucket.
  for (int i = 0; i < GLYPH_BUCKET_SIZE; ++i) {
    GlyphInfo* info = hnd->bucket[i];
    while (info) {
      GlyphInfo* next = info->next;
      free(info);
      info = next;
    }
  }

  // Free the handle data.
  free(packer);
}

bool NR_GlyphPacker_AddGlyph(NR_GlyphPacker* packer, unsigned int codepoint, const NR_GlyphInfo* glyph) {
  HandleType* hnd = (HandleType*)packer;

  // Make sure we don't already have this glyph.
  NR_GlyphInfo found;
  if (!NR_GlyphPacker_GetGlyph(packer, codepoint, &found)) {
    // Not found, so let's add it.
    // Figure out where to place it...
    

    // Add to the hash map.
    unsigned int hash = _hashCodePoint(codepoint) % GLYPH_BUCKET_SIZE;

    GlyphInfo* info = malloc(sizeof(GlyphInfo));
    memset(info, 0, sizeof(GlyphInfo));
    info->info = *glyph;
    info->info.codepoint = codepoint;
    info->next = hnd->bucket[hash];
    hnd->bucket[hash] = info;
  }

  return false;
}

void NR_GlyphPacker_DeleteGlyph(NR_GlyphPacker* packer, unsigned int codepoint);

bool NR_GlyphPacker_GetGlyph(NR_GlyphPacker* packer, unsigned int codepoint, NR_GlyphInfo* glyph) {
  HandleType* hnd = (HandleType*)packer;

  // Get the hash for this codepoint, and the link in the bucket.
  unsigned int hash = _hashCodePoint(codepoint) % GLYPH_BUCKET_SIZE;
  GlyphInfo* link = hnd->bucket[hash];

  // Search through the linked list for this particular codepoint.
  while (true) {
    // Got to the end without finding anything.
    if (!link) return false;

    // Found it!
    if (link->info.codepoint == codepoint) {
      *glyph = link->info;
      return true;
    }

    // Try the next item.
    link = link->next;
  }
}
