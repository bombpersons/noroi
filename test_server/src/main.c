#include <noroi/base/noroi.h>
#include <noroi/glfw_server/noroi_glfw_server.h>

#include <signal.h>
#include <unistd.h>

#include <stdio.h>

static volatile int keepRunning = 1;
void intHandler(int dummy) {
    keepRunning = 0;
}

int main(int argc, char** argv) {
  // Quit if we don't have an address to bind to.
  if (argc < 3) {
    printf("Usage: test_client tcp://respond_address:123 tcp://publish_address:123\n");
  }

  // Catch ctrl+c
  signal(SIGINT, intHandler);

  // Create a noroi context.
  NR_Context* context = NR_Context_New();

  // Create the server.
  NR_Server_Base server = NR_GLFW_Server_New(context, argv[1], argv[2]);

  // Keep running whilst we wait for a quit signal.
  while (keepRunning && NR_Server_Base_Running(server)) {
    // Give other applications a chance to do something whilst we wait.
    sleep(1);
  }

  // Shutdown the server.
  NR_GLFW_Server_Delete(server);

  // Destroy the context.
  NR_Context_Delete(context);

  return 0;
}
