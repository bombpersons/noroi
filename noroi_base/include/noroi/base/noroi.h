#ifndef NOROI_NOROI_INCLUDED
#define NOROI_NOROI_INCLUDED

#include <noroi/base/noroi_types.h>

typedef struct {
  void* zmqContext;
} NR_Context;

// Create a noroi context.
NR_Context* NR_Context_New();

// Destroy a context.
void NR_Context_Delete(NR_Context* context);

#endif
