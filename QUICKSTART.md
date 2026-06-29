# Custom Unix Shell - Quick Start Guide

## Installation

```bash
cd /Users/ubbe/Documents/Custom-Shell
make clean
make
```

## Running the Shell

```bash
./bin/shell
```

## Basic Commands

### External Commands
```bash
shell> ls                    # List files
shell> pwd                   # Print working directory
shell> echo hello world      # Print text
shell> cat filename.txt      # Display file contents
shell> date                  # Show current date/time
```

### Built-in Commands
```bash
shell> cd /tmp               # Change directory
shell> pwd                   # Show current directory
shell> exit                  # Exit shell
```

### File Redirection
```bash
shell> cat < input.txt       # Read from file
shell> echo test > out.txt   # Write to file
shell> echo data >> out.txt  # Append to file
```

### Piping
```bash
shell> ls | grep src         # Filter output
shell> cat file | sort | uniq # Multiple pipes
shell> ps aux | grep shell   # Search processes
```

### Background Jobs
```bash
shell> sleep 100 &           # Run in background
shell> command &             # Returns to prompt immediately
```

## Exiting

Press `Ctrl+D` or type:
```bash
shell> exit
```

## Files Structure

```
Custom-Shell/
├── src/
│   ├── main.c          # Main REPL loop
│   ├── shell.c         # Parsing & execution engine
│   └── shell.h         # Interface definitions
├── bin/
│   └── shell           # Compiled executable
├── build/              # Object files
├── Makefile            # Build configuration
└── *.md                # Documentation (15 files)
```

## Make Commands

```bash
make              # Build shell
make clean        # Remove build artifacts
make run          # Build and run
make debug        # Run with GDB debugger
make help         # Show all targets
```

## Troubleshooting

**Shell won't start**
- Check if compiled: `ls -la bin/shell`
- Rebuild: `make clean && make`

**Command not found**
- Make sure it's in your PATH or use full path
- Check spelling and syntax

**Pipe not working**
- Verify syntax: `cmd1 | cmd2`
- Check intermediate commands work: `cmd1`, then `cmd2`

**File redirection issues**
- For input: `< file.txt` (file must exist)
- For output: `> file.txt` (creates/overwrites)
- For append: `>> file.txt` (adds to end)

## Learning Resources

| Topic | File |
|-------|------|
| Introduction | [README.md](README.md) |
| Phase 1 | [PHASE1.md](PHASE1.md) |
| Phase 2 | [PHASE2.md](PHASE2.md) |
| Phase 3 | [PHASE3.md](PHASE3.md) |
| Phase 4 | [PHASE4.md](PHASE4.md) |
| Phase 5 | [PHASE5.md](PHASE5.md) |
| Code Details | [CODE_WALKTHROUGH.md](CODE_WALKTHROUGH.md) |
| Testing | [TESTING.md](TESTING.md) |
| Quick Ref | [QUICK_REFERENCE.md](QUICK_REFERENCE.md) |

## Key Concepts

**Fork/Exec**
- `fork()` creates a copy of the current process
- `execvp()` replaces the process with a new program
- Parent waits for child with `wait()`

**Pipes**
- Connect output of one command to input of another
- Implemented with `pipe()` syscall
- File descriptors redirected with `dup2()`

**Redirection**
- `<` connects stdin to a file
- `>` connects stdout to a file
- `>>` appends stdout to a file
- Implemented with `open()` and `dup2()`

**Signal Handling**
- Ctrl+C sends SIGINT signal
- Custom handler allows shell to continue
- Background jobs run without blocking prompt

## Performance Tips

- Complex pipelines may take longer
- Background jobs allow parallel execution
- Use redirection to save command output
- Combine pipes for efficient data processing

## Next Steps

1. **Explore commands**: Try different combinations
2. **Read documentation**: Learn how it works
3. **Study code**: See implementation details
4. **Extend features**: Add your own commands
5. **Optimize**: Improve performance/features

## Support

For issues or questions, refer to:
- [TESTING.md](TESTING.md) - How to test features
- [CODE_WALKTHROUGH.md](CODE_WALKTHROUGH.md) - How code works
- [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md) - What's implemented

## Summary

✅ Complete Unix shell implementation  
✅ All 5 phases working  
✅ Comprehensive documentation  
✅ Production-ready code  
✅ Educational resource  

Enjoy your custom shell! 🎉
