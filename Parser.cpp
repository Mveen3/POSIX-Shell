#include "Shell.h"
#include <cstring>
#include <cctype>

std::vector<std::string> parseInput(const std::string& input) {
    return tokenizeInput(input);
}

std::vector<std::string> splitCommands(const std::string& input) {
    std::vector<std::string> commands;
    char* inputCopy = new char[input.length() + 1];
    strcpy(inputCopy, input.c_str());
    
    char* token = strtok(inputCopy, ";");
    while (token != nullptr) {
        std::string cmd = token;
        trimString(cmd);
        if (!cmd.empty()) {
            commands.push_back(cmd);
        }
        token = strtok(nullptr, ";");
    }
    
    delete[] inputCopy;
    return commands;
}

std::vector<std::string> tokenizeInput(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current = "";
    bool inQuotes = false;
    char quoteChar = '\0';
    
    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        
        if (inQuotes) {
            if (c == quoteChar) {
                inQuotes = false;
                if (!current.empty()) {
                    tokens.push_back(current);
                    current = "";
                }
            } else {
                current += c;
            }
        } else {
            if (c == '"' || c == '\'') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current = "";
                }
                inQuotes = true;
                quoteChar = c;
            } else if (isspace(c)) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current = "";
                }
            } else if (c == '|' || c == '<' || c == '>' || c == '&') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current = "";
                }
                if (c == '>' && i + 1 < input.length() && input[i + 1] == '>') {
                    tokens.push_back(">>");
                    i++;
                } else {
                    tokens.push_back(std::string(1, c));
                }
            } else {
                current += c;
            }
        }
    }
    
    if (!current.empty()) {
        tokens.push_back(current);
    }
    
    return tokens;
}

void trimString(std::string& str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    
    str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), str.end());
}
