# Phase 4: Piping

**Status:** ✅ COMPLETE  
**Date:** June 29, 2026  
**Foundation:** Phases 1-3 (REPL, Built-ins, Redirection)

## Overview

Phase 4 adds support for piping, allowing the output of one command to become the input of another, creating **pipelines**.

## Pipe Operator: `|`

The pipe connects stdout of one command to stdin of the next:

```bash
shell> cat file.txt | grep pattern       # cat output → grep input
shell> cat file.txt | sort               # cat output → sort input
shell> cat | sort | uniq                 # Chain multiple commands
shell> cat file.txt | grep x | sort | uniq > result.txt  # With redirection
```

## How Pipes Work

### The `pipe()` System Call

```c
int pipefd[2];
pipe(pipefd);
```

Creates two file descriptors:
- `pipefd[0]` - Read end (stdin side)
- `pipefd[1]` - Write end (stdout side)

### Data Flow

```
Process 1                   Pipe                    Process 2
┌─────────────┐        ┌──────────────┐        ┌─────────────┐
│   cat file  │ ─fd[1]→ │ PIPE BUFFER │ ─fd[0]→ │ grep pattern│
│ (writes)    │        │              │        │ (reads)     │
└─────────────┘        └──────────────┘        └─────────────┘
```

**Key behaviors:**
- Write to `fd[1]`: Data goes into pipe buffer
- Read from `fd[0]`: Data comes from pipe buffer
- If buffer full: Writes block
- If buffer empty: Reads block (wait for data)
- Close both ends: Reader gets EOF

## Pipeline Architecture

### Pipeline Structure

```c
typedef struct {
    PipelineCommand *cmds;  // Array of commands
    int num_cmds;           // Number of commands
} Pipeline;

typedef struct {
    Command cmd;
    Redirection redir;
    int is_background;
} PipelineCommand;
```

### Parsing Pipelines

Input: `cat file.txt | grep pattern | sort`

**Parse algorithm:**
1. Split by `|` operator
2. For each segment:
   - Extract redirection operators (< > >>)
   - Parse remaining text as command
3. Store in Pipeline structure

**Result:**
```
Pipeline:
  cmds[0]: Command: [cat, file.txt, NULL]
           Redir: None
  cmds[1]: Command: [grep, pattern, NULL]
           Redir: None
  cmds[2]: Command: [sort, NULL]
           Redir: None
  num_cmds: 3
```

## Execution Algorithm

### Single Command (Phase 1-3)
```
fork()
  ├─ Child: Setup redirections, execvp
  └─ Parent: waitpid
```

### Pipeline (Phase 4)

```c
for (int i = 0; i < num_cmds; i++) {
    fork()
        ├─ Child 1: Setup pipe[1] as stdout, execvp(cmd1)
        ├─ Child 2: Setup pipe[0] as stdin, pipe[3] as stdout, execvp(cmd2)
        └─ Child N: Setup pipe[n-1] as stdin, execvp(cmdN)
    
    Parent: waitpid all children
}
```

### Detailed Process

**For: `cat file.txt | grep pattern | sort > result.txt`**

1. **Create pipes:**
   ```
   pipe(pipefd[0])  → [3, 4]  (between cat and grep)
   pipe(pipefd[1])  → [5, 6]  (between grep and sort)
   ```

2. **Fork child 1 (cat):**
   - Close read end of pipe 1: `close(pipefd[0][0])`
   - Redirect stdout to write end of pipe 1: `dup2(pipefd[0][1], 1)`
   - Close write end of pipe 2: `close(pipefd[1][1])`
   - execvp("cat", ["cat", "file.txt", NULL])
   - Result: `cat`'s output goes to pipe 1

3. **Fork child 2 (grep):**
   - Redirect stdin from read end of pipe 1: `dup2(pipefd[0][0], 0)`
   - Close write end of pipe 1: `close(pipefd[0][1])`
   - Redirect stdout to write end of pipe 2: `dup2(pipefd[1][1], 1)`
   - Close read end of pipe 2: `close(pipefd[1][0])`
   - execvp("grep", ["grep", "pattern", NULL])
   - Result: stdin from `cat`, output to pipe 2

4. **Fork child 3 (sort):**
   - Redirect stdin from read end of pipe 2: `dup2(pipefd[1][0], 0)`
   - Close both ends of pipes: not needed
   - Redirect stdout to result.txt: `dup2(fd, 1)`
   - execvp("sort", ["sort", NULL])
   - Result: stdin from `grep`, output to result.txt

5. **Parent:**
   - Close all pipe file descriptors (no longer needed)
   - Wait for all 3 children with `waitpid()`

## File Descriptor Management

### Pipe File Descriptors

```
Initial FDs (0, 1, 2 are stdin, stdout, stderr):

After pipe(pipefd):
  pipefd[0] = 3  (read end)
  pipefd[1] = 4  (write end)

After pipe(pipefd2):
  pipefd2[0] = 5  (read end)
  pipefd2[1] = 6  (write end)
```

### Closing Pipes

**Critical: Close unused ends!**

