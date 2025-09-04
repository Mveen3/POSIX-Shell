#include "Shell.h"
#include <iostream>
#include <unistd.h>
#include <pwd.h>
#include <climits>

void initializeShell() {
    struct passwd *pw = getpwuid(getuid());
    userName = pw->pw_name;
    
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    systemName = hostname;
    
    homeDir = getenv("HOME");
    if (homeDir.empty()) {
        homeDir = pw->pw_dir;
    }
    
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        shellHomeDir = cwd;
    } else {
        shellHomeDir = homeDir;
    }
    
    setupSignals();
    loadHistory();
    updateCurrentDir();
}

void updateCurrentDir() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        currentDir = cwd;
        if (currentDir.find(homeDir) == 0) {
            currentDir.replace(0, homeDir.length(), "~");
        } else if (currentDir.find(shellHomeDir) == 0) {
            currentDir.replace(0, shellHomeDir.length(), "~");
        }
    }
}

void displayPrompt() {
    std::cout << userName << "@" << systemName << ":" << currentDir << "> ";
    std::cout.flush();
}

std::string getShellHomeDir() {
    return shellHomeDir;
}

void runShell() {
    initializeShell();
    std::string input;
    
    while (true) {
        displayPrompt();
        input = getInputLine();
        
        if (input == "exit") break;
        if (input.empty()) continue;
        
        addToHistory(input);
        std::vector<std::string> commands = splitCommands(input);
        
        for (const auto& cmd : commands) {
            if (cmd.empty()) continue;
            
            std::vector<std::string> tokens = parseInput(cmd);
            if (tokens.empty()) continue;
            
            bool background = false;
            if (tokens.back() == "&") {
                background = true;
                tokens.pop_back();
            }
            
            if (hasPipeline(tokens)) {
                handlePipeline(tokens, background);
                updateCurrentDir();
                continue;
            }
            
            if (hasRedirection(tokens)) {
                handleRedirection(tokens);
                updateCurrentDir();
                continue;
            }
            
            if (executeBuiltin(tokens)) {
                updateCurrentDir();
                continue;
            }
            
            executeSystemCommand(tokens, background);
        }
    }
    
    saveHistory();
}
