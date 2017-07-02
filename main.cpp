#include <WinSock2.h>
#include <Windows.h>
#include "ProxyUI.h"

int WINAPI WinMain(
	HINSTANCE hInstance			,
	HINSTANCE hPrevInstance		,
	LPSTR lpCmdLine				,
	int nCmdShow
	)
{
	return InitProxyUI(hInstance);
}
