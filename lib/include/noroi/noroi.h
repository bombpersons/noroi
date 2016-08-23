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
  unsigned int codepoint;

  bool flash;
  bool bold;
  bool italic;

  NR_Color color;
} NR_Glyph;

// Keys.
typedef enum {
	NR_KEY_UNKNOWN		= 0,
	NR_KEY_FIRST		= 0,
	NR_KEY_BACKSPACE		= 8,
	NR_KEY_TAB		= 9,
	NR_KEY_CLEAR		= 12,
	NR_KEY_RETURN		= 13,
	NR_KEY_PAUSE		= 19,
	NR_KEY_ESCAPE		= 27,
	NR_KEY_SPACE		= 32,
	NR_KEY_EXCLAIM		= 33,
	NR_KEY_QUOTEDBL		= 34,
	NR_KEY_HASH		= 35,
	NR_KEY_DOLLAR		= 36,
	NR_KEY_AMPERSAND		= 38,
	NR_KEY_QUOTE		= 39,
	NR_KEY_LEFTPAREN		= 40,
	NR_KEY_RIGHTPAREN		= 41,
	NR_KEY_ASTERISK		= 42,
	NR_KEY_PLUS		= 43,
	NR_KEY_COMMA		= 44,
	NR_KEY_MINUS		= 45,
	NR_KEY_PERIOD		= 46,
	NR_KEY_SLASH		= 47,
	NR_KEY_0			= 48,
	NR_KEY_1			= 49,
	NR_KEY_2			= 50,
	NR_KEY_3			= 51,
	NR_KEY_4			= 52,
	NR_KEY_5			= 53,
	NR_KEY_6			= 54,
	NR_KEY_7			= 55,
	NR_KEY_8			= 56,
	NR_KEY_9			= 57,
	NR_KEY_COLON		= 58,
	NR_KEY_SEMICOLON		= 59,
	NR_KEY_LESS		= 60,
	NR_KEY_EQUALS		= 61,
	NR_KEY_GREATER		= 62,
	NR_KEY_QUESTION		= 63,
	NR_KEY_AT			= 64,
	/*
	   Skip uppercase letters
	 */
	NR_KEY_LEFTBRACKET	= 91,
	NR_KEY_BACKSLASH		= 92,
	NR_KEY_RIGHTBRACKET	= 93,
	NR_KEY_CARET		= 94,
	NR_KEY_UNDERSCORE		= 95,
	NR_KEY_BACKQUOTE		= 96,
	NR_KEY_a			= 97,
	NR_KEY_b			= 98,
	NR_KEY_c			= 99,
	NR_KEY_d			= 100,
	NR_KEY_e			= 101,
	NR_KEY_f			= 102,
	NR_KEY_g			= 103,
	NR_KEY_h			= 104,
	NR_KEY_i			= 105,
	NR_KEY_j			= 106,
	NR_KEY_k			= 107,
	NR_KEY_l			= 108,
	NR_KEY_m			= 109,
	NR_KEY_n			= 110,
	NR_KEY_o			= 111,
	NR_KEY_p			= 112,
	NR_KEY_q			= 113,
	NR_KEY_r			= 114,
	NR_KEY_s			= 115,
	NR_KEY_t			= 116,
	NR_KEY_u			= 117,
	NR_KEY_v			= 118,
	NR_KEY_w			= 119,
	NR_KEY_x			= 120,
	NR_KEY_y			= 121,
	NR_KEY_z			= 122,
	NR_KEY_DELETE		= 127,
	/* End of ASCII mapped keysyms */
        /*@}*/

	/** @name Numeric keypad */
        /*@{*/
	NR_KEY_KP0		= 256,
	NR_KEY_KP1		= 257,
	NR_KEY_KP2		= 258,
	NR_KEY_KP3		= 259,
	NR_KEY_KP4		= 260,
	NR_KEY_KP5		= 261,
	NR_KEY_KP6		= 262,
	NR_KEY_KP7		= 263,
	NR_KEY_KP8		= 264,
	NR_KEY_KP9		= 265,
	NR_KEY_KP_PERIOD		= 266,
	NR_KEY_KP_DIVIDE		= 267,
	NR_KEY_KP_MULTIPLY	= 268,
	NR_KEY_KP_MINUS		= 269,
	NR_KEY_KP_PLUS		= 270,
	NR_KEY_KP_ENTER		= 271,
	NR_KEY_KP_EQUALS		= 272,
        /*@}*/

	/** @name Arrows + Home/End pad */
        /*@{*/
	NR_KEY_UP			= 273,
	NR_KEY_DOWN		= 274,
	NR_KEY_RIGHT		= 275,
	NR_KEY_LEFT		= 276,
	NR_KEY_INSERT		= 277,
	NR_KEY_HOME		= 278,
	NR_KEY_END		= 279,
	NR_KEY_PAGEUP		= 280,
	NR_KEY_PAGEDOWN		= 281,
        /*@}*/

	/** @name Function keys */
        /*@{*/
	NR_KEY_F1			= 282,
	NR_KEY_F2			= 283,
	NR_KEY_F3			= 284,
	NR_KEY_F4			= 285,
	NR_KEY_F5			= 286,
	NR_KEY_F6			= 287,
	NR_KEY_F7			= 288,
	NR_KEY_F8			= 289,
	NR_KEY_F9			= 290,
	NR_KEY_F10		= 291,
	NR_KEY_F11		= 292,
	NR_KEY_F12		= 293,
	NR_KEY_F13		= 294,
	NR_KEY_F14		= 295,
	NR_KEY_F15		= 296,
        /*@}*/

	/** @name Key state modifier keys */
        /*@{*/
	NR_KEY_NUMLOCK		= 300,
	NR_KEY_CAPSLOCK		= 301,
	NR_KEY_SCROLLOCK		= 302,
	NR_KEY_RSHIFT		= 303,
	NR_KEY_LSHIFT		= 304,
	NR_KEY_RCTRL		= 305,
	NR_KEY_LCTRL		= 306,
	NR_KEY_RALT		= 307,
	NR_KEY_LALT		= 308,
	NR_KEY_RMETA		= 309,
	NR_KEY_LMETA		= 310,
	NR_KEY_LSUPER		= 311,		/**< Left "Windows" key */
	NR_KEY_RSUPER		= 312,		/**< Right "Windows" key */
	NR_KEY_MODE		= 313,		/**< "Alt Gr" key */
	NR_KEY_COMPOSE		= 314,		/**< Multi-key compose key */
        /*@}*/

	/** @name Miscellaneous function keys */
        /*@{*/
	NR_KEY_HELP		= 315,
	NR_KEY_PRINT		= 316,
	NR_KEY_SYSREQ		= 317,
	NR_KEY_BREAK		= 318,
	NR_KEY_MENU		= 319,
	NR_KEY_POWER		= 320,		/**< Power Macintosh power key */
	NR_KEY_EURO		= 321,		/**< Some european keyboards */
	NR_KEY_UNDO		= 322,		/**< Atari keyboard has Undo */
        /*@}*/

	/* Add any other keys here */
	NR_KEY_LAST
} NR_Key;

typedef enum {
  NR_KEY_MOD_SHIFT = 1,
  NR_KEY_MOD_CONTROL = 1 << 1,
  NR_KEY_MOD_SUPER = 1 << 2,
  NR_KEY_MOD_ALT = 1 << 3,
} NR_KeyMod;

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

  NR_EVENT_KEY_PRESS,
  NR_EVENT_KEY_RELEASE,

  NR_EVENT_RESIZE,
  NR_EVENT_QUIT
} NR_EventType;

// An event.
typedef struct {
  NR_EventType type;
  union {
    struct { int x, y; } mouseData;
    struct { NR_Button button; NR_KeyMod mod; } buttonData;
    struct { int y; } scrollData;
    struct { NR_Key key; NR_KeyMod mod; } keyData;
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

// Set the font.
bool NR_SetFont(NR_Handle hnd, const char* font);
void NR_SetFontSize(NR_Handle hnd, int width, int height);

// Events
bool NR_PollEvent(NR_Handle hnd, NR_Event* event);

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
