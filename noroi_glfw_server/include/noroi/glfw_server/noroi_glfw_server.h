#ifndef NOROI_GLFW_SERVER_INCLUDED
#define NOROI_GLFW_SERVER_INCLUDED

#include <noroi/base/noroi.h>

typedef void* NR_GLFW_Server;

NR_GLFW_Server NR_GLFW_Server_New(NR_Context* context, const char* requestBind, const char* subscribeBind);
void NR_GLFW_Server_Delete(NR_GLFW_Server server);

#endif
