#include "Shell.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <climits>
#include <cstring>

void executePinfo(const std::vector<std::string>& tokens) {
    pid_t pid;

    if (tokens.size() > 1) {
        try {
            pid = static_cast<pid_t>(std::stol(tokens[1]));
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
    std::string procDir = "/proc/" + std::to_string(pid);

    struct stat st;
    if (stat(procDir.c_str(), &st) == -1) {
        std::cerr << "pinfo: process " << pid << " does not exist" << std::endl;
        return;
    }

    // Read and parse /proc/<pid>/stat
    std::string statPath = procDir + "/stat";
    std::ifstream statFile(statPath);
    if (!statFile.is_open()) {
        std::cerr << "pinfo: cannot access process stat for PID " << pid << std::endl;
        return;
    }

    std::string statLine;
    if (!std::getline(statFile, statLine)) {
        std::cerr << "pinfo: failed to read stat for PID " << pid << std::endl;
        return;
    }

    size_t openParen = statLine.find('(');
    size_t closeParen = statLine.rfind(')');
    if (openParen == std::string::npos || closeParen == std::string::npos || closeParen <= openParen) {
        std::cerr << "pinfo: corrupted stat format for PID " << pid << std::endl;
        return;
    }

    std::string remainder = statLine.substr(closeParen + 1);
    std::istringstream iss(remainder);

    char stateChar = '?';
    long ppid = 0, pgrp = 0, session = 0, tty_nr = 0, tpgid = 0;
    if (iss >> stateChar >> ppid >> pgrp >> session >> tty_nr >> tpgid) {
        // Success
    }

    std::string stateStr(1, stateChar);
    if (pgrp > 0 && pgrp == tpgid) {
        stateStr += "+";
    }

    // Read Virtual Memory size from /proc/<pid>/status
    std::string statusPath = procDir + "/status";
    std::ifstream statusFile(statusPath);
    long vmSize = 0;
    if (statusFile.is_open()) {
        std::string line;
        while (std::getline(statusFile, line)) {
            if (line.find("VmSize:") == 0) {
                std::istringstream vmiss(line.substr(7));
                vmiss >> vmSize;
                break;
            }
        }
    }

    // Read Executable Path from /proc/<pid>/exe
    char exeBuf[PATH_MAX];
    std::string exePath = "Unknown";
    ssize_t len = readlink((procDir + "/exe").c_str(), exeBuf, sizeof(exeBuf) - 1);
    if (len != -1) {
        exeBuf[len] = '\0';
        exePath = formatPromptPath(std::string(exeBuf));
    }

    std::cout << "Process Status -- " << stateStr << std::endl;
    std::cout << "memory -- " << vmSize << " {Virtual Memory}" << std::endl;
    std::cout << "Executable Path -- " << exePath << std::endl;
}
