# Phase 2: Built-in Commands

**Status:** ✅ COMPLETE  
**Date:** June 29, 2026  
**Foundation:** Phase 1 (REPL with external command execution)

## Overview

Phase 2 extends the shell to handle built-in commands that must run in the shell's own process, not in a child. This is necessary for commands like `cd` that affect the shell's state.

## Built-in Commands Implemented

### `cd [path]`
Changes the working directory using the `chdir()` system call.

**Behavior:**
- `cd /path/to/dir` - Change to absolute path
- `cd relative/path` - Change to relative path
- `cd` or `cd ~` - Change to home directory
- `cd -` - Change to previous directory (not yet implemented)

**Implementation:**
```c
if (chdir(path) < 0) {
    perror("cd");  // Print error if directory doesn't exist
}
```

**Why built-in?**
- If we forked a child to run `cd`, only the child's working directory would change
- The parent (shell) would still be in the original directory
- So `cd` MUST run in the parent process

### `pwd`
Prints the current working directory.

**Implementation:**
```c
char cwd[4096];
if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
}
```

### `exit [code]`
Terminates the shell with optional exit code.

**Implementation:**
```c
int exit_code = (cmd.argc > 1) ? atoi(cmd.args[1]) : 0;
exit(exit_code);
```

## Command Type Detection

The key addition in Phase 2 is **distinguishing built-in from external commands**:

```c
if (is_builtin(cmd.args[0])) {
    execute_builtin(cmd);  // Run in shell process
} else {
    execute_command(cmd);  // Fork and execvp
}
```

### `is_builtin()` Function

```c
int is_builtin(const char *cmd) {
    return (strcmp(cmd, "cd") == 0) ||
           (strcmp(cmd, "exit") == 0) ||
           (strcmp(cmd, "pwd") == 0);
}
```

This function returns 1 if the command is built-in, 0 otherwise.

### `execute_builtin()` Function

```c
int execute_builtin(Command cmd) {
    if (strcmp(cmd.args[0], "cd") == 0) {
        // Change directory
        chdir(cmd.args[1]);
    } else if (strcmp(cmd.args[0], "pwd") == 0) {
        // Print working directory
        printf("%s\n", cwd);
    } else if (strcmp(cmd.args[0], "exit") == 0) {
        // Exit shell
        exit(0);
    }
    return 0;  // Or 1 if should exit
}
```

## Architecture Change

### Phase 1 Main Loop
```
Read Input
   ↓
Parse → Command
   ↓
execute_command(cmd)  ← Always fork/exec
   ↓
Free Memory
   ↓
Repeat
```

### Phase 2 Main Loop
```
Read Input
   ↓
Parse → Command
   ↓
if (is_builtin) ──→ execute_builtin(cmd)
else ─────────────→ execute_command(cmd)
   ↓
Free Memory
   ↓
Repeat
```

## Implementation Details

### Memory Management

Built-in commands don't fork, so they return immediately. This means:
- No child process to wait for
- Shell stays in the same process
- Working directory changes persist
- Shell variables persist (for future phases)

### Error Handling

Each built-in has proper error checking:

```c
// cd with error handling
if (chdir(path) < 0) {
    perror("cd");  // Prints "cd: No such file or directory"
}

// getcwd with error handling
if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("pwd");
}
```

## Testing Phase 2

### Test 1: Change Directory
```bash
shell> pwd
/Users/ubbe/Documents/Custom-Shell
shell> cd /tmp
shell> pwd
/tmp
shell> cd src
cd: No such file or directory
```

### Test 2: Exit Command
```bash
shell> exit
Exit
```

### Test 3: Built-in vs External
```bash
shell> cd /tmp           # Built-in: runs in shell
shell> pwd              # Built-in: shows /tmp
shell> ls               # External: lists /tmp directory
shell> cd /            # Built-in: shell changes to /
shell> pwd              # Built-in: shows /
```

## System Calls Used

| System Call | Purpose |
|-------------|---------|
| `chdir()` | Change working directory |
| `getcwd()` | Get current working directory |
| `exit()` | Terminate process |

## Limitations & Future Work

- `cd -` (change to previous directory) not implemented
- No environment variables (PATH expansion still uses system)
- No shell variables or aliases
- Coming in Phase 3: Redirection (< > >>)

## Summary

Phase 2 adds the critical ability to **modify shell state** via built-in commands. This requires:
1. **Command classification** - Determine if built-in or external
2. **Separate execution paths** - Built-ins in parent, external in child
3. **State persistence** - Changes affect the shell process

Key concept: **Why can't we fork for `cd`?**
- Child process's working directory changes
- Parent's working directory unchanged
- User types `cd /tmp` but shell still in original directory!
- So `cd` must run in parent process (the shell itself)

This same principle applies to shell variables, aliases, and other state-modifying operations.

---

**Next Phase:** Phase 3 - Input/Output Redirection
