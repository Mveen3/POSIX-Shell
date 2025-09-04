#include "Shell.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <limits.h>

void executePinfo(const std::vector<std::string>& tokens) {
    pid_t pid;
    
    if (tokens.size() > 1) {
        try {
            pid = std::stoi(tokens[1]);
        } catch (...) {
            std::cerr << "pinfo: invalid PID" << std::endl;
            return;
        }
    } else {
        pid = getpid();
    }
    
    printProcessInfo(pid);
}

void printProcessInfo(pid_t pid) {
    std::string procPath = "/proc/" + std::to_string(pid);
    
    // Check if process exists
    struct stat st;
    if (stat(procPath.c_str(), &st) == -1) {
        std::cerr << "pinfo: process " << pid << " does not exist" << std::endl;
        return;
    }

    // Read process status from stat file
    std::ifstream statFile(procPath + "/stat");
    if (!statFile) {
        std::cerr << "pinfo: cannot access process info" << std::endl;
        return;
    }

    std::string line;
    std::getline(statFile, line);
    std::istringstream iss(line);
    
    // Parse the stat file
    std::string pid_str, comm, state, ppid, pgrp, session, tty_nr;
    iss >> pid_str >> comm >> state >> ppid >> pgrp >> session >> tty_nr;
    
    // Read VmSize from status file
    std::ifstream statusFile(procPath + "/status");
    long vmSize = 0;
    while (std::getline(statusFile, line)) {
        if (line.find("VmSize:") == 0) {
            std::istringstream vmiss(line.substr(7));
            vmiss >> vmSize;
            break;
        }
    }

    // Read executable path
    char exePath[PATH_MAX];
    ssize_t len = readlink((procPath + "/exe").c_str(), exePath, sizeof(exePath) - 1);
    if (len != -1) {
        exePath[len] = '\0';
    } else {
        strcpy(exePath, "Unknown");
    }

    std::cout << "Process Status: " << state << std::endl;
    std::cout << "Memory: " << vmSize << " kB" << std::endl;
    std::cout << "Executable Path: " << exePath << std::endl;
}
