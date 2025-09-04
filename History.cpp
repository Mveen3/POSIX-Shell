#include "Shell.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <pwd.h>
#include <cstring>

static std::vector<std::string> historyList;
static const int MAX_HISTORY = 20;

void addToHistory(const std::string& command) {
    if (!command.empty() && (historyList.empty() || command != historyList.back())) {
        historyList.push_back(command);
        if (historyList.size() > MAX_HISTORY) {
            historyList.erase(historyList.begin());
        }
        // Save after each command to ensure persistence
        saveHistory();
    }
}

void printHistory(int count) {
    int start = std::max(0, static_cast<int>(historyList.size()) - count);
    for (size_t i = start; i < historyList.size(); i++) {
        std::cout << i + 1 << " " << historyList[i] << std::endl;
    }
}

void loadHistory() {
    // Get home directory for history file
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        struct passwd *pw = getpwuid(getuid());
        homeDir = pw->pw_dir;
    }
    
    std::string historyFile = std::string(homeDir) + "/.shell_history";
    std::ifstream file(historyFile);
    if (!file) {
        // Create empty history file if it doesn't exist
        std::ofstream createFile(historyFile);
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Remove any carriage return characters
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line = line.substr(0, line.size() - 1);
        }
        if (!line.empty()) {
            historyList.push_back(line);
        }
    }
    
    // Remove duplicates and keep only the last MAX_HISTORY
    if (historyList.size() > MAX_HISTORY) {
        historyList.erase(historyList.begin(), historyList.end() - MAX_HISTORY);
    }
}

void saveHistory() {
    // Get home directory for history file
    const char* homeDir = getenv("HOME");
    if (!homeDir) {
        struct passwd *pw = getpwuid(getuid());
        homeDir = pw->pw_dir;
    }
    
    std::string historyFile = std::string(homeDir) + "/.shell_history";
    std::ofstream file(historyFile, std::ios::trunc); // Truncate to overwrite
    
    if (!file) {
        std::cerr << "Error: Could not save history to " << historyFile << std::endl;
        return;
    }
    
    for (const auto& command : historyList) {
        file << command << std::endl;
    }
}

const std::vector<std::string>& getHistoryList() {
    return historyList;
}

bool isHistoryEmpty() {
    return historyList.empty();
}

size_t getHistorySize() {
    return historyList.size();
}
