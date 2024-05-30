#include <iostream>
#include <ws2tcpip.h>
#include <conio.h>
#include <vector>
#include <fstream>
#include <thread>

#define DEFAULT_BUFLEN 8192
#define DEFAULT_PORT "27015"

using namespace std;

//������ ������� ��� ������������ ��������
SOCKET Connections[100];
SOCKET ServerSocket;
//���������� ������� ��� ������������ �������������
int activeConnections = 0;
bool isServerRunning = true;

//������ ����������� ��� �����
struct User
{
    int id;
    string username;
};

//������������ ������ ��� �������� �������� �������������
vector<User> activeUsers;

//���������� ���������� ������������� ������������
int GenerateUniqueId()
{
    static int idCounter = 0;
    return ++idCounter;
}
//�������, ������� ���������� ��� ������������ � ��� id � ����
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
//������� ��� ���� ����� ������ � ���, ��� ���� �� ������
void Prikol()
{
    ifstream inFile("active_users.txt");
    if (!inFile)
    {
        cerr << "���� �������� ������������� �� ������." << endl;
        return;
    }
}
//������� ��� ������������ ������ �������� �������������
void SendActiveUsersToConnectedClients(SOCKET clientSocket)
{
    string allUserList;
    for (const User &user : activeUsers)
    {
        //���������� id � ��� ������������ � ������ � ��������� �� ����� ������
        allUserList += to_string(user.id) + " " + user.username + "\n";
    }
    //���������� ������ �� ������
    send(clientSocket, allUserList.c_str(), allUserList.size(), 0);
}
//������� ��� ��������� ����� �� ������������
void ReceiveFileFromClient(SOCKET clientSocket)
{
    //��������� ����� ��� �������� ����� �����
    char filename_buf[DEFAULT_BUFLEN] = {0};
    int filename_size;
    if (recv(clientSocket, reinterpret_cast<char*>(&filename_size), sizeof(int), 0) <= 0)
    {
        cerr << "������ ��� ��������� ������� ����� �����." << endl;
        return;
    }
    if (filename_size <= 0 || filename_size >= DEFAULT_BUFLEN)
    {
        cerr << "������������ ������ ����� �����." << endl;
        return;
    }
    if (recv(clientSocket, filename_buf, filename_size, 0) <= 0)
    {
        cerr << "������ ��� ��������� ����� �����." << endl;
        return;
    }
    string filename(filename_buf, filename_size);
    int file_size;
    if (recv(clientSocket, reinterpret_cast<char*>(&file_size), sizeof(int), 0) <= 0)
    {
        cerr << "������ ��� ��������� ������� �����." << endl;
        return;
    }
    int num_chunks;
    if (recv(clientSocket, reinterpret_cast<char*>(&num_chunks), sizeof(int), 0) <= 0)
    {
        cerr << "������ ��� ��������� ���������� ������ �����." << endl;
        return;
    }
    //�������� ����
    TCHAR szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0)
    {
        cerr << "������ ��� ��������� ���� � ������������ �����." << endl;
        return;
    }
    string appDirectory = szPath;
    size_t pos = appDirectory.find_last_of("\\/");
    if (pos != string::npos)
    {
        appDirectory = appDirectory.substr(0, pos);
    }
    //������ ����������
    string saveDirectory = appDirectory + "\\SavedFiles\\";
    CreateDirectory(saveDirectory.c_str(), NULL);
    string filePath = saveDirectory + filename;
    ofstream file(filePath, ios::binary);
    if (!file.is_open())
    {
        cerr << "������ ��� ���������� ����� �� �������: " << filename << endl;
        return;
    }
    for (int i = 0; i < num_chunks; ++i)
    {
        char chunk_buffer[DEFAULT_BUFLEN] = {0};
        int bytesReceived = recv(clientSocket, chunk_buffer, DEFAULT_BUFLEN, 0);
        if (bytesReceived <= 0)
        {
            cerr << "������ ��� ������ ������ �� ������� ��� ������ ��������." << endl;
            file.close();
            return;
        }
        file.write(chunk_buffer, bytesReceived);
    }

    file.close();
    cout << "���� \"" << filename << "\" ������� �������� � ���������� ����������." << endl;
    send(clientSocket, "���� ������� ���������.", strlen("���� ������� ���������."), 0);
}
//�������, ������� ���������� ������ ������
void SendFileListToClient(SOCKET clientSocket)
{
    string file_list;
    string directory = "SavedFiles\\";
    //������ ������ � ��������� ������
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((directory + "*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        //������� ������ �� ���������� ���������� SavedFiles
        cerr << "�� ������� �������� ������ ������ � ����������." << endl;
        //���������� ������ ������������
        string error_message = "���������� �� ������� �� ����������. ���������� ������� ����������.";
        send(clientSocket, error_message.c_str(), error_message.size(), 0);
        return;
    }
    //���� do while ����� ��� ������ ������ � ����������
    do
    {
        const string file_or_dir = findFileData.cFileName;
        if (file_or_dir != "." && file_or_dir != "..")
        {
            file_list += file_or_dir + "\n";
        }
    }
    while (FindNextFile(hFind, &findFileData) != 0);
    FindClose(hFind);
    //���������� ������ ������ ������������
    send(clientSocket, file_list.c_str(), file_list.size(), 0);
}
//������� �������� ����� ������������
void SendFileToClient(SOCKET clientSocket, const string& filename)
{
    //��������� ����� � �����
    string file_path = "SavedFiles\\" + filename;
    //������ ���� � �������� �������
    ifstream file(file_path, ios::binary);
    //���������� ������ �����
    file.seekg(0, ios::end);
    int file_size = file.tellg();
    file.seekg(0, ios::beg);

    send(clientSocket, reinterpret_cast<char*>(&file_size), sizeof(int), 0);
    //���������� ���� �������
    char buffer[DEFAULT_BUFLEN];
    while (!file.eof())
    {
        file.read(buffer, sizeof(buffer));
        int bytes_read = file.gcount();
        send(clientSocket, buffer, bytes_read, 0);
    }
    //��������� ����
    file.close();
}
//������� ���������� ������������
void ClientHandler(SOCKET clientSocket)
{
    char recvbuf[DEFAULT_BUFLEN];
    int bytesReceived;
    while (isServerRunning)
    {
        bytesReceived = recv(clientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0)
        {
            cerr << "������ ��� ������ ������ �� ������� ��� ������ ��������." << endl;
            break;
        }
        //���� ��������� ��������� �� ������������
        string message(recvbuf, bytesReceived);
        if (message == "start_file_transfer")
        {
            ReceiveFileFromClient(clientSocket);
        }
        else if (message == "list")
        {
            SendActiveUsersToConnectedClients(clientSocket);
        }
        else if (message == "list_files")
        {
            SendFileListToClient(clientSocket);
        }
        else if (message == "download_file")
        {
            int filename_size;
            if (recv(clientSocket, reinterpret_cast<char*>(&filename_size), sizeof(int), 0) <= 0)
            {
                cerr << "������ ��� ��������� ������� ����� �����." << endl;
                continue;
            }
            char filename_buf[DEFAULT_BUFLEN] = {0};
            if (recv(clientSocket, filename_buf, filename_size, 0) <= 0)
            {
                cerr << "������ ��� ��������� ����� �����." << endl;
                continue;
            }
            string filename(filename_buf, filename_size);
            SendFileToClient(clientSocket, filename);
        }
        else
        {
            char usernameBuf[DEFAULT_BUFLEN];
            memcpy(usernameBuf, recvbuf, bytesReceived);
            usernameBuf[bytesReceived] = '\0';
            string username(usernameBuf);

            bool isNewUser = true;
            for (const User& user : activeUsers)
            {
                if (user.username == username)
                {
                    isNewUser = false;
                    break;
                }
            }
            //���� ������ ������� ���-�� ����� ���������, �� �� ������ ������ ������������
            if (isNewUser)
            {
                User newUser;
                newUser.id = GenerateUniqueId();
                newUser.username = username;
                activeUsers.push_back(newUser);
                SaveActiveUsersToFile();
                cout << "������������ ��������� � �������. ID: " << newUser.id << ", �������: " << newUser.username << endl;
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
//������� ��������� ������ ������������� ����� �������� �������
void UserInputListener()
{
    string input;
    while (isServerRunning)
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
            isServerRunning = false;
            WSACleanup();
            _getch();
            exit(0);
        }
    }
}

int main()
{
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    SetConsoleTitleA("ADMIN PANEL");

    WSADATA wsaData;
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    WSAStartup(MAKEWORD(2, 2), &wsaData);
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    ServerSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bind(ServerSocket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);

    listen(ServerSocket, SOMAXCONN);

    Prikol();

    thread inputThread(UserInputListener);
    inputThread.detach();

    while (isServerRunning)
    {
        SOCKET ClientSocket = accept(ServerSocket, NULL, NULL);
        if (!isServerRunning) break;
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
