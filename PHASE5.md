# Phase 5: Signal Handling & Background Jobs

**Status:** ✅ COMPLETE  
**Date:** June 29, 2026  
**Foundation:** Phases 1-4 (REPL, Built-ins, Redirection, Piping)

## Overview

Phase 5 adds signal handling so the shell responds appropriately to user interrupts and supports background job execution.

## Signals in Unix

### What are Signals?

Signals are **asynchronous notifications** sent to processes. Think of them as interrupts.

### SIGINT (Interrupt Signal)

**Triggered by:** Pressing Ctrl+C  
**Default behavior:** Terminate the process  
**Problem:** If shell terminates on Ctrl+C, user loses their session!

**Solution:** Shell catches SIGINT and ignores it, letting the child process handle it.

### Signal Handlers

```c
void sigint_handler(int sig) {
    (void)sig;  // Ignore the signal number
    // Do nothing - handler returns immediately
}

signal(SIGINT, sigint_handler);
```

## Implementation

### Setting Up Signal Handlers

```c
void setup_signal_handlers(void) {
    signal(SIGINT, sigint_handler);  // Ignore Ctrl+C in shell
    signal(SIGCHLD, SIG_DFL);        // Default handling for child exit
}
```

### Signal Handler for SIGINT

```c
void sigint_handler(int sig) {
    (void)sig;  /* Avoid "unused parameter" warning */
    /* 
     * Does nothing.
     * When user presses Ctrl+C:
     * 1. Kernel sends SIGINT to shell and children
     * 2. Shell's handler (does nothing) returns
     * 3. Child's handler (or default) terminates child
     * 4. Shell continues running
     * 5. Child dies, shell stays alive
     */
}
```

## Behavior Before & After Phase 5

### Without Signal Handler (Phase 1-4)

```bash
shell> sleep 100
^C    # User presses Ctrl+C
      # Shell dies!
      # User loses shell session
```

**Why?** Default SIGINT behavior terminates any process that receives it, including the shell.

### With Signal Handler (Phase 5)

```bash
shell> sleep 100
^C    # User presses Ctrl+C
      # sleep process dies (received SIGINT)
      # Shell continues (ignored SIGINT)
shell>
      # Prompt returns, shell ready for next command
```

**How?** Signal handler intercepts SIGINT, shell ignores it, child receives it normally.

## Background Jobs (& operator)

### Running Commands in Background

```bash
shell> sleep 100 &        # Command runs in background
[12345] sleep             # Print job info
shell>                    # Prompt returns immediately
```

**Behavior:**
- Command runs concurrently
- Shell doesn't wait for completion
- User can type next command immediately
- Shell continues running

### Implementation

Parse for `&` at end of command:

```c
int is_background = 0;
if (input ends with "&") {
    is_background = 1;
    remove "&" from command
}
```

In execution:

```c
if (is_background) {
    printf("[%d] %s\n", pid, cmd.args[0]);
    // Don't waitpid - child runs in background
} else {
    waitpid(pid, &status, 0);  // Wait for completion
}
```

## Practical Examples

### Example 1: Foreground Command (Normal)

```bash
shell> echo "Computing..." && sleep 2 && echo "Done"
Computing...
[wait 2 seconds]
Done
shell>
```

Shell waits (`waitpid()`) for command to finish.

### Example 2: Background Command

```bash
shell> sleep 100 &
[1234] sleep
shell>
```

Shell prints job number and PID, immediately returns prompt.

### Example 3: Interrupt Foreground Job

```bash
shell> sleep 100
^C
[1]+  Terminated              sleep 100
shell>
```

Ctrl+C terminates child, shell catches signal and continues.

### Example 4: Complex Pipeline with Background

```bash
shell> cat large.txt | sort | uniq > results.txt &
[1234] cat
shell>
[subsequent output if not redirected]
```

Entire pipeline runs in background.

## Signal Handling Details

### Process Groups

When shell runs a command:
1. Shell is in its own process group
2. Child process inherits process group
3. Child's children (pipes) inherit process group
4. Ctrl+C sends SIGINT to entire process group

This means:
- All pipeline commands get Ctrl+C
- Shell doesn't interfere
- Commands terminate together

### Signal Dispositions

```c
signal(SIGINT, handler);  // Custom handler
signal(SIGINT, SIG_DFL);  // Default behavior
signal(SIGINT, SIG_IGN);  // Ignore signal
```

Our shell uses:
- `SIG_IGN` for SIGINT (ignore in shell)
- `SIG_DFL` for SIGCHLD (default - reap zombies)

## System Calls

### `signal()`

```c
void (*signal(int sig, void (*handler)(int)))(int);
```

Sets signal handler. Simple but deprecated in favor of `sigaction()`.

