#include "Shell.h"
#include <iostream>
#include <unistd.h>
#include <climits>
#include <cstdlib>

void initializeShell() {
    userName = getUserName();
    systemName = getSystemName();

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        shellHomeDir = cwd;
    } else {
        const char* h = getenv("HOME");
        shellHomeDir = h ? h : "/";
    }

    previousDir.clear();
    setupSignals();
    loadHistory();
    updateCurrentDir();
}

void updateCurrentDir() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        currentDir = formatPromptPath(std::string(cwd));
    }
}

void displayPrompt() {
    std::cout << getPromptString();
    std::cout.flush();
}

void runShell() {
    initializeShell();
    std::string input;
    bool interactive = isatty(STDIN_FILENO);

    while (true) {
        checkBackgroundJobs();
        if (interactive) {
            displayPrompt();
        }
        input = getInputLine();

        if (input == "exit") {
            break;
        }
        if (input.empty()) {
            continue;
        }

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

            if (tokens.empty()) continue;

            if (tokens.size() == 1 && tokens[0] == "exit") {
                saveHistory();
                exit(0);
            }

            executeCommandPipeline(tokens, background);
            updateCurrentDir();
        }
    }

    saveHistory();
}
