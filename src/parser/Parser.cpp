#include "Shell.h"
#include <cctype>
#include <algorithm>

std::vector<std::string> parseInput(const std::string& input) {
    return tokenizeInput(input);
}

std::vector<std::string> splitCommands(const std::string& input) {
    std::vector<std::string> commands;
    std::string current;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        if (c == '\'' && !inDoubleQuote) {
            inSingleQuote = !inSingleQuote;
            current += c;
        } else if (c == '"' && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
            current += c;
        } else if (!inSingleQuote && !inDoubleQuote && c == ';') {
            trimString(current);
            if (!current.empty()) {
                commands.push_back(current);
            }
            current.clear();
        } else if (!inSingleQuote && !inDoubleQuote && c == '&') {
            current += " &";
            trimString(current);
            if (!current.empty()) {
                commands.push_back(current);
            }
            current.clear();
        } else {
            current += c;
        }
    }

    trimString(current);
    if (!current.empty()) {
        commands.push_back(current);
    }

    return commands;
}

std::vector<std::string> tokenizeInput(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool hadQuote = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];

        if (inSingleQuote) {
            if (c == '\'') {
                inSingleQuote = false;
            } else {
                current += c;
            }
        } else if (inDoubleQuote) {
            if (c == '"') {
                inDoubleQuote = false;
            } else {
                current += c;
            }
        } else {
            if (c == '\'') {
                inSingleQuote = true;
                hadQuote = true;
            } else if (c == '"') {
                inDoubleQuote = true;
                hadQuote = true;
            } else if (std::isspace(static_cast<unsigned char>(c))) {
                if (!current.empty() || hadQuote) {
                    tokens.push_back(current);
                    current.clear();
                    hadQuote = false;
                }
            } else if (c == '>' || c == '<' || c == '|' || c == '&') {
                if (!current.empty() || hadQuote) {
                    tokens.push_back(current);
                    current.clear();
                    hadQuote = false;
                }
                if (c == '>' && i + 1 < input.size() && input[i + 1] == '>') {
                    tokens.push_back(">>");
                    ++i;
                } else {
                    tokens.push_back(std::string(1, c));
                }
            } else {
                current += c;
            }
        }
    }

    if (!current.empty() || hadQuote) {
        tokens.push_back(current);
    }

    return tokens;
}

void trimString(std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        str.clear();
        return;
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    str = str.substr(first, last - first + 1);
}
