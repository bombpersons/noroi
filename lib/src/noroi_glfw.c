#include <noroi/noroi.h>

#ifdef NOROI_USE_GLFW

// Initialize noroi
bool NR_Init() { return false; }
void NR_Shutdown() {}

// Create / destroy a handle.
NR_Handle NR_CreateHandle() { return 0; }
void NR_DestroyHandle(NR_Handle hnd) {}

// Events
NR_Event* PollEvent(NR_Handle hnd) { return (void*)0; }

// Set / get the size of the window.
void NR_SetSize(NR_Handle hnd, int width, int height) {}
void NR_GetSize(NR_Handle hnd, int* width, int* height) {}

// Set / get the caption of the window.
void NR_SetCaption(NR_Handle hnd, const char* caption) {}
int NR_GetCaptionSize(NR_Handle hnd) { return 0; }
void NR_GetCaption(NR_Handle hnd, char* buf) {}

// Set/get a glyph
void NR_SetGlyph(NR_Handle hnd, int x, int y, const NR_Glyph* glyph) {}
NR_Glyph NR_GetGlyph(NR_Handle hnd, int x, int y) { NR_Glyph glyph = {}; return glyph; }

// Draw a square
void NR_RectangleFill(NR_Handle hnd, int x, int y, int w, int h, const NR_Glyph* glyph) {}
void NR_Rectangle(NR_Handle hnd, int x, int y, int w, int h, const NR_Glyph* glyph) {}

// Draw text
void NR_Text(NR_Handle hnd, int x, int y, const char* text) {}

// Clear everything.
void NR_Clear(NR_Handle hnd, const NR_Glyph* glyph) {}

// Apply any changes.
void NR_SwapBuffers(NR_Handle hnd) {}

#endif
