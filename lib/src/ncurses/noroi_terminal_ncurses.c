#include <noroi/noroi.h>

#ifdef NOROI_TERMINAL_NCURSES

#include <ncurses.h>

// Initialize noroi
bool NR_Init() { return true; }
void NR_Shutdown() {}

// Create / destroy a handle.
static NR_Handle g_soleHandle = 0;
NR_Handle NR_CreateHandle() {
  // We can only have one handle when using ncurses!
  if (!g_soleHandle) {
    // Initialize ncurses.
    initscr();
    raw();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);

    // Return our handle.
    g_soleHandle = (void*)1;
    return g_soleHandle;
  }

  return (void*)0;
}
void NR_DestroyHandle(NR_Handle hnd) {
  if (g_soleHandle == hnd && g_soleHandle) {
    // Uninitialize.
    endwin();
    g_soleHandle = (void*)0;
  }
}

// Events
bool NR_PollEvent(NR_Handle hnd, NR_Event* event) {
  // Make sure getch() doesn't block.
  timeout(0);
  char e = getch();
  if (e == ERR)
    return false;

  event->type = NR_EVENT_KEY_PRESS;
  event->data.keyData.key = NR_KEY_UP;
  printf("%c\n", e);
  return true;
}

// Set / get the size of the window.
void NR_SetSize(NR_Handle hnd, int width, int height) {
  // Can't actually do this with ncurses..
}
void NR_GetSize(NR_Handle hnd, int* width, int* height) {
  getmaxyx(stdscr, (*height), (*width));
}

// Set / get the caption of the window.
// Difficult to do this reliably, since it's different for different terminal emulators.
// For now, this isn't implemented.
void NR_SetCaption(NR_Handle hnd, const char* caption) {}
int NR_GetCaptionSize(NR_Handle hnd) { return 0; }
void NR_GetCaption(NR_Handle hnd, char* buf) {}

// Set/get a glyph
void NR_SetGlyph(NR_Handle hnd, int x, int y, const NR_Glyph* glyph) {
  move(y, x);
  if (glyph->italic) attron(A_ITALIC);
  if (glyph->bold) attron(A_BOLD);
  if (glyph->flash) attron(A_BLINK);
  printw("%c", glyph->c);

  attroff(A_ITALIC);
  attroff(A_BOLD);
  attroff(A_BLINK);
}

NR_Glyph NR_GetGlyph(NR_Handle hnd, int x, int y) {
  NR_Glyph glyph = {};
  chtype ch = mvinch(y, x);
  char c = ch & A_CHARTEXT;
  int color = ch & A_COLOR;
  int attr = ch & A_ATTRIBUTES;

  glyph.c = c;
  glyph.color.id = color;
  glyph.italic = attr & A_ITALIC;
  glyph.bold = attr & A_BOLD;
  glyph.flash = attr & A_BLINK;
  return glyph;
}

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
  move(y, x);
  printw(text);
}

// Clear everything.
void NR_Clear(NR_Handle hnd, const NR_Glyph* glyph) {
  int w, h;
  NR_GetSize(hnd, &w, &h);
  NR_RectangleFill(hnd, 0, 0, w, h, glyph);
}

// Apply any changes.
void NR_SwapBuffers(NR_Handle hnd) {
  if (g_soleHandle) {
    refresh();
  }
}

#endif
