#include "Shell.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <set>

std::vector<std::string> completeCommand(const std::string& input) {
    std::set<std::string> matchSet;

    // 1. Built-in shell commands
    static const std::vector<std::string> builtins = {
        "cd", "echo", "pwd", "ls", "history", "pinfo", "search", "exit"
    };
    for (const auto& cmd : builtins) {
        if (cmd.rfind(input, 0) == 0) {
            matchSet.insert(cmd);
        }
    }

    // 2. System binaries in PATH
    const char* pathEnv = getenv("PATH");
    std::string pathStr = pathEnv ? pathEnv : "/bin:/usr/bin:/usr/local/bin";
    std::istringstream pss(pathStr);
    std::string dirPath;

    while (std::getline(pss, dirPath, ':')) {
        if (dirPath.empty()) continue;

        DIR* dir = opendir(dirPath.c_str());
        if (!dir) continue;

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;

            if (name.rfind(input, 0) == 0) {
                std::string fullPath = dirPath + "/" + name;
                if (access(fullPath.c_str(), X_OK) == 0) {
                    matchSet.insert(name);
                }
            }
        }
        closedir(dir);
    }

    return std::vector<std::string>(matchSet.begin(), matchSet.end());
}

std::vector<std::string> completeFile(const std::string& input) {
    std::vector<std::string> matches;

    size_t lastSlash = input.find_last_of('/');
    std::string dirPart = ".";
    std::string prefix;

    if (lastSlash == std::string::npos) {
        prefix = input;
    } else {
        dirPart = input.substr(0, lastSlash);
        prefix = input.substr(lastSlash + 1);
        if (dirPart.empty()) {
            dirPart = "/";
        }
    }

    std::string resolvedDir = expandHomePath(dirPart);
    DIR* dir = opendir(resolvedDir.c_str());
    if (!dir) {
        return matches;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;

        // Skip hidden files if user didn't explicitly start with '.'
        if ((prefix.empty() || prefix[0] != '.') && name[0] == '.') {
            continue;
        }

        if (name.rfind(prefix, 0) == 0) {
            std::string fullPath = resolvedDir + "/" + name;
            struct stat st;
            bool isDir = false;
            if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                isDir = true;
            }

            std::string completedName = name + (isDir ? "/" : "");
            if (lastSlash == std::string::npos) {
                matches.push_back(completedName);
            } else {
                if (dirPart == "/") {
                    matches.push_back("/" + completedName);
                } else {
                    matches.push_back(dirPart + "/" + completedName);
                }
            }
        }
    }

    closedir(dir);
    std::sort(matches.begin(), matches.end());
    return matches;
}
