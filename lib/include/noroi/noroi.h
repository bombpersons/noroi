#ifndef NOROI_NOROI_INCLUDED
#define NOROI_NOROI_INCLUDED

#include <stdbool.h>
#include <stdint.h>

// Handle to a screen.
typedef void* NR_Handle;

// Color of a glyph, from 0 to 15.
typedef struct {
  uint8_t id;
} NR_Color;

// Info for a glyph.
typedef struct {
  char c;

  bool flash;
  bool bold;
  bool italic;

  NR_Color color;
} NR_Glyph;

// Keys.
typedef enum {
  NR_KEY_UP,
  NR_KEY_DOWN,
  NR_KEY_LEFT,
  NR_KEY_RIGHT
} NR_Key;

// Events
typedef enum {
  NR_EVENT_MOUSE_PRESS,
  NR_EVENT_MOUSE_RELEASE,
  NR_EVENT_KEY_PRESS,
  NR_EVENT_KEY_RELEASE,

  NR_EVENT_WIN_RESIZE
} NR_EventType;

// An event.
typedef struct {
  NR_EventType type;
  union {
    struct { int x, y; } mouseData;
    struct { NR_Key key; char c; } keyData;
    struct { int w, h; } resizeData;
  } data;
} NR_Event;

// Functions

// Initialize noroi
bool NR_Init();
void NR_Shutdown();

// Create / destroy a handle.
NR_Handle NR_CreateHandle();
void NR_DestroyHandle(NR_Handle hnd);

// Events
NR_Event* PollEvent(NR_Handle hnd);

// Set / get the size of the window.
void NR_SetSize(NR_Handle hnd, int width, int height);
void NR_GetSize(NR_Handle hnd, int* width, int* height);

// Set / get the caption of the window.
void NR_SetCaption(NR_Handle hnd, const char* caption);
int NR_GetCaptionSize(NR_Handle hnd);
void NR_GetCaption(NR_Handle hnd, char* buf);

// Set/get a glyph
void NR_SetGlyph(NR_Handle hnd, int x, int y, const NR_Glyph* glyph);
NR_Glyph NR_GetGlyph(NR_Handle hnd, int x, int y);

// Draw a square
void NR_RectangleFill(NR_Handle hnd, int x, int y, int w, int h, const NR_Glyph* glyph);
void NR_Rectangle(NR_Handle hnd, int x, int y, int w, int h, const NR_Glyph* glyph);

// Draw text
void NR_Text(NR_Handle hnd, int x, int y, const char* text);

// Clear everything.
void NR_Clear(NR_Handle hnd, const NR_Glyph* glyph);

// Apply any changes.
void NR_SwapBuffers(NR_Handle hnd);

#endif
