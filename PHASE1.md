# Custom Shell - Phase 1 Implementation

## Overview

This is a Unix-like shell implementation in C, built from first principles using POSIX APIs. **Phase 1** implements the fundamental REPL (Read-Eval-Print Loop) with support for executing external commands.

## Architecture

### Key Files

- **`src/main.c`** - Main REPL loop
- **`src/shell.c`** - Command parsing and execution functions
- **`src/shell.h`** - Header with function declarations and constants

### How It Works

```
┌─────────────────────────────────────────┐
│         SHELL MAIN LOOP (main.c)        │
│                                         │
│  while (1) {                            │
│    1. Display prompt "shell> "          │
│    2. Read input line                   │
│    3. Parse into Command struct         │
│    4. Execute command                   │
│    5. Free memory                       │
│  }                                      │
└─────────────────────────────────────────┘
```

### Command Parsing: `parse_command()`

The parser converts a string like `ls -la /tmp` into an array:

```
Input:  "ls -la /tmp"
         ↓
         strtok() splits by whitespace
         ↓
Output: ["ls", "-la", "/tmp", NULL]
        cmd.argc = 3
```

**Key points:**
- Allocates a dynamic array of pointers (`cmd.args`)
- Uses `strtok()` to tokenize by whitespace
- **Null-terminates** the array (required by `execvp()`)
- Returns number of arguments in `cmd.argc`

### Process Execution: `execute_command()`

The heart of the shell uses the **fork/execvp pattern**:

```
Parent (Shell)                 Child (External Program)
─────────────────────────────────────────────────────
    │
    ├─ fork()  ──────────────> Process created
    │
    │                               │
    │                               ├─ execvp("ls", ["ls", "-la", ...])
    │                               │
    │                               ├─ Program image replaced
    │                               │  (ls runs with inherited stdout/stdin)
    │                               │
    │                               └─ Process exits with status
    │
    ├─ waitpid()  ◄────────── Wait for child
    │   (blocked)
    │
    └─ Loop back to prompt
```

### Fork/Execvp Explained

**`fork()`**
- Creates an **exact copy** of the parent process
- Parent receives child's PID; child receives 0
- Both continue executing from this point
- Parent stays alive; child is a separate process

**`execvp(program, argv)`**
- **v**: Takes an array of strings (like `argv` in C)
- **p**: Searches `$PATH` environment variable
- Replaces the child's memory image with the new program
- Never returns on success
- Child inherits file descriptors (stdout, stdin, etc.)

**`waitpid(pid, &status, flags)`**
- Parent blocks until child exits
- Reaps the zombie process
- Without this, child would become a zombie

### Memory Management

**Allocation Flow:**
```
parse_command()
├─ malloc(cmd.args)        → Array of pointers
└─ malloc(each argument)   → Individual strings

Total allocations: argc + 2 (one for array, one for each arg)
```

**Deallocation Flow:**
```
free_command()
├─ for each arg: free(cmd.args[i])
└─ free(cmd.args)
```

**Critical: Free in the correct order!**
1. Free individual strings first
2. Free the array last

## Building and Running

### Compile
```bash
make
```

### Run
```bash
make run
```

Or:
```bash
./bin/shell
```

### Test Commands

Once the shell runs, try:

```bash
shell> ls
shell> pwd
shell> echo hello world
shell> whoami
shell> cat /etc/hostname
shell> date
```

Press **Ctrl+D** to exit gracefully.

## Memory Safety

### Check for Leaks
```bash
make valgrind
```

Run a few commands, then exit with Ctrl+D. Valgrind will report any leaks.

### Debug Execution
```bash
make debug
```

This launches `gdb`. Try:
```
(gdb) run
shell> ls
^C
(gdb) bt          # backtrace
(gdb) quit
```

### Trace System Calls
```bash
make strace
```

This shows all `fork`, `execve`, and `wait4` calls as you run commands.

## Phase 1 Limitations (Fixed in Later Phases)

❌ No built-in commands (`cd`, `exit`)  
❌ No input/output redirection (`<`, `>`, `>>`)  
❌ No piping (`|`)  
❌ Ctrl+C exits the shell (should not)  
❌ No background processes (`&`)  

## Implementation Notes

### Why `strtok()`?

`strtok()` modifies the original string, so we make a copy first. For production code, `strtok_r()` (reentrant version) is safer.

### Why Dynamic Memory?

We don't know how many arguments the user will type. Using fixed-size arrays would be fragile and wasteful. Dynamic allocation is more flexible but requires careful cleanup.

### Why NULL-terminate?

`execvp()` expects `argv` to be NULL-terminated. This is a Unix convention inherited from how the kernel passes arguments to programs. The last element must be `NULL` so `execvp()` knows where the list ends.

### Handles vs. File Descriptors

When a child process is created with `fork()`, it inherits copies of all file descriptors from the parent. This is why the child's output goes to the same terminal—stdout is inherited from the parent.

## Next Steps

**Phase 2** will add:
- Built-in command `cd` (uses `chdir()` system call)
- Built-in command `exit` (terminates the shell)
- Proper command detection (is it built-in or external?)

Phase 1 is complete and production-ready for the purposes of this phase! 🚀
