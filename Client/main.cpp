#include <iostream>
#include <ws2tcpip.h>
#include <conio.h>
#include <string>
#include <fstream>
#include <filesystem>

#define IP_ADDRESS "localhost"
#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 8192

#define UP 72
#define DOWN 80
#define ESC 27
#define ENTER 13

using namespace std;

//Урок для создания этого меню
//https://www.youtube.com/watch?v=AxzaXJMBfi4
//Поучение дескриптора консоли для дальнейшей работы с меню
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

int main()
{
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    SetConsoleTitleA("USER CLIENT");
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
                    //Инициализация сокета с параметрами, определфыми в ptr
                    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
                    if  (connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen) != SOCKET_ERROR)
                    {
                        GoToXYI(x,y);
                        cout << "Вы были успешно подключены!" << endl;
                        connected = true; //Флаг подлючения, до которого никогда никто не дойдёт
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
                                    //Честно без этого листа код программы дальше не воспроизводился
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

                                    for (int i = 0; i < num_chunks; ++i)
                                    {
                                        int current_chunk_size = std::min(chunk_size, file_size - i * chunk_size);
                                        send(ConnectSocket, file_buffer + i * chunk_size, current_chunk_size, 0);
                                    }

                                    //Удаляем динамическую память выделенную на передачу файла
                                    delete[] file_buffer;

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
                                    GoToXYI(50, 12);
                                    SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                                    cout << "Запрос списка файлов на сервере...\n";
                                    Sleep(1500);
                                    system("CLS");
                                    //Запрашиваем список файлов доступных на сервере
                                    const char *request_list_files = "list_files";
                                    send(ConnectSocket, request_list_files, strlen(request_list_files), 0);

                                    char recvbuf[DEFAULT_BUFLEN];
                                    int bytesReceived = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
                                    if (bytesReceived > 0)
                                    {
                                        recvbuf[bytesReceived] = '\0';
                                        string receivedMessage(recvbuf);
                                        //Вывод ошибки о том, что директории на сервере не существует
                                        if (receivedMessage.find("Директории на сервере не существует") != string::npos)
                                        {
                                            GoToXYI(43, 12);
                                            cerr << receivedMessage << endl;
                                            Sleep(2000);
                                            system("CLS");
                                            active_menu = 0;
                                            continue;
                                        }

                                        cout << "Список файлов на сервере:\n" << recvbuf << endl;

                                        string fileName;
                                        GoToXYI(48, 13);
                                        cout << "Введите имя файла для загрузки: ";
                                        ConsoleCursorVisible(true, 10);
                                        getline(cin, fileName);
                                        ConsoleCursorVisible(false, 100);
                                        //Выводим сообщение олб ошибке, если поле пустое
                                        if (fileName.empty())
                                        {
                                            GoToXYI(48, 14);
                                            cerr << "Имя файла не может быть пустым. Пожалуйста, попробуйте снова." << endl;
                                            continue;
                                        }
                                        //А как проверять на неправильно введеные имена я не знаю
                                        //Если ввести имя файла неправильно, то сервер отправит несуществующий файл пользователю
                                        //Который будет называться как его вписал пользователь
                                        //После этого действия пользовательская программа *заморозится*
                                        const char *request_download_file = "download_file";
                                        send(ConnectSocket, request_download_file, strlen(request_download_file), 0);
                                        //Отправляем информацию о файле, которую хотим получить
                                        int filename_size = fileName.size();
                                        send(ConnectSocket, reinterpret_cast<char*>(&filename_size), sizeof(int), 0);
                                        send(ConnectSocket, fileName.c_str(), filename_size, 0);
                                        int file_size;
                                        bytesReceived = recv(ConnectSocket, reinterpret_cast<char*>(&file_size), sizeof(int), 0);
                                        ofstream outFile(fileName, ios::binary);
                                        if (!outFile)
                                        {
                                            cerr << "Ошибка при открытии файла для записи." << endl;
                                            return 1;
                                        }
                                        //Получаем файл
                                        char file_buffer[DEFAULT_BUFLEN];
                                        int total_bytes_received = 0;
                                        while (total_bytes_received < file_size)
                                        {
                                            bytesReceived = recv(ConnectSocket, file_buffer, DEFAULT_BUFLEN, 0);
                                            if (bytesReceived <= 0)
                                            {
                                                cerr << "Ошибка при получении данных от сервера." << endl;
                                                break;
                                            }
                                            outFile.write(file_buffer, bytesReceived);
                                            total_bytes_received += bytesReceived;
                                        }
                                        outFile.close();
                                        system("CLS");
                                        GoToXYI(48, 13);
                                        cout << "Файл успешно загружен." << endl;
                                        Sleep(2000);
                                        system("CLS");
                                    }
                                }
                                else if (active_menu == 3)
                                {
                                    //Блок, приветствующий пользователя
                                    //По-идее он должен быть раньше?
                                    //Потому что когда пользователь впервые заходит в программу, то он не знает о том, что навцигация производится на стрелочки!
                                    //Странное ли это решение?
                                    system("CLS");
                                    SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
                                    GoToXYI(7,11);
                                    cout << "Привет! Тебя приветствует программа, которая может отправлять файлы на сервер и скачивать их оттуда!\n" << endl;
                                    GoToXYI(7,12);
                                    cout << "НО сначала же надо серверу эти файлы как-то получить? Да, именно ТЫ первый кто отправит на сервер первый файл.\n" << endl;
                                    GoToXYI(7,13);
                                    cout << "Форматы файлов, которые может передать пргограмма: pdf,docx,txt,png и другие! Кроме, rar и zip...\n" << endl;
                                    GoToXYI(7,14);
                                    cout << "Максимальный объем файла, который может передать программа более 1ГБ, я проверял, реально передала!\n" << endl;
                                    GoToXYI(7,15);
                                    cout << "Доступные клавиши для навигации по программе: ESC, UP, DOWN, ENTER.\n" << endl;
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
                            //Выводим ошибку 404 о том, что связь с сервером не установлена
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
                //Тиииихий выход
                exit(0);
            }
            break;
        }
    }
    return 0;
}