For this educational shell, `signal()` is sufficient.

### `kill()`

Send signal to process:

```c
kill(pid, SIGTERM);  // Send SIGTERM to pid
```

Not used in basic shell, but useful for job control.

## Testing Phase 5

### Test 1: Ctrl+C with Running Command

```bash
shell> sleep 10
^C
shell>    # Prompt returns, shell didn't die
```

### Test 2: Background Execution

```bash
shell> sleep 5 &
[1234] sleep
shell>
# Wait 5 seconds
[1234] Done
```

### Test 3: Multiple Background Jobs

```bash
shell> sleep 10 &
[1234] sleep
shell> sleep 20 &
[5678] sleep
shell> sleep 30 &
[9012] sleep
shell> # Can type while jobs run
```

### Test 4: Background with Redirection

```bash
shell> sort < input.txt > output.txt &
[1234] sort
shell>
```

### Test 5: Background with Piping

```bash
shell> cat large.txt | sort | uniq &
[1234] cat
shell>
```

## Error Handling

### Job Not Found

```bash
shell> kill 99999
# No error (job doesn't exist)
# Real shells would show error
```

### Zombie Processes

If child doesn't exit immediately:
- Parent must call `waitpid()` to reap
- Else child becomes zombie (sleeping, using PID)
- Our implementation handles this correctly

### Signals During `waitpid()`

If signal arrives during `waitpid()`:
- System call is interrupted
- Signal handler runs
- `waitpid()` resumes or returns with error
- Shell continues normally

## Memory Implications

Background jobs don't change memory management significantly, but important to note:

- Child processes run concurrently
- Each has separate memory space
- Shell memory unchanged (parent process)
- No memory leaks from forking (still free()  on return)

## Advanced Signal Features (Not Implemented)

These are for more advanced shells:

- **Job control:** `fg`, `bg`, `jobs` commands
- **SIGSTOP/SIGCONT:** Suspend/resume jobs
- **SIGTERM:** Graceful termination
- **SIGHUP:** Terminal hangup (log out)
- **Zombie reaping:** SIGCHLD handler with waitpid(WNOHANG)

## Signal Safety

### Signals & Race Conditions

Critical: Signal handlers should do minimal work!

```c
// GOOD: Simple signal handler
void handler(int sig) {
    (void)sig;
    // Do almost nothing
}

// BAD: Dangerous signal handler
void bad_handler(int sig) {
    printf("Got signal %d\n", sig);  // printf might crash!
    malloc(sizeof(int));              // malloc might deadlock!
}
```

Our shell's handler is safe - it does nothing.

## Shell Architecture with Phase 5

```
Start Shell
   │
   ├─ setup_signal_handlers()  ← Handle Ctrl+C
   │
   └─ Main loop:
       ├─ Read input
       ├─ Parse (extract &)
       ├─ fork()
       │   ├─ Child: execvp
       │   └─ Parent: Check if background
       ├─ If background: Print job, continue
       ├─ If foreground: waitpid()
       └─ Repeat
```

## Combining All Phases

### Example: Full Featured Shell Command

```bash
shell> cat input.txt | sort | uniq | \
>      tail -20 > results.txt &
[1234] cat
shell>
# Command runs in background with:
# - PHASE 1: External commands (cat, sort, uniq, tail)
# - PHASE 2: (none in this example)
# - PHASE 3: Input < and output > redirection
# - PHASE 4: Piping (|)
# - PHASE 5: Background execution (&)
```

## Performance Notes

### Context Switching

Multiple background jobs = more context switching overhead. Each process:
- Gets time slice from kernel scheduler
- Runs until time expires or blocking call
- Kernel switches to next process

### CPU Usage

```bash
shell> yes > /dev/null &  # Consumes ~100% CPU
[1234] yes
shell> top | grep yes
yes    96.2%  CPU
```

Background jobs compete for CPU with shell and other processes.

## Summary

Phase 5 adds **signal handling and background jobs**:

**Key concepts:**
1. **Signals** are asynchronous notifications
2. **SIGINT (Ctrl+C)** - Shell ignores, children handle
3. **Signal handlers** intercept signals
4. **Background jobs (&)** - Don't wait for completion
5. **Process groups** - All pipeline jobs get signals together

**Why this matters:**
- User can interrupt long-running commands
- Shell stays alive
- User can run multiple commands concurrently
- Full interactive shell experience

---

**All 5 Phases Complete! 🎉**

Your shell now supports:
- ✅ Phase 1: Basic REPL with external commands
- ✅ Phase 2: Built-in commands (cd, pwd, exit)
- ✅ Phase 3: Input/output redirection (< > >>)
- ✅ Phase 4: Piping (|)
- ✅ Phase 5: Signal handling & background jobs (&)
