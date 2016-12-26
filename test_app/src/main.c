#include <noroi/base/noroi.h>
#include <noroi/base/noroi_event_queue.h>
#include <noroi/glfw_server/noroi_glfw_font.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <noroi/glfw_server/noroi_glfw_server.h>
#include <noroi/client/noroi_client.h>

int main(int argc, char** argv) {
  // Create a noroi context.
  NR_Context* context = NR_Context_New();

  // Create and connect the client to the server.
  NR_Client client = NR_Client_New(context, "inproc://request", "inproc://subscribe");

  // Create the server.
  NR_GLFW_Server server = NR_GLFW_Server_New(context, "inproc://request", "inproc://subscribe");

  // Set the caption and check if we set it correctly.
  NR_Client_SetCaption(client, "My Awesome Caption Text");
  char buff[512];
  unsigned int bytesWritten = 0;
  NR_Client_GetCaption(client, buff, sizeof(buff), &bytesWritten);
  printf("Caption set was: %s\n", buff);

  // Set the font to use.
  NR_Client_SetFont(client, "data/font.ttf");
  NR_Client_SetFontSize(client, 0, 25);

  // A glyph to test with.
  NR_Glyph hashGlyph;
  hashGlyph.codepoint = '#';

  NR_Glyph zeroGlyph;
  zeroGlyph.codepoint = 'O';

  // Start a loop checking any events coming from the server.
  bool running = true;
  while (running) {
    NR_Event event;
    while (NR_Client_HandleEvent(client, &event)) {
      switch (event.type) {
        case NR_EVENT_RESIZE:
          //NR_Client_Clear(client, &hashGlyph);
          NR_Client_Rectangle(client, 0, 0, 10, 10, &hashGlyph);
          NR_Client_Text(client, 1, 1, "Hey there!");
          NR_Client_SwapBuffers(client);
          break;

        case NR_EVENT_MOUSE_MOVE:
          NR_Client_SetGlyph(client, event.data.mouseData.x, event.data.mouseData.y, &zeroGlyph);
          NR_Client_SwapBuffers(client);

          {
            NR_Glyph test;
            if (NR_Client_GetGlyph(client, event.data.mouseData.x, event.data.mouseData.y, &test)) {
              printf("%c\n", test.codepoint);
            }
          }

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

  // Destroy the context.
  NR_Context_Delete(context);

  return 0;
}
