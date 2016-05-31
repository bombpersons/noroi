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
NR_Event* PollEvent(NR_Handle hnd) { return (void*)0; }

// Set / get the size of the window.
void NR_SetSize(NR_Handle hnd, int width, int height) {
  resizeterm(height, width);
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
  move(x, y);
  if (glyph->italic) attron(A_ITALIC); else attroff(A_ITALIC);
  if (glyph->bold) attron(A_BOLD); else attroff(A_BOLD);
  if (glyph->flash) attron(A_BLINK); else attroff(A_BLINK);
  printw("%c", glyph->c);
}

NR_Glyph NR_GetGlyph(NR_Handle hnd, int x, int y) { NR_Glyph glyph = {}; return glyph; }

// Clear everything.
void NR_Clear(NR_Handle hnd, const NR_Glyph* glyph) {
}

// Apply any changes.
void NR_SwapBuffers(NR_Handle hnd) {
  if (g_soleHandle) {
    refresh();
  }
}

#endif
