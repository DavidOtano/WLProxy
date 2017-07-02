#ifndef __PROXYSERVER_H__
#define __PROXYSERVER_H__
#include <WinSock2.h>
#include <Windows.h>

bool InitProxyServer(HWND /*hMain*/);
void KillProxyServer(void);

#endif
