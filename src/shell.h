#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

/* 
 * Maximum number of arguments a command can have.
 */
#define MAX_ARGS 64

/* 
 * Maximum length of a single command input line.
 */
#define MAX_INPUT_SIZE 1024

/*
 * Maximum number of pipes in a single command line.
 * For: cmd1 | cmd2 | cmd3 (2 pipes)
 */
#define MAX_PIPES 10

/*
 * Structure to represent a parsed command.
 * args: array of argument strings (NULL-terminated)
 * argc: argument count
 * 
 * PHASE 1+: Basic command structure
 */
typedef struct {
    char **args;
    int argc;
} Command;

/*
 * Structure for redirection (PHASE 3+)
 * Supports: < (input), > (output overwrite), >> (output append)
 */
typedef struct {
    char *infile;       /* Input file for < redirection */
    char *outfile;      /* Output file for > or >> redirection */
    int append;         /* 1 if >> (append), 0 if > (overwrite) */
} Redirection;

/*
 * Structure for a single command in a pipeline (PHASE 4+)
 * Contains: command, redirection info
 */
typedef struct {
    Command cmd;
    Redirection redir;
    int is_background;  /* 1 if & at end (PHASE 5) */
} PipelineCommand;

/*
 * Structure for a complete pipeline (PHASE 4+)
 * Handles: cmd1 | cmd2 | cmd3
 */
typedef struct {
    PipelineCommand *cmds;  /* Array of commands */
    int num_cmds;           /* Number of commands in pipeline */
} Pipeline;


/* 
 * Function: parse_command
 * ----------------------
 * Parses a single command string into a Command structure.
 * Phase 1: Basic tokenization
 * Phase 3+: Extracts redirection operators
 *
 * input: The input command string to parse
 * redir: Output parameter for redirection info (Phase 3+)
 * 
 * Returns: A Command structure with args array
 */
Command parse_command(char *input);

/* 
 * Function: parse_pipeline
 * -------------------------
 * Parses a complete pipeline with multiple commands and redirections.
 * Phase 4+: Handles | operator
 * Phase 3+: Handles <, >, >> operators
 * Phase 5+: Handles & for background processes
 *
 * input: The input command line
 * 
 * Returns: A Pipeline structure with all commands
 */
Pipeline parse_pipeline(char *input);

/*
 * Function: free_command
 * ----------------------
 * Frees all dynamically allocated memory in a Command structure.
 *
 * cmd: The Command structure to free
 */
void free_command(Command cmd);

/*
 * Function: free_pipeline
 * ------------------------
 * Frees all dynamically allocated memory in a Pipeline structure.
 *
 * pipe: The Pipeline structure to free
 */
void free_pipeline(Pipeline pipe);

/*
 * Function: is_builtin
 * ---------------------
 * Checks if a command is a built-in shell command.
 * Phase 2+: Implements cd and exit
 *
 * cmd: The first argument (command name)
 * 
 * Returns: 1 if built-in, 0 otherwise
 */
int is_builtin(const char *cmd);

/*
 * Function: execute_builtin
 * --------------------------
 * Executes a built-in shell command.
 * Phase 2+: cd, exit
 *
 * cmd: The Command structure to execute
 * 
 * Returns: 1 if shell should exit, 0 otherwise
 */
int execute_builtin(Command cmd);

/*
 * Function: execute_command
 * -------------------------
 * Executes a single external command using fork() and execvp().
 * Phase 1: Basic execution
 * Phase 3+: With redirection
 *
 * cmd: The Command structure to execute
 * redir: Redirection info (Phase 3+)
 */
void execute_command(Command cmd);

/*
 * Function: execute_pipeline
 * ----------------------------
 * Executes a pipeline of commands with proper piping.
 * Phase 4+: Handles multiple commands with |
 *
 * pipe: The Pipeline structure to execute
 */
void execute_pipeline(Pipeline pipe);

/*
 * Function: setup_signal_handlers
 * --------------------------------
 * Sets up signal handlers for proper shell behavior.
 * Phase 5+: Handle SIGINT (Ctrl+C) without exiting shell
 */
void setup_signal_handlers(void);

#endif // SHELL_H
