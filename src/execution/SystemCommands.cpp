#include "Shell.h"

void executeSystemCommand(const std::vector<std::string>& tokens, bool background) {
    executeCommandPipeline(tokens, background);
}
