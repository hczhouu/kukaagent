
#include "framework.h"
#include "kukaAgent.h"
#define MAX_LOADSTRING 100
#include <Windows.h>
#include <TlHelp32.h>
#include <comdef.h>
#include <Shlwapi.h>
#include <vector>
#include <string>



SERVICE_STATUS              g_serviceStatus;
SERVICE_STATUS_HANDLE       g_handStatus;
static HANDLE               g_hExit;
static HMODULE              hDllNetSDK, hDllCommon, hDllService;
static void WINAPI ServiceMain();
static void WINAPI CtrlHandler(DWORD dwRequest);
static void CreateProcessByUserToken();
static void GetLocalDiskList(std::vector<std::string>& vecDiskList);
static std::string GetRealPath(const std::string& strFileName);
std::string  strSvcHostPath, strCfgFilePath,
strSdkDllPath, strCommonDllPath;

int main()
{
	std::string strLogPath;
	char szBuf[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, szBuf, sizeof(szBuf) * sizeof(char));
	PathRemoveFileSpecA(szBuf);
	strLogPath.append(szBuf);
	strLogPath.append("\\logs\\service.log");

	//pLogger->info("启动悦多云系统服务...");

	strSvcHostPath = GetRealPath("YDCloudSvcHost.exe");
	strCfgFilePath = GetRealPath("YDCloudSrv.ini");

	SERVICE_TABLE_ENTRYA srvTable[2];
	char strSrvName[] = "YDCloudService";
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
	g_handStatus = RegisterServiceCtrlHandlerA("YDCloudService", CtrlHandler);

	//向SCM 报告运行状态
	g_serviceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(g_handStatus, &g_serviceStatus);
}


//获取本地磁盘列表
void GetLocalDiskList(std::vector<std::string>& vecDiskList)
{
	DWORD dwSize = MAX_PATH;
	char szLogicalDrives[MAX_PATH] = { 0 };
	DWORD dwResult = GetLogicalDriveStringsA(dwSize, szLogicalDrives);
	vecDiskList.clear();
	if (dwResult > 0 && dwResult <= MAX_PATH)
	{
		char* szSingleDrive = szLogicalDrives;
		if (NULL == szSingleDrive)
		{
			return;
		}

		while (*szSingleDrive)
		{
			UINT uDriverType = GetDriveTypeA(szSingleDrive);
			if (DRIVE_FIXED != uDriverType)
			{
				szSingleDrive += lstrlenA(szSingleDrive) + 1;
				continue;
			}
			vecDiskList.push_back(szSingleDrive);
			szSingleDrive += lstrlenA(szSingleDrive) + 1;
		}
	}
}


void CreateProcessByUserToken()
{
	HANDLE hDuplicatedToken = NULL;
	LPVOID lpEnvironment = NULL;
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(STARTUPINFOA));
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = HIDE_WINDOW;
}



std::string GetRealPath(const std::string& strFileName)
{
	//从其他盘符找依赖的dll路径
	std::string strFilePath;
	std::vector<std::string> vecDiskList;
	GetLocalDiskList(vecDiskList);
	for (std::vector<std::string>::iterator itor = vecDiskList.begin();
		itor != vecDiskList.end(); ++itor)
	{
		std::string& strDiskName = *itor;
		std::string strTemp;
		strTemp.clear();
		strTemp.append(strDiskName);
		strTemp.append("Nmenu\\YDCloudSvcHost\\");
		strTemp.append(strFileName);
		if (PathFileExistsA(strTemp.data()))
		{
			strFilePath = strTemp;
		}

		if (!strFilePath.empty())
			break;
	}

	//其他盘符找不到 则在当前进程目录下找
	char szBuf[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, szBuf, sizeof(szBuf) * sizeof(char));
	PathRemoveFileSpecA(szBuf);
	std::string strBinPath;
	strBinPath.append(szBuf);
	if (strFilePath.empty())
	{
		strFilePath.append(strBinPath);
		strFilePath.append("\\");
		strFilePath.append(strFileName);
	}


	return strFilePath;
}