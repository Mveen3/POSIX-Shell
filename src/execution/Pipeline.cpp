#include "Shell.h"
#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cerrno>

bool hasPipeline(const std::vector<std::string>& tokens) {
    for (const auto& token : tokens) {
        if (token == "|") {
            return true;
        }
    }
    return false;
}

void handlePipeline(const std::vector<std::string>& tokens, bool background) {
    executeCommandPipeline(tokens, background);
}

void executeCommandPipeline(const std::vector<std::string>& tokens, bool background) {
    if (tokens.empty()) return;

    // Split tokens by pipe '|'
    std::vector<std::vector<std::string>> rawStages;
    std::vector<std::string> currentRaw;

    for (const auto& tok : tokens) {
        if (tok == "|") {
            if (currentRaw.empty()) {
                std::cerr << "Syntax error: invalid pipeline sequence" << std::endl;
                return;
            }
            rawStages.push_back(currentRaw);
            currentRaw.clear();
        } else {
            currentRaw.push_back(tok);
        }
    }

    if (!currentRaw.empty()) {
        rawStages.push_back(currentRaw);
    } else if (hasPipeline(tokens)) {
        std::cerr << "Syntax error: trailing pipe" << std::endl;
        return;
    }

    if (rawStages.empty()) return;

    // Parse redirection for each stage
    std::vector<CommandStage> stages;
    for (const auto& raw : rawStages) {
        CommandStage stage;
        if (!parseStageRedirection(raw, stage)) {
            return;
        }
        if (stage.tokens.empty()) {
            std::cerr << "Syntax error: empty command" << std::endl;
            return;
        }
        stages.push_back(stage);
    }

    size_t numStages = stages.size();

    // CASE 1: Single Stage (no pipe)
    if (numStages == 1) {
        const CommandStage& stage = stages[0];

        // Builtin Command
        if (isBuiltin(stage.tokens[0])) {
            if (background) {
                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork");
                    return;
                }
                if (pid == 0) {
                    setpgid(0, 0);
                    restoreDefaultSignals();

                    if (!stage.inputFile.empty()) {
                        int inFd = open(stage.inputFile.c_str(), O_RDONLY);
                        if (inFd < 0) {
                            perror(("open: " + stage.inputFile).c_str());
                            exit(EXIT_FAILURE);
                        }
                        dup2(inFd, STDIN_FILENO);
                        close(inFd);
                    }
                    if (!stage.outputFile.empty()) {
                        int flags = O_WRONLY | O_CREAT | (stage.append ? O_APPEND : O_TRUNC);
                        int outFd = open(stage.outputFile.c_str(), flags, 0644);
                        if (outFd < 0) {
                            perror(("open: " + stage.outputFile).c_str());
                            exit(EXIT_FAILURE);
                        }
                        dup2(outFd, STDOUT_FILENO);
                        close(outFd);
                    }

                    int code = executeBuiltin(stage.tokens);
                    exit(code);
                } else {
                    std::cout << "[" << pid << "]" << std::endl;
                    addJob(pid, stage.tokens[0], false);
                    return;
                }
            } else {
                // Foreground Builtin: execute in shell process (supports cd)
                int savedIn = -1;
                int savedOut = -1;

                if (!stage.inputFile.empty()) {
                    int inFd = open(stage.inputFile.c_str(), O_RDONLY);
                    if (inFd < 0) {
                        perror(("open: " + stage.inputFile).c_str());
                        return;
                    }
                    savedIn = dup(STDIN_FILENO);
                    dup2(inFd, STDIN_FILENO);
                    close(inFd);
                }

                if (!stage.outputFile.empty()) {
                    int flags = O_WRONLY | O_CREAT | (stage.append ? O_APPEND : O_TRUNC);
                    int outFd = open(stage.outputFile.c_str(), flags, 0644);
                    if (outFd < 0) {
                        perror(("open: " + stage.outputFile).c_str());
                        if (savedIn >= 0) {
                            dup2(savedIn, STDIN_FILENO);
                            close(savedIn);
                        }
                        return;
                    }
                    savedOut = dup(STDOUT_FILENO);
                    dup2(outFd, STDOUT_FILENO);
                    close(outFd);
                }

                executeBuiltin(stage.tokens);

                if (savedIn >= 0) {
                    dup2(savedIn, STDIN_FILENO);
                    close(savedIn);
                }
                if (savedOut >= 0) {
                    dup2(savedOut, STDOUT_FILENO);
                    close(savedOut);
                }

                updateCurrentDir();
                return;
            }
        }

        // External System Command
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        }

        if (pid == 0) {
            setpgid(0, 0);
            restoreDefaultSignals();

            if (!stage.inputFile.empty()) {
                int inFd = open(stage.inputFile.c_str(), O_RDONLY);
                if (inFd < 0) {
                    perror(("open: " + stage.inputFile).c_str());
                    exit(EXIT_FAILURE);
                }
                dup2(inFd, STDIN_FILENO);
                close(inFd);
            }

            if (!stage.outputFile.empty()) {
                int flags = O_WRONLY | O_CREAT | (stage.append ? O_APPEND : O_TRUNC);
                int outFd = open(stage.outputFile.c_str(), flags, 0644);
                if (outFd < 0) {
                    perror(("open: " + stage.outputFile).c_str());
                    exit(EXIT_FAILURE);
                }
                dup2(outFd, STDOUT_FILENO);
                close(outFd);
            }

            std::vector<char*> args;
            for (const auto& tok : stage.tokens) {
                args.push_back(const_cast<char*>(tok.c_str()));
            }
            args.push_back(nullptr);

            execvp(args[0], args.data());
            perror(("execvp: " + stage.tokens[0]).c_str());
            exit(127);
        } else {
            if (background) {
                std::cout << "[" << pid << "]" << std::endl;
                addJob(pid, stage.tokens[0], false);
            } else {
                foregroundPid = pid;
                int status;
                while (waitpid(pid, &status, WUNTRACED) == -1 && errno == EINTR) {}
                if (WIFSTOPPED(status)) {
                    addJob(pid, stage.tokens[0], true);
                    std::cout << "\n[" << jobList.back().jobId << "] Stopped " << stage.tokens[0] << std::endl;
                }
                foregroundPid = 0;
            }
            return;
        }
    }

    // CASE 2: Multi-stage Pipeline (stages.size() > 1)
    std::vector<int> pipeFds(2 * (numStages - 1));
    for (size_t i = 0; i < numStages - 1; ++i) {
        if (pipe(&pipeFds[2 * i]) < 0) {
            perror("pipe");
            for (size_t j = 0; j < 2 * i; ++j) close(pipeFds[j]);
            return;
        }
    }

    std::vector<pid_t> pids;
    pid_t pgid = 0;

    for (size_t i = 0; i < numStages; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            break;
        }

        if (pid == 0) {
            // Child process
            if (i == 0) {
                setpgid(0, 0);
            } else {
                setpgid(0, pgid);
            }
            restoreDefaultSignals();

            // Connect input from previous pipe
            if (i > 0) {
                dup2(pipeFds[2 * (i - 1)], STDIN_FILENO);
            }
            // Connect output to next pipe
            if (i < numStages - 1) {
                dup2(pipeFds[2 * i + 1], STDOUT_FILENO);
            }

            // Close all pipe fds in child
            for (size_t j = 0; j < 2 * (numStages - 1); ++j) {
                close(pipeFds[j]);
            }

            // Stage input redirection overrides pipe input
            if (!stages[i].inputFile.empty()) {
                int inFd = open(stages[i].inputFile.c_str(), O_RDONLY);
                if (inFd < 0) {
                    perror(("open: " + stages[i].inputFile).c_str());
                    exit(EXIT_FAILURE);
                }
                dup2(inFd, STDIN_FILENO);
                close(inFd);
            }

            // Stage output redirection overrides pipe output
            if (!stages[i].outputFile.empty()) {
                int flags = O_WRONLY | O_CREAT | (stages[i].append ? O_APPEND : O_TRUNC);
                int outFd = open(stages[i].outputFile.c_str(), flags, 0644);
                if (outFd < 0) {
                    perror(("open: " + stages[i].outputFile).c_str());
                    exit(EXIT_FAILURE);
                }
                dup2(outFd, STDOUT_FILENO);
                close(outFd);
            }

            if (isBuiltin(stages[i].tokens[0])) {
                int code = executeBuiltin(stages[i].tokens);
                exit(code);
            } else {
                std::vector<char*> args;
                for (const auto& tok : stages[i].tokens) {
                    args.push_back(const_cast<char*>(tok.c_str()));
                }
                args.push_back(nullptr);

                execvp(args[0], args.data());
                perror(("execvp: " + stages[i].tokens[0]).c_str());
                exit(127);
            }
        } else {
            // Parent process
            if (i == 0) {
                pgid = pid;
            }
            setpgid(pid, pgid);
            pids.push_back(pid);
        }
    }

    // Parent closes all pipe fds
    for (size_t j = 0; j < 2 * (numStages - 1); ++j) {
        close(pipeFds[j]);
    }

    if (background) {
        if (!pids.empty()) {
            std::cout << "[" << pids.back() << "]" << std::endl;
            std::string summary;
            for (const auto& s : stages) {
                if (!summary.empty()) summary += " | ";
                summary += s.tokens[0];
            }
            addJob(pids.back(), summary, false);
        }
    } else {
        if (!pids.empty()) {
            foregroundPid = pgid;
            for (pid_t cpid : pids) {
                int status;
                while (waitpid(cpid, &status, WUNTRACED) == -1 && errno == EINTR) {}
            }
            foregroundPid = 0;
        }
    }
}
