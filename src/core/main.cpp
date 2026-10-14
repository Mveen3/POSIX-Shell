#include "Shell.h"

// Global variable definitions
std::string currentDir;
std::string homeDir;
std::string shellHomeDir;
std::string previousDir;
std::string userName;
std::string systemName;
volatile sig_atomic_t foregroundPid = 0;
volatile sig_atomic_t sigintReceived = 0;
std::vector<BackgroundJob> jobList;

int main() {
    runShell();
    return 0;
}
