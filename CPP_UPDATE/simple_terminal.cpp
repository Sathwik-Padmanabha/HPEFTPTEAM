#include <windows.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <sstream>
#include <direct.h>
#include <filesystem>
#include <wininet.h>
#include <urlmon.h>
#include <shlobj.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "urlmon.lib")

using namespace std;

// FTP Connection structure
struct FTPConnection {
    HINTERNET hInternet = NULL;
    HINTERNET hFtpSession = NULL;
    string currentRemoteDir = "/";
    bool isConnected = false;
    string server;
    string username;
    string password;
};

FTPConnection ftpConnection;

void setConsoleColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void clearScreen() {
    system("cls");
}

string getCurrentDirectory() {
    char buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, buffer);
    return string(buffer);
}

void listDirectory(const string& path = "") {
    string currentPath = path.empty() ? getCurrentDirectory() : path;
    cout << "\nDirectory of " << currentPath << "\n\n";
    
    try {
        for (const auto& entry : filesystem::directory_iterator(currentPath)) {
            string name = entry.path().filename().string();
            string type = entry.is_directory() ? "<DIR>" : "";
            string size = entry.is_directory() ? "" : to_string(entry.file_size());
            
            cout << name << "\t" << type << "\t" << size << " bytes\n";
        }
    } catch (const filesystem::filesystem_error& e) {
        cout << "Error: " << e.what() << endl;
    }
}

bool createDirectory(const string& dirName) {
    try {
        return filesystem::create_directory(dirName);
    } catch (const filesystem::filesystem_error& e) {
        cout << "Error: " << e.what() << endl;
        return false;
    }
}

bool removeDirectory(const string& dirName) {
    try {
        return filesystem::remove_all(dirName);
    } catch (const filesystem::filesystem_error& e) {
        cout << "Error: " << e.what() << endl;
        return false;
    }
}

bool changeDirectory(const string& path) {
    try {
        if (filesystem::is_directory(path)) {
            SetCurrentDirectoryA(path.c_str());
            return true;
        }
        cout << "Error: Directory not found" << endl;
        return false;
    } catch (const filesystem::filesystem_error& e) {
        cout << "Error: " << e.what() << endl;
        return false;
    }
}

// FTP Functions
bool connectToFTP(const string& server, const string& username, const string& password) {
    if (ftpConnection.isConnected) {
        cout << "Already connected to FTP server. Disconnect first.\n";
        return false;
    }

    ftpConnection.hInternet = InternetOpenA("FTP Client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!ftpConnection.hInternet) {
        cout << "InternetOpen failed (" << GetLastError() << ")\n";
        return false;
    }

    ftpConnection.hFtpSession = InternetConnectA(ftpConnection.hInternet, server.c_str(),
        INTERNET_DEFAULT_FTP_PORT, username.c_str(), password.c_str(),
        INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);

    if (!ftpConnection.hFtpSession) {
        cout << "InternetConnect failed (" << GetLastError() << ")\n";
        InternetCloseHandle(ftpConnection.hInternet);
        return false;
    }

    ftpConnection.isConnected = true;
    ftpConnection.server = server;
    ftpConnection.username = username;
    ftpConnection.password = password;
    ftpConnection.currentRemoteDir = "/";
    cout << "Connected to FTP server: " << server << endl;
    return true;
}

void disconnectFTP() {
    if (ftpConnection.hFtpSession) {
        InternetCloseHandle(ftpConnection.hFtpSession);
        ftpConnection.hFtpSession = NULL;
    }
    if (ftpConnection.hInternet) {
        InternetCloseHandle(ftpConnection.hInternet);
        ftpConnection.hInternet = NULL;
    }
    ftpConnection.isConnected = false;
    cout << "Disconnected from FTP server.\n";
}

bool listFTPDirectory(const string& path = "") {
    if (!ftpConnection.isConnected) {
        cout << "Not connected to FTP server.\n";
        return false;
    }

    string dirToShow = path.empty() ? ftpConnection.currentRemoteDir : path;
    cout << "\nFTP Directory of " << ftpConnection.server << dirToShow << "\n\n";

    WIN32_FIND_DATAA findData;
    HINTERNET hFind = FtpFindFirstFileA(ftpConnection.hFtpSession, 
                                       (dirToShow + "/*").c_str(), 
                                       &findData, 0, 0);

    if (!hFind) {
        cout << "FtpFindFirstFile failed (" << GetLastError() << ")\n";
        return false;
    }

    do {
        string name = findData.cFileName;
        string type = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "<DIR>" : "";
        LARGE_INTEGER size;
        size.LowPart = findData.nFileSizeLow;
        size.HighPart = findData.nFileSizeHigh;
        
        cout << name << "\t" << type << "\t" << size.QuadPart << " bytes\n";
    } while (InternetFindNextFileA(hFind, &findData));

    InternetCloseHandle(hFind);
    return true;
}

