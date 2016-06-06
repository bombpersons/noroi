#include <noroi/misc/noroi_event_queue.h>

#include <stdlib.h>
#include <string.h>

NR_EventQueue* NR_EventQueue_New() {
  NR_EventQueue* q = malloc(sizeof(NR_EventQueue));
  memset(q, 0, sizeof(NR_EventQueue));

  return q;
}

void NR_EventQueue_Delete(NR_EventQueue* q) {
  // Pop everything off the queue
  NR_Event event;
  while (NR_EventQueue_Pop(q, &event)) {}

  // Delete the main queue.
  free(q);
}

bool NR_EventQueue_Pop(NR_EventQueue* q, NR_Event* event) {
  NR_EventQueue_Node* cur = q->end;
  if (!cur) return false;
  *event = cur->event;

  // Remove from the linked list.
  q->end = cur->prev;
  free(cur);

  // Set our start to null, if our end is null.
  if (!q->end) q->start = (void*)0;

  // Dec our count
  q->count--;

  // Queue wasn't empty
  return true;
}

void NR_EventQueue_Push(NR_EventQueue* q, NR_Event* event) {
  // Allocate a node.
  NR_EventQueue_Node* node = malloc(sizeof(NR_EventQueue_Node));
  memset(node, 0, sizeof(NR_EventQueue_Node));
  node->event = *event;

  // Shift around the linked list.
  if (!q->start && !q->end) { // Nothing in the list.
    q->start = node;
    q->end = node;
  } else {
    q->start->prev = node;
    q->start = node;
  }

  q->count++;
}

unsigned int NR_EventQueue_Count(NR_EventQueue* q) {
  // Keep going until we reach null
  return q->count;
}
