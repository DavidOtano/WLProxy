#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <WinSock2.h>
#include <Windows.h>

/*
	you shouldn't have to touch
	the members of this structure
	the proxy server should handle
	everything for you, just pass
	the ptr to the provided functions
*/
typedef struct TAG_CLIENT {
	HWND hMain;
	SOCKET clientsock;
	SOCKET serversock;
	char startpoint_addr[64];
	char endpoint_addr[64];
	char server_name[64];
	char char_name[16];
	char* in_data;
	DWORD in_length;
	char* out_data;
	DWORD out_length;
	char* in_artificial;
	DWORD in_artificial_length;
	char* out_artificial;
	DWORD out_artificial_length;
	DWORD char_id;
} CLIENT, *LPCLIENT;

typedef struct TAG_PROCEDURES {
//[[[[[ For the Plugin to call ]]]]]
	// Print to the log
	void (*LogPrintLine)(COLORREF, const char*);
	// Print an error to the log
	void (*LogPrintError)(const char*);
	// Fake packet sent from server to client
	void (*ArtificialServerToClient)(LPCLIENT, const char*, WORD);
	// Fake packet sent from client to server
	void (*ArtificialClientToServer)(LPCLIENT, const char*, WORD);
//[[[[[ Callbacks for the EXE to call ]]]]]
	// On Connect (both startpoint/endpoint)
	void (*OnConnect)(LPCLIENT);
	// On Character Info (id/name)
	void (*OnCharacterInfo)(LPCLIENT);
	// On Send -> Server
	void (*OnSendToServer)(LPCLIENT, char*, WORD, bool*);
	// On Send -> Client
	void (*OnSendToClient)(LPCLIENT, char*, WORD, bool*);
	// On Login Complete
	void (*OnClientReadyState)(LPCLIENT);
	// On Disconnect
	void (*OnDisconnect)(LPCLIENT);
	// On Plugin Free
	void (*OnFree)(void);
} PROCEDURES, *LPPROCEDURES;

typedef bool (__cdecl *LPINITIALIZE)(LPPROCEDURES, char*);

#define _xor(a, b) { \
	int ___x; \
	for(___x = 0; ___x < b; ++___x) { \
		a[___x] = (BYTE)(a[___x] ^ 0xAD); \
	} \
}

#endif
