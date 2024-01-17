
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define BUFFER_SIZE 1024

DWORD WINAPI receiveMessages(LPVOID lpParam)
{
    SOCKET clientSocket = (SOCKET)lpParam;

    int bytesReceived;
    char buffer[BUFFER_SIZE];

    while (1)
    {
        bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0)
        {
            buffer[bytesReceived] = '\0';
            printf(buffer);
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

    closesocket(clientSocket);
    return 0;
}

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET)
    {
        fprintf(stderr, "Error creating socket\n");
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 5);

    printf("Server listening on port 8888\n");

    while (1)
    {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET)
        {
            fprintf(stderr, "Error accepting client connection\n");
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        HANDLE hThread = CreateThread(NULL, 0, receiveMessages, (LPVOID)clientSocket, 0, NULL);
        if (hThread == NULL)
        {
            fprintf(stderr, "Error creating thread\n");
            closesocket(clientSocket);
        }
        else
        {
            CloseHandle(hThread);
        }

        char message[BUFFER_SIZE];
        int bytesSent;
        while (1)
        {
            fgets(message, sizeof(message), stdin);
            message[strcspn(message, "\n")] = '\0';

            bytesSent = send(clientSocket, message, strlen(message), 0);
            if (bytesSent == SOCKET_ERROR)
            {
                fprintf(stderr, "Error sending data\n");
                break;
            }

            if (strcmp(message, "exit_server") == 0)
                break;
        }
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
