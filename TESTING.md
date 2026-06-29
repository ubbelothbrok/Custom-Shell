# Phase 1 Testing & Debugging Guide

## Quick Test

Run the shell:
```bash
make run
```

Test some commands interactively:
```bash
shell> ls
shell> pwd
shell> whoami
shell> echo "Hello, Shell!"
shell> cat Makefile
shell> date +%A
```

Exit with: **Ctrl+D**

## Automated Testing

Test multiple commands programmatically:
```bash
echo -e "ls\npwd\necho test\ndate" | ./bin/shell
```

## Memory Leak Detection

### On macOS with Leaks Tool (Built-in)

Unfortunately, Valgrind doesn't work well on macOS. Instead, use the built-in `leaks` tool:

```bash
# Run shell in background while controlled
./bin/shell < /dev/null &
PID=$!
sleep 1
leaks $PID
kill $PID 2>/dev/null
```

Or use Xcode's Instruments:
```bash
xcode-select --install  # If needed
instruments -t "Leaks" -D /tmp ./bin/shell
```

### On Linux (Using Valgrind)

If testing on a Linux system, use Valgrind:
```bash
make valgrind
```

Then type commands and press Ctrl+D. Valgrind will report:
```
LEAK SUMMARY:
    definitely lost: 0 bytes
    indirectly lost: 0 bytes
```

This means **zero memory leaks** ✓

## Debugging with GDB

### Interactive Debugging

```bash
make debug
```

Then:
```
(gdb) run
shell> ls
^C                  # Press Ctrl+C to break
(gdb) bt            # Show backtrace
(gdb) p cmd.argc    # Print variable
(gdb) c             # Continue
(gdb) quit
```

### Conditional Breakpoints

In GDB session:
```
(gdb) break shell.c:85 if cmd.argc > 2
(gdb) run
shell> ls -la /tmp
```

It breaks only when argc > 2.

## System Call Tracing

### On macOS (Using dtrace or dtruss)

```bash
sudo dtruss -f -e ./bin/shell 2>&1 | head -100
```

Then type a command and Ctrl+D.

### On Linux (Using strace)

```bash
make strace
```

Shows all `fork`, `execve`, and `wait4` calls in real-time.

Example output:
```
execve("./bin/shell", ["./bin/shell"], 0x7ffee...) = 0
fork()                                  = 12345
execve("/bin/ls", ["ls"], 0x7ffee...) = 0
wait4(12345, [status], 0, 0)            = 12345
```

## Stress Testing

Test for memory leaks with 100 commands:

### Using a Loop

```bash
# Generate 100 commands
for i in {1..100}; do
    echo "ls -l"
    echo "pwd"
    echo "echo test_$i"
done | ./bin/shell
```

### Using a Script

```bash
cat > test_commands.txt << 'EOF'
ls
pwd
echo test1
echo test2
echo test3
ls -la
pwd
whoami
date
echo final
EOF

./bin/shell < test_commands.txt
```

## Code Coverage & Analysis

### Static Analysis (Linux/macOS)

Use `cppcheck` for code issues:
```bash
cppcheck src/
```

### Valgrind on Linux

Complete memory analysis:
```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --log-file=valgrind-out.txt \
         ./bin/shell

# Exit shell with Ctrl+D
cat valgrind-out.txt
```

## Expected Behavior

### Correct Execution

```bash
shell> ls
bin     build   Makefile    PHASE1.md   src
shell> pwd
/Users/ubbe/Documents/Custom-Shell
shell> echo hello world
hello world
shell> ^D
Exit
```

### Error Handling

The shell should handle:

1. **Unknown command**
   ```bash
   shell> nonexistent_command_xyz
   execvp: No such file or directory
   shell>
   ```
   (Error message, then prompt returns)

2. **Empty input**
   ```bash
   shell>                     # Just press Enter
   shell>                     # Prompt returns immediately
   ```

3. **Long input** (up to MAX_INPUT_SIZE = 1024)
   ```bash
   shell> echo very long argument 1 2 3 4 5 ... (stays under 1024 chars)
   ```

4. **Many arguments** (up to MAX_ARGS = 64)
   ```bash
   shell> echo a b c d e ... (up to 64 arguments)
   ```

## Performance Observations

### Expected Performance

Phase 1 should execute each command with minimal overhead:

- **Fork time**: ~0-2ms (typically negligible)
- **Wait time**: Depends on the command
- **Memory per command**: ~2KB for typical 5-10 argument commands

### Profile a Single Command

```bash
time ./bin/shell << 'EOF'
ls
EOF
```

Should be nearly instant (< 50ms).

## Troubleshooting

### Issue: Shell hangs after command

**Cause**: `waitpid()` not working correctly  
**Fix**: Check `execute_command()` in shell.c - make sure `waitpid()` is called

### Issue: Memory grows with each command

**Cause**: `free_command()` not called or not working  
**Fix**: Verify loop in main.c calls `free_command()` after `execute_command()`

### Issue: Arguments not parsed correctly

**Cause**: `strtok()` splitting incorrectly  
**Fix**: Only spaces/tabs work as delimiters; quotes not supported in Phase 1

### Issue: Command not found errors

**Cause**: Command not in PATH  
**Fix**: Use full path `/bin/ls` instead of `ls`, or add directory to PATH

## Summary: Phase 1 Verification Checklist

- [x] Shell compiles with no errors
- [x] Shell compiles with no warnings
- [x] Shell executes simple commands (ls, pwd, date)
- [x] Shell executes commands with arguments (ls -la)
- [x] Shell displays prompt after each command
- [x] Shell exits cleanly with Ctrl+D
- [x] Shell handles empty input gracefully
- [x] Shell reports errors for unknown commands
- [x] Memory is freed after each command (can verify with Valgrind on Linux)
- [x] Parent process stays alive while child executes

**Phase 1 is complete!** ✅

Next: Phase 2 will add built-in commands (cd, exit) and improved command handling.
