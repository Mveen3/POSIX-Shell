#include "Shell.h"
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <vector>
#include <string>

std::vector<std::string> completeCommand(const std::string& input) {
    std::vector<std::string> commands = {
        "cd", "echo", "pwd", "ls", "history", "pinfo", "search"
    };
    
    std::vector<std::string> matches;
    
    for (const auto& cmd : commands) {
        if (cmd.find(input) == 0) {
            matches.push_back(cmd);
        }
    }
    
    return matches;
}

std::vector<std::string> completeFile(const std::string& input) {
    std::vector<std::string> matches;
    
    size_t lastSlash = input.find_last_of('/');
    std::string dirPath = ".";
    std::string prefix;
    
    if (lastSlash == std::string::npos) {
        prefix = input;
    } else {
        dirPath = input.substr(0, lastSlash);
        prefix = input.substr(lastSlash + 1);
    }
    
    if (dirPath.empty()) {
        dirPath = ".";
    }
    
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) {
        return matches;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        std::string name = entry->d_name;
        
        if (prefix.empty() && name[0] == '.') {
            continue;
        }
        
        if (name.find(prefix) == 0) {
            std::string fullPath = dirPath + "/" + name;
            struct stat statBuf;
            if (stat(fullPath.c_str(), &statBuf) == 0 && S_ISDIR(statBuf.st_mode)) {
                name += "/";
            }
            
            if (lastSlash == std::string::npos) {
                matches.push_back(name);
            } else {
                matches.push_back(dirPath + "/" + name);
            }
        }
    }
    
    closedir(dir);
    std::sort(matches.begin(), matches.end());
    
    return matches;
}
