#include "Shell.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <ctime>
#include <climits>

bool isBuiltin(const std::string& cmd) {
    return (cmd == "cd" || cmd == "echo" || cmd == "pwd" ||
            cmd == "ls" || cmd == "history" || cmd == "pinfo" ||
            cmd == "search");
}

int executeBuiltin(const std::vector<std::string>& tokens) {
    if (tokens.empty()) return 0;

    const std::string& command = tokens[0];

    if (command == "cd") {
        cd(tokens);
        return 0;
    } else if (command == "echo") {
        echo(tokens);
        return 0;
    } else if (command == "pwd") {
        pwd();
        return 0;
    } else if (command == "ls") {
        ls(tokens);
        return 0;
    } else if (command == "history") {
        history(tokens);
        return 0;
    } else if (command == "pinfo") {
        executePinfo(tokens);
        return 0;
    } else if (command == "search") {
        executeSearch(tokens);
        return 0;
    }

    return 1;
}

void cd(const std::vector<std::string>& tokens) {
    if (tokens.size() > 2) {
        std::cerr << "cd: too many arguments" << std::endl;
        return;
    }

    std::string target;
    bool isMinus = false;

    if (tokens.size() == 1 || tokens[1] == "~") {
        target = shellHomeDir;
    } else if (tokens[1] == "-") {
        isMinus = true;
        if (previousDir.empty()) {
            std::cerr << "cd: OLDPWD not set" << std::endl;
            return;
        }
        target = previousDir;
    } else {
        target = expandHomePath(tokens[1]);
    }

    char cwdBuf[PATH_MAX];
    std::string oldCwd;
    if (getcwd(cwdBuf, sizeof(cwdBuf))) {
        oldCwd = cwdBuf;
    }

    if (chdir(target.c_str()) != 0) {
        perror("cd");
        return;
    }

    char newCwdBuf[PATH_MAX];
    if (getcwd(newCwdBuf, sizeof(newCwdBuf))) {
        if (isMinus) {
            std::cout << newCwdBuf << std::endl;
        }
    }

    if (!oldCwd.empty()) {
        previousDir = oldCwd;
    }
    updateCurrentDir();
}

void echo(const std::vector<std::string>& tokens) {
    for (size_t i = 1; i < tokens.size(); ++i) {
        if (i > 1) std::cout << " ";
        std::cout << tokens[i];
    }
    std::cout << std::endl;
}

void pwd() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        std::cout << cwd << std::endl;
    } else {
        perror("pwd");
    }
}

static std::string formatFilePermissions(mode_t mode) {
    std::string perm = "----------";

    if (S_ISDIR(mode)) perm[0] = 'd';
    else if (S_ISLNK(mode)) perm[0] = 'l';
    else if (S_ISCHR(mode)) perm[0] = 'c';
    else if (S_ISBLK(mode)) perm[0] = 'b';
    else if (S_ISFIFO(mode)) perm[0] = 'p';
    else if (S_ISSOCK(mode)) perm[0] = 's';

    if (mode & S_IRUSR) perm[1] = 'r';
    if (mode & S_IWUSR) perm[2] = 'w';
    if (mode & S_IXUSR) perm[3] = 'x';

    if (mode & S_IRGRP) perm[4] = 'r';
    if (mode & S_IWGRP) perm[5] = 'w';
    if (mode & S_IXGRP) perm[6] = 'x';

    if (mode & S_IROTH) perm[7] = 'r';
    if (mode & S_IWOTH) perm[8] = 'w';
    if (mode & S_IXOTH) perm[9] = 'x';

    return perm;
}

