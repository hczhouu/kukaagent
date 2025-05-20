
#include "framework.h"
#include "kukaAgent.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <comdef.h>
#include <Shlwapi.h>
#include <vector>
#include <string>


SERVICE_STATUS              g_serviceStatus;
SERVICE_STATUS_HANDLE       g_handStatus;
static HANDLE               g_hExit;
static void WINAPI ServiceMain();
static void WINAPI CtrlHandler(DWORD dwRequest);

int main()
{
	std::string logPath;
	char szBuf[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, szBuf, sizeof(szBuf) * sizeof(char));
	PathRemoveFileSpecA(szBuf);
	logPath.append(szBuf);
	logPath.append("\\logs\\service.log");

	SERVICE_TABLE_ENTRYA srvTable[2];
	char strSrvName[] = "kukaService";
	srvTable[0].lpServiceName = strSrvName;
	srvTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTIONA)ServiceMain;
	srvTable[1].lpServiceName = NULL;
	srvTable[1].lpServiceProc = NULL;
	StartServiceCtrlDispatcherA(srvTable);
	return 0;
}



void WINAPI CtrlHandler(DWORD dwControl)
{
	std::string strEventName = "Global\\{70131F9A-1754-448C-A238-3697B0598B89}";
	g_hExit = OpenEventA(EVENT_ALL_ACCESS, FALSE, strEventName.data());
	switch (dwControl)
	{
	case SERVICE_CONTROL_STOP:
	{
		g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetEvent(g_hExit);
		break;
	}
	case SERVICE_CONTROL_SHUTDOWN:
	{
		g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetEvent(g_hExit);
		break;
	}
	default:
		break;
	}

	SetServiceStatus(g_handStatus, &g_serviceStatus);
}


void WINAPI ServiceMain()
{
	g_serviceStatus.dwServiceType = SERVICE_WIN32;
	g_serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
	g_serviceStatus.dwWin32ExitCode = 0;
	g_serviceStatus.dwServiceSpecificExitCode = 0;
	g_serviceStatus.dwCheckPoint = 0;
	g_serviceStatus.dwWaitHint = 0;
	g_handStatus = RegisterServiceCtrlHandlerA("kukaService", CtrlHandler);

	//向SCM 报告运行状态
	g_serviceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(g_handStatus, &g_serviceStatus);
}