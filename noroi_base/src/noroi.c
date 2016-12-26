#include <noroi/base/noroi.h>

#include <zmq.h>
#include <stdlib.h>
#include <string.h>

// Create a noroi context.
NR_Context* NR_Context_New() {
  NR_Context* context = malloc(sizeof(NR_Context));
  context->zmqContext = zmq_ctx_new();

  return context;
}

// Destroy a context.
void NR_Context_Delete(NR_Context* context) {
  zmq_ctx_destroy(context->zmqContext);

  free(context);
}
