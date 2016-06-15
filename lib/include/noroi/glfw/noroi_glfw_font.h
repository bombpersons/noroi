#ifndef NOROI_GLFW_FONT_INCLUDED
#define NOROI_GLFW_FONT_INCLUDED

#include <noroi/noroi.h>

// Font definition.
typedef void* NR_Font;

// Init / shutdown font stuff.
bool NR_Font_Init();
void NR_Font_Shutdown();

// Load / Destroy fonts.
NR_Font* NR_Font_Load(const char* path);
void NR_Font_Delete(NR_Font* font);

// Set font size.
void NR_Font_SetSize(NR_Font* font, int width, int height);

// Draw a grid of characters
bool NR_Font_Draw(NR_Font* font, unsigned int* data, int width, int height);

#endif
