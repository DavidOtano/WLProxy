#ifndef __LOG_H__
#define __LOG_H__
#include <WinSock2.h>
#include <Windows.h>

void InitializeLog(HWND /*hLog*/);
void LogPrintLine(COLORREF /*crf*/, const char* /*lpszText*/);
void LogPrintError(const char* /*lpszText*/);

#endif