static void printLongStat(const std::string& fullPath, const std::string& displayName) {
    struct stat fileStat;
    if (lstat(fullPath.c_str(), &fileStat) == -1) {
        perror("ls");
        return;
    }

    std::cout << formatFilePermissions(fileStat.st_mode);
    std::cout << " " << std::setw(3) << fileStat.st_nlink;

    struct passwd* pw = getpwuid(fileStat.st_uid);
    struct group* gr = getgrgid(fileStat.st_gid);
    std::string owner = pw ? pw->pw_name : std::to_string(fileStat.st_uid);
    std::string group = gr ? gr->gr_name : std::to_string(fileStat.st_gid);

    std::cout << " " << std::setw(8) << owner;
    std::cout << " " << std::setw(8) << group;
    std::cout << " " << std::setw(8) << fileStat.st_size;

    char timeBuf[80];
    struct tm* timeinfo = localtime(&fileStat.st_mtime);
    strftime(timeBuf, sizeof(timeBuf), "%b %e %H:%M", timeinfo);
    std::cout << " " << timeBuf;

    std::cout << " " << displayName;

    if (S_ISLNK(fileStat.st_mode)) {
        char linkTarget[PATH_MAX];
        ssize_t len = readlink(fullPath.c_str(), linkTarget, sizeof(linkTarget) - 1);
        if (len != -1) {
            linkTarget[len] = '\0';
            std::cout << " -> " << linkTarget;
        }
    }

    std::cout << std::endl;
}

void ls(const std::vector<std::string>& tokens) {
    bool showAll = false;
    bool longFormat = false;
    std::vector<std::string> paths;

    for (size_t i = 1; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        if (!token.empty() && token[0] == '-' && token.size() > 1) {
            for (size_t j = 1; j < token.size(); ++j) {
                if (token[j] == 'a') {
                    showAll = true;
                } else if (token[j] == 'l') {
                    longFormat = true;
                } else {
                    std::cerr << "ls: invalid option -- '" << token[j] << "'" << std::endl;
                    return;
                }
            }
        } else {
            paths.push_back(token);
        }
    }

    if (paths.empty()) {
        paths.push_back(".");
    }

    for (size_t p = 0; p < paths.size(); ++p) {
        std::string rawPath = paths[p];
        std::string resolvedPath = expandHomePath(rawPath);

        struct stat pathStat;
        if (lstat(resolvedPath.c_str(), &pathStat) == -1) {
            perror(("ls: cannot access '" + rawPath + "'").c_str());
            continue;
        }

        if (!S_ISDIR(pathStat.st_mode)) {
            // It's a file
            if (longFormat) {
                printLongStat(resolvedPath, rawPath);
            } else {
                std::cout << rawPath << std::endl;
            }
            continue;
        }

        // It's a directory
        if (paths.size() > 1) {
            std::cout << rawPath << ":" << std::endl;
        }

        DIR* dir = opendir(resolvedPath.c_str());
        if (!dir) {
            perror(("ls: cannot open directory '" + rawPath + "'").c_str());
            continue;
        }

        struct dirent* entry;
        std::vector<std::string> entries;
        long long totalBlocks = 0;

        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (!showAll && !name.empty() && name[0] == '.') {
                continue;
            }
            entries.push_back(name);

            if (longFormat) {
                std::string fullPath = resolvedPath + "/" + name;
                struct stat entryStat;
                if (lstat(fullPath.c_str(), &entryStat) == 0) {
                    totalBlocks += entryStat.st_blocks;
                }
            }
        }
        closedir(dir);

        std::sort(entries.begin(), entries.end());

        if (longFormat) {
            // POSIX ls total blocks is in 1024-byte units (st_blocks is in 512-byte units)
            std::cout << "total " << (totalBlocks / 2) << std::endl;
            for (const auto& name : entries) {
                std::string fullPath = resolvedPath + "/" + name;
                printLongStat(fullPath, name);
            }
        } else {
            for (const auto& name : entries) {
                std::cout << name << std::endl;
            }
        }

        if (paths.size() > 1 && p + 1 < paths.size()) {
            std::cout << std::endl;
        }
    }
}

void history(const std::vector<std::string>& tokens) {
    int count = 10; // Default to 10 commands (Problem Statement requirement)

    if (tokens.size() > 1) {
        try {
            count = std::stoi(tokens[1]);
            if (count < 1) count = 1;
            if (count > 20) count = 20;
        } catch (...) {
            std::cerr << "history: invalid number" << std::endl;
            return;
        }
    }

    printHistory(count);
}
