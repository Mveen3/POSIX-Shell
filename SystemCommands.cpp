#include "Shell.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <vector>
#include <cstring>

void executeSystemCommand(const std::vector<std::string>& tokens, bool background) {
    if (tokens.empty()) return;
    
    // Check for I/O redirection
    if (hasRedirection(tokens)) {
        handleRedirection(tokens);
        return;
    }
    
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork");
        return;
    }
    
    if (pid == 0) {
        // Child process
        std::vector<char*> args;
        for (const auto& token : tokens) {
            args.push_back(const_cast<char*>(token.c_str()));
        }
        args.push_back(nullptr);
        
        execvp(args[0], args.data());
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        if (background) {
            std::cout << "[" << pid << "]" << std::endl;
        } else {
            int status;
            waitpid(pid, &status, 0);
        }
    }
}
