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

  // Callbacks
  NR_Server_Base_Initializer initializer;
  NR_Server_Base_Updater updater;
  NR_Server_Base_RequestHandler requestHandler;

  // User data.
  void* userData;

} InternalData;

static int _runServer(void* data) {
  // Get the data.
  InternalData* internal = (InternalData*)data;

  // Initialization before we actually start.
  if (!internal->initializer(internal->userData)) {
    // We failed to initialize somehow, fail gracefully.
    printf("[Error] Server failed to initialize!\n");
    internal->running = false;
    return 0;
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

      internal->requestHandler(data, internal->userData, buffer, bytes);
    }

    // Update our window.
    internal->updater(data, internal->userData);
  }

  return 0;
}

NR_Server_Base NR_Server_Base_New(const char* replyAddress, const char* publisherAddress,
                                  void* userData,
                                  NR_Server_Base_Initializer initializer,
                                  NR_Server_Base_Updater updater,
                                  NR_Server_Base_RequestHandler requestHandler) {
  // Make a new handle.
  InternalData* internal = malloc(sizeof(InternalData));

  // Set default variables.
  internal->running = true;
  internal->replyAddress = malloc(strlen(replyAddress) + 1);
  strcpy(internal->replyAddress, replyAddress);
  internal->publisherAddress = malloc(strlen(publisherAddress) + 1);
  strcpy(internal->publisherAddress, publisherAddress);

  // Set callbacks
  internal->requestHandler = requestHandler;
  internal->updater = updater;
  internal->initializer = initializer;

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

void NR_Server_Base_Reply(NR_Server_Base server, void* data, unsigned int size) {
  InternalData* internal = (InternalData*)server;
  zmq_send(internal->responder, data, size, 0);
}

void NR_Server_Base_Event(NR_Server_Base server, NR_Event* event) {
  InternalData* internal = (InternalData*)server;
  zmq_send(internal->publisher, event, sizeof(NR_Event), 0);
}
