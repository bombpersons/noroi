#include <noroi/server/noroi_server_base.h>
#include <noroi/noroi.h>

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <tinycthread.h>
#include <zmq.h>

typedef struct {
  // Thread we are running on.
  thrd_t threadId;

  // Whether or not we are running.
  bool running;

  // The address to bind to.
  char* replyAddress;
  char* publisherAddress;

  // Sockets
  void* responder;
  void* publisher;

  // Server config.
  NR_Server_Base_Callbacks callbacks;

  // User data.
  void* userData;

} InternalData;

static void _successOrError(NR_Server_Base server, bool success, const char* errorMsg) {
  if (success) {
    NR_Server_Base_Reply(server, NR_Response_Type_Success, (void*)0, 0);
  } else {
    NR_Server_Base_Reply(server, NR_Response_Type_Failure, (void*)errorMsg, strlen(errorMsg));
  }
}

#define NOROI_SERVER_BASE_HANDLE_REQUEST_BEGIN(requestType, callback) \
  case requestType: \
    { \
      if (!callback) \
        break; \

#define NOROI_SERVER_BASE_HANDLE_REQUEST_END \
        break; \
      } \

static void _handleRequest(NR_Server_Base server, void* userData, void* data, unsigned int size) {
  NR_Request_Header* requestHeader = (NR_Request_Header*)data;
  InternalData* internalData = (InternalData*)server;

  // Figure out what the request was.
  switch (requestHeader->type) {
    // Font face.
    NOROI_SERVER_BASE_HANDLE_REQUEST_BEGIN(NR_Request_Type_SetFont, internalData->callbacks.setFont) {
      _successOrError(server, internalData->callbacks.setFont(userData, requestHeader->contents), "Error occurred calling SetFont.");
    } NOROI_SERVER_BASE_HANDLE_REQUEST_END;

    // Font size.
    NOROI_SERVER_BASE_HANDLE_REQUEST_BEGIN(NR_Request_Type_SetFontSize, internalData->callbacks.setFontSize) {
      NR_Request_SetFontSize_Contents* contents = (NR_Request_SetFontSize_Contents*)requestHeader->contents;
      _successOrError(server, internalData->callbacks.setFontSize(userData, contents->width, contents->height), "Error occurred calling SetFontSize.");
    } NOROI_SERVER_BASE_HANDLE_REQUEST_END;

    // Size of window.
    NOROI_SERVER_BASE_HANDLE_REQUEST_BEGIN(NR_Request_Type_GetSize, internalData->callbacks.getSize) {
      NR_Response_GetSize_Contents contents;
      internalData->callbacks.getSize(userData, &contents.width, &contents.height);
      NR_Server_Base_Reply(server, NR_Response_Type_GetSize, &contents, sizeof(contents));
    } NOROI_SERVER_BASE_HANDLE_REQUEST_END;

    NOROI_SERVER_BASE_HANDLE_REQUEST_BEGIN(NR_Request_Type_SetSize, internalData->callbacks.setSize) {
      NR_Request_SetSize_Contents* contents = (NR_Request_SetSize_Contents*)requestHeader->contents;
      _successOrError(server, internalData->callbacks.setSize(userData, contents->width, contents->height), "Error occurred calling SetSize.");
    } NOROI_SERVER_BASE_HANDLE_REQUEST_END;

    // The caption of the window.
    NOROI_SERVER_BASE_HANDLE_REQUEST_BEGIN(NR_Request_Type_SetCaption, internalData->callbacks.setCaption) {
      _successOrError(server, internalData->callbacks.setCaption(userData, requestHeader->contents, requestHeader->size), "Error setting caption!");
    } NOROI_SERVER_BASE_HANDLE_REQUEST_END;

    NOROI_SERVER_BASE_HANDLE_REQUEST_BEGIN(NR_Request_Type_GetCaption, internalData->callbacks.getCaption) {
      char buff[512];
      unsigned int bytesWritten = 0;
      if (internalData->callbacks.getCaption(userData, buff, sizeof(buff), &bytesWritten)) {
        NR_Server_Base_Reply(server, NR_Response_Type_GetCaption, buff, bytesWritten);
      } else {
        const char* error = "Error getting caption!";
        NR_Server_Base_Reply(server, NR_Response_Type_Failure, error, strlen(error));
      }
    } NOROI_SERVER_BASE_HANDLE_REQUEST_END;

    // Glyph.
    NOROI_SERVER_BASE_HANDLE_REQUEST_BEGIN(NR_Request_Type_SetGlyph, internalData->callbacks.setGlyph) {
      NR_Request_SetGlyph_Contents* contents = (NR_Request_SetGlyph_Contents*)requestHeader->contents;
      _successOrError(server, internalData->callbacks.setGlyph(userData, contents->x, contents->y, &contents->glyph), "Error occurred calling SetGlyph.");
    } NOROI_SERVER_BASE_HANDLE_REQUEST_END;

    // Clear.
    NOROI_SERVER_BASE_HANDLE_REQUEST_BEGIN(NR_Request_Type_Clear, internalData->callbacks.clear) {
      NR_Request_Clear_Contents* contents = (NR_Request_Clear_Contents*)requestHeader->contents;
      _successOrError(server, internalData->callbacks.clear(userData, &contents->glyph), "Error occurred calling Clear.");
    } NOROI_SERVER_BASE_HANDLE_REQUEST_END;

    // Swap buffers.
    NOROI_SERVER_BASE_HANDLE_REQUEST_BEGIN(NR_Request_Type_SwapBuffers, internalData->callbacks.swapBuffers) {
      _successOrError(server, internalData->callbacks.swapBuffers(userData), "Error occurred calling SwapBuffers.");
    } NOROI_SERVER_BASE_HANDLE_REQUEST_END;

    default:
      {
        // Construct an error message.
        char errorBuff[1024];
        sprintf(errorBuff, "Server has no handler for request type %i\n", requestHeader->type);
        NR_Server_Base_Reply(server, NR_Response_Type_Failure, errorBuff, strlen(errorBuff));
        break;
      }
  }
}

