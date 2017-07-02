#include <WinSock2.h>
#include <Windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <cstdio>
#include <cstring>
#include <strsafe.h>
#include "ProxyUI.h"
#include "ProxyServer.h"
#include "PluginManager.h"
#include "Log.h"
#include "resource.h"

#pragma warning(disable : 4996)

using namespace std;

static HWND g_hMain = NULL;

BOOL CALLBACK MainDlgProc(
	HWND, UINT, WPARAM, LPARAM);
void CreateShellIcon(void);
void DeleteShellIcon(void);

NOTIFYICONDATA g_nid;

int InitProxyUI(HINSTANCE hInstance) {
	HMODULE hmRichEdit;
	HWND hwndMain;
	MSG msg = {0};
	LV_COLUMN lvc;

	hmRichEdit = LoadLibrary("RICHED20.DLL");

#ifdef _CONSOLE_IO
	HWND hTop;
	DWORD dwID;
	char szLineBuffer[1024] = {0x00};
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
	freopen("CON", "r", stdin);
	Plugins_Load();
	InitProxyServer(NULL);
	hTop = FindWindowEx(0, 0, NULL, NULL);
	SetConsoleTitle("Wonderland Proxy");
	while(fgets(szLineBuffer, 1024, stdin)) {
		if(strnicmp(szLineBuffer,"exit", 4) == 0)
			break;
	}
	KillProxyServer();
	Plugins_OnFree();
#else
	InitCommonControls();

	if(!hmRichEdit) {
		MessageBox(NULL, "Rich Edit Initialization Failed!",
			"Error!", MB_ICONEXCLAMATION | MB_OK);

		return -1;
	}

	g_hMain = hwndMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), 
										HWND_DESKTOP, MainDlgProc);

	if(!hwndMain) {
		MessageBox(NULL, "Main Dialog Initialization Failed", 
			"Error!", MB_ICONEXCLAMATION | MB_OK);

		return -1;
	}

	SendMessage(hwndMain, WM_SETICON, ICON_BIG, 
		(LPARAM)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN), 
		IMAGE_ICON, 32, 32, LR_SHARED));
	SendMessage(hwndMain, WM_SETICON, ICON_SMALL, 
		(LPARAM)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN), 
		IMAGE_ICON, 16, 16, LR_SHARED));

	lvc.mask		= LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
	lvc.iSubItem	= 0;
	lvc.pszText		= "Character";
	lvc.cx			= 140;
	ListView_InsertColumn(GetDlgItem(hwndMain,
		IDC_CONLIST), 0, &lvc);

	/*
	--- useless information --- [removed]
	lvc.iSubItem	= 1;
	lvc.pszText		= "Startpoint";
	lvc.cx			= 110;
	ListView_InsertColumn(GetDlgItem(hwndMain, 
		IDC_CONLIST), 1, &lvc);
		*/

	lvc.iSubItem	= 1;
	lvc.pszText		= "Endpoint";
	lvc.cx			= 140;
	ListView_InsertColumn(GetDlgItem(hwndMain,
		IDC_CONLIST), 1, &lvc);

	lvc.iSubItem	= 0;
	lvc.pszText		= "Module Name";
	lvc.cx			= 150;
	ListView_InsertColumn(GetDlgItem(hwndMain,
		IDC_PLUGLIST), 0, &lvc);

	lvc.iSubItem	= 1;
	lvc.pszText		= "Description";
	lvc.cx			= 180;
	ListView_InsertColumn(GetDlgItem(hwndMain,
		IDC_PLUGLIST), 1, &lvc);

	ListView_SetExtendedListViewStyle(GetDlgItem(
		hwndMain, IDC_CONLIST), LVS_EX_FULLROWSELECT);

	ListView_SetExtendedListViewStyle(GetDlgItem(
		hwndMain, IDC_PLUGLIST), LVS_EX_FULLROWSELECT);

	InitializeLog(GetDlgItem(hwndMain, IDC_LOG));

	Plugins_Load();

	InitProxyServer(hwndMain);

	CreateShellIcon();

	while(GetMessage(&msg, NULL, 0, 0) > 0) {
		if(!IsDialogMessage(hwndMain, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	KillProxyServer();
	Plugins_OnFree();
#endif

	if(NULL != hmRichEdit)
		FreeLibrary(hmRichEdit);

	return msg.wParam;
}

BOOL CALLBACK MainDlgProc(
	HWND hwnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam
	)
{
	BOOL bRet;
	switch(uMsg) {
	case WM_CLOSE:
		DestroyWindow(hwnd);
		DeleteShellIcon();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SYSCOMMAND:
		if((wParam & 0xFFF0) == SC_MINIMIZE) {
			bRet = DefWindowProc(hwnd, uMsg, wParam, lParam);
//			CreateShellIcon();
			ShowWindow(hwnd, SW_HIDE);
			return bRet;
		}
		return FALSE;
	case 110101:
		switch(LOWORD(lParam)) {
		case WM_LBUTTONDOWN:
//			DeleteShellIcon();
			ShowWindow(hwnd, SW_SHOWNORMAL);
			SetForegroundWindow(hwnd);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void ui_InsertClient(LPCLIENT pClient) {
#ifndef _CONSOLE_IO
	char szBuffer[1024] = {0x00};
	char szInfo[ARRAYSIZE(g_nid.szInfo)]  = {0x00};
	char szInfoTitle[ARRAYSIZE(g_nid.szInfoTitle)] = {0x00};

	LV_ITEM lvi;

	sprintf(szBuffer, "[%s] %s", 
		pClient->server_name, 
		pClient->char_name);

	lvi.mask		= LVIF_PARAM | LVIF_TEXT;
	lvi.iItem		= ListView_GetItemCount(GetDlgItem(g_hMain, IDC_CONLIST));
	lvi.iSubItem	= 0;
	lvi.pszText		= szBuffer;
	lvi.lParam		= (LPARAM)pClient;
	ListView_InsertItem(GetDlgItem(g_hMain, IDC_CONLIST), &lvi);

	lvi.mask		= LVIF_TEXT;
	/*
	--- useless information --- [removed]
	lvi.iSubItem	= 1;
	lvi.pszText		= pClient->startpoint_addr;
	lvi.lParam		= (LPARAM)pClient;
	ListView_SetItem(GetDlgItem(g_hMain, IDC_CONLIST), &lvi);
	*/

	lvi.iSubItem	= 1;
	lvi.pszText		= pClient->endpoint_addr;
	lvi.lParam		= (LPARAM)pClient;
	ListView_SetItem(GetDlgItem(g_hMain, IDC_CONLIST), &lvi);

	
	sprintf(szInfo, "%s has connected.", 
		pClient->char_name);

	sprintf(szInfoTitle, "%s Server",
		pClient->server_name);

	ui_PopBalloonTip(szInfo, szInfoTitle);
#else
	printf("Character Connected [%s : %s (%s)]\n", 
		pClient->server_name, pClient->char_name, pClient->endpoint_addr);
#endif
}

void ui_RemoveClient(LPCLIENT pClient) {
#ifndef _CONSOLE_IO
	LVFINDINFO lvfi;
	int nItem;
	char szInfo[ARRAYSIZE(g_nid.szInfo)];
	char szInfoTitle[ARRAYSIZE(g_nid.szInfoTitle)];

	lvfi.flags		= LVFI_PARAM;
	lvfi.lParam		= (LPARAM)pClient;

	nItem = ListView_FindItem(GetDlgItem(
		g_hMain, IDC_CONLIST), -1, &lvfi);
	if(nItem != -1) {
		ListView_DeleteItem(GetDlgItem(
			g_hMain, IDC_CONLIST), nItem);
	}

	if(strlen(pClient->server_name)
		&& strlen(pClient->char_name)) {
		sprintf(szInfo, "%s has disconnected", pClient->char_name);
		sprintf(szInfoTitle, "%s Server", pClient->server_name);
		ui_PopBalloonTip(szInfo, szInfoTitle);
	}

#else
	if(strlen(pClient->char_name))
		printf("Character [%s] Disconnected.\n", pClient->char_name);
#endif
}

void ui_InsertPlugin(char* lpszName, 
	char* lpszDescription) 
{
#ifndef _CONSOLE_IO
	LV_ITEM lvi;
	lvi.mask		= LVIF_TEXT | LVIF_PARAM;
	lvi.iItem		= ListView_GetItemCount(GetDlgItem(g_hMain, IDC_PLUGLIST));
	lvi.iSubItem	= 0;
	lvi.pszText		= lpszName;
	lvi.lParam		= (LPARAM)lpszName;
	ListView_InsertItem(GetDlgItem(g_hMain, IDC_PLUGLIST), &lvi);

	lvi.mask		= LVIF_TEXT;
	lvi.iSubItem	= 1;
	lvi.pszText		= lpszDescription;
	ListView_SetItem(GetDlgItem(g_hMain, IDC_PLUGLIST), &lvi);
#else 
	printf("Plugin Loaded: %s - [%s]\n", lpszName, lpszDescription);
#endif
}

void ui_PopBalloonTip(const char* lpszTip, 
	const char* lpszTitle) {
	HRESULT hr = StringCchCopy(
		g_nid.szInfo, ARRAYSIZE(g_nid.szInfo),
		lpszTip);

	if(SUCCEEDED(hr)) {
		hr = StringCchCopy(
			g_nid.szInfoTitle, 
			ARRAYSIZE(g_nid.szInfoTitle),
			lpszTitle);

		if(SUCCEEDED(hr)) {
			g_nid.uFlags = NIF_INFO | NIF_ICON | NIF_TIP | NIF_MESSAGE;
			Shell_NotifyIcon(NIM_MODIFY, &g_nid);
		}
	}
}

void CreateShellIcon(void) {
	memset(&g_nid, 0, sizeof(g_nid));
	g_nid.cbSize				= sizeof(NOTIFYICONDATA);
	g_nid.uID					= 110101;
	g_nid.uCallbackMessage	= 110101;
	g_nid.hWnd				= g_hMain;
	g_nid.uFlags				= NIF_ICON | NIF_TIP | NIF_MESSAGE;
	g_nid.hIcon				= (HICON)SendMessage(g_hMain, WM_GETICON, ICON_SMALL, 0);
	strcpy(g_nid.szTip, "Wonderland Proxy [Dormant]");
	Shell_NotifyIcon(NIM_ADD, &g_nid);
}

void DeleteShellIcon(void) {
	Shell_NotifyIcon(NIM_DELETE, &g_nid);
}
