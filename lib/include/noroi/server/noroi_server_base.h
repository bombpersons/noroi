#ifndef NOROI_SERVER_BASE_INCLUDED
#define NOROI_SERVER_BASE_INCLUDED

#include <noroi/noroi.h>

typedef void* NR_Server_Base;
typedef bool(*NR_Server_Base_Initializer)(void*);
typedef void(*NR_Server_Base_Updater)(NR_Server_Base, void*);
typedef void(*NR_Server_Base_RequestHandler)(NR_Server_Base, void*, void*, unsigned int);

NR_Server_Base NR_Server_Base_New(const char* requestBind, const char* subscribeBind,
                                  void* userData,
                                  NR_Server_Base_Initializer initializer,
                                  NR_Server_Base_Updater updater,
                                  NR_Server_Base_RequestHandler requestHandler);
void NR_Server_Base_Delete(NR_Server_Base server);

void NR_Server_Base_Reply(NR_Server_Base server, void* data, unsigned int size);
void NR_Server_Base_Event(NR_Server_Base server, NR_Event* event);

#endif