```c
// In child that reads from pipe
close(pipefd[1]);  // Don't need write end

// In child that writes to pipe
close(pipefd[0]);  // Don't need read end

// Close all in parent
for (int i = 0; i < num_pipes * 2; i++) {
    close(pipes[i]);
}
```

**Why close unused ends?**
- If write end stays open, reader doesn't get EOF
- Reader blocks forever waiting for more data
- Deadlock!

### Parent Process Cleanup

```c
/* Parent: close all pipes and wait for children */
for (int i = 0; i < (num_cmds - 1) * 2; i++) {
    close(pipes_array[i]);
}

for (int i = 0; i < num_cmds; i++) {
    int status;
    waitpid(pids[i], &status, 0);
}
```

## Data Flow Example

**Command: `echo hello | cat`**

```
Time 1:
  Parent: fork() → Child 1 (echo)
           fork() → Child 2 (cat)
  
Time 2:
  Echo:   Pipe write "hello\n"
  Pipe:   Buffer: [hello\n]
  Cat:    Pipe read waiting

Time 3:
  Cat:    Reads "hello\n" from pipe
  Pipe:   Buffer empty

Time 4:
  Echo:   Exits (closes write end)
  Pipe:   EOF signal sent to reader

Time 5:
  Cat:    Gets EOF, exits
  Parent: Both children done, prompt returns
```

## Testing Phase 4

### Test 1: Simple Pipe
```bash
shell> ls | grep src
src
```

### Test 2: Multiple Pipes
```bash
shell> cat Makefile | grep "make" | sort
help:
make:
valgrind:
```

### Test 3: Pipe with Redirection
```bash
shell> ls -la | grep "\.c" > cfiles.txt
shell> cat cfiles.txt
-rw-r--r--  114 user group ... main.c
-rw-r--r--  191 user group ... shell.c
```

### Test 4: Complex Pipeline
```bash
shell> ps aux | grep shell | grep -v grep | wc -l
1
```

### Test 5: Long Pipeline
```bash
shell> cat Makefile | sort | uniq | tail -5
valgrind:
wc: (counts output)
```

## Common Issues & Solutions

### Issue: Deadlock with Circular Pipes
```bash
shell> cat | cat | cat
# Hangs waiting for input
```
**Solution:** Pipe chains need proper closing of ends. This works when connected to stdin.

### Issue: Broken Pipe Error
```bash
shell> yes | head -1
yes: write error: Broken pipe
```
**Explanation:** `yes` tries to write to closed pipe. This is normal.

### Issue: Pipe Buffer Full
Large data through pipes can cause buffers to fill. Most systems handle this automatically with blocking writes.

## Memory Management

### Pipeline Cleanup

```c
void free_pipeline(Pipeline pipe) {
    if (pipe.cmds != NULL) {
        for (int i = 0; i < pipe.num_cmds; i++) {
            free_command(pipe.cmds[i].cmd);
            if (pipe.cmds[i].redir.infile != NULL) {
                free(pipe.cmds[i].redir.infile);
            }
            if (pipe.cmds[i].redir.outfile != NULL) {
                free(pipe.cmds[i].redir.outfile);
            }
        }
        free(pipe.cmds);
    }
}
```

## System Calls Used

| Call | Purpose |
|------|---------|
| `pipe()` | Create bidirectional pipe |
| `fork()` | Create child process |
| `dup2()` | Redirect file descriptors |
| `execvp()` | Execute command |
| `waitpid()` | Wait for child |
| `close()` | Close file descriptor |

## Advanced Features

### Combining Phases 3 & 4

**Pipe + Input Redirection:**
```bash
shell> sort < numbers.txt | uniq | wc -l
```

**Pipe + Output Redirection:**
```bash
shell> ps aux | grep ssh > ssh_processes.txt
```

**Full Pipeline:**
```bash
shell> cat input.txt | sort | uniq | grep pattern > output.txt
```

**Stderr NOT included in pipes** (same as real shells):
```bash
shell> cat nonexistent 2>&1 | grep "cannot"
# This shows error (if stderr redirection supported)
```

## Performance Considerations

### Pipe Buffer Size
Most systems: 4KB to 64KB default buffer.

### Number of Processes
Each command in pipeline = one process.

Example: `cat | sort | uniq | grep | wc`
- Creates 5 child processes
- 4 pipes between them
- All run in parallel (concurrent execution)

## Limitations & Future Work

- No stderr redirection (2>)
- No file descriptor redirection (>&, >&2)
- No process substitution (<())
- No named pipes (mkfifo)
- Coming in Phase 5: Background execution (&)

## Summary

Phase 4 adds **piping** - connecting command outputs to inputs:

**Key concepts:**
1. **pipe()** creates read/write file descriptors
2. **dup2()** redirects stdin/stdout to pipes
3. **Multiple children** run in parallel
4. **Parent waits** for all children

**Critical insight: EOF detection**
When write end of pipe is closed, reader gets EOF and can exit. This is how pipes know when to stop reading.

---

**Next Phase:** Phase 5 - Signal Handling
