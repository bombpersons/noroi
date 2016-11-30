#include <noroi/noroi.h>
#include <noroi/misc/noroi_event_queue.h>
#include <noroi/glfw/noroi_glfw_font.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <noroi/server/noroi_glfw_server.h>
#include <noroi/client/noroi_client.h>

int main(int argc, char** argv) {
  NR_GLFW_Server server = NR_GLFW_Server_New("tcp://*:12345", "tcp://*:12346");
  NR_Client client = NR_Client_New("tcp://localhost:12345", "tcp://localhost:12346");

  NR_Client_SetCaption(client, "My Awesome Caption Text");
  char buff[512];
  unsigned int bytesWritten = 0;
  NR_Client_GetCaption(client, buff, sizeof(buff), &bytesWritten);
  printf("Caption set was: %s\n", buff);

  NR_Client_SetFont(client, "data/font.ttf");
  NR_Client_SetFontSize(client, 0, 25);

  NR_Glyph hashGlyph;
  hashGlyph.codepoint = '#';

  bool running = true;
  while (running) {
    NR_Event event;
    while (NR_Client_HandleEvent(client, &event)) {
      switch (event.type) {
        case NR_EVENT_RESIZE:
          NR_Client_Clear(client, &hashGlyph);
          NR_Client_SwapBuffers(client);
          break;

        case NR_EVENT_QUIT:
          running = false;
          break;

        default:
          break;
      }
      printf("Event type: %i\n", event.type);
    }
  }

  // Shutdown the client
  NR_Client_Delete(client);

  // Shutdown the server.
  NR_GLFW_Server_Delete(server);

  return 0;

  // // Try to initialize.
  // if (!NR_Init()) {
  //   printf("Noroi initialization failed!\n");
  //   return 1;
  // }
  //
  // NR_Glyph glyph;
  // memset(&glyph, 0, sizeof(NR_Glyph));
  //
  // glyph.codepoint = '#';
  // glyph.flash = false;
  // glyph.bold = true;
  //
  // // Make a handle.
  // NR_Handle hnd = NR_CreateHandle();
  // if (hnd) {
  //   // Test font stuff.
  //   if (!NR_SetFont(hnd, "data/font.ttf")) {
  //     printf("Couldn't load font!\n");
  //   }
  //
  //   // Set the font size.
  //   NR_SetFontSize(hnd, 40, 0);
  //
  //   // Set the size of the window (in characters)
  //   NR_SetSize(hnd, 20, 10);
  //
  //   // Draw some stuff.
  //   NR_Clear(hnd, &glyph);
  //   NR_Text(hnd, 2, 2, "Hey, this is text!");
  //   NR_SwapBuffers(hnd);
  //
  //   // Handle any events.
  //   NR_Event event = {};
  //   bool running = true;
  //   while (running) {
  //     while (NR_PollEvent(hnd, &event)) {
  //       switch (event.type) {
  //         case NR_EVENT_QUIT:
  //           running = false;
  //           break;
  //
  //         case NR_EVENT_RESIZE:
  //           // NR_Clear(hnd, &glyph);
  //           // NR_Text(hnd, 2, 2, "Hey, this is text!");
  //           //
  //           // NR_SwapBuffers(hnd);
  //           break;
  //
  //         case NR_EVENT_MOUSE_PRESS:
  //           break;
  //
  //         default:
  //           break;
  //       }
  //     }
  //   }
  //
  //   // Destroy our handle
  //   NR_DestroyHandle(hnd);
  //
  // } else {
  //   printf("Couldn't create a handle.\n");
  // }
  //
  // // Unload
  // NR_Shutdown();
  //
  // return 0;
}
