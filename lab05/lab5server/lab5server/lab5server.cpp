#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080

using namespace std;

// thread control
atomic<int> activeThreads(0);
const int MAX_THREADS = 100;

bool sendAll(SOCKET sock, const string& data) {
    int total = 0;
    int len = data.size();

    while (total < len) {
        int sent = send(sock, data.c_str() + total, len - total, 0);
        if (sent == SOCKET_ERROR) return false;
        total += sent;
    }
    return true;
}

string readFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) return "";

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void sendResponse(SOCKET client, const string& status, const string& content) {
    string response =
        "HTTP/1.1 " + status + "\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + to_string(content.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        content;

    sendAll(client, response);
}

void handleClient(SOCKET client) {
    char buffer[4096];
    string request;
    
    while (true) {
        int bytes = recv(client, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            closesocket(client);
            activeThreads--;
            return;
        }

        request.append(buffer, bytes);

        // end of HTTP headers
        if (request.find("\r\n\r\n") != string::npos)
            break;
    }

    // get path
    size_t pos = request.find("GET ");
    if (pos == string::npos) {
        closesocket(client);
        activeThreads--;
        return;
    }

    size_t start = pos + 4;
    size_t end = request.find(" ", start);

    string path = request.substr(start, end - start);

    if (path == "/")
        path = "/index.html";

    string filename = "." + path;

    string content = readFile(filename);

    if (content.empty()) {
        sendResponse(client, "404 Not Found", "<h1>404 Not Found</h1>");
    }
    else {
        sendResponse(client, "200 OK", content);
    }

    closesocket(client);
    activeThreads--;
}

int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) {
        cerr << "Socket error\n";
        return 1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cerr << "Bind failed\n";
        return 1;
    }

    if (listen(server, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Listen failed\n";
        return 1;
    }

    cout << "Server running on http://localhost:" << PORT << endl;

    while (true) {
        SOCKET client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET) continue;

        // limit threads (important for jMeter)
        if (activeThreads >= MAX_THREADS) {
            closesocket(client);
            continue;
        }

        activeThreads++;

        thread([client]() {
            handleClient(client);
            }).detach();
    }

    closesocket(server);
    WSACleanup();
}
