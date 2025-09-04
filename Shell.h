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

// Global variables
extern std::string currentDir;
extern std::string homeDir;
extern std::string shellHomeDir;
extern std::string userName;
extern std::string systemName;

// Function declarations
std::vector<std::string> completeCommand(const std::string& input);
std::vector<std::string> completeFile(const std::string& input);

bool executeBuiltin(const std::vector<std::string>& tokens);
bool executeBuiltinWithRedirection(const std::vector<std::string>& commandTokens, const std::string& inputFile, const std::string& outputFile, bool append);
void cd(const std::vector<std::string>& tokens);
void echo(const std::vector<std::string>& tokens);
void pwd();
void ls(const std::vector<std::string>& tokens);
void history(const std::vector<std::string>& tokens);
void echoWithRedirection(const std::vector<std::string>& tokens, const std::string& outputFile, bool append);
void lsWithRedirection(const std::vector<std::string>& tokens, const std::string& outputFile, bool append);

void addToHistory(const std::string& command);
void printHistory(int count = 10);
void loadHistory();
void saveHistory();
const std::vector<std::string>& getHistoryList();
bool isHistoryEmpty();
size_t getHistorySize();

std::string getInputLine();
void resetHistoryIndex();

std::vector<std::string> parseInput(const std::string& input);
std::vector<std::string> splitCommands(const std::string& input);
std::vector<std::string> tokenizeInput(const std::string& input);
void trimString(std::string& str);

void executePinfo(const std::vector<std::string>& tokens);
void printProcessInfo(pid_t pid);

bool hasPipeline(const std::vector<std::string>& tokens);
void handlePipeline(const std::vector<std::string>& tokens, bool background = false);

bool hasRedirection(const std::vector<std::string>& tokens);
void handleRedirection(const std::vector<std::string>& tokens);
void executeWithRedirection(const std::vector<std::string>& commandTokens, const std::string& inputFile, const std::string& outputFile, bool append);

void executeSearch(const std::vector<std::string>& tokens);
bool searchRecursive(const std::string& path, const std::string& target);

void runShell();
void initializeShell();
void updateCurrentDir();
void displayPrompt();
std::string getShellHomeDir();

void setupSignals();
void handleSIGINT(int sig);
void handleSIGTSTP(int sig);

void executeSystemCommand(const std::vector<std::string>& tokens, bool background);

std::string getHomeDir();
std::string getUserName();
std::string getSystemName();

#endif
