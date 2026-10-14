#include "Shell.h"
#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <cerrno>
#include <cstdlib>

static struct termios originalTermios;
static bool rawModeActive = false;
static bool termiosInitialized = false;

static void resetTerminalAtExit() {
    if (rawModeActive && termiosInitialized) {
        tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
        rawModeActive = false;
    }
}

void setRawMode(bool enable) {
    if (!isatty(STDIN_FILENO)) {
        return;
    }

    if (!termiosInitialized) {
        if (tcgetattr(STDIN_FILENO, &originalTermios) == 0) {
            termiosInitialized = true;
            atexit(resetTerminalAtExit);
        } else {
            return;
        }
    }

    if (enable) {
        struct termios raw = originalTermios;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        rawModeActive = true;
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
        rawModeActive = false;
    }
}

static void redisplayPromptAndInput(const std::string& input, size_t cursorPos) {
    std::string prompt = getPromptString();
    std::cout << "\r" << prompt << input << "\033[K";
    if (cursorPos < input.size()) {
        std::cout << "\033[" << (input.size() - cursorPos) << "D";
    }
    std::cout.flush();
}

static std::string savedUncommittedInput;
static int historyOffset = -1;

void resetHistoryIndex() {
    historyOffset = -1;
    savedUncommittedInput.clear();
}

