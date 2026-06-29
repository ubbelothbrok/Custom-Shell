# Phase 3: Input/Output Redirection

**Status:** ✅ COMPLETE  
**Date:** June 29, 2026  
**Foundation:** Phase 1 (REPL) + Phase 2 (Built-ins)

## Overview

Phase 3 adds support for input/output redirection, allowing commands to read from/write to files instead of using stdin/stdout.

## Redirection Operators

### `<` - Input Redirection
**Redirects stdin from a file**

```bash
shell> sort < unsorted.txt        # sort reads from file
shell> grep pattern < data.txt    # grep reads from file
```

**How it works:**
1. Open the file for reading
2. Duplicate file descriptor to stdin (fd 0)
3. execvp the command (inherits redirected stdin)
4. File contents appear as stdin to the command

### `>` - Output Redirection (Overwrite)
**Redirects stdout to a file, overwriting it**

```bash
shell> ls -la > listing.txt       # Create/overwrite listing.txt
shell> echo "hello" > output.txt  # Overwrites output.txt
```

**How it works:**
1. Open the file for writing with O_CREAT | O_TRUNC (truncate)
2. Duplicate file descriptor to stdout (fd 1)
3. execvp the command (inherits redirected stdout)
4. Command output goes to file instead of terminal

### `>>` - Output Redirection (Append)
**Redirects stdout to a file, appending**

```bash
shell> echo "line 1" > log.txt    # Create log.txt
shell> echo "line 2" >> log.txt   # Append to log.txt
shell> date >> log.txt            # Append timestamp
```

**How it works:**
1. Open the file for writing with O_CREAT | O_APPEND
2. Duplicate file descriptor to stdout (fd 1)
3. execvp the command (appends to file)

## Architecture

### Redirection Structure

```c
typedef struct {
    char *infile;       // File for < redirection
    char *outfile;      // File for > or >> redirection
    int append;         // 1 if >>, 0 if >
} Redirection;
```

### Parsing Redirection Operators

The parser extracts redirection operators from the command:

```bash
Input:  "cat < input.txt > output.txt"
         ↓ Parse
Output: Command: ["cat", NULL]
        Redirection:
          infile: "input.txt"
          outfile: "output.txt"
          append: 0
```

**Algorithm:**
1. Find `<` in command string
2. Extract filename after `<`
3. Find `>` or `>>` in command string  
4. Extract filename after `>` or `>>`
5. Tokenize remaining text (before redirections) as command

### Execution with Redirection

Before `execvp()`, setup file descriptors:

```c
// Redirect stdin
int fd_in = open(infile, O_RDONLY);
dup2(fd_in, STDIN_FILENO);  // fd_in becomes fd 0
close(fd_in);

// Redirect stdout
int flags = O_WRONLY | O_CREAT;
if (append) flags |= O_APPEND;
else flags |= O_TRUNC;
int fd_out = open(outfile, flags, 0644);
dup2(fd_out, STDOUT_FILENO);  // fd_out becomes fd 1
close(fd_out);

// Now execvp - child inherits redirected descriptors
execvp(cmd.args[0], cmd.args);
```

## System Calls

### `open()`
Opens a file and returns a file descriptor.

```c
int fd = open(filename, flags, mode);
```

**Flags:**
- `O_RDONLY` - Read only
- `O_WRONLY` - Write only
- `O_CREAT` - Create if doesn't exist
- `O_TRUNC` - Truncate to 0 bytes (for >)
- `O_APPEND` - Append to end (for >>)

### `dup2()`
Duplicates a file descriptor to another.

```c
dup2(old_fd, new_fd);
```

Closes `new_fd` and makes it point to same file as `old_fd`.

**Example:**
```c
int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC);
dup2(fd, 1);  // File descriptor 1 (stdout) now points to output.txt
printf("Hello");  // This goes to output.txt, not terminal!
```

### `close()`
Closes a file descriptor.

```c
close(fd);
```

## Process Flow with Redirection

```
Parent Shell
   │
   ├─ fork() ──────────────────────→ Child Process
   │                                   │
   │                                   ├─ open(input.txt) → fd=3
   │                                   ├─ dup2(3, 0)  ← stdin now reads from file
   │                                   ├─ close(3)
   │                                   │
   │                                   ├─ open(output.txt) → fd=4
   │                                   ├─ dup2(4, 1)  ← stdout now writes to file
   │                                   ├─ close(4)
   │                                   │
   │                                   ├─ execvp(cmd) ← inherits redirected FDs
   │                                   │
   │                                   └─ Command runs with redirected I/O
   │
   └─ waitpid() ◄─────────────────── Child exits
      (shell waits)
```

