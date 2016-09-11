#ifndef NOROI_GLFW_SERVER_INCLUDED
#define NOROI_GLFW_SERVER_INCLUDED

typedef void* NR_GLFW_Server;

NR_GLFW_Server NR_GLFW_Server_New(const char* requestBind, const char* subscribeBind);
void NR_GLFW_Server_Delete(NR_GLFW_Server server);

#endif