std::string getInputLine() {
    // Non-interactive fallback (pipes, redirected input, test scripts)
    if (!isatty(STDIN_FILENO)) {
        std::string line;
        if (!std::getline(std::cin, line)) {
            return "exit";
        }
        return line;
    }

    setRawMode(true);
    std::string currentInput;
    size_t cursorPosition = 0;
    resetHistoryIndex();

    char c;
    while (true) {
        ssize_t bytesRead = read(STDIN_FILENO, &c, 1);

        if (bytesRead < 0) {
            if (errno == EINTR) {
                if (sigintReceived) {
                    sigintReceived = 0;
                    currentInput.clear();
                    cursorPosition = 0;
                    resetHistoryIndex();
                    redisplayPromptAndInput(currentInput, cursorPosition);
                }
                continue;
            }
            break;
        }

        if (bytesRead == 0) {
            // EOF
            std::cout << std::endl;
            setRawMode(false);
            saveHistory();
            exit(0);
        }

        // Ctrl-D (EOF)
        if (c == 4) {
            if (currentInput.empty()) {
                std::cout << std::endl;
                setRawMode(false);
                saveHistory();
                exit(0);
            }
            continue;
        }

        // Enter key (Newline / Carriage Return)
        if (c == '\n' || c == '\r') {
            std::cout << std::endl;
            break;
        }

        // Backspace
        if (c == 127 || c == 8) {
            if (cursorPosition > 0) {
                currentInput.erase(cursorPosition - 1, 1);
                cursorPosition--;
                redisplayPromptAndInput(currentInput, cursorPosition);
            }
            continue;
        }

        // TAB Autocomplete
        if (c == '\t') {
            // Identify word being completed
            size_t lastSpace = currentInput.rfind(' ', (cursorPosition > 0 ? cursorPosition - 1 : 0));
            std::string prefix;
            size_t wordStart = 0;

            if (lastSpace == std::string::npos) {
                wordStart = 0;
                prefix = currentInput.substr(0, cursorPosition);
            } else {
                wordStart = lastSpace + 1;
                prefix = currentInput.substr(wordStart, cursorPosition - wordStart);
            }

            std::vector<std::string> suggestions;
            if (wordStart == 0 && prefix.find('/') == std::string::npos) {
                // First word: command autocomplete
                suggestions = completeCommand(prefix);
                if (suggestions.empty()) {
                    suggestions = completeFile(prefix);
                }
            } else {
                // Arguments: file autocomplete
                suggestions = completeFile(prefix);
            }

            if (!suggestions.empty()) {
                if (suggestions.size() == 1) {
                    std::string match = suggestions[0];
                    currentInput.replace(wordStart, prefix.size(), match);
                    cursorPosition = wordStart + match.size();
                    redisplayPromptAndInput(currentInput, cursorPosition);
                } else {
                    // Compute longest common prefix
                    std::string commonPrefix = suggestions[0];
                    for (size_t i = 1; i < suggestions.size(); ++i) {
                        size_t j = 0;
                        while (j < commonPrefix.size() && j < suggestions[i].size() &&
                               commonPrefix[j] == suggestions[i][j]) {
                            ++j;
                        }
                        commonPrefix = commonPrefix.substr(0, j);
                    }

                    if (commonPrefix.size() > prefix.size()) {
                        currentInput.replace(wordStart, prefix.size(), commonPrefix);
                        cursorPosition = wordStart + commonPrefix.size();
                        redisplayPromptAndInput(currentInput, cursorPosition);
                    } else {
                        // Display suggestions matching problem statement
                        std::cout << "\n";
                        for (size_t s = 0; s < suggestions.size(); ++s) {
                            if (s > 0) std::cout << " ";
                            std::cout << suggestions[s];
                        }
                        std::cout << "\n";
                        redisplayPromptAndInput(currentInput, cursorPosition);
                    }
                }
            }
            continue;
        }

        // Escape Sequences (Arrow keys, Delete, Home, End)
        if (c == '\x1b') {
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;

            if (seq[0] == '[') {
                if (seq[1] == 'A') {
                    // UP Arrow: navigate backward in history
                    if (!isHistoryEmpty()) {
                        if (historyOffset == -1) {
                            savedUncommittedInput = currentInput;
                        }
                        if (historyOffset + 1 < static_cast<int>(getHistorySize())) {
                            historyOffset++;
                            currentInput = getHistoryList()[getHistorySize() - 1 - historyOffset];
                            cursorPosition = currentInput.size();
                            redisplayPromptAndInput(currentInput, cursorPosition);
                        }
                    }
                } else if (seq[1] == 'B') {
                    // DOWN Arrow: navigate forward in history
                    if (historyOffset > 0) {
                        historyOffset--;
                        currentInput = getHistoryList()[getHistorySize() - 1 - historyOffset];
                        cursorPosition = currentInput.size();
                        redisplayPromptAndInput(currentInput, cursorPosition);
                    } else if (historyOffset == 0) {
                        historyOffset = -1;
                        currentInput = savedUncommittedInput;
                        cursorPosition = currentInput.size();
                        redisplayPromptAndInput(currentInput, cursorPosition);
                    }
                } else if (seq[1] == 'C') {
                    // RIGHT Arrow: move cursor right
                    if (cursorPosition < currentInput.size()) {
                        cursorPosition++;
                        std::cout << "\033[C";
                        std::cout.flush();
                    }
                } else if (seq[1] == 'D') {
                    // LEFT Arrow: move cursor left
                    if (cursorPosition > 0) {
                        cursorPosition--;
                        std::cout << "\033[D";
                        std::cout.flush();
                    }
                } else if (seq[1] == 'H' || seq[1] == '1') {
                    // HOME
                    cursorPosition = 0;
                    redisplayPromptAndInput(currentInput, cursorPosition);
                } else if (seq[1] == 'F' || seq[1] == '4') {
                    // END
                    cursorPosition = currentInput.size();
                    redisplayPromptAndInput(currentInput, cursorPosition);
                } else if (seq[1] == '3') {
                    // DELETE key (seq[1]=='3', seq[2]=='~')
                    if (read(STDIN_FILENO, &seq[2], 1) == 1 && seq[2] == '~') {
                        if (cursorPosition < currentInput.size()) {
                            currentInput.erase(cursorPosition, 1);
                            redisplayPromptAndInput(currentInput, cursorPosition);
                        }
                    }
                }
            }
            continue;
        }

        // Printable Characters
        if (static_cast<unsigned char>(c) >= 32 && static_cast<unsigned char>(c) <= 126) {
            currentInput.insert(cursorPosition, 1, c);
            cursorPosition++;
            redisplayPromptAndInput(currentInput, cursorPosition);
        }
    }

    setRawMode(false);
    return currentInput;
}
