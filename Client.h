#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <WinSock2.h>
#include <Windows.h>
#include "Globals.h"

void HandleClient(HWND /*hMain*/, SOCKET /*clientsock*/, const char* /*addr*/);
void ArtificialServerToClient(LPCLIENT, const char*, WORD);
void ArtificialClientToServer(LPCLIENT, const char*, WORD);

#endif
