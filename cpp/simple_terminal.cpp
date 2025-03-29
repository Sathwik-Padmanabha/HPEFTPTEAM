#include <windows.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <sstream>
#include <direct.h>
#include <filesystem>

using namespace std;

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

int main() {
    string command;
    bool running = true;
    
    SetConsoleTitleA("Simple C++ Terminal");
    setConsoleColor(10); // Green text
    
    cout << "Welcome to Simple C++ Terminal" << endl;
    cout << "Type 'help' for available commands" << endl << endl;
    
    while (running) {
        cout << getCurrentDirectory() << "> ";
        getline(cin, command);
        
        if (command == "exit") {
            running = false;
            break;
        }
        else if (command == "clear") {
            clearScreen();
            cout << "Welcome to Simple C++ Terminal" << endl;
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
        else if (!command.empty()) {
            system(command.c_str());
        }
    }
    
    cout << "Goodbye!" << endl;
    return 0;
} 