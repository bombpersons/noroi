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
  NR_GlyphPacker_Glyph info;
  struct GlyphInfoLinked_s* next;
} GlyphInfo;

// Uses the algorithm described here
// http://www.blackpawn.com/texts/lightmaps/default.html
// to pack rectangles into pages.
typedef struct RectTreeNode_s {
	struct RectTreeNode_s* child[2];
	int x, y, width, height;
	unsigned int codepoint;
	bool occupied;
} RectTreeNode;

RectTreeNode* RectTreeNode_New(unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
	RectTreeNode* node = malloc(sizeof(RectTreeNode));
	memset(node, 0, sizeof(RectTreeNode));
	node->x = x;
	node->y = y;
	node->width = width;
	node->height = height;
	node->occupied = false;

	return node;
}

void RectTreeNode_Delete(RectTreeNode* node) {
	if (!node) return;

	RectTreeNode_Delete(node->child[0]);
	RectTreeNode_Delete(node->child[1]);
	free(node);
}

bool RectTreeNode_Insert(RectTreeNode* parent, unsigned int codepoint, unsigned int width, unsigned int height, unsigned int* x, unsigned int* y) {
	// Not a leaf node.
	if (parent->child[0] && parent->child[1]) {
		// Try inserting into the first child.
		bool inserted = RectTreeNode_Insert(parent->child[0], codepoint, width, height, x, y);
		if (inserted) return true;

		// Try the second (there was no room in the first).
		return RectTreeNode_Insert(parent->child[1], codepoint, width, height, x, y);
	} else {
		// Something is already here!
		if (parent->occupied) return (void*)0;

		// Check we are too big, then quit.
		if (width > parent->width || height > parent->height)
			return (void*)0;

		// If we fit exactly
		if (width == parent->width && height == parent->height) {
			parent->codepoint = codepoint;
			parent->occupied = true;

			*x = parent->x;
			*y = parent->y;
			return true;
		}

		// Split the node.
		unsigned int diffX = parent->width - width;
		unsigned int diffY = parent->height - height;
		if (diffX > diffY) {
			parent->child[0] = RectTreeNode_New(parent->x, parent->y, width, parent->height);
			parent->child[1] = RectTreeNode_New(parent->x + width, parent->y, parent->width - width, parent->height);
		} else {
			parent->child[0] = RectTreeNode_New(parent->x, parent->y, parent->width, height);
			parent->child[1] = RectTreeNode_New(parent->x, parent->y + height, parent->width, parent->height - height);
		}

		// Insert into the first node we created (it should fit perfectly.)
		return RectTreeNode_Insert(parent->child[0], codepoint, width, height, x, y);
	}
}

// Handle type.
typedef struct {
  GlyphInfo* bucket[GLYPH_BUCKET_SIZE];

	RectTreeNode** pages;
	unsigned int maxPage;

  unsigned int width, height;
} HandleType;

NR_GlyphPacker* NR_GlyphPacker_New(int width, int height, int maxPage) {
	// Allocate a handle.
	HandleType* hnd = malloc(sizeof(HandleType));
  if (!hnd)
    return (void*)0;
  memset(hnd, 0, sizeof(HandleType));

	// Set page width, and the maximum page count.
  hnd->width = width;
  hnd->height = height;
  hnd->maxPage = maxPage;

	// Allocate trees for each page.
	hnd->pages = malloc(sizeof(RectTreeNode*) * maxPage);
	if (!hnd->pages)
		return (void*)0;
	for (int i = 0; i < maxPage; ++i) {
		hnd->pages[i] = RectTreeNode_New(0, 0, width, height);
	}

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

	// Free pages.
	for (int i = 0; i < hnd->maxPage; ++i) {
		RectTreeNode_Delete(hnd->pages[i]);
	}
	free(hnd->pages);

  // Free the handle data.
  free(packer);
}

bool NR_GlyphPacker_Add(NR_GlyphPacker* packer, const NR_GlyphPacker_Glyph* glyph) {
  HandleType* hnd = (HandleType*)packer;

  // Make sure we don't already have this glyph.
  NR_GlyphPacker_Glyph found;
  if (!NR_GlyphPacker_Find(packer, glyph->codepoint, &found)) {
    // Not found, so let's add it.
    // Figure out where to place it...

		// Try each page.
		unsigned int x, y;
		unsigned int page = 0;
		bool inserted = false;
		for (int i = 0; i < hnd->maxPage; ++i) {
			// +1 to width and height so that there is a 1 pixel border between glyphs.
			if (RectTreeNode_Insert(hnd->pages[i], glyph->codepoint, glyph->width+1, glyph->height+1, &x, &y)) {
				inserted = true;
				page = i;
				break;
			}
		}

		// If we couldn't find any pages to insert into.. we're full!
		if (!inserted) return false;

    // Add to the hash map.
    unsigned int hash = _hashCodePoint(glyph->codepoint) % GLYPH_BUCKET_SIZE;

    GlyphInfo* info = malloc(sizeof(GlyphInfo));
    memset(info, 0, sizeof(GlyphInfo));
		info->info = *glyph;
		info->info.x = x;
		info->info.y = y;
		info->info.page = page;
    info->next = hnd->bucket[hash];
    hnd->bucket[hash] = info;
  }

  return true;
}

bool NR_GlyphPacker_Find(NR_GlyphPacker* packer, unsigned int codepoint, NR_GlyphPacker_Glyph* glyph) {
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
