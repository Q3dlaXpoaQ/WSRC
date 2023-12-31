#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

DWORD WINAPI receiveMessages(LPVOID lpParam)
{
    SOCKET clientSocket = (SOCKET)lpParam;
    char buffer[10240];
    int bytesReceived;
    while (1)
    {
        bytesReceived = recv(clientSocket, buffer, 10240, 0); // 接收数据
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            printf("%s", buffer);
        }
        else if (bytesReceived == 0)
        {
            printf("Client disconnected\n");
            break;
        }
        else
        {
            fprintf(stderr, "Error receiving data\n");
            break;
        }
    }
    return 0;
}

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData); // 初始化Winsock库

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0); // 创建套接字
    if (serverSocket == INVALID_SOCKET)
    {
        fprintf(stderr, "Error creating socket\n");
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;         // 地址族
    serverAddr.sin_addr.s_addr = INADDR_ANY; // 服务器IP地址
    serverAddr.sin_port = htons(8888);       // 服务器端口号

    bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)); // 绑定套接字到地址和端口
    listen(serverSocket, 5);                                                 // 监听连接请求

    printf("Server listening on port 8888\n");

    SOCKET clientSocket = accept(serverSocket, NULL, NULL); // 接受客户端连接
    if (clientSocket == INVALID_SOCKET)
    {
        fprintf(stderr, "Error accepting client connection\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // 创建接收信息的线程
    HANDLE hThread = CreateThread(NULL, 0, receiveMessages, (LPVOID)clientSocket, 0, NULL);

    char message[10240];
    while (1)
    {
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = '\0'; // remove newline character

        send(clientSocket, message, strlen(message), 0); // 发送消息

        if (strcmp(message, "exit_server") == 0) // 如果输入exit则退出循环
            break;
    }

    WaitForSingleObject(hThread, INFINITE); // 等待接收信息的线程结束
    CloseHandle(hThread);                   // 关闭线程句柄

    closesocket(clientSocket); // 关闭客户端套接字
    closesocket(serverSocket); // 关闭服务器套接字
    WSACleanup();              // 关闭Winsock库

    return 0;
}
