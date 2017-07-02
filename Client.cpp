#include <cstdio>
#include <cstring>
#include "Client.h"
#include "ProxyUI.h"
#include "ThreadLock.h"
#include "PluginManager.h"
#include "Log.h"

#pragma warning(disable:4996)

// Protection from rogue plugin threads
static ThreadLock g_ArtificialThreadLock;

DWORD WINAPI ClientThread(LPVOID);
void DeInitializeClient(CLIENT**);
bool FixedRecv(SOCKET, char*, int);
bool FixedSend(SOCKET, char*, int);
void ServerToClient(LPCLIENT, const char*, WORD);
void ClientToServer(LPCLIENT, const char*, WORD);
bool PacketCheckServer(LPCLIENT, WORD*);
bool PacketCheckArtificialServer(LPCLIENT, WORD*);
bool PacketCheckClient(LPCLIENT, WORD*);
bool PacketCheckArtificialClient(LPCLIENT, WORD*);
char* GetClientPacket(LPCLIENT);
char* GetArtificialClientPacket(LPCLIENT);
char* GetServerPacket(LPCLIENT);
char* GetArtificialServerPacket(LPCLIENT);

void HandleClient(HWND hMain, SOCKET clientsock, const char* addr)
{
	LPCLIENT pClient = new CLIENT;

	if(NULL != pClient) {
		memset(pClient, 0, sizeof(CLIENT));
		pClient->hMain		= hMain;
		pClient->clientsock = clientsock;
		pClient->serversock = INVALID_SOCKET;
		strcpy(pClient->startpoint_addr, addr);
		CreateThread(NULL, 0, ClientThread, (LPVOID)pClient, 0, NULL);
	}
}