bool changeFTPDirectory(const string& path) {
    if (!ftpConnection.isConnected) {
        cout << "Not connected to FTP server.\n";
        return false;
    }

    string newPath;
    if (path == "..") {
        // Go up one directory
        size_t lastSlash = ftpConnection.currentRemoteDir.find_last_of('/');
        if (lastSlash != string::npos && lastSlash != 0) {
            newPath = ftpConnection.currentRemoteDir.substr(0, lastSlash);
        } else {
            newPath = "/";
        }
    } else if (path[0] == '/') {
        // Absolute path
        newPath = path;
    } else {
        // Relative path
        newPath = ftpConnection.currentRemoteDir;
        if (newPath.back() != '/') newPath += '/';
        newPath += path;
    }

    if (FtpSetCurrentDirectoryA(ftpConnection.hFtpSession, newPath.c_str())) {
        ftpConnection.currentRemoteDir = newPath;
        cout << "FTP directory changed to: " << newPath << endl;
        return true;
    }

    cout << "Failed to change FTP directory (" << GetLastError() << ")\n";
    return false;
}

bool downloadFTPFile(const string& remoteFile, const string& localPath) {
    if (!ftpConnection.isConnected) {
        cout << "Not connected to FTP server.\n";
        return false;
    }

    string remotePath = ftpConnection.currentRemoteDir;
    if (remotePath.back() != '/') remotePath += '/';
    remotePath += remoteFile;

    string localFilePath = localPath + "\\" + remoteFile;

    if (FtpGetFileA(ftpConnection.hFtpSession, remotePath.c_str(), localFilePath.c_str(),
                   FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0)) {
        cout << "File downloaded successfully: " << localFilePath << endl;
        return true;
    }

    cout << "Failed to download file (" << GetLastError() << ")\n";
    return false;
}

bool uploadFTPFile(const string& localFile, const string& remotePath = "") {
    if (!ftpConnection.isConnected) {
        cout << "Not connected to FTP server.\n";
        return false;
    }

    string targetRemotePath = remotePath.empty() ? 
        ftpConnection.currentRemoteDir + "/" + filesystem::path(localFile).filename().string() : 
        remotePath;

    if (FtpPutFileA(ftpConnection.hFtpSession, localFile.c_str(), targetRemotePath.c_str(),
                   FTP_TRANSFER_TYPE_BINARY, 0)) {
        cout << "File uploaded successfully to: " << targetRemotePath << endl;
        return true;
    }

    cout << "Failed to upload file (" << GetLastError() << ")\n";
    return false;
}

void showHelp() {
    cout << "Available commands:" << endl;
    cout << "clear - Clear the screen" << endl;
    cout << "color - Change text color" << endl;
    cout << "help - Show this help message" << endl;
    cout << "exit - Exit the terminal" << endl;
    cout << "dir - List files and directories" << endl;
    cout << "cd <path> - Change directory" << endl;
    cout << "mkdir <name> - Create new directory" << endl;
    cout << "rmdir <name> - Remove directory" << endl;
    cout << "pwd - Show current directory" << endl;
    cout << "\nFTP Commands:" << endl;
    cout << "ftpconnect <server> <username> <password> - Connect to FTP server" << endl;
    cout << "ftpdisconnect - Disconnect from FTP server" << endl;
    cout << "ftpls - List files on FTP server" << endl;
    cout << "ftpcd <path> - Change directory on FTP server" << endl;
    cout << "ftppwd - Show current FTP directory" << endl;
    cout << "ftpget <remoteFile> [localPath] - Download file from FTP" << endl;
    cout << "ftpput <localFile> [remotePath] - Upload file to FTP" << endl;
}

