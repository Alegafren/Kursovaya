#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <conio.h>
#include <vector>
#include <fstream>
#include <thread>

#define DEFAULT_BUFLEN 8192
#define DEFAULT_PORT "27015"

using namespace std;

// Задает фиксированный размер консольного окна сервера.
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

//Массив сокетов для подключенных клиентов
SOCKET Connections[100];
SOCKET ServerSocket;
//Переменная счётчик для подключенных пользователей
int activeConnections = 0;
bool isServerRunning = true;

//Создаём производный тип днных
struct User
{
    int id;
    string username;
};

// Список активных пользователей.
vector<User> activeUsers;

//Генерируем уникальный идентификатор пользователя
int GenerateUniqueId()
{
    static int idCounter = 0;
    return ++idCounter;
}
//Функция, которая записывает имя пользователя и его id в файл
void SaveActiveUsersToFile()
{
    ofstream outFile("active_users.txt", ios::trunc);
    if (!outFile)
    {
        cerr << "Ошибка при открытии файла для записи!" << endl;
        return;
    }

    for (const User &user : activeUsers)
    {
        outFile << user.id << " " << user.username << endl;
    }

    outFile.close();
}
// Создает файл со списком активных пользователей, если его еще нет.
void EnsureActiveUsersFileExists()
{
    ofstream file("active_users.txt", ios::app);
    if (!file)
    {
        cerr << "Не удалось открыть active_users.txt." << endl;
    }
}

string GetApplicationDirectory()
{
    char path[MAX_PATH] = {0};
    if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0)
    {
        return ".\\";
    }

    string appPath(path);
    size_t pos = appPath.find_last_of("\\/");
    if (pos == string::npos)
    {
        return ".\\";
    }
    return appPath.substr(0, pos + 1);
}

