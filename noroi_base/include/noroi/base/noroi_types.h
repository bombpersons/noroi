#ifndef NOROI_TYPES_INCLUDED
#define NOROI_TYPES_INCLUDED

#include <stdbool.h>
#include <stdint.h>

// Info for a glyph.
typedef struct {
  unsigned int codepoint;
  
  bool flashing;
  bool bold;
  bool italic;

  unsigned int color;
  unsigned int bgColor;
} NR_Glyph;

typedef enum {
  NR_BUTTON_LEFT,
  NR_BUTTON_MIDDLE,
  NR_BUTTON_RIGHT
} NR_Button;

// Events
typedef enum {
  NR_EVENT_MOUSE_PRESS,
  NR_EVENT_MOUSE_RELEASE,
  NR_EVENT_MOUSE_MOVE,
  NR_EVENT_MOUSE_SCROLL,

  NR_EVENT_CHARACTER,

  NR_EVENT_RESIZE,
  NR_EVENT_QUIT
} NR_EventType;

// An event.
typedef struct {
  NR_EventType type;
  union {
    struct { int x, y; } mouseData;
    struct { NR_Button button; } buttonData;
    struct { int y; } scrollData;
    struct { unsigned int codepoint; } charData;
    struct { int w, h; } resizeData;
  } data;
} NR_Event;

// Header for a request message.
typedef enum {
  NR_Request_Type_SetSize,
  NR_Request_Type_GetSize,
  NR_Request_Type_SetFont,
  NR_Request_Type_GetFont,
  NR_Request_Type_SetFontSize,
  NR_Request_Type_GetFontSize,
  NR_Request_Type_SetCaption,
  NR_Request_Type_GetCaption,
  NR_Request_Type_SetGlyph,
  NR_Request_Type_GetGlyph,

  NR_Request_Type_Rectangle,
  NR_Request_Type_Text,

  NR_Request_Type_Clear,
  NR_Request_Type_SwapBuffers
} NR_Request_Type;

// http://stackoverflow.com/questions/2060974/how-to-include-a-dynamic-array-inside-a-struct-in-c
typedef struct {
  NR_Request_Type type;
  unsigned int size;
  char contents[];
} NR_Request_Header;

// Packet contents
typedef struct {
  unsigned int width, height;
} NR_Request_SetSize_Contents;

typedef char* NR_Request_SetFont_Contents;

typedef struct {
  unsigned int width, height;
} NR_Request_SetFontSize_Contents;

typedef char* NR_Request_SetCaption_Contents;

typedef struct {
  unsigned int x, y;
  NR_Glyph glyph;
} NR_Request_SetGlyph_Contents;

typedef struct {
  unsigned int x, y;
} NR_Request_GetGlyph_Contents;

typedef struct {
  unsigned int x, y;
  unsigned int color;
  unsigned int bgColor;
  bool flash;
  char text[];
} NR_Request_Text_Contents;

typedef struct {
  unsigned int x, y;
  unsigned int w, h;
  NR_Glyph glyph;
  bool fill;
} NR_Request_Rectangle_Contents;

typedef struct {
  NR_Glyph glyph;
} NR_Request_Clear_Contents;

// Header for a response message.
typedef enum {
  NR_Response_Type_Success,
  NR_Response_Type_Failure,

  NR_Response_Type_GetSize,
  NR_Response_Type_GetCaption,
  NR_Response_Type_GetGlyph

} NR_Response_Type;

typedef struct {
  NR_Response_Type type;
  unsigned int size;
  char contents[];
} NR_Response_Header;

// Packet contents
typedef char* NR_Response_Failure_Contents;

typedef struct {
  unsigned int width, height;
} NR_Response_GetSize_Contents;

typedef char* NR_Response_GetCaption_Contents;

typedef struct {
  NR_Glyph glyph;
} NR_Response_GetGlyph_Contents;

#endif
