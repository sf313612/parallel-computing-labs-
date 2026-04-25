#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <sstream>
#include <cmath>
#include <windows.h>

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

double calculateNorm(const std::vector<double>& v) {
    double sum = 0;
    for (double x : v) sum += x * x;
    return sqrt(sum);
}

void handleClient(SOCKET client) {
    char buffer[1024];

    vector<double> data;
    bool hasData = false;
    bool isProcessing = false;
    bool isDone = false;
    double result = 0;

    while (true) {
        int bytes = recv(client, buffer, sizeof(buffer), 0);

        if (bytes <= 0) {
            cout << "Client [ID: " << client << "] disconnected" << endl;
            break;
        }

        if (bytes == SOCKET_ERROR) {
            std::cout << "Recv error: " << WSAGetLastError() << "\n";
            break;
        }

        string request(buffer, bytes);
        stringstream ss(request);
        string command;
        ss >> command;

        // INIT n
        if (command == "INIT") {
            int n;
            if (!(ss >> n)) {
                sendAll(client, "ERROR Invalid INIT\n");
                continue;
            }
            send(client, "OK\n", 3, 0);
        }

        // DATA x1 x2 x3 etc
        else if (command == "DATA") {
            data.clear();
            double num;

            if (!(ss >> num)) {
                sendAll(client, "ERROR Invalid DATA\n");
                continue;
            }

            data.push_back(num);

            while (ss >> num) data.push_back(num);

            hasData = true;
            isDone = false;
            sendAll(client, "OK\n");
        }

        // START
        else if (command == "START") {
            if (!hasData) {
                sendAll(client, "ERROR No data\n");
                continue;
            }

            isProcessing = true;
            isDone = false;

            sendAll(client, "IN_PROGRESS\n");

            // calculations in separate thread (Передаємо data по значенню (копіюємо), а змінні стану (&result...) по посиланню)
            thread([data, &result, &isProcessing, &isDone]() mutable {
                Sleep(5000);
                result = calculateNorm(data);
                isProcessing = false;
                isDone = true;
                }).detach();
        }

        // STATUS
        else if (command == "STATUS") {
            if (isProcessing)
                sendAll(client, "IN_PROGRESS\n");
            else if (isDone)
                sendAll(client, "DONE\n");
            else
                sendAll(client, "IDLE\n");
        }

        else if (command == "RESULT") {
            if (isDone) {
                std::string res = "DONE " + std::to_string(result) + "\n";
                sendAll(client, res);
            }
            else if (isProcessing) {
                sendAll(client, "IN_PROGRESS\n");
            }
            else {
                sendAll(client, "ERROR\n");
            }
        }

        else {
            sendAll(client, "ERROR unknown command\n");
        }
    }

    closesocket(client);
}

int main()
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "WSAStartup failed\n";
        return 1;
    }

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) {
        std::cout << "Socket error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cout << "Bind error\n";
        closesocket(server);
        WSACleanup();
        return 1;
    }

    if (listen(server, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Listen error\n";
        closesocket(server);
        WSACleanup();
        return 1;
    }

    std::cout << "Server started...\n";

    while (true) {
        SOCKET client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET) {
            std::cout << "Accept error\n";
            continue;
        }

        std::thread(handleClient, client).detach();
    }

    closesocket(server);
    WSACleanup();
}