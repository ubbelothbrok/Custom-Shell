# Quick Reference Guide - Phase 1

## Build & Run

```bash
# Compile the shell
make

# Run the shell
make run
# or: ./bin/shell

# Exit the shell
Ctrl+D
```

## Testing Commands

```bash
# Basic commands
shell> ls
shell> pwd
shell> whoami
shell> date
shell> echo "hello world"

# Commands with arguments
shell> ls -la
shell> echo arg1 arg2 arg3
shell> cat Makefile

# Multiple commands (automated)
echo -e "ls\npwd\necho test" | ./bin/shell
```

## Development Tasks

```bash
# Build only
make

# Build and run
make run

# Clean build
make clean && make

# Debug with GDB
make debug
  (gdb) run
  (gdb) break main.c:50
  (gdb) continue
  (gdb) quit

# Trace system calls (Linux only)
make strace

# Check memory leaks (Linux only)
make valgrind

# Show help
make help
```

## File Structure

```
src/
  shell.h    ← Declarations, Command struct, MAX_ARGS, MAX_INPUT_SIZE
  shell.c    ← parse_command(), free_command(), execute_command()
  main.c     ← REPL loop: prompt → read → parse → execute → free

Documentation/
  README.md              ← Project overview and usage
  PHASE1.md              ← Architecture and design details
  CODE_WALKTHROUGH.md    ← Line-by-line code explanation
  TESTING.md             ← Testing and debugging procedures
  COMPLETION_SUMMARY.md  ← This phase summary
  
Build/
  Makefile               ← Compilation configuration
  bin/shell              ← Compiled executable
  build/                 ← Object files (.o)
```

## Key Functions

### parse_command(char *input)
**What:** Parse command string into array of arguments  
**Returns:** Command struct with args array and argc  
**Example:** "ls -la" → ["ls", "-la", NULL]  
**Memory:** Must call free_command() to clean up  

### execute_command(Command cmd)
**What:** Fork and execvp the command  
**Process:**
  1. fork() creates child
  2. Child: execvp() replaces memory with program
  3. Parent: waitpid() waits for child to finish  
**Returns:** Nothing; parent continues loop  

### free_command(Command cmd)
**What:** Free all memory allocated by parse_command()  
**Must:** Free in correct order:
  1. Each arg string
  2. Args array itself  
**Returns:** Nothing  

## Constants

```c
#define MAX_ARGS 64         // Max arguments per command
#define MAX_INPUT_SIZE 1024 // Max input line length
```

## Main Loop Flow

```
while (1) {
  1. printf("shell> ")                    // Display prompt
  2. fgets(input, MAX_INPUT_SIZE, stdin)  // Read input
  3. if EOF (Ctrl+D): break               // Exit cleanly
  4. parse_command(input)                 // Parse
  5. execute_command(cmd)                 // Execute
  6. free_command(cmd)                    // Cleanup
  7. Loop to step 1                       // Repeat
}
```

## Important Details

### NULL-Termination
- `execvp()` **requires** argv to be NULL-terminated
- Last element of cmd.args must be NULL
- parser sets: `cmd.args[cmd.argc] = NULL;`

### Memory Order
- **Allocate:** array → strings
- **Deallocate:** strings → array (reverse order!)

### Fork/Execvp
- fork() returns 0 in child, PID in parent
- execvp() **never returns** on success
- Parent must call waitpid() to wait for child
- Child inherits parent's file descriptors (stdout, stdin, etc.)

## Common Errors & Fixes

| Problem | Cause | Fix |
|---------|-------|-----|
| Compiler warnings | Unused parameters | Use `(void)argc;` |
| Command not found | Not in PATH | Use full path `/bin/ls` |
| Shell hangs | waitpid() missing | Verify execute_command() |
| Memory leak | free_command() not called | Check main loop |
| Segmentation fault | Accessing freed memory | Free in correct order |
| Extra text output | strtok not removing newline | Check input[strcspn(...)] |

## Compilation Details

```
Source Files:
  src/main.c (114 lines) + src/shell.c (191 lines) + src/shell.h (66 lines)
  = 371 lines total

Compilation:
  gcc -Wall -Wextra -pedantic -std=c99 -g [files] -o bin/shell

Flags:
  -Wall -Wextra    : All warnings
  -pedantic        : Strict C standard
  -std=c99         : C99 standard
  -g               : Debug symbols

Result:
  ✅ Zero errors
  ✅ Zero warnings
  ✅ Executable size: ~40-50 KB
```

## Performance

- **Compile time:** ~0.1 seconds
- **Execute command:** ~5-20ms
- **Memory per command:** ~500 bytes (temporary)
- **Memory growth:** Zero (fully cleaned up)

## Limits & Constants

```c
#define MAX_ARGS 64         // Can handle up to 64 arguments
#define MAX_INPUT_SIZE 1024 // Input limited to 1024 characters

Actual limits:
  - Argument count: 63 (MAX_ARGS - 1 for NULL)
  - Max argument length: Unlimited (each malloc'd individually)
  - Max input line: 1023 (MAX_INPUT_SIZE - 1 for NULL)
  - Max command length: 1023
```

## Example Trace (ls command)

```
User types: "ls -la"
              ↓
parse_command() called
  ├─ malloc(cmd.args array)
  ├─ strtok() → "ls"
  │   └─ malloc("ls") and copy
  ├─ strtok() → "-la"
  │   └─ malloc("-la") and copy
  └─ NULL-terminate array
              ↓
execute_command() called
  ├─ fork()
  │   ├─ Child: execvp("ls", ["ls", "-la", NULL])
  │   │         (ls program runs)
  │   └─ Parent: waitpid() (waits)
  │
  ├─ Child outputs: total 24
  │                 drwxr-xr-x ...
  │                 (file listing)
  ├─ Child exits
  │
  └─ Parent wakes up, continue
              ↓
free_command() called
  ├─ free("ls")
  ├─ free("-la")
  └─ free(cmd.args)
              ↓
Loop back to prompt
```

## Next Steps After Phase 1

1. **Understand Phase 1:**
   - Read CODE_WALKTHROUGH.md
   - Run and experiment with shell
   - Modify code and recompile

2. **Prepare for Phase 2:**
   - Learn about chdir() system call
   - Design built-in command framework
   - Plan how to distinguish built-in vs external

3. **Extensions to try:**
   - Add command history
   - Add tab completion
   - Parse quotes properly
   - Support background jobs (&)

## Useful Resources

- `man fork` - Fork system call
- `man execvp` - Execute program
- `man wait` - Wait for process
- `man strtok` - String tokenization
- `man malloc` - Memory allocation
- APUE (Advanced Programming in UNIX Environment) - Chapter 8 (Process Control)

---

**Phase 1 is complete and ready to use!** 🚀

For detailed explanations, see CODE_WALKTHROUGH.md  
For testing procedures, see TESTING.md  
For architecture overview, see PHASE1.md
