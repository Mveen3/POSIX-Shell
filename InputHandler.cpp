#include "Shell.h"

static std::string currentInput;
static size_t cursorPosition = 0;
static int historyIndex = -1;

void setRawMode(bool enable) {
    static struct termios original;
    static bool initialized = false;
    
    if (!initialized) {
        tcgetattr(STDIN_FILENO, &original);
        initialized = true;
    }
    
    if (enable) {
        struct termios raw = original;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &original);
    }
}

void moveCursorLeft(int count) {
    for (int i = 0; i < count; i++) {
        std::cout << "\b";
    }
    std::cout.flush();
}

void moveCursorRight(int count) {
    for (int i = 0; i < count; i++) {
        std::cout << "\033[C";
    }
    std::cout.flush();
}

void clearInputDisplay() {
    std::cout << "\r\033[K";
    std::cout.flush();
}

void redisplayPromptAndInput(const std::string& input, size_t cursorPos) {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    std::string currentDir = cwd;
    std::string homeDir = getenv("HOME");
    if (currentDir.find(homeDir) == 0) {
        currentDir.replace(0, homeDir.length(), "~");
    }
    
    std::cout << "\033[1;32m" << getenv("USER") << "@";
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    std::cout << hostname << "\033[0m:\033[1;34m";
    std::cout << currentDir << "\033[0m> ";
    
    std::cout << input;
    
    if (cursorPos < input.size()) {
        moveCursorLeft(input.size() - cursorPos);
    }
    std::cout.flush();
}

std::string getInputLine() {
    setRawMode(true);
    currentInput.clear();
    cursorPosition = 0;
    historyIndex = -1;
    
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 4) {
            std::cout << std::endl;
            setRawMode(false);
            exit(0);
        }
        
        if (c == '\n' || c == '\r') {
            std::cout << std::endl;
            break;
        }
        
        else if (c == '\t') {
            std::string lastWord = currentInput;
            size_t lastSpace = currentInput.find_last_of(" \t");
            if (lastSpace != std::string::npos) {
                lastWord = currentInput.substr(lastSpace + 1);
            } else if (currentInput.empty()) {
                lastWord = "";
            }
            
            std::vector<std::string> suggestions;
            suggestions = completeFile(lastWord);
            
            if (suggestions.empty() && lastSpace == std::string::npos) {
                suggestions = completeCommand(lastWord);
            }
            
            if (!suggestions.empty()) {
                clearInputDisplay();
                
                if (suggestions.size() == 1) {
                    if (lastSpace != std::string::npos) {
                        currentInput = currentInput.substr(0, lastSpace + 1) + suggestions[0];
                    } else {
                        currentInput = suggestions[0];
                    }
                    cursorPosition = currentInput.size();
                } else {
                    std::string commonPrefix = suggestions[0];
                    for (size_t i = 1; i < suggestions.size(); i++) {
                        size_t j = 0;
                        while (j < commonPrefix.size() && j < suggestions[i].size() && 
                               commonPrefix[j] == suggestions[i][j]) {
                            j++;
                        }
                        commonPrefix = commonPrefix.substr(0, j);
                    }
                    
                    if (!commonPrefix.empty() && commonPrefix != lastWord) {
                        if (lastSpace != std::string::npos) {
                            currentInput = currentInput.substr(0, lastSpace + 1) + commonPrefix;
                        } else {
                            currentInput = commonPrefix;
                        }
                        cursorPosition = currentInput.size();
                    } else {
                        std::cout << "\n";
                        for (const auto& suggestion : suggestions) {
                            std::cout << suggestion << " ";
                        }
                        std::cout << "\n";
                    }
                }
                
                redisplayPromptAndInput(currentInput, cursorPosition);
            }
        }
        
        else if (c == 127 || c == 8) {
            if (cursorPosition > 0) {
                currentInput.erase(cursorPosition - 1, 1);
                cursorPosition--;
                clearInputDisplay();
                redisplayPromptAndInput(currentInput, cursorPosition);
            }
        }
        
        else if (c == '\x1b') {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;
            
            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A':
                        if (!isHistoryEmpty()) {
                            if (historyIndex < static_cast<int>(getHistorySize()) - 1) {
                                historyIndex++;
                            }
                            clearInputDisplay();
                            currentInput = getHistoryList()[getHistorySize() - 1 - historyIndex];
                            cursorPosition = currentInput.size();
                            redisplayPromptAndInput(currentInput, cursorPosition);
                        }
                        break;
                        
                    case 'B':
                        if (historyIndex > 0) {
                            historyIndex--;
                            clearInputDisplay();
                            currentInput = getHistoryList()[getHistorySize() - 1 - historyIndex];
                            cursorPosition = currentInput.size();
                            redisplayPromptAndInput(currentInput, cursorPosition);
                        } else if (historyIndex == 0) {
                            historyIndex = -1;
                            clearInputDisplay();
                            currentInput.clear();
                            cursorPosition = 0;
                            redisplayPromptAndInput(currentInput, cursorPosition);
                        }
                        break;
                        
                    case 'C':
                        if (cursorPosition < currentInput.size()) {
                            cursorPosition++;
                            moveCursorRight(1);
                        }
                        break;
                        
                    case 'D':
                        if (cursorPosition > 0) {
                            cursorPosition--;
                            moveCursorLeft(1);
                        }
                        break;
                }
            }
        }
        
        else if (c >= 32 && c <= 126) {
            currentInput.insert(cursorPosition, 1, c);
            cursorPosition++;
            
            clearInputDisplay();
            redisplayPromptAndInput(currentInput, cursorPosition);
        }
    }
    
    setRawMode(false);
    return currentInput;
}

void resetHistoryIndex() {
    historyIndex = -1;
}
