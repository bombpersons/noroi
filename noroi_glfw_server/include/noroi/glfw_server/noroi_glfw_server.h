#ifndef NOROI_GLFW_SERVER_INCLUDED
#define NOROI_GLFW_SERVER_INCLUDED

#include <noroi/base/noroi.h>
#include <noroi/base/noroi_server_base.h>

NR_Server_Base NR_GLFW_Server_New(NR_Context* context, const char* requestBind, const char* subscribeBind);
void NR_GLFW_Server_Delete(NR_Server_Base server);

#endif
