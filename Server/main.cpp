#include <iostream>
#include <ws2tcpip.h>
#include <vector>
#include <fstream>
#include <thread>

#define DEFAULT_BUFLEN 8192
#define DEFAULT_PORT "27015"

using namespace std;

SOCKET Connections[100];
SOCKET ServerSocket;
int activeConnections = 0;
bool activeUsersChanged = false;

struct User
{
    int id;
    string username;
};

int GenerateUniqueId()
{
    static int idCounter = 0;
    return ++idCounter;
}

vector<User> activeUsers;

void SaveActiveUsersToFile()
{
    ofstream outFile("active_users.txt", ios::trunc);
    if (!outFile)
    {
        cerr << "������ ��� �������� ����� ��� ������!" << endl;
        return;
    }

    for (const User &user : activeUsers)
    {
        outFile << user.id << " " << user.username << endl;
    }

    outFile.close();
}

void LoadActiveUsersFromFile()
{
    ifstream inFile("active_users.txt");
    if (!inFile)
    {
        cerr << "���� �������� ������������� �� ������." << endl;
        return;
    }

    int id;
    string username;
    while (inFile >> id >> username)
    {
        User user;
        user.id = id;
        user.username = username;
        activeUsers.push_back(user);
    }

    inFile.close();
}

void SendActiveUsersToConnectedClients(SOCKET clientSocket)
{
    string allUserList;
    for (const User &user : activeUsers)
    {
        allUserList += to_string(user.id) + " " + user.username + "\n";
    }

    send(clientSocket, allUserList.c_str(), allUserList.size(), 0);
}

void SendActiveUsersToAllClients()
{
    if (activeUsersChanged)
    {
        for (int i = 0; i < activeConnections; ++i)
        {
            SendActiveUsersToConnectedClients(Connections[i]);
        }
        activeUsersChanged = false;
    }
}

void ReceiveFileFromClient(SOCKET clientSocket)
{
    // �������� ��� �����
    char filename_buf[DEFAULT_BUFLEN];
    int filename_size;
    recv(clientSocket, reinterpret_cast<char*>(&filename_size), sizeof(int), 0);
    recv(clientSocket, filename_buf, filename_size, 0);
    string filename(filename_buf, filename_size);

    // �������� ������ �����
    long long total_file_size;
    recv(clientSocket, reinterpret_cast<char*>(&total_file_size), sizeof(long long), 0);

    // �������� ���������� ������ �����
    int num_chunks;
    recv(clientSocket, reinterpret_cast<char*>(&num_chunks), sizeof(int), 0);

    // ���������� ���������� ��� ���������� ����� (������� ���� ������������)
    string saveDirectory = "C:\\Users\\%Public%\\Desktop\\";

    // ������ ���� � ����� �� ������� �����
    string filePath = saveDirectory + filename;

    // ��������� ���� ��� ������ � �������� ������
    ofstream file(filePath, ios::binary);
    if (!file.is_open())
    {
        cerr << "������ ��� ���������� ����� �� �������: " << filename << endl;
        return;
    }

    // ��������� � ���������� ������ ����� �����
    for (int i = 0; i < num_chunks; ++i)
    {
        char chunk_buffer[DEFAULT_BUFLEN];
        int bytesReceived = recv(clientSocket, chunk_buffer, DEFAULT_BUFLEN, 0);
        if (bytesReceived <= 0)
        {
            cerr << "������ ��� ������ ������ �� ������� ��� ������ ��������." << endl;
            file.close();
            return;
        }
        file.write(chunk_buffer, bytesReceived);
    }

    // ��������� ����
    file.close();
    // ������� ��������� � �������� ���������� �����
    cout << "���� \"" << filename << "\" ������� �������� �� ������� �����." << endl;
}

void ClientHandler(SOCKET clientSocket)
{
    char recvbuf[DEFAULT_BUFLEN];
    int bytesReceived;
    while (true)
    {
        bytesReceived = recv(clientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0)
        {
            cerr << "������ ��� ������ ������ �� ������� ��� ������ ��������." << endl;
            break;
        }

        string message(recvbuf, bytesReceived);
        if (message == "start_file_transfer")
        {
            ReceiveFileFromClient(clientSocket);
        }
        else if (message == "list")
        {
            SendActiveUsersToConnectedClients(clientSocket);
        }
        else
        {
            char usernameBuf[DEFAULT_BUFLEN];
            memcpy(usernameBuf, recvbuf, bytesReceived);
            usernameBuf[bytesReceived] = '\0';
            string username(usernameBuf);

            bool isNewUser = true;
            for (const User &user : activeUsers)
            {
                if (user.username == username)
                {
                    isNewUser = false;
                    break;
                }
            }
            if (isNewUser)
            {
                User newUser;
                newUser.id = GenerateUniqueId();
                newUser.username = username;
                activeUsers.push_back(newUser);
                SaveActiveUsersToFile();
                cout << "������������ ��������� � �������. ID: " << newUser.id << ", �������: " << newUser.username << endl;
                activeUsersChanged = true;
                SendActiveUsersToAllClients();
            }
        }
    }

    closesocket(clientSocket);
    for (int i = 0; i < activeConnections; ++i)
    {
        if (Connections[i] == clientSocket)
        {
            for (int j = i; j < activeConnections - 1; ++j)
            {
                Connections[j] = Connections[j + 1];
            }
            break;
        }
    }
    activeConnections--;
}

void UserInputListener()
{
    string input;
    while (true)
    {
        cout << "������� 'exit', ����� ��������� ������ �������: ";
        cin >> input;
        if (input == "exit")
        {
            cout << "���������� ����� active_users.txt:" << endl;
            ifstream inFile("active_users.txt");
            string line;
            while (getline(inFile, line))
            {
                cout << line << endl;
            }
            inFile.close();
            ofstream clearFile("active_users.txt", ios::trunc);
            clearFile.close();
            WSACleanup();
            exit(0);
        }
    }
}

int main()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    SetConsoleTitleA("ADMIN");

    WSADATA wsaData;
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    WSAStartup(MAKEWORD(2, 2), &wsaData);
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    ServerSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bind(ServerSocket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);

    listen(ServerSocket, SOMAXCONN);

    LoadActiveUsersFromFile();

    thread inputThread(UserInputListener);
    inputThread.detach();

    while (true)
    {
        SOCKET ClientSocket = accept(ServerSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET)
        {
            cerr << "������ ��� �������� �����������." << endl;
            continue;
        }

        activeConnections++;

        thread clientThread(ClientHandler, ClientSocket);
        clientThread.detach();
    }

    return 0;
}
