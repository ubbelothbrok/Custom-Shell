# Custom Shell

A Unix-like shell implementation in C with full support for external commands, built-ins, piping, redirection, and background jobs.

## About This Project

This is a complete shell implementation in pure C using POSIX system calls. It reads commands from the user, parses them, and executes them either as external programs or built-in commands. The shell supports piping, input/output redirection, background jobs, and proper signal handling.

**Features:**
- ✅ External command execution
- ✅ Built-in commands (cd, pwd, exit)
- ✅ Input/output redirection (<, >, >>)
- ✅ Piping (|) between commands
- ✅ Background job execution (&)
- ✅ Signal handling (Ctrl+C immunity)
- ✅ Zero memory leaks

## Quick Start

### Using Makefile

```bash
# Build the shell
make

# Run the shell
make run

# Or run directly
./bin/shell
```

### Without Makefile

```bash
# Compile manually
gcc -o shell src/main.c src/shell.c -Wall -Wextra

# Run it
./shell
```

### Exit the Shell

```bash
shell> exit
# or
shell> Ctrl+D
```

## Compilation Methods

### Method 1: With Makefile (Recommended)

```bash
# Build
make

# Build and run
make run

# Clean build artifacts
make clean
```

### Method 2: Manual Compilation

**On macOS/Linux:**
```bash
gcc -o shell src/main.c src/shell.c -Wall -Wextra
./shell
```

**With optimization:**
```bash
gcc -O2 -o shell src/main.c src/shell.c -Wall -Wextra
./shell
```

**Debug build:**
```bash
gcc -g -o shell src/main.c src/shell.c -Wall -Wextra
gdb ./shell
```

### Prerequisites

- GCC or Clang compiler
- Standard C library
- POSIX-compliant OS (Linux, macOS, Unix)

No external dependencies required!

## Shell Commands

### External Commands

Any standard Unix command can be executed:

```bash
shell> ls                       # List files
shell> ls -la                   # List with details
shell> pwd                      # Print working directory
shell> cat file.txt             # Display file contents
shell> echo hello               # Print text
shell> date                     # Show current date/time
shell> grep pattern file.txt    # Search text
shell> sort file.txt            # Sort lines
shell> find . -name "*.txt"     # Find files
```

### Built-in Commands

These commands run inside the shell:

**`cd [path]`** - Change directory
```bash
shell> cd /tmp                  # Go to /tmp
shell> cd ~                     # Go to home directory
shell> cd -                     # Go to previous directory
shell> cd                       # Go to home
shell> cd ..                    # Go to parent directory
```

**`pwd`** - Print current working directory
```bash
shell> pwd
/Users/ubbe/Documents/Custom-Shell
```

**`exit [code]`** - Exit the shell
```bash
shell> exit                     # Exit with code 0
shell> exit 1                   # Exit with code 1
```

## Input/Output Redirection

### Input Redirection `<`

Read from a file instead of keyboard:
```bash
shell> sort < data.txt              # sort reads from data.txt
shell> grep pattern < input.txt     # grep reads from input.txt
shell> wc -l < document.txt         # Count lines
```

### Output Redirection `>`

Write to a file (overwrites if exists):
```bash
shell> ls > listing.txt             # Save file list to listing.txt
shell> echo "hello" > output.txt    # Create output.txt with "hello"
shell> date > timestamp.txt         # Save date to file
```

### Append Redirection `>>`

Append to a file:
```bash
shell> echo "line 2" >> log.txt     # Add to end of log.txt
shell> date >> timestamps.txt       # Append timestamp
shell> ps >> process_log.txt        # Append process list
```

## Piping

Connect output of one command to input of another using `|`:

```bash
shell> cat file.txt | grep pattern          # Search in file
shell> cat file.txt | sort                  # Sort file contents
shell> cat file.txt | sort | uniq           # Sort and remove duplicates
shell> ps aux | grep shell                  # Find shell process
shell> ls | grep src                        # Find items with "src"
shell> cat data.txt | sort | uniq > result.txt  # Sort and save
shell> echo "1 3 2" | tr ' ' '\n' | sort   # Sort numbers
```

Multiple pipes can be chained together.

## Background Jobs

Run commands in the background using `&`:

```bash
shell> sleep 100 &              # Run sleep in background
[1234] sleep                    # Shows job ID and command

shell>                          # Prompt returns immediately
shell> long_command &           # Command runs in background
shell> ps                       # Check running jobs
```

**Signal Handling:**
- Press `Ctrl+C` to terminate the background job (shell continues)
- Shell ignores Ctrl+C and keeps running
- Background jobs continue executing

## Project Files

```
src/
  main.c      - Main REPL loop
  shell.c     - Command parsing and execution
  shell.h     - Header definitions

bin/
  shell       - Compiled executable

Makefile      - Build configuration
README.md     - This file
```

## All Features Summary

| Feature | Syntax | Example |
|---------|--------|---------|
| External Commands | `command [args]` | `ls -la /tmp` |
| Change Directory | `cd [path]` | `cd /tmp` |
| Print Directory | `pwd` | `pwd` |
| Exit Shell | `exit [code]` | `exit 0` |
| Input Redirect | `command < file` | `sort < data.txt` |
| Output Redirect | `command > file` | `ls > output.txt` |
| Append Redirect | `command >> file` | `echo text >> log.txt` |
| Pipe | `cmd1 \| cmd2` | `cat file \| grep text` |
| Background Job | `command &` | `sleep 100 &` |
| Ctrl+C | Press Ctrl+C | Terminates job (not shell) |

## Makefile Targets

```bash
make              # Compile the shell
make run          # Compile and run
make clean        # Remove compiled files
make debug        # Run with GDB debugger
make valgrind     # Check memory leaks (Linux)
make strace       # Trace system calls (Linux)
make help         # Show all targets
```

## Testing Examples

### Basic Commands
```bash
shell> ls
shell> pwd
shell> whoami
shell> date
```

### Redirection
```bash
shell> echo "hello world" > test.txt
shell> cat < test.txt
shell> sort < unsorted.txt > sorted.txt
shell> echo "more text" >> test.txt
```

### Piping
```bash
shell> cat file.txt | grep pattern
shell> ls | sort
shell> cat data.txt | grep "search" | wc -l
shell> find . -type f | head -10
```

### Combined Operations
```bash
shell> cat input.txt | sort | uniq > output.txt
shell> ps aux | grep shell | head -5
shell> find . -name "*.txt" | wc -l
```

### Background Jobs
```bash
shell> sleep 50 &
shell> find / -name "*.txt" > results.txt &
shell> ls -R > directory_listing.txt &
shell> Ctrl+C
```

## Project Statistics

- **Source Code:** ~900 lines of C
- **Compilation Warnings:** 0
- **Compilation Errors:** 0
- **Memory Leaks:** 0
- **POSIX System Calls:** 10+ different calls