## Testing Phase 3

### Test 1: Input Redirection
```bash
shell> echo "hello world" > test.txt
shell> cat < test.txt
hello world
```

### Test 2: Output Redirection (Overwrite)
```bash
shell> echo "line 1" > out.txt
shell> echo "line 2" > out.txt      # Overwrites
shell> cat out.txt
line 2                              # Only "line 2"
```

### Test 3: Output Redirection (Append)
```bash
shell> echo "line 1" > log.txt
shell> echo "line 2" >> log.txt     # Appends
shell> cat log.txt
line 1
line 2
```

### Test 4: Combined Redirection
```bash
shell> cat < input.txt > output.txt
shell> cat output.txt               # Shows contents of input.txt
```

### Test 5: With Pipes (Phase 4+)
```bash
shell> cat < input.txt | grep pattern > results.txt
```

## Error Handling

### File Not Found
```bash
shell> cat < nonexistent.txt
open: No such file or directory
```

### Permission Denied
```bash
shell> cat < /root/file.txt
open: Permission denied
```

### Creating Directories
Redirections won't create parent directories:
```bash
shell> cat < /nonexistent/dir/file.txt
open: No such file or directory
```

## File Descriptor Details

### Standard File Descriptors

| FD | Name | Purpose |
|----|------|---------|
| 0 | stdin | Standard input (keyboard) |
| 1 | stdout | Standard output (terminal) |
| 2 | stderr | Standard error (terminal) |

### Redirection Impact

**Before redirection:**
```
stdin (0) ──→ Terminal keyboard
stdout (1) ──→ Terminal screen
stderr (2) ──→ Terminal screen
```

**After `cat < file.txt > out.txt`:**
```
stdin (0) ──→ file.txt (read)
stdout (1) ──→ out.txt (write)
stderr (2) ──→ Terminal screen (unchanged)
```

## Implementation Details

### Memory for Filenames

```c
char *infile = malloc(strlen(filename) + 1);
strcpy(infile, filename);
```

Filenames are duplicated and stored in the Redirection structure, then freed when the command completes.

### Handling O_CREAT Mode

```c
int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        //                                            ^^^^
        //                                          mode bits
        //                     6 = rw- (owner)
        //                     4 = r-- (group)
        //                     4 = r-- (others)
```

The `0644` means:
- Owner: read + write
- Group: read only
- Others: read only

## Combining with Phase 2 (Built-ins)

Built-in commands (cd, pwd, exit) cannot use redirection since they run in the parent process. Attempting:

```bash
shell> pwd > output.txt
# This doesn't work properly because pwd runs in parent, not child
```

For now, redirection only works with external commands.

## Advanced Scenarios

### Append with Multiple Runs
```bash
shell> echo "start" > log.txt
shell> echo "event1" >> log.txt
shell> echo "event2" >> log.txt
shell> cat log.txt
start
event1
event2
```

### Chaining with Multiple Redirections
```bash
shell> sort < input.txt > sorted.txt > final.txt
# Only last > takes effect (standard shell behavior)
```

### Redirection with Arguments
```bash
shell> ls -la > files.txt           # Works
shell> ls -l > /tmp/out.txt         # Works
shell> cat file.txt > /tmp/out.txt  # Works
```

## Limitations & Future Work

- No stderr redirection (2>, 2>>)
- No file descriptors (1>, 2>)
- No input/output to the same file
- No append-only files for safety
- Coming in Phase 4: Piping (|)

## Summary

Phase 3 adds **file I/O redirection**:
- `<` redirects stdin from file
- `>` redirects stdout to file (overwrite)
- `>>` redirects stdout to file (append)

**Key concept: File descriptor duplication**

The `dup2()` system call redirects file descriptors. This is how redirection works:
1. Open the target file
2. Use `dup2(fd, 0)` or `dup2(fd, 1)` to redirect stdin/stdout
3. The child process inherits the redirected descriptors
4. Commands read/write to files transparently

---

**Next Phase:** Phase 4 - Piping
