#include <noroi/noroi.h>
#include <stdio.h>

int main(int argc, char** argv) {
  // Try to initialize.
  if (!NR_Init()) {
    printf("Noroi initialization failed!\n");
    return 1;
  }

  // Make a handle.
  NR_Handle hnd = NR_CreateHandle();
  if (hnd) {
    NR_SetSize(hnd, 20, 30);

    // Clear the screen to '#'
    NR_Glyph glyph;
    memset(&glyph, 0, sizeof(glyph));

    glyph.c = '#';
    glyph.flash = true;
    glyph.bold = true;

    //NR_Clear(hnd, &glyph);
    NR_SetGlyph(hnd, 10, 10, &glyph);

    // Update the screen.
    NR_SwapBuffers(hnd);

    sleep(10);

    // Destroy our handle
    NR_DestroyHandle(hnd);
  } else {
    printf("Couldn't create a handle.\n");
  }

  // Unload
  NR_Shutdown();

  return 0;
}
