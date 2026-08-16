#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <conio.h>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>

#define IP_ADDRESS "localhost"
#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 8192

#define UP 72
#define DOWN 80
#define ESC 27
#define ENTER 13

using namespace std;

// Дескриптор консоли используется для отображения меню.
HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

// Функция для перемещения курсора в консоли
void GoToXYI(short x, short y)
{
    SetConsoleCursorPosition(hStdOut, {x, y});
}

//Функция для регулирования отображения каретки консоли
void ConsoleCursorVisible(bool show, short size)
{
    CONSOLE_CURSOR_INFO structCursorInfo;
    GetConsoleCursorInfo(hStdOut, &structCursorInfo);
    structCursorInfo.bVisible = show;
    structCursorInfo.dwSize = size;
    SetConsoleCursorInfo(hStdOut, &structCursorInfo);
}

// Задает фиксированный размер консольного окна и запрещает изменение его размера.
void SetFixedConsoleWindow(short width, short height)
{
    HANDLE consoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    SMALL_RECT targetWindow = {0, 0, static_cast<short>(width - 1), static_cast<short>(height - 1)};
    SetConsoleWindowInfo(consoleOutput, TRUE, &targetWindow);

    COORD bufferSize = {width, height};
    SetConsoleScreenBufferSize(consoleOutput, bufferSize);
    SetConsoleWindowInfo(consoleOutput, TRUE, &targetWindow);

    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow != NULL)
    {
        LONG style = GetWindowLongA(consoleWindow, GWL_STYLE);
        style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
        SetWindowLongA(consoleWindow, GWL_STYLE, style);
        SetWindowPos(consoleWindow, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

int main()
{
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    SetConsoleTitleA("FILE TRANSFER CLIENT");
    SetFixedConsoleWindow(120, 30);
    system("CLS");

    ConsoleCursorVisible(false, 100);
    string Menu[] = { "Подключиться к серверу", "Не подключаться к серверу", "Выход" };
    string UserMenu[] = { "Информация об активных пользователях", "Отправить файл на сервер", "Статус серверного хранилища", "Помощь", "Выход" };

    char ch;
    bool connected = false;
    bool errorDisplayed = false;
    int active_menu = 0;

    // Инициализация Winsock
    WSADATA wsaData;
    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints;
    SOCKET ConnectSocket = INVALID_SOCKET;
    WSAStartup(MAKEWORD(2,2), &wsaData);
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;

    if (getaddrinfo(IP_ADDRESS, DEFAULT_PORT, &hints, &result) != 0)
    {
        cerr << "Не удалось определить адрес сервера." << endl;
        WSACleanup();
        return 1;
    }

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
                    //Инициализация сокета с параметрами, определфыми в ptr
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
                        //Отправляем ник пользователя на сервер
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
                                    GoToXYI(x, y);
                                    SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                                    char recvbuf[DEFAULT_BUFLEN];
                                    const char *request = "list";
                                    send(ConnectSocket, request, strlen(request), 0); //Отправляем реквест серверу о том, что надо кинуть список пользователей
                                    //Задаём таймаут для приема данных от сервера, для обновления информации об пользователях
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
                                }
                                else if (active_menu == 1)
                                {
                                    system("CLS");
                                    SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                                    // Обновляем состояние соединения перед отправкой файла.
                                    const char *request_list = "list";
                                    send(ConnectSocket, request_list, strlen(request_list), 0);
                                    char recvbuf[DEFAULT_BUFLEN];
                                    int bytesReceived = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);

                                    string filePath;
                                    ifstream file;
                                    while (true)
                                    {
                                        GoToXYI(35, 14);
                                        cout << "Выберите файл для отправки (введите абсолютный путь): ";
                                        ConsoleCursorVisible(true, 10);
                                        getline(cin, filePath);
                                        ConsoleCursorVisible(false, 100);
                                        //Открываем файл бинарно
                                        file.open(filePath, ios::binary | ios::ate);
                                        if (!file.is_open())
                                        {
                                            system("CLS");
                                            GoToXYI(35, 12);
                                            cout << "Не удалось открыть файл или путь написан некорректно: " << filePath << endl;
                                            GoToXYI(35, 13);
                                            cout << "Пожалуйста, попробуйте снова." << endl;
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }
                                    //Извлекаем имя файла из абсолютного пути
                                    //Находим последний символ / или \\ и извлекаем имя файла
                                    string file_name = filePath.substr(filePath.find_last_of("/\\") + 1);
                                    //Определяем размер файла
                                    //https://www.youtube.com/watch?v=gZpVut4KUbo
                                    //Откуда был взят код для передачи файла
                                    int file_size = file.tellg();
                                    //Начинаем читать файл с начала
                                    file.seekg(0, ios::beg);
                                    //СОздаём буфер для содержимого файла
                                    char* file_buffer = new char[file_size];
                                    file.read(file_buffer, file_size);
                                    //Закрываем файл
                                    file.close();
                                    //Кидаем реквест серверу на начало отправки файла на сервер
                                    const char *request_file = "start_file_transfer";
                                    send(ConnectSocket, request_file, strlen(request_file), 0);
                                    //Отправляем имя файла и его размер на сервер
                                    int filename_size = file_name.size();
                                    send(ConnectSocket, reinterpret_cast<char*>(&filename_size), sizeof(int), 0);
                                    send(ConnectSocket, file_name.c_str(), filename_size, 0);
                                    send(ConnectSocket, reinterpret_cast<char*>(&file_size), sizeof(int), 0);
                                    //Начинаем отправлять файл частями
                                    const int chunk_size = DEFAULT_BUFLEN;
                                    int num_chunks = (file_size + chunk_size - 1) / chunk_size;
                                    send(ConnectSocket, reinterpret_cast<char*>(&num_chunks), sizeof(int), 0);

                                    bool transferFailed = false;
                                    for (int i = 0; i < num_chunks && !transferFailed; ++i)
                                    {
                                        int current_chunk_size = std::min(chunk_size, file_size - i * chunk_size);
                                        int sent = 0;

                                        while (sent < current_chunk_size)
                                        {
                                            int bytesSent = send(
                                                ConnectSocket,
                                                file_buffer + i * chunk_size + sent,
                                                current_chunk_size - sent,
                                                0
                                            );

                                            if (bytesSent == SOCKET_ERROR)
                                            {
                                                transferFailed = true;
                                                break;
                                            }
                                            sent += bytesSent;
                                        }
                                    }

                                    // Удаляем динамическую память, выделенную под файл.
                                    delete[] file_buffer;

                                    if (transferFailed)
                                    {
                                        system("CLS");
                                        GoToXYI(35, 14);
                                        cerr << "Ошибка при передаче файла. Код Winsock: " << WSAGetLastError() << endl;
                                        _getch();
                                        system("CLS");
                                        continue;
                                    }

                                    system("CLS");
                                    GoToXYI(35, 14);
                                    cout << "Файл \"" << file_name << "\" успешно отправлен на сервер." << endl;
                                    //Получаем статус доставки файла
                                    char status[DEFAULT_BUFLEN];
                                    bytesReceived = recv(ConnectSocket, status, DEFAULT_BUFLEN, 0);
                                    if (bytesReceived > 0)
                                    {
                                        GoToXYI(35, 13);
                                        status[bytesReceived] = '\0';
                                        cout << "Статус доставки файла: " << status << endl;
                                        _getch();
                                        system("CLS");
                                    }
                                    else
                                    {
                                        GoToXYI(35, 13);
                                        cerr << "Ошибка при получении статуса доставки файла от сервера." << endl;
                                        _getch();
                                        system("CLS");
                                    }
                                }
                                else if (active_menu == 2)
                                {
                                    system("CLS");
                                    GoToXYI(45, 10);
                                    SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                                    cout << "Запрос списка файлов на сервере...\n";
                                    Sleep(800);
                                    system("CLS");

                                    const char *request_list_files = "list_files";
                                    send(ConnectSocket, request_list_files, strlen(request_list_files), 0);

                                    char recvbuf[DEFAULT_BUFLEN + 1] = {0};
                                    int bytesReceived = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
                                    if (bytesReceived > 0)
                                    {
                                        recvbuf[bytesReceived] = '\0';
                                        string receivedMessage(recvbuf);

                                        if (receivedMessage.find("Серверное хранилище пусто") != string::npos)
                                        {
                                            GoToXYI(43, 12);
                                            cout << receivedMessage << endl;
                                            _getch();
                                            system("CLS");
                                            active_menu = 0;
                                            continue;
                                        }

                                        vector<string> files;
                                        string line;
                                        stringstream fileListStream(receivedMessage);
                                        while (getline(fileListStream, line))
                                        {
                                            if (!line.empty() && line.back() == '\r')
                                                line.pop_back();
                                            if (!line.empty())
                                                files.push_back(line);
                                        }

                                        if (files.empty())
                                        {
                                            cout << "На сервере нет файлов." << endl;
                                            _getch();
                                            system("CLS");
                                            continue;
                                        }

                                        cout << "Файлы на сервере:\n\n";
                                        for (size_t i = 0; i < files.size(); ++i)
                                            cout << i + 1 << ". " << files[i] << endl;

                                        cout << "\nВведите номер файла для загрузки: ";
                                        ConsoleCursorVisible(true, 10);

                                        string choiceText;
                                        getline(cin, choiceText);
                                        ConsoleCursorVisible(false, 100);

                                        int choice = 0;
                                        try
                                        {
                                            size_t parsed = 0;
                                            choice = stoi(choiceText, &parsed);
                                            if (parsed != choiceText.size())
                                                choice = 0;
                                        }
                                        catch (...)
                                        {
                                            choice = 0;
                                        }

                                        if (choice < 1 || choice > static_cast<int>(files.size()))
                                        {
                                            cout << "Неверный номер файла." << endl;
                                            _getch();
                                            system("CLS");
                                            continue;
                                        }

                                        string fileName = files[choice - 1];

                                        const char *request_download_file = "download_file";
                                        send(ConnectSocket, request_download_file, strlen(request_download_file), 0);

                                        int filename_size = static_cast<int>(fileName.size());
                                        send(ConnectSocket, reinterpret_cast<char*>(&filename_size), sizeof(int), 0);
                                        send(ConnectSocket, fileName.c_str(), filename_size, 0);

                                        int file_size;
                                        bytesReceived = recv(ConnectSocket, reinterpret_cast<char*>(&file_size), sizeof(int), 0);
                                        if (bytesReceived <= 0 || file_size < 0)
                                        {
                                            cout << "Файл не найден на сервере или соединение было прервано." << endl;
                                            _getch();
                                            system("CLS");
                                            continue;
                                        }

                                        ofstream outFile(fileName, ios::binary);
                                        if (!outFile)
                                        {
                                            cerr << "Ошибка при открытии файла для записи." << endl;
                                            _getch();
                                            system("CLS");
                                            continue;
                                        }

                                        char file_buffer[DEFAULT_BUFLEN];
                                        int total_bytes_received = 0;
                                        while (total_bytes_received < file_size)
                                        {
                                            int bytesToReceive = min(DEFAULT_BUFLEN, file_size - total_bytes_received);
                                            bytesReceived = recv(ConnectSocket, file_buffer, bytesToReceive, 0);
                                            if (bytesReceived <= 0)
                                            {
                                                cerr << "Ошибка при получении данных от сервера." << endl;
                                                break;
                                            }

                                            outFile.write(file_buffer, bytesReceived);
                                            total_bytes_received += bytesReceived;
                                        }
                                        outFile.close();

                                        if (total_bytes_received == file_size)
                                            cout << "Файл \"" << fileName << "\" успешно загружен." << endl;
                                        else
                                            cout << "Файл загружен не полностью." << endl;

                                        _getch();
                                        system("CLS");
                                    }
                                    else
                                    {
                                        cerr << "Не удалось получить список файлов с сервера." << endl;
                                        _getch();
                                        system("CLS");
                                    }
                                }
                                else if (active_menu == 3)
                                {
                                    system("CLS");
                                    SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

                                    GoToXYI(7, 10);
                                    cout << "Перед началом работы должен быть запущен сервер." << endl;
                                    GoToXYI(7, 12);
                                    cout << "Управление: стрелки UP/DOWN - выбор пункта, ENTER - подтвердить, ESC - выход." << endl;
                                    GoToXYI(7, 14);
                                    cout << "При отправке файла указывайте полный путь к нему, например: C:\\Users\\User\\Desktop\\file.txt" << endl;
                                    GoToXYI(7, 16);
                                    cout << "При скачивании выберите нужный файл из списка по его номеру." << endl;
                                    GoToXYI(7, 18);
                                    cout << "Для возврата в меню нажмите любую клавишу." << endl;

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
                        closesocket(ConnectSocket);
                        ConnectSocket = INVALID_SOCKET;
                        if (!errorDisplayed)
                        {
                            GoToXYI(50,11);
                            cerr << "Ошибка при попытке подключения к серверу. Код Winsock: " << WSAGetLastError() << endl;
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
                    //Раньше я подразумевал, что пользователь сможет попасть на первый экран меню
                    //Но когда продолжил писать меню понял, что реализовать такое Я не могу
                    //Даже, наверное, просто не представляю
                    cout << "Вы уже подключены!\n";
                    Sleep(1500);
                }
                if(!connected)
                {
                    GoToXYI(52, 14);
                    //Проверка на дальнейшее использование программы
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
