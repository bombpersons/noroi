#include <noroi/client/noroi_client.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zmq.h>

typedef struct {
  // Context.
  void* context;

  // Connection to the server.
  void* requestSocket; // For sending commands.
  void* subscriberSocket; // For recieving input / changes.
} InternalData;

NR_Client NR_Client_New(const char* requestAddress, const char* subscribeAddress) {
  // Allocate internal memory
  InternalData* internal = malloc(sizeof(InternalData));
  internal->context = zmq_ctx_new();

  // The request socket, this is used to ask for information from the server.
  internal->requestSocket = zmq_socket(internal->context, ZMQ_REQ);
  zmq_connect(internal->requestSocket, requestAddress);

  // The subscribe socket, this is used for the server to send us stuff
  // we didn't ask for, like keyboard input, window resizing, etc.
  internal->subscriberSocket = zmq_socket(internal->context, ZMQ_SUB);
  zmq_connect(internal->subscriberSocket, subscribeAddress);

  // Subscribe to all events.
  zmq_setsockopt(internal->subscriberSocket, ZMQ_SUBSCRIBE, (void*)0, 0);

  return (void*)internal;
}

void NR_Client_Delete(NR_Client client) {
  InternalData* internal = (InternalData*)client;

  // Close sockets.
  zmq_close(internal->requestSocket);
  zmq_close(internal->subscriberSocket);
  zmq_ctx_destroy(internal->context);

  free(internal);
}

bool NR_Client_Send(NR_Client client, NR_Request_Type type, const void* contents, unsigned int size) {
  InternalData* internal = (InternalData*)client;

  // Create the header + packet.
  NR_Request_Header* header = malloc(sizeof(NR_Request_Header) + size);
  header->type = type;
  header->size = size;
  memcpy(header->contents, contents, size);

  // Send the request to the server.
  printf("[Client] Sending a request of type %i\n", header->type);
  char buffer[1024];
  zmq_send(internal->requestSocket, header, sizeof(NR_Request_Header) + header->size, 0);

  // Delete our header + packet.
  free(header);
  header = (void*)0;

  // *fingers crossed* the server has some sort of response for us.
  int bytes = zmq_recv(internal->requestSocket, buffer, sizeof(buffer), 0);
  if (bytes != 0) {

    // Read the response header and decide what to do.
    NR_Response_Header* header = (NR_Response_Header*)buffer;
    switch (header->type) {
      // Server failed!
      case NR_Response_Type_Failure:
        printf("[Client] Server failed to fulfill request. %s\n", header->contents);
        return false;
      default:
        break;
    }

    printf("[Client] Recieved response with type id: %i\n", header->type);

  } else {
    printf("[Client] Did not recieve a response!\n");
  }

  return true;
}

// Set the font.
bool NR_Client_SetFont(NR_Client client, const char* font) {
  return NR_Client_Send(client, NR_Request_Type_SetFont, font, strlen(font) + 1);
}

void NR_Client_SetFontSize(NR_Client client, int width, int height) {
  NR_Request_SetFontSize_Contents contents;
  contents.width = width;
  contents.height = height;
  NR_Client_Send(client, NR_Request_Type_SetFontSize, &contents, sizeof(contents));
}

// Events
bool NR_Client_HandleEvent(NR_Client client, NR_Event* event) {
  InternalData* internal = (InternalData*)client;

  // Check if there are any events to get.
  int size = zmq_recv(internal->subscriberSocket, event, sizeof(NR_Event), ZMQ_DONTWAIT);
  if (size == -1) {
    // No events for us to handle.
    return false;
  }

  if (size != sizeof(NR_Event)) {
    printf("Recieved an event from server, but it was the wrong size!\n");
    return false;
  }

  // Got a valid event.
  return true;
}

// Set / get the size of the window.
void NR_Client_SetSize(NR_Client client, int width, int height) {

}

void NR_Client_GetSize(NR_Client client, int* width, int* height) {

}

// Set / get the caption of the window.
void NR_Client_SetCaption(NR_Client client, const char* caption) {

}

int NR_Client_GetCaptionSize(NR_Client client) {
  return 0;
}

void NR_Client_GetCaption(NR_Client client, char* buf) {

}

// Set/get a glyph
void NR_Client_SetGlyph(NR_Client client, int x, int y, const NR_Glyph* glyph) {

}

NR_Glyph NR_Client_GetGlyph(NR_Client client, int x, int y) {
  NR_Glyph glyph;
  return glyph;
}

// Draw a square
void NR_Client_RectangleFill(NR_Client client, int x, int y, int w, int h, const NR_Glyph* glyph) {}
void NR_Client_Rectangle(NR_Client client, int x, int y, int w, int h, const NR_Glyph* glyph) {}

// Draw text
void NR_Client_Text(NR_Client client, int x, int y, const char* text) {}

// Clear everything.
void NR_Client_Clear(NR_Client client, const NR_Glyph* glyph) {}

// Apply any changes.
void NR_Client_SwapBuffers(NR_Client client) {}