DWORD WINAPI ClientThread(LPVOID lpParam) {
	LPCLIENT pClient = (LPCLIENT)lpParam;
	u_long	nArg = 1		,
			nBytes			;
	int nRet				;
	DWORD dwTicks			;
	SOCKADDR_IN sin			;
	char szLineBuffer[1024]	;
	char buff[32767]		; // 32kb
	WORD wLen				,
		 wTemp				,
		 wIter				;
	char* pkt				;
	bool bExclude			;
	int ref					;
	bool bCharacter = false	;
	BYTE bTemp				;

	if(!pClient)
		return 0;

	if(ioctlsocket(pClient->clientsock,
		FIONBIO, &nArg) == SOCKET_ERROR) {
		sprintf(szLineBuffer, "Connection (%s) rejected, socket [ioctlsocket() fail]", pClient->startpoint_addr);
		DeInitializeClient(&pClient);
		return 0;
	}

	dwTicks = GetTickCount();
	do {
		nRet = ioctlsocket(pClient->clientsock, FIONREAD, &nBytes);
	} while((nRet != SOCKET_ERROR && (nBytes < sizeof(SOCKADDR_IN))) 
		&& ((GetTickCount() - dwTicks) < 10000));

	if(nRet == SOCKET_ERROR || (nBytes < sizeof(SOCKADDR_IN))) {
		sprintf(szLineBuffer, "Connection (%s) rejected (socket error || timeout)", pClient->startpoint_addr);
		LogPrintLine(0x00007F, szLineBuffer);
		DeInitializeClient(&pClient);
		return 0;
	}

	if(!FixedRecv(pClient->clientsock, 
		(char*)&sin, sizeof(SOCKADDR_IN))) {
		DeInitializeClient(&pClient);
		return 0;
	}

	if(ntohs(sin.sin_port) != 6414) {
		sprintf(szLineBuffer, "Connection (%s) rejected "
							"(Invalid endpoint received)", 
							pClient->startpoint_addr);
		LogPrintLine(0x00007F, szLineBuffer);
		DeInitializeClient(&pClient);
		return 0;
	}

	sprintf(pClient->endpoint_addr, "%s:%d", 
		inet_ntoa(sin.sin_addr), htons(sin.sin_port));

	sprintf(szLineBuffer, "Received Endpoint (%s) From Client (%s)", 
		pClient->endpoint_addr, pClient->startpoint_addr);
	LogPrintLine(0xFF0000, szLineBuffer);

	pClient->serversock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(pClient->serversock == SOCKET_ERROR) {
		sprintf(szLineBuffer, "Connection (%s) rejected "
							"(endpoint -> socket() failed)", 
							pClient->startpoint_addr);
		LogPrintLine(0x00007F, szLineBuffer);
		DeInitializeClient(&pClient);
		return 0;
	}

	if(connect(pClient->serversock, (struct sockaddr*)&sin, 
		sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
		sprintf(szLineBuffer, "Connection (%s) rejected "
							"(endpoint -> connect() failed)",
							pClient->startpoint_addr);
		LogPrintLine(0x00007F, szLineBuffer);
		DeInitializeClient(&pClient);
		return 0;
	}

	if(ioctlsocket(pClient->serversock, 
		FIONBIO, &nArg) == SOCKET_ERROR) {
		sprintf(szLineBuffer, "Connection (%s) rejected "
							"(endpoint -> ioctlsocket() failed)", 
							pClient->startpoint_addr);
		LogPrintLine(0x00007F, szLineBuffer);
		DeInitializeClient(&pClient);
		return 0;
	}

	sprintf(szLineBuffer, "Exchange started (%s) <-> (%s)",
		pClient->startpoint_addr, pClient->endpoint_addr);
	LogPrintLine(0xFF0000, szLineBuffer);

	Plugins_OnConnect(pClient);

	while(true) {
		ref = recv(pClient->clientsock, buff, 32767, 0);
		if(ref == 0) {
			sprintf(szLineBuffer, "Connection (%s) <-> (%s) rejected "
								"(startpoint -> connection forcefully aborted)", 
								pClient->startpoint_addr, pClient->endpoint_addr);
			LogPrintLine(0x00007F, szLineBuffer);
			break;
		} else if(ref == SOCKET_ERROR) {
			ref = WSAGetLastError();
			if(ref != WSAEWOULDBLOCK) {
				sprintf(szLineBuffer, "Connection (%s) <-> (%s) rejected "
										"(startpoint -> recv() failed)",
										pClient->startpoint_addr, 
										pClient->endpoint_addr);
				LogPrintLine(0x00007F, szLineBuffer);
				break;
			}
		} else {
			wLen = ref;
			ClientToServer(pClient, buff, wLen);
			
			wLen = 0;
			while(PacketCheckClient(pClient, &wTemp)) {
				if((wLen + wTemp) > 32767)
					break;

				bExclude = false;
				pkt = GetClientPacket(pClient);

				if(NULL != pkt) {

					Plugins_OnSendToServer(pClient, 
						pkt, wTemp, &bExclude);

					if(!bExclude) {
						memcpy(buff + wLen, pkt, wTemp);
						_xor((buff + wLen), wTemp);
						wLen += wTemp;
					}

					delete[] pkt;
				}
			}
			while(PacketCheckArtificialClient(pClient, &wTemp)) {
				if((wLen + wTemp) > 32767)
					break;

				pkt = GetArtificialClientPacket(pClient);
				memcpy(buff + wLen, pkt, wTemp);
				_xor((buff + wLen), wTemp);
				wLen += wTemp;

				delete[] pkt;
			}

			if(wLen > 0) {
				if(!FixedSend(pClient->serversock, buff, wLen)) {
					sprintf(szLineBuffer, "Connection (%s) <-> (%s) rejected "
										"(endpoint -> FixedSend() failed)",
										pClient->startpoint_addr, 
										pClient->endpoint_addr);
					LogPrintLine(0x00007F, szLineBuffer);
					break;
				}
			}
		}
		ref = recv(pClient->serversock, buff, 32767, 0);
		if(ref == 0) {
			sprintf(szLineBuffer, "Connection (%s) <-> (%s) rejected "
								"(endpoint -> connection forcefully aborted)", 
								pClient->startpoint_addr, 
								pClient->endpoint_addr);
			LogPrintLine(0x00007F, szLineBuffer);
			break;
		} else if(ref == SOCKET_ERROR) {
			ref = WSAGetLastError();
			if(ref != WSAEWOULDBLOCK) {
				sprintf(szLineBuffer, "Connection (%s) <-> (%s) rejected "
									"(endpoint -> recv() failed)",
									pClient->startpoint_addr, 
									pClient->endpoint_addr);
				LogPrintLine(0x00007F, szLineBuffer);
				break;
			}
		} else {
			wLen = ref;
			ServerToClient(pClient, buff, wLen);

			wLen = 0;
			while(PacketCheckServer(pClient, &wTemp)) {
				if((wLen + wTemp) > 32767)
					break;

				bExclude = false;
				pkt = GetServerPacket(pClient);

				if(NULL != pkt) {
					if(*(BYTE*)(pkt + 4) == 1 &&
						*(BYTE*)(pkt + 5) == 9) {
						memcpy(pClient->server_name, 
							pkt + 9, wTemp - 9);
					}
					if(!bCharacter && *(BYTE*)(pkt + 4) == 3) {
						wIter = 5;
						pClient->char_id = *(DWORD*)(pkt + wIter);
						wIter += 22;
						bTemp = *(BYTE*)(pkt + wIter);
						++wIter;
						wIter += (bTemp * 2);
						wIter += 4;
						bTemp = *(BYTE*)(pkt + wIter);
						++wIter;
						memcpy(pClient->char_name, pkt + wIter, bTemp);
						ui_InsertClient(pClient);
						Plugins_OnCharacterInfo(pClient);
						bCharacter = true;
					}

					if(*(BYTE*)(pkt + 4) == 23 &&
						*(BYTE*)(pkt + 5) == 11) {
						Plugins_OnClientReadyState(pClient);
					}

					Plugins_OnSendToClient(pClient, 
						pkt, wTemp, &bExclude);

					if(!bExclude) {
						memcpy(buff + wLen, pkt, wTemp);
						_xor((buff + wLen), wTemp);
						wLen += wTemp;
					}

					delete[] pkt;
				}
			}

			if(wLen > 0) {
				if(!FixedSend(pClient->clientsock, buff, wLen)) {
					sprintf(szLineBuffer, "Connection (%s) <-> (%s) rejected "
										"(startpoint -> FixedSend() failed)",
										pClient->startpoint_addr, 
										pClient->endpoint_addr);
					LogPrintLine(0x00007F, szLineBuffer);
					break;
				}
			}
		}

		wLen = 0;
		while(PacketCheckArtificialClient(pClient, &wTemp)) {
			if((wLen + wTemp) > 32767)
				break;

			pkt = GetArtificialClientPacket(pClient);
			memcpy(buff + wLen, pkt, wTemp);
			_xor((buff + wLen), wTemp);
			wLen += wTemp;

			delete[] pkt;
		}

		if(wLen > 0) {
			if(!FixedSend(pClient->serversock, buff, wLen)) {
				sprintf(szLineBuffer, "Connection (%s) <-> (%s) rejected "
									"(endpoint -> FixedSend() failed)",
									pClient->startpoint_addr, 
									pClient->endpoint_addr);
				LogPrintLine(0x00007F, szLineBuffer);
				break;
			}
		}

		wLen = 0;
		while(PacketCheckArtificialServer(pClient, &wTemp)) {
			if((wLen + wTemp) > 32767)
				break;

			pkt = GetArtificialServerPacket(pClient);
			memcpy(buff + wLen, pkt, wTemp);
			_xor((buff + wLen), wTemp);
			wLen += wTemp;

			delete[] pkt;
		}

		if(wLen > 0) {
			if(!FixedSend(pClient->clientsock, buff, wLen)) {
				sprintf(szLineBuffer, "Connection (%s) <-> (%s) rejected "
									"(startpoint -> FixedSend() failed)",
									pClient->startpoint_addr, 
									pClient->endpoint_addr);
				LogPrintLine(0x00007F, szLineBuffer);
				break;
			}
		}

		Sleep(1);
	}

	DeInitializeClient(&pClient);
	return 0;
}

void DeInitializeClient(CLIENT** ppClient)  {
	if(NULL != *ppClient) {
		ui_RemoveClient(*ppClient);
		closesocket((*ppClient)->clientsock);
		if((*ppClient)->serversock != INVALID_SOCKET)
			closesocket((*ppClient)->serversock);

		Plugins_OnDisconnect(*ppClient);

		if(NULL != (*ppClient)->in_data)
			delete[] (*ppClient)->in_data;
		if(NULL != (*ppClient)->in_artificial)
			delete[] (*ppClient)->in_artificial;
		if(NULL != (*ppClient)->out_data)
			delete[] (*ppClient)->out_data;
		if(NULL != (*ppClient)->out_artificial)
			delete[] (*ppClient)->out_artificial;

		memset(*ppClient, 0, sizeof(CLIENT));
		delete *ppClient;
		*ppClient = NULL;
	}
}

bool FixedRecv(SOCKET s, char* buf, int len) {
	int ref		,
		in = 0	;
	bool ret = true;
	do {
		ref = recv(s, buf+in, len-in, 0);
		if(ref == SOCKET_ERROR) {
			ref = WSAGetLastError();
			if(ref != WSAEWOULDBLOCK) {
				ret = false;
				break;
			}
		} else if(ref == 0) {
			ret = false;
			break;
		} else in += ref;
	} while(in < len);

	return ret;
}

bool FixedSend(SOCKET s, char* buf, int len) {
	int ref		,
		out = 0	;
	bool ret = true;
	do {
		ref = send(s, buf+out, len-out, 0);
		if(ref == SOCKET_ERROR) {
			ref = WSAGetLastError();
			if(ref != WSAEWOULDBLOCK) {
				ret = false;
				break;
			}
		} else if(ref == 0) {
			ret = false;
			break;
		} else out += ref;
	} while(out < len);

	return ret;
}

void ServerToClient(LPCLIENT pClient, const char* buf, WORD wLen) {
	char* tmp = pClient->in_data;
	DWORD nOffset = 0;
	pClient->in_data = new char[pClient->in_length + wLen + 1];
	memset(pClient->in_data, 0, sizeof(char) *
		(pClient->in_length + wLen + 1));
	if(NULL != tmp && pClient->in_length > 0) {
		memcpy(pClient->in_data, tmp, pClient->in_length);
		nOffset = pClient->in_length; 
		delete[] tmp;
	}
	memcpy(pClient->in_data + nOffset, buf, wLen);
	_xor((pClient->in_data + nOffset), wLen);
	pClient->in_length += wLen;
}

void ArtificialServerToClient(LPCLIENT pClient, const char* buf, WORD wLen) {
	SetLock lock(g_ArtificialThreadLock);

	char* tmp = pClient->in_artificial;
	DWORD nOffset = 0;
	pClient->in_artificial = new char[pClient->in_artificial_length + wLen + 1];
	memset(pClient->in_artificial, 0, sizeof(char) *
		(pClient->in_artificial_length + wLen + 1));
	if(NULL != tmp && pClient->in_artificial_length > 0) {
		memcpy(pClient->in_artificial, tmp, pClient->in_artificial_length);
		nOffset = pClient->in_artificial_length;
		delete[] tmp;
	}
	memcpy(pClient->in_artificial + nOffset, buf, wLen);
	pClient->in_artificial_length += wLen;
}

void ClientToServer(LPCLIENT pClient, const char* buf, WORD wLen) {
	char* tmp = pClient->out_data;
	DWORD nOffset = 0;
	pClient->out_data = new char[pClient->out_length + wLen + 1];
	memset(pClient->out_data, 0, sizeof(char) *
		(pClient->out_length + wLen + 1));
	if(NULL != tmp && pClient->out_length > 0) {
		memcpy(pClient->out_data, tmp, pClient->out_length);
		nOffset = pClient->out_length;
		delete[] tmp;
	}
	memcpy(pClient->out_data + nOffset, buf, wLen);
	_xor((pClient->out_data + nOffset), wLen);
	pClient->out_length += wLen;
}

void ArtificialClientToServer(LPCLIENT pClient, const char* buf, WORD wLen) {
	SetLock lock(g_ArtificialThreadLock);

	char* tmp = pClient->out_artificial;
	DWORD nOffset = 0;
	pClient->out_artificial = new char[pClient->out_artificial_length + wLen + 1];
	memset(pClient->out_artificial, 0, sizeof(char) *
		(pClient->out_artificial_length + wLen + 1));
	if(NULL != tmp && pClient->out_artificial_length > 0) {
		memcpy(pClient->out_artificial, tmp, pClient->out_artificial_length);
		nOffset = pClient->out_artificial_length;
		delete[] tmp;
	}
	memcpy(pClient->out_artificial + nOffset, buf, wLen);
	pClient->out_artificial_length += wLen;
}

bool PacketCheckServer(LPCLIENT pClient, WORD* pwLen) {
	bool bRet = false;
	WORD wLen;
	if(NULL != pClient->in_data 
		&& pClient->in_length > 4) {
		wLen = *(WORD*)(pClient->in_data + 2);
		if(pClient->in_length >= (DWORD)(wLen + 4)) {
			bRet = true;
			if(NULL != pwLen)
				*pwLen = wLen + 4;
		}
	}
	return bRet;
}

bool PacketCheckArtificialServer(LPCLIENT pClient, WORD* pwLen) {
	bool bRet = false;
	WORD wLen;
	if(NULL != pClient->in_artificial 
		&& pClient->in_artificial_length > 4) {
		wLen = *(WORD*)(pClient->in_artificial + 2);
		if(pClient->in_artificial_length >= (DWORD)(wLen + 4)) {
			bRet = true;
			if(NULL != pwLen)
				*pwLen = wLen + 4;
		}
	}
	return bRet;
}

bool PacketCheckClient(LPCLIENT pClient, WORD* pwLen) {
	bool bRet = false;
	WORD wLen;
	if(NULL != pClient->out_data 
		&& pClient->out_length > 4) {
		wLen = *(WORD*)(pClient->out_data + 2);
		if(pClient->out_length >= (DWORD)(wLen + 4)) {
			bRet = true;
			if(NULL != pwLen)
				*pwLen = wLen + 4;
		}
	}
	return bRet;
}

bool PacketCheckArtificialClient(LPCLIENT pClient, WORD* pwLen) {
	bool bRet = false;
	WORD wLen;
	if(NULL != pClient->out_artificial
		&& pClient->out_artificial_length > 4) {
		wLen = *(WORD*)(pClient->out_artificial + 2);
		if(pClient->out_artificial_length >= (DWORD)(wLen + 4)) {
			bRet = true;
			if(NULL != pwLen)
				*pwLen = wLen + 4;
		}
	}
	return bRet;
}

char* GetClientPacket(LPCLIENT pClient) {
	char* pRet = NULL;
	WORD wLen;
	if(PacketCheckClient(pClient, &wLen)) {
		pRet = new char[wLen];
		memcpy(pRet, pClient->out_data, wLen);
		memcpy(pClient->out_data, 
			pClient->out_data + wLen, 
			(pClient->out_length - wLen));
		pClient->out_length -= wLen;
	}
	return pRet;
}

char* GetArtificialClientPacket(LPCLIENT pClient) {
	SetLock lock(g_ArtificialThreadLock);

	char* pRet = NULL;
	WORD wLen;
	if(PacketCheckArtificialClient(pClient, &wLen)) {
		pRet = new char[wLen];
		memcpy(pRet, pClient->out_artificial, wLen);
		memcpy(pClient->out_artificial,
			pClient->out_artificial + wLen,
			(pClient->out_artificial_length - wLen));
		pClient->out_artificial_length -= wLen;
	}
	return pRet;
}

char* GetServerPacket(LPCLIENT pClient) {
	char* pRet = NULL;
	WORD wLen;
	if(PacketCheckServer(pClient, &wLen)) {
		pRet = new char[wLen];
		memcpy(pRet, pClient->in_data, wLen);
		memcpy(pClient->in_data,
			pClient->in_data + wLen,
			(pClient->in_length - wLen));
		pClient->in_length -= wLen;
	}
	return pRet;
}

char* GetArtificialServerPacket(LPCLIENT pClient) {
	SetLock lock(g_ArtificialThreadLock);

	char* pRet = NULL;
	WORD wLen;
	if(PacketCheckArtificialServer(pClient, &wLen)) {
		pRet = new char[wLen];
		memcpy(pRet, pClient->in_artificial, wLen);
		memcpy(pClient->in_artificial,
			pClient->in_artificial + wLen,
			(pClient->in_artificial_length - wLen));
		pClient->in_artificial_length -= wLen;
	}
	return pRet;
}

