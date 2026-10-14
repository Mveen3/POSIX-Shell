#include "Shell.h"
#include <iostream>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>
#include <cerrno>

// Async-signal-safe circular queue for child termination events
struct ChildExitEvent {
    pid_t pid;
    int status;
};

static const int MAX_SIGNAL_EVENTS = 128;
static volatile ChildExitEvent exitEvents[MAX_SIGNAL_EVENTS];
static volatile sig_atomic_t eventHead = 0;
static volatile sig_atomic_t eventTail = 0;

void handleSIGINT(int) {
    if (foregroundPid > 0) {
        kill(-foregroundPid, SIGINT);
        kill(foregroundPid, SIGINT);
    } else {
        sigintReceived = 1;
        const char newline[] = "\n";
        (void)write(STDOUT_FILENO, newline, 1);
    }
}

void handleSIGTSTP(int) {
    if (foregroundPid > 0) {
        kill(-foregroundPid, SIGTSTP);
        kill(foregroundPid, SIGTSTP);
    }
}

void handleSIGCHLD(int) {
    int savedErrno = errno;
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        // Record event in async-signal-safe circular buffer
        int nextHead = (eventHead + 1) % MAX_SIGNAL_EVENTS;
        if (nextHead != eventTail) {
            exitEvents[eventHead].pid = pid;
            exitEvents[eventHead].status = status;
            eventHead = nextHead;
        }
    }

    errno = savedErrno;
}

void setupSignals() {
    struct sigaction saInt;
    saInt.sa_handler = handleSIGINT;
    sigemptyset(&saInt.sa_mask);
    saInt.sa_flags = SA_RESTART;
    sigaction(SIGINT, &saInt, nullptr);

    struct sigaction saTstp;
    saTstp.sa_handler = handleSIGTSTP;
    sigemptyset(&saTstp.sa_mask);
    saTstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &saTstp, nullptr);

    struct sigaction saChld;
    saChld.sa_handler = handleSIGCHLD;
    sigemptyset(&saChld.sa_mask);
    saChld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &saChld, nullptr);

    // Ignore terminal stop signals in the shell process
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
}

void restoreDefaultSignals() {
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
}

void addJob(pid_t pid, const std::string& command, bool stopped) {
    BackgroundJob job;
    job.jobId = static_cast<int>(jobList.size()) + 1;
    job.pid = pid;
    job.command = command;
    job.isRunning = !stopped;
    job.isStopped = stopped;
    jobList.push_back(job);
}

void checkBackgroundJobs() {
    // Process reaped events collected by SIGCHLD handler
    while (eventTail != eventHead) {
        pid_t pid = exitEvents[eventTail].pid;
        int status = exitEvents[eventTail].status;
        eventTail = (eventTail + 1) % MAX_SIGNAL_EVENTS;

        // If this was the foreground process, it is handled by the foreground wait loop
        if (pid == foregroundPid) {
            continue;
        }

        bool found = false;
        for (auto it = jobList.begin(); it != jobList.end(); ) {
            if (it->pid == pid) {
                found = true;
                if (WIFEXITED(status)) {
                    std::cout << it->command << " with pid " << pid
                              << " exited normally with status " << WEXITSTATUS(status) << std::endl;
                    it = jobList.erase(it);
                } else if (WIFSIGNALED(status)) {
                    std::cout << it->command << " with pid " << pid
                              << " terminated by signal " << WTERMSIG(status) << std::endl;
                    it = jobList.erase(it);
                } else if (WIFSTOPPED(status)) {
                    std::cout << it->command << " with pid " << pid
                              << " stopped by signal " << WSTOPSIG(status) << std::endl;
                    it->isRunning = false;
                    it->isStopped = true;
                    ++it;
                } else if (WIFCONTINUED(status)) {
                    std::cout << it->command << " with pid " << pid << " continued" << std::endl;
                    it->isRunning = true;
                    it->isStopped = false;
                    ++it;
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }

        if (!found && (WIFEXITED(status) || WIFSIGNALED(status))) {
            // Background process not in job list (e.g., pipeline stage child)
            // Still reaped properly to prevent zombies
        }
    }
}
