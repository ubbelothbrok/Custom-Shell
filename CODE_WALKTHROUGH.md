# Code Walkthrough: Phase 1 Detailed Explanation

This document walks through the Phase 1 code with detailed explanations of how each part works.

## File Overview

```
src/
├── shell.h    ← Declarations, constants, Command struct
├── shell.c    ← Implementation of parse_command() and execute_command()
└── main.c     ← Main REPL loop
```

## Part 1: The Shell Header (shell.h)

```c
#define MAX_ARGS 64           // Max args per command
#define MAX_INPUT_SIZE 1024   // Max input line length

typedef struct {
    char **args;              // Pointer to array of strings
    int argc;                 // Number of arguments
} Command;
```

**Why a struct?**
- Encapsulates related data (arguments and count)
- Easier to pass between functions
- Cleaner than returning multiple values

## Part 2: Command Parser (shell.c - parse_command)

### The Big Picture

```
Input:  "  ls  -la  /tmp  "
        ↓
Remove trailing newline
        ↓
Create a copy (strtok modifies strings)
        ↓
Use strtok() to split on whitespace
        ↓
For each token:
  • Allocate memory for the string
  • Copy token into allocated memory
  • Add pointer to cmd.args array
        ↓
NULL-terminate the array
        ↓
Output: Command { args: ["ls", "-la", "/tmp", NULL], argc: 3 }
```

### Step-by-Step Walkthrough

#### Step 1: Prepare

```c
Command cmd;
cmd.args = NULL;
cmd.argc = 0;
```

Initialize structure with safe defaults. If something fails later, we won't have garbage values.

#### Step 2: Copy Input

```c
char *input_copy = malloc(strlen(input) + 1);
strcpy(input_copy, input);
```

Why copy? Because `strtok()` **modifies the string it's tokenizing**. We don't want to destroy the user's input.

Example:
```c
strtok("ls -la", " ");  // After this, string is modified!
```

#### Step 3: Allocate Args Array

```c
cmd.args = malloc(MAX_ARGS * sizeof(char *));
```

We allocate an array of `char *` (pointers to strings). Each element will point to one argument.

**Memory layout:**
```
cmd.args:
  [0] → "ls"
  [1] → "-la"
  [2] → "/tmp"
  [3] → NULL
  [4] → ??? (uninitialized)
  ...
```

#### Step 4: Tokenize Loop

```c
char *token = strtok(input_copy, " \t\n");
while (token != NULL && cmd.argc < MAX_ARGS - 1) {
    cmd.args[cmd.argc] = malloc(strlen(token) + 1);
    strcpy(cmd.args[cmd.argc], token);
    cmd.argc++;
    token = strtok(NULL, " \t\n");
}
```

**How strtok() works:**
- First call: `strtok(string, delimiters)` searches from start
- Subsequent calls: `strtok(NULL, delimiters)` continues from last position
- Returns pointer to next token, or NULL when done

**Loop behavior:**
```
Iteration 1:
  token = "ls"
  malloc and copy "ls"
  cmd.argc = 1

Iteration 2:
  token = "-la"
  malloc and copy "-la"
  cmd.argc = 2

Iteration 3:
  token = "/tmp"
  malloc and copy "/tmp"
  cmd.argc = 3

Iteration 4:
  token = NULL
  Loop exits
```

#### Step 5: NULL-Terminate

```c
cmd.args[cmd.argc] = NULL;
```

This is **essential**. `execvp()` expects `argv` to be NULL-terminated.

**Why?** The kernel needs to know where the argument list ends. It looks for NULL to stop:
```c
// How execvp internally finds the end:
for (int i = 0; argv[i] != NULL; i++) {
    // ... process argument
}
// When argv[i] is NULL, we stop
```

#### Step 6: Cleanup

```c
free(input_copy);
return cmd;
```

Free the temporary copy we made, but NOT `cmd.args`—the caller needs that!

## Part 3: Free Command (shell.c - free_command)

### The Deallocation Chain

```c
void free_command(Command cmd) {
    if (cmd.args != NULL) {
        for (int i = 0; i < cmd.argc; i++) {
            if (cmd.args[i] != NULL) {
                free(cmd.args[i]);      // Free each string
            }
        }
        free(cmd.args);                 // Free the array itself
    }
}
```

