#include <noroi/noroi.h>

#include <string.h>

// Draw a square
void NR_RectangleFill(NR_Handle hnd, int x, int y, int w, int h, const NR_Glyph* glyph) {
  for (int i = x; i < x+w; i++) {
    for (int j = y; j < y+h; j++) {
      NR_SetGlyph(hnd, i, j, glyph);
    }
  }
}

void NR_Rectangle(NR_Handle hnd, int x, int y, int w, int h, const NR_Glyph* glyph) {
  for (int i = x; i <= x+w; i++) {
    NR_SetGlyph(hnd, i, y, glyph);
    NR_SetGlyph(hnd, i, y+h, glyph);
  }
  for (int i = y; i <= y+h; i++) {
    NR_SetGlyph(hnd, x, i, glyph);
    NR_SetGlyph(hnd, x+w, i, glyph);
  }
}

// Draw text
void NR_Text(NR_Handle hnd, int x, int y, const char* text) {
  for (const char* c = text; *c; c++) {
    NR_Glyph g;
    memset(&g, 0, sizeof(NR_Glyph));
    g.codepoint = (unsigned int)*c;

    NR_SetGlyph(hnd, x + (c - text), y, &g);
  }
}

// Clear everything.
void NR_Clear(NR_Handle hnd, const NR_Glyph* glyph) {
  int w, h;
  NR_GetSize(hnd, &w, &h);
  NR_RectangleFill(hnd, 0, 0, w, h, glyph);
}
