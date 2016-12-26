#ifndef NOROI_CLIENT_INCLUDED
#define NOROI_CLIENT_INCLUDED

#include <stdbool.h>
#include <noroi/base/noroi.h>

typedef void* NR_Client;

// Creating a client.
NR_Client NR_Client_New(NR_Context* context, const char* requestAddress, const char* subscriberAddress);
void NR_Client_Delete(NR_Client client);

// Manually send events and messages (Mostly for internal use)
bool NR_Client_Send(NR_Client client, NR_Request_Type type, const void* contents, unsigned int size,
                                                            NR_Response_Header* received, unsigned int recievedSize);

// Set the font.
bool NR_Client_SetFont(NR_Client hnd, const char* font);
void NR_Client_SetFontSize(NR_Client client, int width, int height);

// Events
bool NR_Client_HandleEvent(NR_Client client, NR_Event* event);

// Set / get the size of the window.
void NR_Client_SetSize(NR_Client client, int width, int height);
void NR_Client_GetSize(NR_Client client, int* width, int* height);

// Set / get the caption of the window.
void NR_Client_SetCaption(NR_Client client, const char* caption);
void NR_Client_GetCaption(NR_Client client, char* buf, unsigned int size, unsigned int* bytesWritten);

// Set/get a glyph
void NR_Client_SetGlyph(NR_Client client, int x, int y, const NR_Glyph* glyph);
bool NR_Client_GetGlyph(NR_Client client, int x, int y, NR_Glyph* glyph);

// Draw a square
void NR_Client_RectangleFill(NR_Client client, int x, int y, int w, int h, const NR_Glyph* glyph);
void NR_Client_Rectangle(NR_Client client, int x, int y, int w, int h, const NR_Glyph* glyph);

// Draw text
void NR_Client_Text(NR_Client client, int x, int y, const char* text);

// Clear everything.
void NR_Client_Clear(NR_Client client, const NR_Glyph* glyph);

// Apply any changes.
void NR_Client_SwapBuffers(NR_Client client);

#endif
