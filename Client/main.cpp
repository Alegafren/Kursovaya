#include <iostream>
#include <ws2tcpip.h>
#include <conio.h>
#include <string>
#include <fstream>
#include <filesystem>

#define IP_ADDRESS "localhost"
#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

#define UP 72
#define DOWN 80
#define ESC 27
#define ENTER 13

using namespace std;

HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

void GoToXYI(short x, short y)
{
    SetConsoleCursorPosition(hStdOut, {x, y});
}

void ConsoleCursorVisible(bool show, short size)
{
    CONSOLE_CURSOR_INFO structCursorInfo;
    GetConsoleCursorInfo(hStdOut, &structCursorInfo);
    structCursorInfo.bVisible = show;
    structCursorInfo.dwSize = size;
    SetConsoleCursorInfo(hStdOut, &structCursorInfo);
}

int main()
{
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    SetConsoleTitleA("User client");
    system("CLS");
    ConsoleCursorVisible(false, 100);
    string Zagolovok = "���������� �������";
    string Menu[] = { "������������ � �������", "�� ������������ � �������", "�����" };
    string UserMenu[] = { "���������� �� �������� �������������", "��������� ���� ������������", "������ ���������� ���������", "������", "�����" };

    char ch;
    bool connected = false;
    bool errorDisplayed = false;
    int active_menu = 0;

    WSADATA wsaData;
    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints;
    SOCKET ConnectSocket = INVALID_SOCKET;
    WSAStartup(MAKEWORD(2,2), &wsaData);
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(IP_ADDRESS, DEFAULT_PORT, &hints, &result);

    while (true)
    {
        int x = 50, y = 10;
        GoToXYI(x, y);

        for (int i = 0; i < (sizeof(Menu) / sizeof(*Menu)); i++)
        {
            if (i == active_menu) SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            else SetConsoleTextAttribute(hStdOut, FOREGROUND_INTENSITY);
            GoToXYI(x, y++);
            cout << Menu[i] << endl;
        }

        ch = _getch();
        if (ch == -32) ch = _getch();

        switch (ch)
        {
        case ESC:
            exit(0);
            break;
        case UP:
            if (active_menu > 0)
                --active_menu;
            break;
        case DOWN:
            if (active_menu < sizeof(Menu) / sizeof(*Menu) - 1)
                ++active_menu;
            break;
        case ENTER:
            if (active_menu == 0)
            {
                system("CLS");
                GoToXYI(x, y);
                SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                cout << "��� ����������� � �������!\n" << endl;
                Sleep(1500);
                system("CLS");

                for(ptr=result; ptr != NULL ; ptr=ptr->ai_next)
                {
                    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
                    if  (connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen) != SOCKET_ERROR)
                    {
                        GoToXYI(x,y);
                        cout << "�� ���� ������� ����������!" << endl;
                        connected = true;
                        Sleep(1500);
                        system("CLS");
                        GoToXYI(30,y);
                        string UserName;
                        cout << "����������, ������� ���� ���, ��� ������� ��� ����� ������ ������ ������������!" << endl;
                        GoToXYI(30,14);
                        cout << "Login: ";
                        ConsoleCursorVisible(true, 10);
                        getline(cin, UserName);
                        send(ConnectSocket, UserName.c_str(), UserName.size(), 0);
                        ConsoleCursorVisible(false, 100);
                        Sleep(1000);
                        system("CLS");
                        while (true)
                        {
                            int x = 50, y = 10;
                            GoToXYI(x, y);
                            for (int i = 0; i < (sizeof(UserMenu) / sizeof(*UserMenu)); i++)
                            {
                                if (i == active_menu) SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                                else SetConsoleTextAttribute(hStdOut, FOREGROUND_INTENSITY);
                                GoToXYI(x, y++);
                                cout << UserMenu[i] << endl;
                            }
                            ch = _getch();
                            if (ch == -32) ch = _getch();
                            switch (ch)
                            {
                            case ESC:
                                exit(0);
                                break;
                            case UP:
                                if (active_menu > 0)
                                    --active_menu;
                                break;
                            case DOWN:
                                if (active_menu < sizeof(UserMenu) / sizeof(*UserMenu) - 1)
                                    ++active_menu;
                                break;
                            case ENTER:
                                if (active_menu == 0)
                                {
                                    SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                                    char recvbuf[DEFAULT_BUFLEN];
                                    const char *request = "list";
                                    send(ConnectSocket, request, strlen(request), 0);
                                    timeval timeout;
                                    timeout.tv_sec = 5;
                                    timeout.tv_usec = 0;
                                    setsockopt(ConnectSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

                                    int bytesReceived = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
                                    if (bytesReceived > 0)
                                    {
                                        recvbuf[bytesReceived] = '\0';
                                        cout << "�������� ������������:\n";
                                        cout << recvbuf << endl;
                                    }
                                    else
                                    {
                                        cerr << "������ ��� ��������� ������ �� �������." << endl;
                                    }

                                    active_menu = 0;
                                }
                                else if (active_menu == 1)
                                {
                                    system("CLS");
                                    GoToXYI(x, y);
                                    string recipientId;
                                    cout << "������� ID ���������� �����: ";
                                    ConsoleCursorVisible(true, 10);
                                    getline(cin, recipientId);
                                    send(ConnectSocket, recipientId.c_str(), recipientId.size(), 0);
                                    ConsoleCursorVisible(false, 100);
                                    system("CLS");
                                    GoToXYI(x, y);
                                    cout << "������ �� ��������� ������ ������ ���������...\n";
                                    Sleep(1500);
                                    system("CLS");
                                    // ���������� ������ �� ��������� ������ ������ �� �������
                                    const char *request_list = "list";
                                    send(ConnectSocket, request_list, strlen(request_list), 0);

                                    // �������� ������ ������ � ��������� � �������
                                    char recvbuf[DEFAULT_BUFLEN];
                                    int bytesReceived = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
                                    if (bytesReceived > 0)
                                    {
                                        recvbuf[bytesReceived] = '\0';
                                        cout << "������ ������ � ��������� �� �������:\n";
                                        cout << recvbuf << endl;
                                    }
                                    else
                                    {
                                        cerr << "������ ��� ��������� ������ �� �������." << endl;
                                    }
                                    cout << "�������� ���� ��� �������� (������� ���������� ���� ��� ��� ����� �� ������): ";
                                    string filePath;
                                    ConsoleCursorVisible(true, 10);
                                    getline(cin, filePath);
                                    ConsoleCursorVisible(false, 100);

                                    ifstream file(filePath, ios::binary | ios::ate);
                                    if (!file.is_open())
                                    {
                                        cout << "�� ������� ������� ����: \n" << filePath << endl;
                                        return 1;
                                    }

                                    // ��������� ����� ����� � ��� �������
                                    string file_name = filePath.substr(filePath.find_last_of("/\\") + 1);
                                    int file_size = file.tellg();
                                    file.seekg(0, ios::beg);

                                    // ��������� ������ ��� ������ ����� � ������ ����� � �����
                                    char* file_buffer = new char[file_size];
                                    file.read(file_buffer, file_size);
                                    file.close();

                                    // �������� ����� �� ������
                                    const char *request_file = "start_file_transfer";
                                    send(ConnectSocket, request_file, strlen(request_file), 0);
                                    send(ConnectSocket, reinterpret_cast<char*>(&file_name), sizeof(int), 0);
                                    send(ConnectSocket, reinterpret_cast<char*>(&file_size), sizeof(int), 0);
                                    send(ConnectSocket, file_buffer, file_size, 0);

                                    // ������������ ������, ���������� ��� �����
                                    delete[] file_buffer;

                                    cout << "���� \"" << file_name << "\" ������� ��������� �� ������." << endl;
                                    char status[DEFAULT_BUFLEN];
                                    bytesReceived = recv(ConnectSocket, status, DEFAULT_BUFLEN, 0);
                                    if (bytesReceived > 0)
                                    {
                                        status[bytesReceived] = '\0';
                                        cout << "������ �������� �����: " << status << endl;
                                    }
                                    else
                                    {
                                        cerr << "������ ��� ��������� ������� �������� ����� �� �������." << endl;
                                    }

                                    // ����������� ��������� ��� ���� ���� ��� ��������� � ������� ����
                                    char answer;
                                    cout << "������ ��������� ��� ���� ����? (�/�): ";
                                    ConsoleCursorVisible(true, 10);
                                    cin >> answer;
                                    cin.ignore();
                                    if (answer == '�' || answer == '�')
                                    {
                                        ConsoleCursorVisible(false, 100);
                                        active_menu = 0; // ������� � ������� ����
                                    }
                                    else if (answer != '�' && answer != '�')
                                    {
                                        cout << "������������ ����. ����������, ���������� �����." << endl;
                                    }
                                    ConsoleCursorVisible(false, 100);
                                }
                                else if (active_menu == 2)
                                {
                                    system("CLS");
                                    SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                                    GoToXYI(x,y);
                                    cout << "�����-�� ��� ����� ��������� ��������� ���������\n" << endl;
                                }
                                else if (active_menu == 3)
                                {
                                    system("CLS");
                                    SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                                    GoToXYI(x,y);
                                    cout << "������! ���� ������������ ���������, ������� �������� ����� ���� ������� ������������!\n" << endl;
                                    GoToXYI(10,11);
                                    cout << "� ������� ���� ��������� �� ������ ���������� ����� ����� ����� 1��! �������������� ����� ��������!\n" << endl;
                                    GoToXYI(10,12);
                                    cout << "�� ����� ���� ����� ������ ��� ���������, ������ ��� ��� �� �������� �� ��������� �������� TCP, �� UDP\n" << endl;
                                    GoToXYI(10,13);
                                    cout << "��� �� ������������� ������� ���������, ��� ����� ��������������, ������ ��� � ����� ���� ����� ������ ������ ���� ��� �� ���� ������(\n" << endl;
                                    _getch();
                                    system("CLS");
                                }
                                else if (active_menu == 4)
                                {
                                    exit(0);
                                }
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (!errorDisplayed)
                        {
                            GoToXYI(50,11);
                            cerr << "������ ��� ������� ����������� � �������: " << WSAGetLastError() + 404 << endl;
                            GoToXYI(50,12);
                            cout << "���������� ������������ �������!" << endl;
                            errorDisplayed = true;
                        }
                    }
                }
                _getch();
                system("CLS");
            }
            else if (active_menu == 1)
            {
                system("CLS");
                GoToXYI(x, y);
                SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                if (!connected)
                {
                    cout << "��� ���������� ������ ��������� ����� ����������� � �������\n";
                }
                else
                {
                    cout << "�� ��� ����������!\n";
                    Sleep(1500);
                }
                if(!connected)
                {
                    GoToXYI(52, 14);
                    char answer;
                    cout << "�� ������ ���������� ������������� ���������? (�/�): ";
                    ConsoleCursorVisible(true, 10);
                    cin >> answer;
                    cin.ignore();
                    if (answer == '�' || answer == '�')
                    {
                        ConsoleCursorVisible(false, 100);
                        exit(0);
                    }
                    else if (answer != '�' && answer != '�')
                    {
                        GoToXYI(62, 15);
                        cout << "������������ ����. ���������� �����!\n";
                        ConsoleCursorVisible(false, 100);
                        _getch();
                        system("CLS");
                        continue;
                    }
                    ConsoleCursorVisible(false, 100);
                    system("CLS");
                    GoToXYI(x, y);
                    cout << "����������� �� ������� ����...\n";
                    _getch();
                    system("CLS");
                    active_menu = 0;
                }
                system("CLS");
            }
            else if (active_menu == 2)
            {
                exit(0);
            }
            break;
        }
    }
    return 0;
}
