#include <noroi/noroi.h>
#include <noroi/misc/noroi_event_queue.h>
#include <noroi/glfw/noroi_glfw_font.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
  // Try to initialize.
  if (!NR_Init()) {
    printf("Noroi initialization failed!\n");
    return 1;
  }

  NR_Glyph glyph;
  memset(&glyph, 0, sizeof(NR_Glyph));

  glyph.c = '#';
  glyph.flash = false;
  glyph.bold = true;

  // Make a handle.
  NR_Handle hnd = NR_CreateHandle();
  if (hnd) {
    // Test font stuff.
    NR_SetFont(hnd, "data/font.ttf");
    NR_SetFontSize(hnd, 40, 0);
    NR_SetSize(hnd, 20, 20);

    //NR_Clear(hnd, &glyph);
    NR_SetGlyph(hnd, 10, 10, &glyph);
    NR_Text(hnd, 20, 20, "Hey, this is text!");

    // Clear the screen to '#'
    //NR_Rectangle(hnd, 0, 0, 5, 5, &glyph);
    //NR_RectangleFill(hnd, 0, 0, 5, 5, &glyph);
    NR_Clear(hnd, &glyph);

    NR_Event event = {};
    bool running = true;
    while (running) {
      while (NR_PollEvent(hnd, &event)) {
        switch (event.type) {
          case NR_EVENT_QUIT:
            running = false;
            break;

          default:
            break;
        }
      }

      // Update the screen.
      NR_Render(hnd);
    }

    // Destroy our handle
    NR_DestroyHandle(hnd);

  } else {
    printf("Couldn't create a handle.\n");
  }

  // Unload
  NR_Shutdown();

  return 0;
}
