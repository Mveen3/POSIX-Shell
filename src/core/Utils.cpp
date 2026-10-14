#include "Shell.h"
#include <unistd.h>
#include <pwd.h>
#include <climits>

std::string getUserName() {
    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_name) {
        return pw->pw_name;
    }
    const char* userEnv = getenv("USER");
    if (userEnv) {
        return userEnv;
    }
    return "user";
}

std::string getSystemName() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        hostname[sizeof(hostname) - 1] = '\0';
        return hostname;
    }
    return "linux";
}

std::string getShellHomeDir() {
    return shellHomeDir;
}

std::string expandHomePath(const std::string& path) {
    if (path == "~") {
        return shellHomeDir;
    }
    if (path.rfind("~/", 0) == 0) {
        return shellHomeDir + path.substr(1);
    }
    return path;
}

std::string formatPromptPath(const std::string& path) {
    if (path == shellHomeDir) {
        return "~";
    }
    if (path.rfind(shellHomeDir + "/", 0) == 0) {
        return "~" + path.substr(shellHomeDir.length());
    }
    return path;
}

std::string getPromptString() {
    return "<" + userName + "@" + systemName + ":" + currentDir + "> ";
}
