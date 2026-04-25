#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <sstream>
#include <cmath>
#include <windows.h>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

bool sendAll(SOCKET sock, const std::string& msg) {
    int total = 0;
    int len = msg.size();

    while (total < len) {
        int sent = send(sock, msg.c_str() + total, len - total, 0);
        if (sent == SOCKET_ERROR) {
            std::cout << "Send error: " << WSAGetLastError() << "\n";
            return false;
        }
        total += sent;
    }
    return true;
}

string recvSafe(SOCKET sock) {
    char buffer[1024];
    int bytes = recv(sock, buffer, sizeof(buffer), 0);

    if (bytes == 0) {
        cout << "Server disconnected\n";
        return "";
    }

    if (bytes == SOCKET_ERROR) {
        cout << "Recv error\n";
        return "";
    }

    return string(buffer, bytes);
}

int main()
{
    srand(time(0));

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cout << "Socket error\n";
        WSACleanup();
        return 1;
    }
    
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        cout << "Connection failed\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    int n = 5;
    vector<double> v(n);

    cout << "Vector: ";
    for (int i = 0; i < n; i++) {
        v[i] = rand() % 10;
        cout << v[i] << " ";
    }
    cout << endl;

    sendAll(sock, "INIT 5\n");
    cout << recvSafe(sock);

    stringstream ss;
    ss << "DATA ";
    for (double x : v) ss << x << " ";
    ss << "\n";

    sendAll(sock, ss.str());
    cout << recvSafe(sock);

    sendAll(sock, "START\n");
    cout << recvSafe(sock);

    while (true) {
        sendAll(sock, "STATUS\n");
        string resp = recvSafe(sock);
        cout << resp;

        if (resp.find("DONE") != std::string::npos)
            break;
        Sleep(500);
    }

    sendAll(sock, "RESULT\n");
    cout << "Result: " << recvSafe(sock);

    closesocket(sock);
    WSACleanup();
}
