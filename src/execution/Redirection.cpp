#include "Shell.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>

bool hasRedirection(const std::vector<std::string>& tokens) {
    for (const auto& token : tokens) {
        if (token == "<" || token == ">" || token == ">>") {
            return true;
        }
    }
    return false;
}

bool parseStageRedirection(const std::vector<std::string>& stageTokens, CommandStage& stage) {
    stage.tokens.clear();
    stage.inputFile.clear();
    stage.outputFile.clear();
    stage.append = false;

    for (size_t i = 0; i < stageTokens.size(); ++i) {
        const std::string& tok = stageTokens[i];
        if (tok == "<") {
            if (i + 1 >= stageTokens.size()) {
                std::cerr << "Syntax error: missing input redirection target" << std::endl;
                return false;
            }
            stage.inputFile = expandHomePath(stageTokens[i + 1]);
            ++i;
        } else if (tok == ">" || tok == ">>") {
            if (i + 1 >= stageTokens.size()) {
                std::cerr << "Syntax error: missing output redirection target" << std::endl;
                return false;
            }
            stage.outputFile = expandHomePath(stageTokens[i + 1]);
            stage.append = (tok == ">>");
            ++i;
        } else {
            stage.tokens.push_back(tok);
        }
    }
    return true;
}

void handleRedirection(const std::vector<std::string>& tokens) {
    // Forwarded to unified command pipeline execution engine
    executeCommandPipeline(tokens, false);
}
