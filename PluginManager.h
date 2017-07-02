#ifndef __PLUGINMANAGER_H__
#define __PLUGINMANAGER_H__

#include "Globals.h"

void Plugins_Load(void);
void Plugins_OnConnect(LPCLIENT /*pClient*/);
void Plugins_OnCharacterInfo(LPCLIENT /*pClient*/);
void Plugins_OnSendToClient(LPCLIENT /*pClient*/, 
	char* /*lpszBuffer*/, WORD /*wLen*/, bool* /*pbExclude*/);
void Plugins_OnSendToServer(LPCLIENT /*pClient*/, 
	char* /*lpszBuffer*/, WORD /*wLen*/, bool* /*pbExclude*/);
void Plugins_OnClientReadyState(LPCLIENT /*pClient*/);
void Plugins_OnDisconnect(LPCLIENT /*pClient*/);
void Plugins_OnFree(void);

#endif