**Order matters!**

```
Before free_command():
  cmd.args ──┐
             ├─→ [0] ──→ "ls" (heap memory)
             ├─→ [1] ──→ "-la" (heap memory)
             ├─→ [2] ──→ "/tmp" (heap memory)
             └─→ [3] ──→ NULL

After free_command():
  All memory returned to the system
```

**Common mistake:**
```c
// WRONG - this creates a use-after-free!
free(cmd.args);
for (int i = 0; i < cmd.argc; i++) {
    free(cmd.args[i]);  // Accessing freed memory!
}
```

## Part 4: Process Execution (shell.c - execute_command)

### The Fork/Execvp Pattern

This is the heart of the shell. Here's what happens:

#### Before Fork

```
Memory:
┌──────────────────────────────┐
│  Shell Process (PID 1000)    │
│  ├─ Variables: cmd, input... │
│  ├─ File Descriptors         │
│  │  ├─ stdin (0)             │
│  │  ├─ stdout (1) → terminal │
│  │  └─ stderr (2) → terminal │
│  └─ Code: main.c, shell.c    │
└──────────────────────────────┘
```

#### After Fork (Before Execvp)

```
Memory:
┌──────────────────────────┐    ┌──────────────────────────┐
│  Shell Process (1000)    │    │  Child Process (1001)    │
│  ├─ Variables (copies)   │    │  ├─ Variables (copies)   │
│  ├─ FDs: (inherited)     │    │  ├─ FDs: (same as parent)│
│  │  ├─ stdin ← terminal  │    │  │  ├─ stdin ← terminal  │
│  │  └─ stdout → terminal │    │  │  └─ stdout → terminal │
│  └─ Code: Shell code     │    │  └─ Code: Shell code     │
└──────────────────────────┘    └──────────────────────────┘
                                       ↓ execvp("ls", ...)
                                ┌──────────────────────────┐
                                │  Child Process (1001)    │
                                │  ├─ Program: ls          │
                                │  ├─ FDs: (still same)    │
                                │  └─ Code: ls executable  │
                                └──────────────────────────┘
```

### Code Walkthrough

#### Step 1: Fork

```c
pid_t pid = fork();
```

**What fork() returns:**
```
In parent:  pid = 12345 (child's PID)
In child:   pid = 0
On error:   pid = -1
```

#### Step 2: Error Check

```c
if (pid < 0) {
    perror("fork");
    return;
}
```

If fork failed (out of memory, too many processes), we print an error and return.

#### Step 3: Child Process (pid == 0)

```c
if (pid == 0) {
    execvp(cmd.args[0], cmd.args);
    perror("execvp");
    exit(EXIT_FAILURE);
}
```

**This code runs in the child process ONLY.**

What happens:
1. Call `execvp("ls", ["ls", "-la", "/tmp", NULL])`
2. Kernel loads `/bin/ls` into memory
3. Child's memory image is **completely replaced**
4. `ls` program starts from its `main()` function
5. When `ls` exits, the child process dies

**Critical:** If `execvp()` fails (returns), it means the executable wasn't found. We call `perror()` to print an error message.

#### Step 4: Parent Process (pid > 0)

```c
} else {
    int status;
    waitpid(pid, &status, 0);
}
```

**This code runs in the parent process ONLY.**

What happens:
1. Call `waitpid(12345, &status, 0)` — wait for child PID 12345
2. Parent **blocks** (sleeps) until child exits
3. When child exits, `waitpid()` returns with exit status
4. Parent continues (loops back for next prompt)

### Execution Timeline

```
Time    Parent                      Child
────────────────────────────────────────────────────
0ms     fork()
        │
1ms     │                           Running shell code
        │
2ms     │                           execvp("ls", ...)
        │
3ms     │                           ls program running
        │                           (output to terminal)
        │
100ms   │                           ls exits
        │
101ms   waitpid() returns  ◄─────── Process dies
        Loop back to prompt
```

## Part 5: Main Loop (main.c)

### The REPL Structure

