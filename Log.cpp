#include <cstdio>
#include <WinSock2.h>
#include <Windows.h>
#include <Richedit.h>
#include "Log.h"
#include "ThreadLock.h"

#pragma warning(disable:4996)

static HWND			g_hLog		= NULL;
static ThreadLock	g_LogLock;

void InitializeLog(HWND hLog) {
	g_hLog = hLog;
}

void LogPrintLine(COLORREF crf, const char* lpszText) {
#ifdef _CONSOLE_IO
	printf("%s\n", lpszText);
#else
	CHARFORMAT cf	= { sizeof(CHARFORMAT) };

	SetLock lock(g_LogLock);

	if(NULL != g_hLog) {
		cf.dwMask		= CFM_COLOR | CFM_PROTECTED;
		cf.crTextColor = crf;

		SendMessage(g_hLog, EM_SETSEL, -1, -1);
		SendMessage(g_hLog, EM_SETCHARFORMAT, (WPARAM)(SCF_SELECTION | SCF_WORD), (LPARAM)&cf);
		SendMessage(g_hLog, EM_REPLACESEL, (WPARAM)0, (LPARAM)lpszText);
		SendMessage(g_hLog, EM_REPLACESEL, (WPARAM)0, (LPARAM)"\r\n\0");
		SendMessage(g_hLog, EM_SETSEL, -1, -1);
		SendMessage(g_hLog, WM_VSCROLL, (WPARAM)(SB_BOTTOM), (LPARAM)NULL);
	}
#endif
}

void LogPrintError(const char* lpszText) {
#ifdef _CONSOLE_IO
	printf("Error: %s\n", lpszText);
#else
	char* pError = new char[strlen(lpszText) + 8];
	sprintf(pError, "Error: %s\0", lpszText);
	LogPrintLine(0x0000FF, pError);
	delete[] pError;
#endif
}
