# POSIX Shell Implementation

A comprehensive custom shell implementation in C++ that supports advanced features like pipelines, I/O redirection, background processes, job control, and interactive command-line editing.

## Features

- **Interactive Shell Prompt**: Dynamic prompt showing `username@hostname:current_directory>`
- **Built-in Commands**: `cd`, `echo`, `pwd`, `ls`, `history`, `pinfo`, `search`
- **Background & Foreground Execution**: Support for `&` operator
- **Command Pipelines**: Chain multiple commands with `|`
- **I/O Redirection**: Input/output redirection with `<`, `>`, and `>>`
- **Signal Handling**: Proper handling of Ctrl-C, Ctrl-Z, and Ctrl-D
- **TAB Autocomplete**: Smart completion for commands and file paths
- **Command History**: Persistent history with arrow key navigation
- **Process Management**: Advanced process information and control

## Requirements

- **Compiler**: C++ compatible compiler
- **Operating System**: Linux/Unix-based system
- **Dependencies**: Standard POSIX libraries

## Build Instructions Using Makefile

```bash
make
```

## Usage

### Starting the Shell
```bash
./shell
```

The shell will display a prompt in the format:
```
username@hostname:current_directory> 
```

### Exiting the Shell
- Type `exit` and press Enter or Press `Ctrl+D`

## Built-in Commands

### 1. **cd** - Change Directory
```bash
cd                    # Go to shell home directory
cd ~                  # Go to shell home directory
cd /path/to/dir       # Change to specified directory
cd ..                 # Go to parent directory
cd .                  # Stay in current directory
cd -                  # Go to previous directory (prints path)
```

**Error Handling:**
- Too many arguments: "cd: too many arguments"
- Invalid directory: Standard system error message

### 2. **echo** - Display Text
```bash
echo Hello World      # Prints: Hello World
```

### 3. **pwd** - Print Working Directory
```bash
pwd                   # Prints current absolute path
```

### 4. **ls** - List Directory Contents
```bash
ls                    # List current directory
ls -a                 # Include hidden files
ls -l                 # Long format listing
ls -al                # Both flags combined
ls -la                # Both flags combined (alternate)
ls directory_name     # List specific directory
ls -al dir1 dir2      # Multiple directories with flags
```

**Features:**
- `-a`: Shows hidden files (starting with '.')
- `-l`: Detailed listing with permissions, size, date, etc.
- Multiple flags can be combined: `-al`, `-la`
- Multiple directories supported
- Flags and directories can be in any order

### 5. **history** - Command History
```bash
history               # Show last 20 commands (default)
history 10            # Show last 10 commands
history 5             # Show last 5 commands
```

**Features:**
- Stores up to 20 commands
- Persistent across sessions (saved to `~/.shell_history`)
- Numbered command listing
- Duplicate consecutive commands are filtered

### 6. **pinfo** - Process Information
```bash
pinfo                 # Show info about shell process
pinfo 1234            # Show info about process with PID 1234
```

**Output Format:**
```
Process Status: S
Memory: 12345 kB
Executable Path: /path/to/executable
```

**Status Codes:**
- `R/R+`: Running
- `S/S+`: Sleeping in interruptible wait
- `Z`: Zombie
- `T`: Stopped (on signal)
- `+` indicates foreground process

### 7. **search** - Recursive File Search
```bash
search filename.txt   # Search for file recursively from current directory
search dirname        # Search for directory recursively
```

**Output:**
- `True`: File/directory found
- `False`: File/directory not found

## Advanced Features

### Background Execution
Run any command in the background by appending `&`:
```bash
sleep 10 &            # Runs sleep in background
gedit file.txt &      # Opens gedit without blocking shell
```

**Behavior:**
- Prints PID of background process: `[1234]`
- Shell immediately returns to prompt
- Multiple background processes supported

### Command Pipelines
Chain commands using the pipe operator `|`:
```bash
# Two commands
cat file.txt | wc

# Three commands  
cat sample.txt | head -7 | tail -5

# Complex pipeline
ls -la | grep ".txt" | wc -l
```

**Features:**
- Supports unlimited number of pipes
- Each command's output becomes next command's input
- Works with both built-in and system commands

### I/O Redirection

#### Output Redirection
```bash
echo "Hello" > output.txt        # Overwrite file
echo "World" >> output.txt       # Append to file
ls -la > directory_listing.txt   # Redirect ls output
```

#### Input Redirection
```bash
cat < input.txt                  # Read from file
sort < unsorted.txt              # Sort file contents
```

#### Combined Redirection
```bash
sort < input.txt > sorted.txt    # Input from file, output to file
cat < data.txt | wc > count.txt  # Pipeline with redirection
```

### Signal Handling

#### Ctrl+C (SIGINT)
- Interrupts currently running foreground process
- No effect if no foreground process is running
- Shell continues normally

#### Ctrl+Z (SIGTSTP)
- Stops currently running foreground process
- Pushes process to background in stopped state
- No effect if no foreground process is running

#### Ctrl+D (EOF)
- Gracefully exits the shell
- Saves command history before exit

### TAB Autocomplete

#### Command Completion
```bash
ec<TAB>               # Completes to "echo"
hi<TAB>               # Completes to "history"
```