static int _runServer(void* data) {
  // Get the data.
  InternalData* internal = (InternalData*)data;

  // Initialization before we actually start.
  if (internal->callbacks.initialize) {
    if (!internal->callbacks.initialize(internal->userData)) {
      // We failed to initialize somehow, fail gracefully.
      printf("[Error] Server failed to initialize!\n");
      internal->running = false;
      return 0;
    }
  }

  // Create a zmq context.
  void* context = zmq_ctx_new();

  // Create a socket to reply to requests.
  internal->responder = zmq_socket(context, ZMQ_REP);
  int rc = zmq_bind(internal->responder, internal->replyAddress);
  assert(rc == 0);

  // Create a socket to send events.
  internal->publisher = zmq_socket(context, ZMQ_PUB);
  rc = zmq_bind(internal->publisher, internal->publisherAddress);
  assert(rc == 0);

  // Start running.
  while (internal->running) {

    // Handle any requests.
    char buffer[1024];
    while (true) {
      int bytes = zmq_recv(internal->responder, buffer, 1024, ZMQ_DONTWAIT);
      if (bytes == -1)
        break;

      _handleRequest(data, internal->userData, buffer, bytes);
    }

    // Update our window.
    if (internal->callbacks.update)
      internal->callbacks.update(data, internal->userData);
  }

  return 0;
}

NR_Server_Base NR_Server_Base_New(const char* replyAddress, const char* publisherAddress,
                                  void* userData, NR_Server_Base_Callbacks callbacks) {
  // Make a new handle.
  InternalData* internal = malloc(sizeof(InternalData));

  // Set default variables.
  internal->running = true;
  internal->replyAddress = malloc(strlen(replyAddress) + 1);
  strcpy(internal->replyAddress, replyAddress);
  internal->publisherAddress = malloc(strlen(publisherAddress) + 1);
  strcpy(internal->publisherAddress, publisherAddress);

  // Set callbacks
  internal->callbacks = callbacks;

  // userData
  internal->userData = userData;

  // Start a thread.
  thrd_create(&internal->threadId, _runServer, (void*)internal);

  // Return our handle.
  return (void*)internal;
}

void NR_Server_Base_Delete(NR_Server_Base server) {
  InternalData* internal = (InternalData*)server;

  // Stop the thread
  internal->running = false;
  thrd_join(internal->threadId, NULL);

  // Free anything we allocated.
  free(internal->replyAddress);
  free(internal->publisherAddress);

  // Finaly free the whole handle.
  free(internal);
}

void NR_Server_Base_Reply(NR_Server_Base server, NR_Response_Type type, const void* data, unsigned int size) {
  InternalData* internal = (InternalData*)server;

  // Allocate a header.
  NR_Response_Header* header = malloc(sizeof(NR_Response_Header) + size);
  header->type = type;
  header->size = size;
  memcpy(header->contents, data, size);

  // Send it.
  zmq_send(internal->responder, header, sizeof(NR_Response_Header) + size, 0);

  // Delete the header.
  free(header);
}

void NR_Server_Base_Event(NR_Server_Base server, NR_Event* event) {
  InternalData* internal = (InternalData*)server;
  zmq_send(internal->publisher, event, sizeof(NR_Event), 0);
}
