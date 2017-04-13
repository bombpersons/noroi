#include <noroi/client/noroi_client.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <zmq.h>

typedef struct {
  // Context.
  void* context;

  // Connection to the server.
  void* requestSocket; // For sending commands.
  void* subscriberSocket; // For recieving input / changes.
} InternalData;

NR_Client NR_Client_New(NR_Context* context, const char* requestAddress, const char* subscribeAddress) {
  // Allocate internal memory
  InternalData* internal = malloc(sizeof(InternalData));
  internal->context = context->zmqContext;

  // The request socket, this is used to ask for information from the server.
  internal->requestSocket = zmq_socket(internal->context, ZMQ_REQ);
  int rc = zmq_connect(internal->requestSocket, requestAddress);
  assert(rc == 0);

  // The subscribe socket, this is used for the server to send us stuff
  // we didn't ask for, like keyboard input, window resizing, etc.
  internal->subscriberSocket = zmq_socket(internal->context, ZMQ_SUB);
  rc = zmq_connect(internal->subscriberSocket, subscribeAddress);
  assert(rc == 0);

  // Subscribe to all events.
  zmq_setsockopt(internal->subscriberSocket, ZMQ_SUBSCRIBE, (void*)0, 0);

  return (void*)internal;
}

void NR_Client_Delete(NR_Client client) {
  InternalData* internal = (InternalData*)client;

  // Close sockets.
  zmq_close(internal->requestSocket);
  zmq_close(internal->subscriberSocket);

  free(internal);
}

bool NR_Client_Send(NR_Client client, NR_Request_Type type, const void* contents, unsigned int contentSize,
                                                            NR_Response_Header* received, unsigned int receivedSize) {
  InternalData* internal = (InternalData*)client;

  // Create the header + packet.
  NR_Request_Header* header = malloc(sizeof(NR_Request_Header) + contentSize);
  header->type = type;
  header->size = contentSize;
  memcpy(header->contents, contents, contentSize);

  // Send the request to the server.
  //printf("[Client] Sending a request of type %i\n", header->type);
  zmq_send(internal->requestSocket, header, sizeof(NR_Request_Header) + header->size, 0);

  // Delete our header + packet.
  free(header);
  header = (void*)0;

  // If a received buffer isn't specified use our own.
  char tempBuffer[1024];
  unsigned int bufferSize = sizeof(tempBuffer);
  void* buffer = tempBuffer;

  if (received) {
    buffer = received;
    bufferSize = receivedSize;
  }

  // *fingers crossed* the server has some sort of response for us.
  int bytes = zmq_recv(internal->requestSocket, buffer, bufferSize, 0);
  if (bytes != 0) {

    // Read the response header and decide what to do.
    NR_Response_Header* header = (NR_Response_Header*)buffer;
    if (header->type == NR_Response_Type_Failure) {
      printf("[Client] Server failed to fulfill request. %s\n", header->contents);
      return false;
    }

  } else {
    printf("[Client] Did not recieve a response!\n");
  }

  return true;
}

// Set the font.
bool NR_Client_SetFont(NR_Client client, const char* font) {
  return NR_Client_Send(client, NR_Request_Type_SetFont, font, strlen(font) + 1, (void*)0, 0);
}

void NR_Client_SetFontSize(NR_Client client, int width, int height) {
  NR_Request_SetFontSize_Contents contents;
  contents.width = width;
  contents.height = height;
  NR_Client_Send(client, NR_Request_Type_SetFontSize, &contents, sizeof(contents), (void*)0, 0);
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
  // Store the result.
  char buffer[1024];
  NR_Response_Header* header = (NR_Response_Header*)buffer;

  // Get the data from the server.
  NR_Client_Send(client, NR_Request_Type_GetSize, (void*)0, 0, header, sizeof(buffer));

  // Unpack the results.
  NR_Response_GetSize_Contents* contents = (NR_Response_GetSize_Contents*)header->contents;
  *width = contents->width;
  *height = contents->height;
}

// Set / get the caption of the window.
void NR_Client_SetCaption(NR_Client client, const char* caption) {
  NR_Client_Send(client, NR_Request_Type_SetCaption, caption, strlen(caption) + 1, (void*)0, 0);
}

