#include "Shell.h"
#include <unistd.h>
#include <pwd.h>
#include <limits.h>

std::string getHomeDir() {
    const char* home = getenv("HOME");
    if (home) return home;
    
    struct passwd *pw = getpwuid(getuid());
    return pw->pw_dir;
}

std::string getUserName() {
    struct passwd *pw = getpwuid(getuid());
    return pw->pw_name;
}

std::string getSystemName() {
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    return hostname;
}
