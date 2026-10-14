# POSIX Interactive Shell & Systems Programming Project

A high-performance, robust, and POSIX-compliant custom shell implementation in modern C++ featuring advanced process management, asynchronous signal handling, pipeline processing, I/O redirection, terminal line discipline with tab autocomplete, and virtual memory inspection via `/proc`.

Engineered for systems software and embedded engineering standards (Qualcomm, NVIDIA, Texas Instruments).

---

## Architecture Overview

```mermaid
graph TD
    User([User Terminal / TTY]) <-->|Raw Input / VT100 Escapes| InputHandler[Input Handler & Line Editor]
    InputHandler -->|Raw Command String| Parser[Quote-Aware Tokenizer & Parser]
    Parser -->|Command Stages & Pipes| Pipeline[Execution Engine & Pipeline Coordinator]
    
    subgraph Execution Coordination
        Pipeline -->|Built-in Commands| Builtins[Built-in Handlers: cd, pwd, echo, ls, history, pinfo, search]
        Pipeline -->|Fork & Execvp| SystemCommands[External Process Groups via setpgid]
        Pipeline -->|IPC & Redirection| FdManagement[File Descriptor Management: pipe, dup2]
    end
    
    subgraph Signal & Process Management
        Kernel([Linux Kernel]) -->|SIGINT / SIGTSTP / SIGCHLD| Signals[Async-Signal-Safe Event Queue]
        Signals -->|Reap Non-blocking| ZombieReaping[waitpid WNOHANG]
        Signals -->|Forward Signals| JobControl[Foreground Group / Job Table]
    end
    
    Builtins -->|/proc Virtual FS| ProcFS[/proc/PID/stat & status]
```

---

## Core Features

- **Interactive Shell Prompt**: Dynamic POSIX prompt formatted as `<username@hostname:current_directory> ` where the invoking directory is recognized as `~`.
- **Async-Signal-Safe Signal Handling & Job Control**:
  - `Ctrl+C` (`SIGINT`): Terminates foreground process group; clears line and resets prompt safely if shell is idle.
  - `Ctrl+Z` (`SIGTSTP`): Pushes foreground process group to background in stopped state (`[jobId] Stopped`).
  - `Ctrl+D` (`EOF`): Clean logout with terminal restoration and history persistence.
  - `SIGCHLD`: Reentrant, async-signal-safe circular event queue for asynchronous background process reaping without blocking the prompt.
- **Arbitrary Command Pipelines (`|`)**: Multi-stage pipelines supporting arbitrary depth, with complete descriptor leak prevention.
- **Full I/O Redirection (`<`, `>`, `>>`)**: Stage-level redirection overriding pipe ends when specified, supporting permissions `0644`.
- **Integrated Built-in Support in Pipelines**: Built-in commands (`echo`, `pwd`, `ls`, `history`, `pinfo`, `search`, `cd`) execute natively in parent shell or as child stages within pipelines (`echo "hi" | wc -w`, `ls -l | grep cpp`).
- **Terminal Raw Mode & Line Editing**:
  - Custom VT100 / ANSI escape sequence decoder.
  - Arrow keys: Up/Down history browsing (with buffer restoration), Left/Right cursor positioning, Home/End, Backspace, Delete.
  - Smart TAB autocomplete: Auto-completes builtins and system executables in `$PATH` for commands, plus files and directories with trailing `/` and common prefix completion.
  - Non-interactive fallback: Auto-detects non-TTY stdin (`!isatty`) for automated scripting and test suites.
- **Process Inspection (`pinfo`)**: Direct Linux `/proc/<pid>/stat` and `/proc/<pid>/status` virtual filesystem inspection reporting Virtual Memory, execution path, and foreground/background state via `tpgid == pgrp`.
- **Persistent Command History**: Stores up to 20 commands persistently across sessions in `~/.shell_history` with consecutive duplicate filtering.
- **Guideline Compliance**: Strictly conforms to zero use of disallowed environment variables (`OLDPWD`, `PWD`), no external libraries (`<filesystem>`, `ncurses`), and zero use of `popen`/`system`.

