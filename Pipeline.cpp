#include "Shell.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <cstring>

bool hasPipeline(const std::vector<std::string>& tokens) {
    for (const auto& token : tokens) {
        if (token == "|") {
            return true;
        }
    }
    return false;
}

void handlePipeline(const std::vector<std::string>& tokens, bool background) {
    std::vector<std::vector<std::string>> commands;
    std::vector<std::string> currentCommand;
    
    for (const auto& token : tokens) {
        if (token == "|") {
            if (!currentCommand.empty()) {
                commands.push_back(currentCommand);
                currentCommand.clear();
            }
        } else {
            currentCommand.push_back(token);
        }
    }
    
    if (!currentCommand.empty()) {
        commands.push_back(currentCommand);
    }
    
    if (commands.size() < 2) {
        std::cerr << "Syntax error: invalid pipeline" << std::endl;
        return;
    }
    
    int numCommands = commands.size();
    std::vector<int> pipefds(2 * (numCommands - 1));
    
    // Create all pipes first
    for (int i = 0; i < numCommands - 1; i++) {
        if (pipe(&pipefds[2 * i]) < 0) {
            perror("pipe");
            return;
        }
    }
    
    std::vector<pid_t> pids;
    
    for (int i = 0; i < numCommands; i++) {
        pid_t pid = fork();
        
        if (pid == -1) {
            perror("fork");
            return;
        }
        
        if (pid == 0) {
            // Child process
            
            // Connect input from previous pipe
            if (i > 0) {
                dup2(pipefds[2 * (i - 1)], STDIN_FILENO);
            }
            
            // Connect output to next pipe
            if (i < numCommands - 1) {
                dup2(pipefds[2 * i + 1], STDOUT_FILENO);
            }
            
            // Close all pipe file descriptors
            for (int j = 0; j < 2 * (numCommands - 1); j++) {
                close(pipefds[j]);
            }
            
            // Prepare arguments
            std::vector<char*> args;
            for (const auto& arg : commands[i]) {
                args.push_back(const_cast<char*>(arg.c_str()));
            }
            args.push_back(nullptr);
            
            execvp(args[0], args.data());
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            pids.push_back(pid);
        }
    }
    
    // Parent closes all pipe file descriptors
    for (int i = 0; i < 2 * (numCommands - 1); i++) {
        close(pipefds[i]);
    }
    
    // Wait for children
    if (!background) {
        for (pid_t pid : pids) {
            int status;
            waitpid(pid, &status, 0);
        }
    } else {
        for (pid_t pid : pids) {
            std::cout << "[" << pid << "]" << std::endl;
        }
    }
}
