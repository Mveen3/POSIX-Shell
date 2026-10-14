#ifndef SHELL_H
#define SHELL_H

#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <termios.h>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <cctype>
#include <climits>
#include <csignal>
#include <memory>
#include <cerrno>

// Job structure for background and job control
struct BackgroundJob {
    int jobId;
    pid_t pid;
    std::string command;
    bool isRunning;
    bool isStopped;
};

// Command stage in a pipeline
struct CommandStage {
    std::vector<std::string> tokens;
    std::string inputFile;
    std::string outputFile;
    bool append;

    CommandStage() : append(false) {}
};

// Global variables
extern std::string currentDir;
extern std::string homeDir;
extern std::string shellHomeDir;
extern std::string previousDir; // Internal previous directory tracking (NO OLDPWD env!)
extern std::string userName;
extern std::string systemName;
extern volatile sig_atomic_t foregroundPid;
extern volatile sig_atomic_t sigintReceived;
extern std::vector<BackgroundJob> jobList;

// RAII helper for file descriptors
class FdGuard {
private:
    int fd;
public:
    explicit FdGuard(int f = -1) : fd(f) {}
    ~FdGuard() {
        if (fd >= 0) close(fd);
    }
    int get() const { return fd; }
    int release() {
        int temp = fd;
        fd = -1;
        return temp;
    }
    void reset(int f = -1) {
        if (fd >= 0) close(fd);
        fd = f;
    }
    FdGuard(const FdGuard&) = delete;
    FdGuard& operator=(const FdGuard&) = delete;
};

// Shell Lifecycle & Environment
void initializeShell();
void runShell();
void updateCurrentDir();
void displayPrompt();
std::string getPromptString();
std::string getShellHomeDir();
std::string expandHomePath(const std::string& path);
std::string formatPromptPath(const std::string& path);
std::string getUserName();
std::string getSystemName();

// Signal & Job Management
void setupSignals();
void restoreDefaultSignals();
void handleSIGINT(int sig);
void handleSIGTSTP(int sig);
void handleSIGCHLD(int sig);
void addJob(pid_t pid, const std::string& command, bool stopped = false);
void checkBackgroundJobs();

// Command Execution Engine
bool isBuiltin(const std::string& cmd);
int executeBuiltin(const std::vector<std::string>& tokens);
void executeCommandPipeline(const std::vector<std::string>& tokens, bool background);
void executeSystemCommand(const std::vector<std::string>& tokens, bool background);

// Built-in Commands
void cd(const std::vector<std::string>& tokens);
void echo(const std::vector<std::string>& tokens);
void pwd();
void ls(const std::vector<std::string>& tokens);
void history(const std::vector<std::string>& tokens);
void executePinfo(const std::vector<std::string>& tokens);
void printProcessInfo(pid_t pid);
void executeSearch(const std::vector<std::string>& tokens);
bool searchRecursive(const std::string& path, const std::string& target);

// Pipeline & Redirection parsing
bool hasPipeline(const std::vector<std::string>& tokens);
bool hasRedirection(const std::vector<std::string>& tokens);
bool parseStageRedirection(const std::vector<std::string>& stageTokens, CommandStage& stage);
void handlePipeline(const std::vector<std::string>& tokens, bool background = false);
void handleRedirection(const std::vector<std::string>& tokens);

// Input & Parsing
std::string getInputLine();
std::vector<std::string> splitCommands(const std::string& input);
std::vector<std::string> parseInput(const std::string& input);
std::vector<std::string> tokenizeInput(const std::string& input);
void trimString(std::string& str);
void setRawMode(bool enable);

// History
void addToHistory(const std::string& command);
void printHistory(int count = 10);
void loadHistory();
void saveHistory();
const std::vector<std::string>& getHistoryList();
bool isHistoryEmpty();
size_t getHistorySize();
void resetHistoryIndex();

// Autocomplete
std::vector<std::string> completeCommand(const std::string& input);
std::vector<std::string> completeFile(const std::string& input);

#endif // SHELL_H
