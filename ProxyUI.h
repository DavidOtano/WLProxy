#ifndef __PROXYUI_H__
#define __PROXYUI_H__

#include <WinSock2.h>
#include <Windows.h>
#include "Globals.h"

int InitProxyUI(HINSTANCE /*hInstance*/);
void ui_InsertClient(LPCLIENT /*pClient*/);
void ui_RemoveClient(LPCLIENT /*pClient*/);
void ui_InsertPlugin(char* /*lpszName*/, char* /*lpszDescription*/);
void ui_PopBalloonTip(const char* lpszTip, 
	const char* lpszTitle);

#endif
