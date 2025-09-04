#include "Shell.h"

static void lsWithRedirectionImpl(
    const std::vector<std::string>& tokens,
    const std::string& outputFile,
    bool append
);

bool executeBuiltin(const std::vector<std::string>& tokens) {
    if (tokens.empty()) return false;
    
    const std::string& command = tokens[0];
    
    if (command == "cd") {
        cd(tokens);
        return true;
    } else if (command == "echo") {
        echo(tokens);
        return true;
    } else if (command == "pwd") {
        pwd();
        return true;
    } else if (command == "ls") {
        ls(tokens);
        return true;
    } else if (command == "history") {
        history(tokens);
        return true;
    } else if (command == "pinfo") {
        executePinfo(tokens);
        return true;
    } else if (command == "search") {
        executeSearch(tokens);
        return true;
    }
    
    return false;
}

bool executeBuiltinWithRedirection(
    const std::vector<std::string>& commandTokens,
    const std::string& inputFile,
    const std::string& outputFile,
    bool append
) {
    (void)inputFile;
    if (commandTokens.empty()) return false;
    
    const std::string& command = commandTokens[0];
    
    if (command == "echo") {
        echoWithRedirection(commandTokens, outputFile, append);
        return true;
    } else if (command == "ls") {
        lsWithRedirection(commandTokens, outputFile, append);
        return true;
    }
    
    return false;
}

void cd(const std::vector<std::string>& tokens) {
    if (tokens.size() > 2) {
        std::cerr << "cd: too many arguments" << std::endl;
        return;
    }
    
    std::string path;
    if (tokens.size() == 1) {
        path = getShellHomeDir();
    } else {
        path = tokens[1];
        if (path == "~") {
            path = getShellHomeDir();
        } else if (path == "-") {
            const char* oldpwd = getenv("OLDPWD");
            if (oldpwd) {
                path = oldpwd;
                std::cout << path << std::endl;
            } else {
                std::cerr << "cd: OLDPWD not set" << std::endl;
                return;
            }
        }
    }
    
    char currentDir[PATH_MAX];
    if (!getcwd(currentDir, sizeof(currentDir))) {
        perror("cd");
        return;
    }
    
    setenv("OLDPWD", currentDir, 1);
    
    if (chdir(path.c_str()) != 0) {
        perror("cd");
    }
    
    char newDir[PATH_MAX];
    if (getcwd(newDir, sizeof(newDir))) {
        setenv("PWD", newDir, 1);
    }
}

void echo(const std::vector<std::string>& tokens) {
    for (size_t i = 1; i < tokens.size(); i++) {
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

void ls(const std::vector<std::string>& tokens) {
    lsWithRedirection(tokens, "", false);
}

void lsWithRedirection(
    const std::vector<std::string>& tokens,
    const std::string& outputFile,
    bool append
) {
    lsWithRedirectionImpl(tokens, outputFile, append);
}

void echoWithRedirection(
    const std::vector<std::string>& tokens,
    const std::string& outputFile,
    bool append
) {
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork");
        return;
    }
    
    if (pid == 0) {
        if (!outputFile.empty()) {
            int flags = O_WRONLY | O_CREAT;
            if (append) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }
            
            int fd = open(outputFile.c_str(), flags, 0644);
            if (fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        
        for (size_t i = 1; i < tokens.size(); i++) {
            if (i > 1) std::cout << " ";
            std::cout << tokens[i];
        }
        std::cout << std::endl;
        
        exit(EXIT_SUCCESS);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

static void lsWithRedirectionImpl(
    const std::vector<std::string>& tokens,
    const std::string& outputFile,
    bool append
) {
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork");
        return;
    }
    
    if (pid == 0) {
        if (!outputFile.empty()) {
            int flags = O_WRONLY | O_CREAT;
            if (append) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }
            
            int fd = open(outputFile.c_str(), flags, 0644);
            if (fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        
        bool showAll = false;
        bool longFormat = false;
        std::vector<std::string> paths;
        
        for (size_t i = 1; i < tokens.size(); i++) {
            if (tokens[i][0] == '-') {
                for (size_t j = 1; j < tokens[i].size(); j++) {
                    if (tokens[i][j] == 'a') showAll = true;
                    else if (tokens[i][j] == 'l') longFormat = true;
                }
            } else {
                if (tokens[i] == "~") {
                    paths.push_back(getShellHomeDir());
                } else {
                    paths.push_back(tokens[i]);
                }
            }
        }
        
        if (paths.empty()) {
            paths.push_back(".");
        }
        
        for (const auto& path : paths) {
            if (paths.size() > 1) {
                std::cout << path << ":" << std::endl;
            }
            
            DIR* dir = opendir(path.c_str());
            if (!dir) {
                perror("ls");
                continue;
            }
            
            struct dirent* entry;
            std::vector<std::string> entries;
            
            while ((entry = readdir(dir))) {
                if (!showAll && entry->d_name[0] == '.') {
                    continue;
                }
                entries.push_back(entry->d_name);
            }
            
            closedir(dir);
            std::sort(entries.begin(), entries.end());
            
            for (const auto& name : entries) {
                if (longFormat) {
                    std::string fullPath = path + "/" + name;
                    struct stat fileStat;
                    
                    if (stat(fullPath.c_str(), &fileStat) == -1) {
                        perror("ls");
                        continue;
                    }
                    
                    std::cout << ((S_ISDIR(fileStat.st_mode)) ? "d" : "-");
                    std::cout << ((fileStat.st_mode & S_IRUSR) ? "r" : "-");
                    std::cout << ((fileStat.st_mode & S_IWUSR) ? "w" : "-");
                    std::cout << ((fileStat.st_mode & S_IXUSR) ? "x" : "-");
                    std::cout << ((fileStat.st_mode & S_IRGRP) ? "r" : "-");
                    std::cout << ((fileStat.st_mode & S_IWGRP) ? "w" : "-");
                    std::cout << ((fileStat.st_mode & S_IXGRP) ? "x" : "-");
                    std::cout << ((fileStat.st_mode & S_IROTH) ? "r" : "-");
                    std::cout << ((fileStat.st_mode & S_IWOTH) ? "w" : "-");
                    std::cout << ((fileStat.st_mode & S_IXOTH) ? "x" : "-");
                    
                    std::cout << " " << std::setw(3) << fileStat.st_nlink;
                    
                    struct passwd* pw = getpwuid(fileStat.st_uid);
                    struct group* gr = getgrgid(fileStat.st_gid);
                    std::cout << " " << (pw ? pw->pw_name : std::to_string(fileStat.st_uid));
                    std::cout << " " << (gr ? gr->gr_name : std::to_string(fileStat.st_gid));
                    
                    std::cout << " " << std::setw(8) << fileStat.st_size;
                    
                    char timeBuf[80];
                    struct tm* timeinfo = localtime(&fileStat.st_mtime);
                    strftime(timeBuf, sizeof(timeBuf), "%b %d %H:%M", timeinfo);
                    std::cout << " " << timeBuf;
                    
                    std::cout << " " << name << std::endl;
                } else {
                    std::cout << name << " ";
                }
            }
            
            if (!longFormat) {
                std::cout << std::endl;
            }
            
            if (paths.size() > 1) {
                std::cout << std::endl;
            }
        }
        
        exit(EXIT_SUCCESS);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

void history(const std::vector<std::string>& tokens) {
    int count = 20; // Default to 20
    
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
