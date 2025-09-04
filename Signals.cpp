#include "Shell.h"
#include <iostream>
#include <csignal>
#include <unistd.h>

void setupSignals() {
    signal(SIGINT, handleSIGINT);
    signal(SIGTSTP, handleSIGTSTP);
    signal(SIGCHLD, SIG_IGN);
}

void handleSIGINT(int) {
    // CTRL-C - interrupt foreground process
    std::cout << std::endl;
}

void handleSIGTSTP(int) {
    // CTRL-Z - stop foreground process
    std::cout << std::endl;
}
