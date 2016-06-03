#ifndef NOROI_EVENT_QUEUE
#define NOROI_EVENT_QUEUE

#include <noroi/noroi.h>

// An item in the queue.
typedef struct NR_EventQueue_Node_s {
  NR_Event event;
  struct NR_EventQueue_Node_s* prev;
} NR_EventQueue_Node;

// Represents the queue itself.
typedef struct {
  NR_EventQueue_Node* start;
  NR_EventQueue_Node* end;
} NR_EventQueue;

NR_EventQueue* NR_EventQueue_New();
void NR_EventQueue_Delete(NR_EventQueue* q);
bool NR_EventQueue_Pop(NR_EventQueue* q, NR_Event* event);
void NR_EventQueue_Push(NR_EventQueue* q, NR_Event* event);

#endif