int main() {
    string command;
    bool running = true;
    
    SetConsoleTitleA("Enhanced C++ Terminal with FTP");
    setConsoleColor(10); // Green text
    
    cout << "Welcome to Enhanced C++ Terminal with FTP" << endl;
    cout << "Type 'help' for available commands" << endl << endl;
    
    while (running) {
        cout << (ftpConnection.isConnected ? "[FTP] " : "") << getCurrentDirectory() << "> ";
        getline(cin, command);
        
        if (command == "exit") {
            if (ftpConnection.isConnected) {
                disconnectFTP();
            }
            running = false;
            break;
        }
        else if (command == "clear") {
            clearScreen();
            cout << "Welcome to Enhanced C++ Terminal with FTP" << endl;
            cout << "Type 'help' for available commands" << endl << endl;
        }
        else if (command == "color") {
            cout << "Available colors:" << endl;
            cout << "1. Green" << endl;
            cout << "2. Red" << endl;
            cout << "3. Blue" << endl;
            cout << "4. Yellow" << endl;
            cout << "Enter color number (1-4): ";
            
            int colorChoice;
            cin >> colorChoice;
            cin.ignore();
            
            switch (colorChoice) {
                case 1: setConsoleColor(10); break;
                case 2: setConsoleColor(12); break;
                case 3: setConsoleColor(9); break;
                case 4: setConsoleColor(14); break;
                default: cout << "Invalid color choice!" << endl;
            }
        }
        else if (command == "help") {
            showHelp();
        }
        else if (command == "dir") {
            listDirectory();
        }
        else if (command.find("cd ") == 0) {
            string path = command.substr(3);
            if (path == "..") {
                string currentPath = getCurrentDirectory();
                size_t lastSlash = currentPath.find_last_of("\\/");
                if (lastSlash != string::npos) {
                    changeDirectory(currentPath.substr(0, lastSlash));
                }
            } else {
                changeDirectory(path);
            }
        }
        else if (command.find("mkdir ") == 0) {
            string dirName = command.substr(6);
            if (createDirectory(dirName)) {
                cout << "Directory created successfully" << endl;
            } else {
                cout << "Failed to create directory" << endl;
            }
        }
        else if (command.find("rmdir ") == 0) {
            string dirName = command.substr(6);
            if (removeDirectory(dirName)) {
                cout << "Directory removed successfully" << endl;
            } else {
                cout << "Failed to remove directory" << endl;
            }
        }
        else if (command == "pwd") {
            cout << getCurrentDirectory() << endl;
        }
        else if (command.find("ftpconnect ") == 0) {
            istringstream iss(command.substr(11));
            string server, username, password;
            iss >> server >> username >> password;
            
            if (server.empty() || username.empty()) {
                cout << "Usage: ftpconnect <server> <username> [password]\n";
                cout << "If password is omitted, anonymous login will be attempted.\n";
            } else {
                if (password.empty()) {
                    password = ""; // Anonymous login
                    cout << "Attempting anonymous login...\n";
                }
                connectToFTP(server, username, password);
            }
        }
        else if (command == "ftpdisconnect") {
            if (ftpConnection.isConnected) {
                disconnectFTP();
            } else {
                cout << "Not connected to FTP server.\n";
            }
        }
        else if (command == "ftpls") {
            listFTPDirectory();
        }
        else if (command.find("ftpcd ") == 0) {
            string path = command.substr(6);
            changeFTPDirectory(path);
        }
        else if (command == "ftppwd") {
            if (ftpConnection.isConnected) {
                cout << "FTP Current directory: " << ftpConnection.currentRemoteDir << endl;
            } else {
                cout << "Not connected to FTP server.\n";
            }
        }
        else if (command.find("ftpget ") == 0) {
            istringstream iss(command.substr(7));
            string remoteFile, localPath;
            iss >> remoteFile >> localPath;
            
            if (remoteFile.empty()) {
                cout << "Usage: ftpget <remoteFile> [localPath]\n";
            } else {
                if (localPath.empty()) {
                    localPath = getCurrentDirectory();
                }
                downloadFTPFile(remoteFile, localPath);
            }
        }
        else if (command.find("ftpput ") == 0) {
            istringstream iss(command.substr(7));
            string localFile, remotePath;
            iss >> localFile >> remotePath;
            
            if (localFile.empty()) {
                cout << "Usage: ftpput <localFile> [remotePath]\n";
            } else {
                uploadFTPFile(localFile, remotePath);
            }
        }
        else if (!command.empty()) {
            system(command.c_str());
        }
    }
    
    cout << "Goodbye!" << endl;
    return 0;
}