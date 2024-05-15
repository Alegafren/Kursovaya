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
    string Zagolovok = "Московский Политех";
    string Menu[] = { "Подключиться к серверу", "Не подключаться к серверу", "Выход" };
    string UserMenu[] = { "Информация об активных пользователях", "Отправить файл пользователю", "Статус серверного хранилища", "Помощь", "Выход" };

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
                cout << "Идёт подключение к серверу!\n" << endl;
                Sleep(1500);
                system("CLS");

                for(ptr=result; ptr != NULL ; ptr=ptr->ai_next)
                {
                    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
                    if  (connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen) != SOCKET_ERROR)
                    {
                        GoToXYI(x,y);
                        cout << "Вы были успешно подключены!" << endl;
                        connected = true;
                        Sleep(1500);
                        system("CLS");
                        GoToXYI(30,y);
                        string UserName;
                        cout << "Пожалуйста, укажите свой ник, под которым вас будут видеть другие пользователи!" << endl;
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
                                        cout << "Активные пользователи:\n";
                                        cout << recvbuf << endl;
                                    }
                                    else
                                    {
                                        cerr << "Ошибка при получении данных от сервера." << endl;
                                    }

                                    active_menu = 0;
                                }
                                else if (active_menu == 1)
                                {
                                    system("CLS");
                                    GoToXYI(x, y);
                                    string recipientId;
                                    cout << "Введите ID получателя файла: ";
                                    ConsoleCursorVisible(true, 10);
                                    getline(cin, recipientId);
                                    send(ConnectSocket, recipientId.c_str(), recipientId.size(), 0);
                                    ConsoleCursorVisible(false, 100);
                                    system("CLS");
                                    GoToXYI(x, y);
                                    cout << "Запрос на получение списка файлов отправлен...\n";
                                    Sleep(1500);
                                    system("CLS");
                                    // Отправляем запрос на получение списка файлов на сервере
                                    const char *request_list = "list";
                                    send(ConnectSocket, request_list, strlen(request_list), 0);

                                    // Получаем список файлов и каталогов с сервера
                                    char recvbuf[DEFAULT_BUFLEN];
                                    int bytesReceived = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
                                    if (bytesReceived > 0)
                                    {
                                        recvbuf[bytesReceived] = '\0';
                                        cout << "Список файлов и каталогов на сервере:\n";
                                        cout << recvbuf << endl;
                                    }
                                    else
                                    {
                                        cerr << "Ошибка при получении данных от сервера." << endl;
                                    }
                                    cout << "Выберите файл для отправки (введите абсолютный путь или имя файла из списка): ";
                                    string filePath;
                                    ConsoleCursorVisible(true, 10);
                                    getline(cin, filePath);
                                    ConsoleCursorVisible(false, 100);

                                    ifstream file(filePath, ios::binary | ios::ate);
                                    if (!file.is_open())
                                    {
                                        cout << "Не удалось открыть файл: \n" << filePath << endl;
                                        return 1;
                                    }

                                    // Получение имени файла и его размера
                                    string file_name = filePath.substr(filePath.find_last_of("/\\") + 1);
                                    int file_size = file.tellg();
                                    file.seekg(0, ios::beg);

                                    // Выделение памяти для буфера файла и чтение файла в буфер
                                    char* file_buffer = new char[file_size];
                                    file.read(file_buffer, file_size);
                                    file.close();

                                    // Отправка файла на сервер
                                    const char *request_file = "start_file_transfer";
                                    send(ConnectSocket, request_file, strlen(request_file), 0);
                                    send(ConnectSocket, reinterpret_cast<char*>(&file_name), sizeof(int), 0);
                                    send(ConnectSocket, reinterpret_cast<char*>(&file_size), sizeof(int), 0);
                                    send(ConnectSocket, file_buffer, file_size, 0);

                                    // Освобождение памяти, выделенной под буфер
                                    delete[] file_buffer;

                                    cout << "Файл \"" << file_name << "\" успешно отправлен на сервер." << endl;
                                    char status[DEFAULT_BUFLEN];
                                    bytesReceived = recv(ConnectSocket, status, DEFAULT_BUFLEN, 0);
                                    if (bytesReceived > 0)
                                    {
                                        status[bytesReceived] = '\0';
                                        cout << "Статус доставки файла: " << status << endl;
                                    }
                                    else
                                    {
                                        cerr << "Ошибка при получении статуса доставки файла от сервера." << endl;
                                    }

                                    // Предложение отправить еще один файл или вернуться в главное меню
                                    char answer;
                                    cout << "Хотите отправить еще один файл? (Д/Н): ";
                                    ConsoleCursorVisible(true, 10);
                                    cin >> answer;
                                    cin.ignore();
                                    if (answer == 'Н' || answer == 'н')
                                    {
                                        ConsoleCursorVisible(false, 100);
                                        active_menu = 0; // Возврат в главное меню
                                    }
                                    else if (answer != 'Д' && answer != 'д')
                                    {
                                        cout << "Некорректный ввод. Пожалуйста, попробуйте снова." << endl;
                                    }
                                    ConsoleCursorVisible(false, 100);
                                }
                                else if (active_menu == 2)
                                {
                                    system("CLS");
                                    SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                                    GoToXYI(x,y);
                                    cout << "Когда-то тут может появиться серверное хранилище\n" << endl;
                                }
                                else if (active_menu == 3)
                                {
                                    system("CLS");
                                    SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                                    GoToXYI(x,y);
                                    cout << "Привет! Тебя приветствует программа, которая передаст любой файл другому пользователю!\n" << endl;
                                    GoToXYI(10,11);
                                    cout << "С помощью этой программы ты можешь передавать файлы весом более 1ГБ! Поддерживается много форматов!\n" << endl;
                                    GoToXYI(10,12);
                                    cout << "Не бойся твои файлы придут без искажений, потому что для их передачи мы исользуем протокол TCP, не UDP\n" << endl;
                                    GoToXYI(10,13);
                                    cout << "Это не окончательный вариант программы, она будет дорабатываться, потому что с таким меню можно нажать только один раз на одну кнопку(\n" << endl;
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
                            cerr << "Ошибка при попытке подключения к серверу: " << WSAGetLastError() + 404 << endl;
                            GoToXYI(50,12);
                            cout << "Попробуйте подключиться позднее!" << endl;
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
                    cout << "Для дальнейшей работы программы нужно подключение к серверу\n";
                }
                else
                {
                    cout << "Вы уже подключены!\n";
                    Sleep(1500);
                }
                if(!connected)
                {
                    GoToXYI(52, 14);
                    char answer;
                    cout << "Вы хотите продолжить использование программы? (Д/Н): ";
                    ConsoleCursorVisible(true, 10);
                    cin >> answer;
                    cin.ignore();
                    if (answer == 'Н' || answer == 'н')
                    {
                        ConsoleCursorVisible(false, 100);
                        exit(0);
                    }
                    else if (answer != 'Д' && answer != 'д')
                    {
                        GoToXYI(62, 15);
                        cout << "Некорректный ввод. Попробуйте снова!\n";
                        ConsoleCursorVisible(false, 100);
                        _getch();
                        system("CLS");
                        continue;
                    }
                    ConsoleCursorVisible(false, 100);
                    system("CLS");
                    GoToXYI(x, y);
                    cout << "Возвращение на главное меню...\n";
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
