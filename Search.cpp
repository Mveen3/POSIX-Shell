#include "Shell.h"
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>

void executeSearch(const std::vector<std::string>& tokens) {
    if (tokens.size() != 2) {
        std::cerr << "search: requires exactly one argument" << std::endl;
        return;
    }
    
    bool found = searchRecursive(".", tokens[1]);
    std::cout << (found ? "True" : "False") << std::endl;
}

bool searchRecursive(const std::string& path, const std::string& target) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return false;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir))) {
        std::string name = entry->d_name;
        
        if (name == "." || name == "..") {
            continue;
        }
        
        std::string fullPath = path + "/" + name;
        
        if (name == target) {
            closedir(dir);
            return true;
        }
        
        struct stat statBuf;
        if (stat(fullPath.c_str(), &statBuf) == 0 && S_ISDIR(statBuf.st_mode)) {
            if (searchRecursive(fullPath, target)) {
                closedir(dir);
                return true;
            }
        }
    }
    
    closedir(dir);
    return false;
}
