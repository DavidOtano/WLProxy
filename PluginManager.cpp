#include <WinSock2.h>
#include <Windows.h>
#include <vector>
#include "PluginManager.h"
#include "ThreadLock.h"
#include "ProxyUI.h"
#include "Client.h"
#include "Log.h"

static ThreadLock g_PluginsLock;

#pragma warning (disable : 4996)

typedef struct TAG_PLUGIN {
	HMODULE hmModule;
	PROCEDURES procs;
} PLUGIN, *LPPLUGIN;

static std::vector<LPPLUGIN> g_plugins;


void Plugins_Load(void) {
	char szEXEPath[MAX_PATH] = {0x00};
	char szDir[MAX_PATH] = {0x00};
	char szPlugin[MAX_PATH] = {0x00};
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA wfd;
	HMODULE hPlugin = NULL;
	LPPLUGIN lpPlugin;
	LPINITIALIZE InitProc = NULL;
	char szDescription[256];
	int n;

	GetModuleFileName(NULL, szEXEPath, MAX_PATH);
	for(n = strlen(szEXEPath); n > 0; --n) {
		if(szEXEPath[n] == '\\')
			break;
		szEXEPath[n] = 0x00;
	}

	strcpy(szDir, szEXEPath);
	strcat(szDir, "Plugins\\*.wlp");
	hFind = FindFirstFile(szDir, &wfd);
	if(hFind != INVALID_HANDLE_VALUE) {
		do {
			strcpy(szPlugin, szEXEPath);
			strcat(szPlugin, "Plugins\\");
			strcat(szPlugin, wfd.cFileName);
			hPlugin = LoadLibrary(szPlugin);
			if(NULL != hPlugin) {
				InitProc = (LPINITIALIZE)GetProcAddress(hPlugin, "ProxyPlugin_Initialize");
				if(NULL != InitProc) {
					lpPlugin = new PLUGIN;
					memset(lpPlugin, 0, sizeof(PLUGIN));
					lpPlugin->hmModule = hPlugin;
					lpPlugin->procs.LogPrintLine				= LogPrintLine;
					lpPlugin->procs.LogPrintError				= LogPrintError;
					lpPlugin->procs.ArtificialServerToClient	= ArtificialServerToClient;
					lpPlugin->procs.ArtificialClientToServer	= ArtificialClientToServer;

					if(!InitProc(&lpPlugin->procs, szDescription)) {
						FreeLibrary(hPlugin);
						delete lpPlugin;
					} else {
						ui_InsertPlugin(wfd.cFileName, szDescription);
						g_plugins.push_back(lpPlugin);
					}
				}
			}
		} while(FindNextFile(hFind, &wfd));
	}
}

void Plugins_OnConnect(LPCLIENT pClient) {
	SetLock lock(g_PluginsLock);
	std::vector<LPPLUGIN>::iterator it;

	for(it = g_plugins.begin(); 
		it != g_plugins.end(); ++it) {
		if((*it)->procs.OnConnect != NULL)
			(*it)->procs.OnConnect(pClient);
	}
}

void Plugins_OnCharacterInfo(LPCLIENT pClient) {
	SetLock lock(g_PluginsLock);
	std::vector<LPPLUGIN>::iterator it;

	for(it = g_plugins.begin();
		it != g_plugins.end(); ++it) {
		if((*it)->procs.OnCharacterInfo != NULL)
			(*it)->procs.OnCharacterInfo(pClient);
	}
}

void Plugins_OnSendToClient(LPCLIENT pClient, 
	char* lpszBuffer, WORD wLen, bool* pbExclude) {
	SetLock lock(g_PluginsLock);
	std::vector<LPPLUGIN>::iterator it;

	for(it = g_plugins.begin();
		it != g_plugins.end(); ++it) {
		if((*it)->procs.OnSendToClient != NULL)
			(*it)->procs.OnSendToClient(pClient,
				lpszBuffer, wLen, pbExclude);

		if(*pbExclude)
			break;
	}
}

void Plugins_OnSendToServer(LPCLIENT pClient,
	char* lpszBuffer, WORD wLen, bool* pbExclude) {
	SetLock lock(g_PluginsLock);
	std::vector<LPPLUGIN>::iterator it;

	for(it = g_plugins.begin();
		it!= g_plugins.end(); ++it) {
		if((*it)->procs.OnSendToServer != NULL)
			(*it)->procs.OnSendToServer(pClient,
				lpszBuffer, wLen, pbExclude);

		if(*pbExclude)
			break;
	}
}

void Plugins_OnClientReadyState(LPCLIENT pClient) {
	SetLock lock(g_PluginsLock);
	std::vector<LPPLUGIN>::iterator it;

	for(it = g_plugins.begin();
		it != g_plugins.end(); ++it) {
		if((*it)->procs.OnClientReadyState != NULL)
			(*it)->procs.OnClientReadyState(pClient);
	}
}

void Plugins_OnDisconnect(LPCLIENT pClient) {
	SetLock lock(g_PluginsLock);
	std::vector<LPPLUGIN>::iterator it;

	for(it = g_plugins.begin();
		it != g_plugins.end(); ++it) {
		if((*it)->procs.OnDisconnect != NULL)
			(*it)->procs.OnDisconnect(pClient);
	}
}

void Plugins_OnFree(void) {
	SetLock lock(g_PluginsLock);
	std::vector<LPPLUGIN>::iterator it;

	for(it = g_plugins.begin();
		it != g_plugins.end(); ++it) {
		if((*it)->procs.OnFree) {
			(*it)->procs.OnFree();
		}
		FreeLibrary((*it)->hmModule);
		delete *it;
	}
	
	g_plugins.clear();
}
