#include <WinSock2.h>
#include <Windows.h>
#include <cstdio>
#include "ProxyServer.h"
#include "ThreadLock.h"
#include "Loopback.h"
#include "Client.h"
#include "Log.h"

#pragma warning(disable:4996)

static HWND			g_hMain			= NULL	;
static ThreadLock	g_ProxyLock				;
static HANDLE		g_hProxyThread	= NULL	;
static bool			g_bKeepAlive			;
static int			g_nLoopback				;

DWORD WINAPI ProxyThread(LPVOID);

bool InitProxyServer(HWND hMain) {
	WSADATA		wsaData;
	SOCKET		proxysock;
	SOCKADDR_IN sin;
	u_long		nArg = 1;
	g_hMain = hMain;

	LogPrintLine(0x007F00, "Initializing Proxy Server...\0");

	if(WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
		LogPrintError("Winsock initialization failed.\0");
		return false;
	}

	proxysock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(proxysock == SOCKET_ERROR) {
		LogPrintError("Socket creation failed.\0");
		return false;
	}

	sin.sin_addr.S_un.S_addr	= INADDR_ANY;
	sin.sin_family				= AF_INET;
	sin.sin_port				= htons(6414);

	if(bind(proxysock, (struct sockaddr*)&sin,
		sizeof(sin)) == SOCKET_ERROR) {
		LogPrintError("Socket bind failed on port 6414.\0");
		return false;
	}

	LogPrintLine(0x007F00, "Server bound to port 6414\0");

	LogPrintLine(0x007F00, "Resolving loopback identifier...\0");

	g_nLoopback = GetLoopback();

	if(listen(proxysock, 25) != 0) {
		LogPrintError("Listen operation on proxy socket failed.\0");
		return false;
	}
	
	LogPrintLine(0x007F00, "Now listening for clients!\0");

	if(ioctlsocket(proxysock, FIONBIO, &nArg) == SOCKET_ERROR) {
		LogPrintError("Ioctlsocket operation on proxy socket failed.\0");
		return false;
	}

	g_bKeepAlive = true;
	g_hProxyThread = CreateThread(NULL, 0, ProxyThread, (LPVOID)proxysock, 0, NULL);

	return true;
}

DWORD WINAPI ProxyThread(LPVOID lpParam) {
	SOCKET proxysock = (SOCKET)lpParam;
	SOCKADDR_IN sin;
	int sinsize;
	SOCKET client;
	bool bContinue;
	char szLineBuffer[1024];
	do {
		EnterCriticalSection(g_ProxyLock.GetLock());
		bContinue = g_bKeepAlive;
		
		sinsize = sizeof(SOCKADDR_IN);
		client = accept(proxysock, (struct sockaddr*)&sin, &sinsize);

		if(client != INVALID_SOCKET) {
			sprintf(szLineBuffer, "New Client Connection (%s:%d)\0", 
				inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
			LogPrintLine(0xFF0000, szLineBuffer);
			if((*(char*)(&sin.sin_addr.S_un.S_addr)) != g_nLoopback) {
				sprintf(szLineBuffer, "Client Rejected (%s:%d) [Not Loopback]\0",
					inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
				LogPrintLine(0x00007F, szLineBuffer);
				closesocket(client);
			} else {
				sprintf(szLineBuffer, "Client (%s:%d) accepted\0",
					inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
				LogPrintLine(0x007F00, szLineBuffer);
				sprintf(szLineBuffer, "%s:%d", 
					inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
				HandleClient(g_hMain, client, szLineBuffer);
			}
		}
		LeaveCriticalSection(g_ProxyLock.GetLock());
		Sleep(3);
	} while(bContinue);

	closesocket(proxysock);
	return 0;
}

void KillProxyServer(void) {
	EnterCriticalSection(g_ProxyLock.GetLock());
	g_bKeepAlive = false;
	LeaveCriticalSection(g_ProxyLock.GetLock());

	if(g_hProxyThread != NULL)
		WaitForSingleObject(g_hProxyThread, INFINITE);
}