---

## Project Structure

```
├── include/
│   └── Shell.h               # Core header, types, and RAII file descriptor / terminal guards
├── src/
│   ├── core/                 # Shell lifecycle, main entry, signals, utilities
│   │   ├── main.cpp
│   │   ├── Shell.cpp
│   │   ├── Signals.cpp
│   │   └── Utils.cpp
│   ├── parser/               # Command tokenization and quote handling
│   │   └── Parser.cpp
│   ├── execution/            # Pipeline coordination and redirection
│   │   ├── Pipeline.cpp
│   │   ├── Redirection.cpp
│   │   └── SystemCommands.cpp
│   ├── builtins/             # Internal built-in command handlers
│   │   ├── Builtins.cpp
│   │   ├── History.cpp
│   │   ├── Pinfo.cpp
│   │   └── Search.cpp
│   └── terminal/             # Line editor, termios raw mode, TAB autocomplete
│       ├── InputHandler.cpp
│       └── Autocomplete.cpp
├── build/                    # Object files and header dependencies (.o, .d)
├── Makefile                  # Build system with test & sanitize targets
├── test_suite.py             # Automated Python verification test suite (16 tests)
├── TECHNICAL_REPORT.md       # In-depth systems engineering technical report
└── README.md                 # Project documentation
```

---

## Build and Run

### Compilation
The project requires a standard POSIX-compliant C++11 compiler (`g++` or `clang++`):

```bash
make
```

### Running the Shell
```bash
./shell
```

### Running the Automated Test Suite (16 Tests)
```bash
make test
```

### Clean Build Artifacts
```bash
make clean
```

### Compile & Test with Memory & Undefined Behavior Sanitizers
```bash
make sanitize
```


---

## Command Reference

### 1. `cd` (Change Directory)
- `cd`: Navigates to shell home directory (`~`).
- `cd ~`: Navigates to shell home directory.
- `cd ~/subdir`: Navigates to subpath relative to shell home directory.
- `cd -`: Navigates to previous working directory and prints destination path.
- `cd ..`: Moves to parent directory. Outside shell root, displays absolute path.
- `cd <path>`: Moves to arbitrary relative or absolute directory path.
*Internal directory tracking preserves compliance with constraints forbidding `OLDPWD`/`PWD` environment variables.*

### 2. `pwd`
Prints current absolute working directory using `getcwd`.

### 3. `echo`
Prints tokens with single-space normalization, preserving single/double quotes:
```bash
echo "Hello from POSIX shell"
```

### 4. `ls` (List Directory)
Supports `-a` (hidden entries), `-l` (long listing), combined flags (`-al`, `-la`), and arbitrary ordering of files and directories:
```bash
ls -la
ls -l Makefile
ls -l .. /tmp
```
- Computes and prints POSIX 1024-byte `total <blocks>` header for directory listings.
- Uses `lstat` to correctly identify symbolic links (`-> target`) without following broken links.

### 5. `pinfo` (Process Information)
Reads `/proc/<pid>/stat`, `/proc/<pid>/status`, and `/proc/<pid>/exe`:
```bash
pinfo        # Inspect shell process
pinfo 1234   # Inspect arbitrary PID
```
Output:
```
Process Status -- R+
memory -- 12345 {Virtual Memory}
Executable Path -- ~/shell
```
*Appends `+` if the process group matches the terminal process group (`pgrp == tpgid`).*

### 6. `search`
Recursively searches current working directory tree for a target file or folder:
```bash
search filename.txt   # Outputs: True or False
```

### 7. `history`
```bash
history       # Displays latest 10 commands (default)
history 5     # Displays latest 5 commands
```

### 8. Pipelines & Redirection
```bash
cat < input.txt | grep "pattern" | wc -l > output.txt
echo "hello world" | wc -w
ls -la | grep "\.cpp" >> sources.txt
```

### 9. Background Execution
Append `&` to run external or pipeline commands asynchronously:
```bash
sleep 5 &
```
Prints `[PID]` immediately, remains interactive, and announces normal or abnormal exit asynchronously on completion.
