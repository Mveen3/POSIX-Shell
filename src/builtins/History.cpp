#include "Shell.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <pwd.h>

static std::vector<std::string> historyList;
static const size_t MAX_HISTORY = 20;

static std::string getHistoryFilePath() {
    const char* envHome = getenv("HOME");
    std::string base;
    if (envHome && *envHome) {
        base = envHome;
    } else {
        struct passwd *pw = getpwuid(getuid());
        if (pw && pw->pw_dir) {
            base = pw->pw_dir;
        } else {
            base = shellHomeDir;
        }
    }
    return base + "/.shell_history";
}

void addToHistory(const std::string& command) {
    if (command.empty()) return;

    if (historyList.empty() || command != historyList.back()) {
        historyList.push_back(command);
        while (historyList.size() > MAX_HISTORY) {
            historyList.erase(historyList.begin());
        }
        saveHistory();
    }
}

void printHistory(int count) {
    if (count <= 0) return;
    if (count > static_cast<int>(MAX_HISTORY)) {
        count = static_cast<int>(MAX_HISTORY);
    }

    int total = static_cast<int>(historyList.size());
    int start = std::max(0, total - count);

    for (int i = start; i < total; ++i) {
        std::cout << historyList[i] << std::endl;
    }
}

void loadHistory() {
    historyList.clear();
    std::string path = getHistoryFilePath();
    std::ifstream file(path);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            if (historyList.empty() || line != historyList.back()) {
                historyList.push_back(line);
            }
        }
    }

    while (historyList.size() > MAX_HISTORY) {
        historyList.erase(historyList.begin());
    }
}

void saveHistory() {
    std::string path = getHistoryFilePath();
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        return;
    }

    for (const auto& cmd : historyList) {
        file << cmd << "\n";
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
