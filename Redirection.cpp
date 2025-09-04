#include "Shell.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <cstring>

bool hasRedirection(const std::vector<std::string>& tokens) {
    for (const auto& token : tokens) {
        if (token == "<" || token == ">" || token == ">>") {
            return true;
        }
    }
    return false;
}

void handleRedirection(const std::vector<std::string>& tokens) {
    std::vector<std::string> commandTokens;
    std::string inputFile, outputFile;
    bool append = false;
    
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "<") {
            if (i + 1 < tokens.size()) {
                inputFile = tokens[i + 1];
                i++;
            }
        } else if (tokens[i] == ">" || tokens[i] == ">>") {
            if (i + 1 < tokens.size()) {
                outputFile = tokens[i + 1];
                append = (tokens[i] == ">>");
                i++;
            }
        } else {
            commandTokens.push_back(tokens[i]);
        }
    }
    
    if (commandTokens.empty()) {
        std::cerr << "Syntax error: no command specified" << std::endl;
        return;
    }
    
    if (executeBuiltinWithRedirection(commandTokens, inputFile, outputFile, append)) {
        return;
    }
    
    executeWithRedirection(commandTokens, inputFile, outputFile, append);
}

void executeWithRedirection(
    const std::vector<std::string>& commandTokens,
    const std::string& inputFile,
    const std::string& outputFile,
    bool append
) {
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork");
        return;
    }
    
    if (pid == 0) {
        if (!inputFile.empty()) {
            int fd = open(inputFile.c_str(), O_RDONLY);
            if (fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        
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
        
        std::vector<char*> args;
        for (const auto& token : commandTokens) {
            args.push_back(const_cast<char*>(token.c_str()));
        }
        args.push_back(nullptr);
        
        execvp(args[0], args.data());
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}
