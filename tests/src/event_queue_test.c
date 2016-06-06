#include <unity.h>
#include <noroi/misc/noroi_event_queue.h>

#define START_QUEUE_TEST \
  NR_EventQueue* q = NR_EventQueue_New();

#define END_QUEUE_TEST \
  NR_EventQueue_Delete(q);

void test_add_remove() {
  START_QUEUE_TEST

  const int toAdd = 200;
  for (int i = 0; i < toAdd; ++i) {
    NR_Event event;
    NR_EventQueue_Push(q, &event);
  }

  TEST_ASSERT_EQUAL_MESSAGE(NR_EventQueue_Count(q), toAdd, "Couldn't add to the event queue!");

  for (int i = 0; i < toAdd; ++i) {
    NR_Event event;
    NR_EventQueue_Pop(q, &event);
  }

  TEST_ASSERT_EQUAL_MESSAGE(NR_EventQueue_Count(q), 0, "Couldn't remove from the event queue!");

  END_QUEUE_TEST
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_add_remove);
  return UNITY_END();
}