```c
while (1) {
    // Read
    printf("shell> ");
    fgets(input, MAX_INPUT_SIZE, stdin);
    
    // Remove newline
    input[strcspn(input, "\n")] = '\0';
    
    // Eval (Parse)
    Command cmd = parse_command(input);
    
    // Print (Execute)
    execute_command(cmd);
    
    // Clean up
    free_command(cmd);
}
```

### Input Reading

```c
if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
    printf("\nExit\n");
    break;
}
```

**What is fgets()?**
- Reads up to MAX_INPUT_SIZE-1 characters
- Includes the newline character
- Returns NULL on EOF (Ctrl+D)

**EOF Detection:**
When user presses Ctrl+D at the prompt, `fgets()` returns NULL, and we exit gracefully.

### Newline Removal

```c
input[strcspn(input, "\n")] = '\0';
```

**What does this do?**
- `strcspn(str, chars)` returns the index of the first occurrence of any character in `chars`
- We replace that newline with NULL terminator
- Converts "ls -la\n" to "ls -la\0"

This is important because our tokenizer doesn't expect newlines.

## Memory in Action

### Single Command Example

```
User types:  "ls -la /tmp"
Press Enter

                PARSING
──────────────────────────────────────────
malloc(4 * sizeof(char*))
├─ args[0] → malloc(3) ──→ "ls"
├─ args[1] → malloc(4) ──→ "-la"
├─ args[2] → malloc(5) ──→ "/tmp"
└─ args[3] ──→ NULL

cmd.argc = 3

                EXECUTING
──────────────────────────────────────────
Child: execvp("ls", ["ls", "-la", "/tmp", NULL])
Parent: waits for child
Child: ls program runs (processes /tmp directory)
Child: ls exits

                FREEING
──────────────────────────────────────────
free(args[0])  ← "ls" freed
free(args[1])  ← "-la" freed
free(args[2])  ← "/tmp" freed
free(args)     ← array freed

Ready for next command
```

### After 100 Commands

With proper cleanup:
- Memory stays constant (~few KB)
- No memory leaks

Without cleanup:
- Memory grows by ~100-200 bytes per command
- After 100 commands: ~10-20 MB wasted
- After 1000 commands: system might be out of memory!

## Common Execution Scenarios

### Scenario 1: Normal Command

```bash
shell> echo hello
hello
shell>
```

**What happens:**
1. Parse "echo hello" → ["echo", "hello", NULL]
2. fork() → parent gets child PID
3. Child: execvp("echo", ["echo", "hello", NULL])
4. Kernel finds /bin/echo and runs it
5. echo prints "hello"
6. echo exits
7. Parent wakes from waitpid()
8. Loop continues

### Scenario 2: Unknown Command

```bash
shell> xyz_nonexistent
execvp: No such file or directory
shell>
```

**What happens:**
1. Parse "xyz_nonexistent" → ["xyz_nonexistent", NULL]
2. fork() → parent gets child PID
3. Child: execvp("xyz_nonexistent", ["xyz_nonexistent", NULL])
4. Kernel searches PATH—not found!
5. execvp returns -1
6. Child: perror("execvp") prints error
7. Child: exit(EXIT_FAILURE)
8. Child exits
9. Parent wakes from waitpid()
10. Loop continues

### Scenario 3: EOF (Ctrl+D)

```bash
shell> ^D
Exit
```

**What happens:**
1. Prompt displayed
2. User presses Ctrl+D
3. fgets() returns NULL
4. Condition "fgets() == NULL" is true
5. Print "Exit\n"
6. break (exit loop)
7. main() returns
8. Process exits

## Summary

**The three critical functions:**

| Function | Purpose | Key Calls |
|----------|---------|-----------|
| `parse_command()` | String → Args array | strtok, malloc |
| `execute_command()` | Run command | fork, execvp, waitpid |
| `free_command()` | Cleanup memory | free |

**The fundamental pattern:**
```
Allocate ──> Parse ──> Execute ──> Free ──> Repeat
```

This pattern ensures:
- Flexibility (parse any command)
- Safety (no memory leaks)
- Simplicity (each function does one thing)
- Learning (clear educational value)

---

**Next Phase:** Built-in commands (cd, exit)
