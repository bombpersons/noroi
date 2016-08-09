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

// Set the font resolution. (The resolution of a character on the underlying texture page.)
void NR_Font_SetResolution(NR_Font* font, int width, int height);

// Set the size of a character when drawn (pixels). Leave height or width to 0 to be automatically set based on the aspect ratio.
void NR_Font_SetSize(NR_Font* font, int width, int height);
void NR_Font_GetSize(NR_Font* font, int* width, int* height);

// Draw a grid of characters
bool NR_Font_Draw(NR_Font* font, unsigned int* data, int dataWidth, int dataHeight, int width, int height);

#endif
