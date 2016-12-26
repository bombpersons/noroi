#ifndef NOROI_SERVER_BASE_INCLUDED
#define NOROI_SERVER_BASE_INCLUDED

#include <noroi/base/noroi.h>

typedef void* NR_Server_Base;

typedef bool(*NR_Server_Base_Initializer)(void*);
typedef void(*NR_Server_Base_Updater)(NR_Server_Base, void*);
typedef void(*NR_Server_Base_RequestHandler)(NR_Server_Base, void*, void*, unsigned int);

typedef bool(*NR_Server_Base_SetSize)(void*, unsigned int x, unsigned int y);
typedef bool(*NR_Server_Base_GetSize)(void*, unsigned int* x, unsigned int* y);
typedef bool(*NR_Server_Base_SetFont)(void*, const char* fontName);
typedef bool(*NR_Server_Base_GetFont)(void*, char* fontName, unsigned int size);
typedef bool(*NR_Server_Base_SetFontSize)(void*, unsigned int width, unsigned int height);
typedef bool(*NR_Server_Base_GetFontSize)(void*, unsigned int* width, unsigned int* height);

typedef bool(*NR_Server_Base_SetCaption)(void*, const char* caption, unsigned int size);
typedef bool(*NR_Server_Base_GetCaption)(void*, char* buf, unsigned int size, unsigned int* bytesWritten);

typedef bool(*NR_Server_Base_SetGlyph)(void*, unsigned int, unsigned int, const NR_Glyph*);
typedef bool(*NR_Server_Base_GetGlyph)(void*, unsigned int, unsigned int, NR_Glyph*);

typedef bool(*NR_Server_Base_Text)(void*, unsigned int x, unsigned int y, const char* text);
typedef bool(*NR_Server_Base_Rectangle)(void*, unsigned int x, unsigned int y, unsigned int w, unsigned int h, bool fill, const NR_Glyph* glyph);

typedef bool(*NR_Server_Base_Clear)(void*, const NR_Glyph*);
typedef bool(*NR_Server_Base_SwapBuffers)(void*);

typedef struct {
  NR_Server_Base_Initializer initialize;
  NR_Server_Base_Updater update;
  NR_Server_Base_RequestHandler handleRequest;

  // Called when appropriate request is made.
  NR_Server_Base_SetSize setSize;
  NR_Server_Base_GetSize getSize;
  NR_Server_Base_SetFont setFont;
  NR_Server_Base_GetFont getFont;
  NR_Server_Base_SetFontSize setFontSize;
  NR_Server_Base_GetFontSize getFontSize;
  NR_Server_Base_SetCaption setCaption;
  NR_Server_Base_GetCaption getCaption;
  NR_Server_Base_SetGlyph setGlyph;
  NR_Server_Base_GetGlyph getGlyph;

  NR_Server_Base_Text text;
  NR_Server_Base_Rectangle rectangle;

  NR_Server_Base_Clear clear;
  NR_Server_Base_SwapBuffers swapBuffers;

} NR_Server_Base_Callbacks;

NR_Server_Base NR_Server_Base_New(NR_Context* context, const char* requestBind, const char* subscribeBind,
                                  void* userData, NR_Server_Base_Callbacks);
void NR_Server_Base_Delete(NR_Server_Base server);

void NR_Server_Base_Reply(NR_Server_Base server, NR_Response_Type type, const void* data, unsigned int size);
void NR_Server_Base_Event(NR_Server_Base server, NR_Event* event);

#endif