#### File/Directory Completion
```bash
cat data<TAB>         # Completes to matching files
cd /usr/l<TAB>        # Completes to "/usr/local/" or shows options
```

**Behavior:**
- Single match: Auto-completes immediately
- Multiple matches: Shows common prefix or lists all options
- Works with relative and absolute paths
- Directories shown with trailing `/`

### Command History Navigation

#### Arrow Key Navigation
- **Up Arrow**: Navigate to previous commands
- **Down Arrow**: Navigate to newer commands (after using up)
- **Left/Right Arrows**: Move cursor within current line

#### Features
- Edit retrieved commands before execution
- Wraps around history boundaries
- Maintains cursor position during editing

### Multiple Commands
Execute multiple commands in sequence using semicolon `;`:
```bash
ls; pwd; echo "Done"              # Execute three commands sequentially
cd /tmp; ls -la; cd -             # Change dir, list, return
```

## Code Architecture

### File Structure
```
├── main.cpp              # Entry point
├── Shell.h               # Main header file with all declarations
├── Shell.cpp             # Core shell initialization and main loop
├── Autocomplete.cpp      # Tab completion functionality
├── Builtins.cpp          # Built-in command implementations
├── History.cpp           # Command history management
├── InputHandler.cpp      # Raw input handling and line editing
├── Parser.cpp            # Command parsing and tokenization
├── Pinfo.cpp             # Process information retrieval
├── Pipeline.cpp          # Pipeline handling and execution
├── Redirection.cpp       # I/O redirection implementation
├── Search.cpp            # Recursive file search
├── Signals.cpp           # Signal handler setup
├── SystemCommands.cpp    # External command execution
├── Utils.cpp             # Utility functions
├── makefile              # Genrates the executable file
└── README.md             # About the project and its usage instructions
```

### Key Components

#### Shell Core (`Shell.cpp`)
- Initializes shell environment (username, hostname, directories)
- Main command processing loop
- Prompt display and directory tracking
- Signal setup and history loading

#### Input Processing (`InputHandler.cpp`)
- Raw terminal mode for character-by-character input
- Real-time command editing with cursor movement
- Tab completion integration
- Arrow key history navigation

#### Command Parsing (`Parser.cpp`)
- Tokenizes input using `strtok` for semicolon separation
- Handles quoted strings and special characters
- Separates operators (`|`, `<`, `>`, `>>`, `&`)

#### Process Management
- **Background Execution**: Fork without waiting
- **Foreground Execution**: Fork with `waitpid()`
- **Pipeline Management**: Multiple pipe coordination
- **Signal Handling**: Proper cleanup and forwarding

## Technical Implementation Details

### Memory Management
- Dynamic allocation for command parsing
- Proper cleanup of file descriptors
- Safe string handling throughout

### Process Creation
- Uses `fork()` for creating child processes
- `execvp()` for executing external commands
- Proper error handling with `perror()`

### File Operations
- Direct system calls for file operations
- Proper permission handling (0644 for created files)
- Error checking for all file operations

### Terminal Control
- Raw mode for character input
- ANSI escape sequences for cursor movement
- Terminal state restoration on exit

## Error Handling

### Built-in Commands
- **cd**: Invalid arguments, non-existent directories
- **ls**: Permission denied, invalid flags
- **pinfo**: Invalid PID, non-existent process
- **history**: Invalid number format

### System Commands
- Command not found
- Permission denied
- Invalid arguments
- File operation errors

### I/O Operations
- Input file not found
- Output file creation failure
- Pipe creation failure
- File descriptor exhaustion

## Example Usage Sessions

### Basic Commands
```bash
user@hostname:~> pwd
/home/user
user@hostname:~> echo "Hello Shell"
Hello Shell
user@hostname:~> cd /tmp
user@hostname:/tmp> ls -la
total 12
drwxrwxrwt 10 root root 4096 Sep  4 20:36 .
drwxr-xr-x 19 root root 4096 Aug 15 10:22 ..
```

### Background Processes
```bash
user@hostname:~> sleep 5 &
[12345]
user@hostname:~> ps
# Shows running processes including background sleep
```

### Pipelines and Redirection
```bash
user@hostname:~> ls | grep ".cpp" > cpp_files.txt
user@hostname:~> cat cpp_files.txt
main.cpp
Shell.cpp
Builtins.cpp
...
```

### History and Navigation
```bash
user@hostname:~> ls
user@hostname:~> pwd  
user@hostname:~> echo "test"
user@hostname:~> history
1 ls
2 pwd
3 echo test
4 history
```

## Security Considerations

- Input validation for all commands
- Proper file permission handling
- Safe memory management
- Protection against command injection
- Bounds checking for arrays and strings

## Known Limitations

- No support for complex shell features (job control beyond basic signals)
- No variable expansion or environment variable substitution
- No glob pattern matching
- No command substitution
- Limited to basic redirection operators

## Support

For technical issues or questions about the implementation, refer to the source code comments and function documentation in the header files.

---

**Note**: This shell is designed for educational purposes as part of an Operating Systems course assignment. It demonstrates fundamental concepts of process management, inter-process communication, and system programming in Unix-like environments.