string GetSavedFilesDirectory()
{
    string directory = GetApplicationDirectory() + "SavedFiles\\";
    CreateDirectoryA(directory.c_str(), NULL);
    return directory;
}
//Функция для формирования списка активных пользователей
void SendActiveUsersToConnectedClients(SOCKET clientSocket)
{
    string allUserList;
    for (const User &user : activeUsers)
    {
        //Записываем id и имя пользователя в строку и переносим на новую строку
        allUserList += to_string(user.id) + " " + user.username + "\n";
    }
    //Отправляем список на сервер
    send(clientSocket, allUserList.c_str(), allUserList.size(), 0);
}
//Функция для получения файла от пользователя
void ReceiveFileFromClient(SOCKET clientSocket)
{
    //объявляем буфер для хранения имени файла
    char filename_buf[DEFAULT_BUFLEN] = {0};
    int filename_size;
    if (recv(clientSocket, reinterpret_cast<char*>(&filename_size), sizeof(int), 0) <= 0)
    {
        cerr << "Ошибка при получении размера имени файла." << endl;
        return;
    }
    if (filename_size <= 0 || filename_size >= DEFAULT_BUFLEN)
    {
        cerr << "Некорректный размер имени файла." << endl;
        return;
    }
    if (recv(clientSocket, filename_buf, filename_size, 0) <= 0)
    {
        cerr << "Ошибка при получении имени файла." << endl;
        return;
    }
    string filename(filename_buf, filename_size);
    int file_size;
    if (recv(clientSocket, reinterpret_cast<char*>(&file_size), sizeof(int), 0) <= 0)
    {
        cerr << "Ошибка при получении размера файла." << endl;
        return;
    }
    int num_chunks;
    if (recv(clientSocket, reinterpret_cast<char*>(&num_chunks), sizeof(int), 0) <= 0)
    {
        cerr << "Ошибка при получении количества частей файла." << endl;
        return;
    }
    // Все серверные файлы хранятся рядом с исполняемым файлом в SavedFiles.
    string filePath = GetSavedFilesDirectory() + filename;
    ofstream file(filePath, ios::binary);
    if (!file.is_open())
    {
        cerr << "Ошибка при сохранении файла на сервере: " << filename << endl;
        return;
    }
    int totalBytesReceived = 0;
    for (int i = 0; i < num_chunks; ++i)
    {
        char chunk_buffer[DEFAULT_BUFLEN] = {0};
        int expectedChunkSize = std::min(DEFAULT_BUFLEN, file_size - totalBytesReceived);
        int chunkBytesReceived = 0;

        while (chunkBytesReceived < expectedChunkSize)
        {
            int bytesReceived = recv(
                clientSocket,
                chunk_buffer + chunkBytesReceived,
                expectedChunkSize - chunkBytesReceived,
                0
            );

            if (bytesReceived <= 0)
            {
                cerr << "Ошибка при приеме данных от клиента или клиент отключен." << endl;
                file.close();
                return;
            }

            chunkBytesReceived += bytesReceived;
        }

        file.write(chunk_buffer, chunkBytesReceived);
        totalBytesReceived += chunkBytesReceived;
    }

    file.close();
    cout << "Файл \"" << filename << "\" успешно сохранен в директории приложения." << endl;
    send(clientSocket, "Файл успешно доставлен.", strlen("Файл успешно доставлен."), 0);
}
//Функция, которая отправляет список файлов
void SendFileListToClient(SOCKET clientSocket)
{
    string file_list;
    string directory = GetSavedFilesDirectory();
    //Храним данные о найденных файлах
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((directory + "*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        string empty_message = "Серверное хранилище пусто.";
        send(clientSocket, empty_message.c_str(), empty_message.size(), 0);
        return;
    }
    //Цикл do while нужен для поиска файлов в директории
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
    //Отправляем список файлов пользователю
    send(clientSocket, file_list.c_str(), file_list.size(), 0);
}
//Функция отправки файла пользователю
void SendFileToClient(SOCKET clientSocket, const string& filename)
{
    //Формируем путьт к файлу
    string file_path = GetSavedFilesDirectory() + filename;
    //Читаем файл в бинарном формате
    ifstream file(file_path, ios::binary);
    if (!file.is_open())
    {
        int file_size = -1;
        send(clientSocket, reinterpret_cast<char*>(&file_size), sizeof(int), 0);
        cerr << "Запрошенный файл не найден: " << filename << endl;
        return;
    }

    // Определяем размер файла.
    file.seekg(0, ios::end);
    int file_size = static_cast<int>(file.tellg());
    file.seekg(0, ios::beg);

    send(clientSocket, reinterpret_cast<char*>(&file_size), sizeof(int), 0);
    //Отправляем файл частями
    char buffer[DEFAULT_BUFLEN];
    while (!file.eof())
    {
        file.read(buffer, sizeof(buffer));
        int bytes_read = file.gcount();
        send(clientSocket, buffer, bytes_read, 0);
    }
    //Закрываем файл
    file.close();
}
//Функция обработчик пользователя
void ClientHandler(SOCKET clientSocket)
{
    char recvbuf[DEFAULT_BUFLEN];
    int bytesReceived;
    while (isServerRunning)
    {
        bytesReceived = recv(clientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0)
        {
            cerr << "Ошибка при приеме данных от клиента или клиент отключен." << endl;
            break;
        }
        //Блок получения реквестов от пользователя
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
                cerr << "Ошибка при получении размера имени файла." << endl;
                continue;
            }
            char filename_buf[DEFAULT_BUFLEN] = {0};
            if (recv(clientSocket, filename_buf, filename_size, 0) <= 0)
            {
                cerr << "Ошибка при получении имени файла." << endl;
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
            //если сервер получил что-то кроме реквестов, то он создаёт нового пользователя
            if (isNewUser)
            {
                User newUser;
                newUser.id = GenerateUniqueId();
                newUser.username = username;
                activeUsers.push_back(newUser);
                SaveActiveUsersToFile();
                cout << "Пользователь подключен к серверу. ID: " << newUser.id << ", Никнейм: " << newUser.username << endl;
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
//Функция выводящая список пользователей после закрытия сервера
void UserInputListener()
{
    string input;
    while (isServerRunning)
    {
        cout << "Введите 'exit', чтобы завершить работу сервера: ";
        cin >> input;
        if (input == "exit")
        {
            cout << "Содержимое файла active_users.txt:" << endl;
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
    SetConsoleTitleA("FILE TRANSFER SERVER");
    SetFixedConsoleWindow(100, 30);

    WSADATA wsaData;
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "Не удалось инициализировать Winsock." << endl;
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, DEFAULT_PORT, &hints, &result) != 0)
    {
        cerr << "Не удалось получить адрес для сервера." << endl;
        WSACleanup();
        return 1;
    }

    ServerSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ServerSocket == INVALID_SOCKET)
    {
        cerr << "Не удалось создать серверный сокет." << endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    if (bind(ServerSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR)
    {
        cerr << "Не удалось привязать сервер к порту " << DEFAULT_PORT
             << ". Код Winsock: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        closesocket(ServerSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);

    if (listen(ServerSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        cerr << "Не удалось перевести сервер в режим прослушивания." << endl;
        closesocket(ServerSocket);
        WSACleanup();
        return 1;
    }

    cout << "Сервер запущен. TCP-порт: " << DEFAULT_PORT << endl;

    EnsureActiveUsersFileExists();

    thread inputThread(UserInputListener);
    inputThread.detach();

    while (isServerRunning)
    {
        SOCKET ClientSocket = accept(ServerSocket, NULL, NULL);
        if (!isServerRunning) break;
        if (ClientSocket == INVALID_SOCKET)
        {
            cerr << "Ошибка при принятии подключения." << endl;
            continue;
        }

        activeConnections++;

        thread clientThread(ClientHandler, ClientSocket);
        clientThread.detach();
    }

    return 0;
}