void NR_Client_GetCaption(NR_Client client, char* buf, unsigned int size, unsigned int* bytesWritten) {
  // Store the response.
  char buffer[1024];
  NR_Response_Header* header = (NR_Response_Header*)buffer;

  // Ask for a response.
  NR_Client_Send(client, NR_Request_Type_GetCaption, (void*)0, 0, header, sizeof(buffer));

  // Unpack the results.
  *bytesWritten = header->size < size ? header->size : size;
  memcpy(buf, header->contents, *bytesWritten);
}

// Set/get a glyph
void NR_Client_SetGlyph(NR_Client client, int x, int y, const NR_Glyph* glyph) {
  NR_Request_SetGlyph_Contents contents;
  contents.x = x;
  contents.y = y;
  contents.glyph = *glyph;
  NR_Client_Send(client, NR_Request_Type_SetGlyph, &contents, sizeof(contents), (void*)0, 0);
}

bool NR_Client_GetGlyph(NR_Client client, int x, int y, NR_Glyph* glyph) {
  // The position of the glyph to request.
  NR_Request_GetGlyph_Contents contents;
  contents.x = x;
  contents.y = y;

  // A buffer the correct size to contain the response.
  char buffer[sizeof(NR_Response_Header) + sizeof(NR_Response_GetGlyph_Contents)];

  // The header and contents structures.
  NR_Response_Header* header = (NR_Response_Header*)buffer;
  NR_Response_GetGlyph_Contents* response = (NR_Response_GetGlyph_Contents*)header->contents;

  // Actually request the value.
  if (NR_Client_Send(client, NR_Request_Type_GetGlyph, &contents, sizeof(contents), header, sizeof(buffer))) {
    *glyph = response->glyph;
    return true;
  }
  return false;
}

// Draw a square
void NR_Client_RectangleFill(NR_Client client, int x, int y, int w, int h, const NR_Glyph* glyph) {
  // Create the request.
  NR_Request_Rectangle_Contents contents;
  contents.x = x;
  contents.y = y;
  contents.w = w;
  contents.h = h;
  contents.fill = true;
  contents.glyph = *glyph;

  // Send it.
  NR_Client_Send(client, NR_Request_Type_Rectangle, &contents, sizeof(contents), (void*)0, 0);
}

void NR_Client_Rectangle(NR_Client client, int x, int y, int w, int h, const NR_Glyph* glyph) {
  // Create the request.
  NR_Request_Rectangle_Contents contents;
  contents.x = x;
  contents.y = y;
  contents.w = w;
  contents.h = h;
  contents.fill = false;
  contents.glyph = *glyph;

  // Send it.
  NR_Client_Send(client, NR_Request_Type_Rectangle, &contents, sizeof(contents), (void*)0, 0);
}

// Draw text
void NR_Client_Text(NR_Client client, int x, int y, const char* text, unsigned int color, unsigned int bgColor, bool flash) {
  // Create the request.
  int contentsSize = sizeof(NR_Request_Text_Contents) + strlen(text) + 1;
  NR_Request_Text_Contents* contents = (NR_Request_Text_Contents*)malloc(contentsSize);
  contents->x = x;
  contents->y = y;
  contents->color = color;
  contents->bgColor = bgColor;
  contents->flash = flash;
  strcpy(contents->text, text);

  // Send it.
  NR_Client_Send(client, NR_Request_Type_Text, contents, contentsSize, (void*)0, 0);

  // Free the contents we allocated.
  free(contents);
}

// Clear everything.
void NR_Client_Clear(NR_Client client, const NR_Glyph* glyph) {
  NR_Request_Clear_Contents contents;
  contents.glyph = *glyph;

  NR_Client_Send(client, NR_Request_Type_Clear, &contents, sizeof(contents), (void*)0, 0);
}

// Apply any changes.
void NR_Client_SwapBuffers(NR_Client client) {
  NR_Client_Send(client, NR_Request_Type_SwapBuffers, (void*)0, 0, (void*)0, 0);
}
