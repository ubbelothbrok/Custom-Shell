#include "shell.h"

/*
 * Function: parse_command
 * ----------------------
 * This function implements a simple command-line parser.
 * It splits the input string by whitespace and creates an array of pointers
 * to each argument, similar to how the kernel creates argv for a process.
 *
 * Algorithm:
 * 1. Make a copy of the input to avoid modifying the original
 * 2. Use strtok() to split by whitespace
 * 3. Store pointers to each token in a dynamic array
 * 4. Add a NULL terminator (required by execvp)
 *
 * Time Complexity: O(n) where n is the length of the input string
 * Space Complexity: O(m) where m is the number of arguments
 */
Command parse_command(char *input) {
    Command cmd;
    cmd.args = NULL;
    cmd.argc = 0;

    /* Make a copy of input because strtok() modifies the string */
    char *input_copy = malloc(strlen(input) + 1);
    if (input_copy == NULL) {
        perror("malloc failed in parse_command");
        return cmd;
    }
    strcpy(input_copy, input);

    /* Allocate space for pointers to arguments (plus one for NULL terminator) */
    cmd.args = malloc(MAX_ARGS * sizeof(char *));
    if (cmd.args == NULL) {
        perror("malloc failed for cmd.args");
        free(input_copy);
        return cmd;
    }

    /* 
     * Tokenize the input string. strtok() splits on whitespace by default.
     * The first call passes the string, subsequent calls pass NULL.
     */
    char *token = strtok(input_copy, " \t\n");
    while (token != NULL && cmd.argc < MAX_ARGS - 1) {
        /* Allocate space for each argument and copy it */
        cmd.args[cmd.argc] = malloc(strlen(token) + 1);
        if (cmd.args[cmd.argc] == NULL) {
            perror("malloc failed for argument");
            free_command(cmd);
            free(input_copy);
            return cmd;
        }
        strcpy(cmd.args[cmd.argc], token);
        cmd.argc++;
        token = strtok(NULL, " \t\n");
    }

    /* 
     * NULL-terminate the args array.
     * This is REQUIRED by execvp(), which expects argv to be NULL-terminated.
     * This is how execvp knows where the argument list ends.
     */
    cmd.args[cmd.argc] = NULL;

    free(input_copy);
    return cmd;
}

/*
 * Function: free_command
 * ----------------------
 * Properly deallocates all memory used by a Command structure.
 * This is critical to avoid memory leaks.
 *
 * We must free in the correct order:
 * 1. Free each individual argument string
 * 2. Free the args array itself
 */
void free_command(Command cmd) {
    if (cmd.args != NULL) {
        /* Free each argument string */
        for (int i = 0; i < cmd.argc; i++) {
            if (cmd.args[i] != NULL) {
                free(cmd.args[i]);
            }
        }
        /* Free the args array itself */
        free(cmd.args);
    }
}

/*
 * Function: execute_command
 * -------------------------
 * This function demonstrates the fork/execvp pattern, which is fundamental
 * to how Unix shells work.
 *
 * Process Flow:
 * ============
 * 1. FORK: The parent process creates a child process (identical copy)
 *    - Parent gets child's PID
 *    - Child gets 0 as return value from fork()
 *
 * 2. CHILD PROCESS: 
 *    - Calls execvp() to replace its memory image with the new program
 *    - execvp() searches PATH for the executable and runs it
 *    - If execvp() succeeds, this code never returns
 *    - If execvp() fails, we print an error and exit
 *
 * 3. PARENT PROCESS:
 *    - Calls wait() to block until child completes
 *    - wait() returns the child's exit status
 *    - Parent can then prompt for the next command
 *
 * This design keeps the shell alive while running external programs.
 *
 * execvp() function:
 * - v: takes an array of strings (like argv)
 * - p: searches for the executable in PATH environment variable
 * - This is why we don't need full paths (/bin/ls vs just ls)
 */
void execute_command(Command cmd) {
    /* 
     * Sanity check: make sure we have a command to execute
     */
    if (cmd.argc == 0 || cmd.args[0] == NULL) {
        fprintf(stderr, "Error: empty command\n");
        return;
    }

    /*
     * FORK: Create a new child process
     * Returns:
     * - In parent: child's process ID (positive number)
     * - In child: 0
     * - On error: -1
     */
    pid_t pid = fork();

    if (pid < 0) {
        /* fork() failed */
        perror("fork");
        return;
    }

    if (pid == 0) {
        /* 
         * CHILD PROCESS
         * 
         * We are now in the child process. Our job is to replace this process's
         * memory image with the new program.
         * 
         * execvp(program, argv):
         * - argv[0] should be the program name (conventionally)
         * - argv must be NULL-terminated
         * - searches PATH environment variable for 'program'
         * 
         * If execvp() succeeds, we never return from this call.
         * The program takes over completely.
         * If it fails (e.g., command not found), execvp returns -1.
         */
        execvp(cmd.args[0], cmd.args);

        /* 
         * If we reach here, execvp() failed
         */
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        /* 
         * PARENT PROCESS
         * 
         * We are the shell. Our child is now running the user's command.
         * We need to wait for it to finish before showing the prompt again.
         * 
         * wait() system call:
         * - Blocks the parent until ANY child process exits
         * - Returns the child's PID when it exits
         * - status pointer receives exit information
         * - We pass NULL since we don't need the exit status
         */
        int status;
        waitpid(pid, &status, 0);

        /* 
         * At this point, the child has finished executing.
         * The shell will loop back and display the prompt again.
         */
    }
}

/* ========================================================================== */
/* PHASE 2: BUILT-IN COMMANDS                                               */
/* ========================================================================== */

/*
 * Function: is_builtin
 * ---------------------
 * Determines if a command is a built-in shell command (cd, exit).
 * Built-in commands are handled directly by the shell, not via fork/exec.
 * This is necessary for cd (chdir affects shell process) and exit (kill shell).
 */
int is_builtin(const char *cmd) {
    if (cmd == NULL) return 0;
    
    return (strcmp(cmd, "cd") == 0) ||
           (strcmp(cmd, "exit") == 0) ||
           (strcmp(cmd, "pwd") == 0);
}

/*
 * Function: execute_builtin
 * --------------------------
 * Executes a built-in shell command.
 * 
 * cd <path>:  Changes directory using chdir() system call
 * exit:       Terminates the shell
 * pwd:        Prints current working directory
 * 
 * Returns: 1 if shell should exit, 0 otherwise
 */
int execute_builtin(Command cmd) {
    if (cmd.argc == 0 || cmd.args[0] == NULL) {
        return 0;
    }

    if (strcmp(cmd.args[0], "cd") == 0) {
        /* cd command: change directory */
        if (cmd.argc < 2) {
            /* cd with no arguments - go to home directory */
            const char *home = getenv("HOME");
            if (home == NULL || chdir(home) < 0) {
                perror("cd");
            }
        } else {
            /* cd to specified directory */
            if (chdir(cmd.args[1]) < 0) {
                perror("cd");
            }
        }
        return 0;
    }
    
    if (strcmp(cmd.args[0], "pwd") == 0) {
        /* pwd command: print working directory */
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
        return 0;
    }

    if (strcmp(cmd.args[0], "exit") == 0) {
        /* exit command: terminate shell */
        int exit_code = 0;
        if (cmd.argc > 1) {
            exit_code = atoi(cmd.args[1]);
        }
        printf("Exit\n");
        exit(exit_code);
    }

    return 0;
}

/* ========================================================================== */
/* PHASE 3+: PIPELINE & REDIRECTION SUPPORT                                 */
/* ========================================================================== */

/*
 * Function: parse_pipeline
 * -------------------------
 * Parses a complete command line with pipes and redirections.
 * 
 * Supports:
 * - Single commands: ls -la
 * - Redirections: cat < file.txt, ls > output.txt, echo >> log.txt
 * - Pipes: cat file.txt | grep pattern | sort
 * - Combined: cat < input.txt | sort > output.txt
 * - Background: sleep 10 &
 */
Pipeline parse_pipeline(char *input) {
    Pipeline pipe;
    pipe.cmds = NULL;
    pipe.num_cmds = 0;

    if (input == NULL || strlen(input) == 0) {
        return pipe;
    }

    /* Copy input for processing */
    char *input_copy = malloc(strlen(input) + 1);
    if (input_copy == NULL) {
        perror("malloc failed in parse_pipeline");
        return pipe;
    }
    strcpy(input_copy, input);

    /* Split by pipes */
    pipe.cmds = malloc(MAX_PIPES * sizeof(PipelineCommand));
    if (pipe.cmds == NULL) {
        perror("malloc failed for pipeline");
        free(input_copy);
        return pipe;
    }

    /* Initialize pipeline commands */
    for (int i = 0; i < MAX_PIPES; i++) {
        pipe.cmds[i].cmd.args = NULL;
        pipe.cmds[i].cmd.argc = 0;
        pipe.cmds[i].redir.infile = NULL;
        pipe.cmds[i].redir.outfile = NULL;
        pipe.cmds[i].redir.append = 0;
        pipe.cmds[i].is_background = 0;
    }

    /* Split by pipe operator */
    char *saveptr = NULL;
    char *cmd_str = strtok_r(input_copy, "|", &saveptr);
    
    while (cmd_str != NULL && pipe.num_cmds < MAX_PIPES) {
        /* Trim whitespace */
        while (*cmd_str == ' ' || *cmd_str == '\t') cmd_str++;
        char *end = cmd_str + strlen(cmd_str) - 1;
        while (end > cmd_str && (*end == ' ' || *end == '\t')) end--;
        *(end + 1) = '\0';

        /* Check for background operator (&) */
        int is_bg = 0;
        if (strlen(cmd_str) > 0 && cmd_str[strlen(cmd_str) - 1] == '&') {
            is_bg = 1;
            cmd_str[strlen(cmd_str) - 1] = '\0';
            /* Trim again */
            while (strlen(cmd_str) > 0 && cmd_str[strlen(cmd_str) - 1] == ' ') {
                cmd_str[strlen(cmd_str) - 1] = '\0';
            }
        }

        /* Parse redirections */
        char *infile = NULL, *outfile = NULL;
        int append = 0;
        
        /* Look for < (input redirection) */
        char *redir_ptr = strchr(cmd_str, '<');
        if (redir_ptr != NULL) {
            *redir_ptr = '\0';  /* Terminate command part */
            char *file_start = redir_ptr + 1;
            while (*file_start == ' ' || *file_start == '\t') file_start++;
            infile = malloc(strlen(file_start) + 1);
            if (infile != NULL) {
                strcpy(infile, file_start);
                /* Remove trailing whitespace */
                end = infile + strlen(infile) - 1;
                while (end > infile && (*end == ' ' || *end == '\t')) {
                    *end = '\0';
                    end--;
                }
            }
        }

        /* Look for > or >> (output redirection) */
        if (strstr(cmd_str, ">>") != NULL) {
            redir_ptr = strstr(cmd_str, ">>");
            append = 1;
        } else {
            redir_ptr = strchr(cmd_str, '>');
        }
        
        if (redir_ptr != NULL) {
            if (append) {
                *redir_ptr = '\0';
                *(redir_ptr + 1) = '\0';  /* Mark end of command */
                redir_ptr += 2;
            } else {
                *redir_ptr = '\0';
                redir_ptr++;
            }
            
            char *file_start = redir_ptr;
            while (*file_start == ' ' || *file_start == '\t') file_start++;
            outfile = malloc(strlen(file_start) + 1);
            if (outfile != NULL) {
                strcpy(outfile, file_start);
                /* Remove trailing whitespace */
                end = outfile + strlen(outfile) - 1;
                while (end > outfile && (*end == ' ' || *end == '\t')) {
                    *end = '\0';
                    end--;
                }
            }
        }

        /* Parse command arguments */
        if (strlen(cmd_str) > 0) {
            pipe.cmds[pipe.num_cmds].cmd = parse_command(cmd_str);
            pipe.cmds[pipe.num_cmds].redir.infile = infile;
            pipe.cmds[pipe.num_cmds].redir.outfile = outfile;
            pipe.cmds[pipe.num_cmds].redir.append = append;
            pipe.cmds[pipe.num_cmds].is_background = is_bg;
            pipe.num_cmds++;
        }

        cmd_str = strtok_r(NULL, "|", &saveptr);
    }

    free(input_copy);
    return pipe;
}

/*
 * Function: free_pipeline
 * ------------------------
 * Frees all memory allocated for a pipeline structure.
 */
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

/* ========================================================================== */
/* PHASE 4: PIPELINE EXECUTION                                              */
/* ========================================================================== */

/*
 * Wrapper for pipe() syscall to avoid naming conflict with Pipeline struct
 */
static inline int pipe_create(int pipefd[2]) {
    return pipe(pipefd);
}

/*
 * Function: execute_pipeline
 * ----------------------------
 * Executes a pipeline of commands with proper piping and redirection.
 * 
 * Algorithm:
 * 1. For each command in pipeline:
 *    - Create a pipe() for next command (if not last)
 *    - fork() a child process
 *    - Setup file descriptors (stdin/stdout with pipes and redirections)
 *    - execvp() the command
 * 2. Parent waits for all children to complete
 * 
 * Example: cat file.txt | grep pattern | sort > output.txt
 *   - Child 1: cat reads file, writes to pipe1
 *   - Child 2: reads from pipe1, writes to pipe2
 *   - Child 3: reads from pipe2, writes to output.txt
 *   - Parent: waits for all 3 children
 */
void execute_pipeline(Pipeline pipe) {
    if (pipe.num_cmds == 0) {
        return;
    }

    /* Single command without pipes */
    if (pipe.num_cmds == 1) {
        PipelineCommand *pcmd = &pipe.cmds[0];
        
        /* Check if it's a built-in command */
        if (is_builtin(pcmd->cmd.args[0])) {
            execute_builtin(pcmd->cmd);
            return;
        }

        /* Handle redirection for single command */
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        }

        if (pid == 0) {
            /* Child process */
            
            /* Handle input redirection */
            if (pcmd->redir.infile != NULL) {
                int fd = open(pcmd->redir.infile, O_RDONLY);
                if (fd < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            /* Handle output redirection */
            if (pcmd->redir.outfile != NULL) {
                int flags = O_WRONLY | O_CREAT;
                if (pcmd->redir.append) {
                    flags |= O_APPEND;
                } else {
                    flags |= O_TRUNC;
                }
                int fd = open(pcmd->redir.outfile, flags, 0644);
                if (fd < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            execvp(pcmd->cmd.args[0], pcmd->cmd.args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            /* Parent process */
            int status;
            if (pcmd->is_background) {
                /* Background process: don't wait */
                printf("[%d] %s\n", pid, pcmd->cmd.args[0]);
            } else {
                /* Foreground: wait for completion */
                waitpid(pid, &status, 0);
            }
        }
        return;
    }

    /* Multiple commands in pipeline */
    pid_t *pids = malloc(pipe.num_cmds * sizeof(pid_t));
    int *pipes_array = malloc((pipe.num_cmds - 1) * 2 * sizeof(int));
    
    if (pids == NULL || pipes_array == NULL) {
        perror("malloc");
        free(pids);
        free(pipes_array);
        return;
    }

    /* Create all pipes first */
    for (int i = 0; i < pipe.num_cmds - 1; i++) {
        if (pipe_create(pipes_array + i * 2) < 0) {
            perror("pipe");
            free(pids);
            free(pipes_array);
            return;
        }
    }

    /* Fork each command */
    for (int i = 0; i < pipe.num_cmds; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            free(pids);
            free(pipes_array);
            return;
        }

        if (pid == 0) {
            /* Child process */
            
            /* Connect stdin from previous pipe */
            if (i > 0) {
                dup2(pipes_array[(i - 1) * 2], STDIN_FILENO);
            }

            /* Handle input redirection */
            if (pipe.cmds[i].redir.infile != NULL) {
                int fd = open(pipe.cmds[i].redir.infile, O_RDONLY);
                if (fd < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            /* Connect stdout to next pipe */
            if (i < pipe.num_cmds - 1) {
                dup2(pipes_array[i * 2 + 1], STDOUT_FILENO);
            }

            /* Handle output redirection */
            if (pipe.cmds[i].redir.outfile != NULL) {
                int flags = O_WRONLY | O_CREAT;
                if (pipe.cmds[i].redir.append) {
                    flags |= O_APPEND;
                } else {
                    flags |= O_TRUNC;
                }
                int fd = open(pipe.cmds[i].redir.outfile, flags, 0644);
                if (fd < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            /* Close all pipe file descriptors */
            for (int j = 0; j < (pipe.num_cmds - 1) * 2; j++) {
                close(pipes_array[j]);
            }

            execvp(pipe.cmds[i].cmd.args[0], pipe.cmds[i].cmd.args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            pids[i] = pid;
        }
    }

    /* Parent: close all pipes and wait for children */
    for (int i = 0; i < (pipe.num_cmds - 1) * 2; i++) {
        close(pipes_array[i]);
    }

    /* Wait for all children */
    for (int i = 0; i < pipe.num_cmds; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }

    free(pids);
    free(pipes_array);
}

/* ========================================================================== */
/* PHASE 5: SIGNAL HANDLING                                                 */
/* ========================================================================== */

/*
 * Signal handler for SIGINT (Ctrl+C)
 * Does nothing - shell ignores Ctrl+C and continues
 */
void sigint_handler(int sig) {
    (void)sig;
    /* Ignore SIGINT - shell should not exit on Ctrl+C */
}

/*
 * Function: setup_signal_handlers
 * --------------------------------
 * Registers signal handlers for proper shell behavior.
 * 
 * SIGINT (Ctrl+C): Ignore in shell, let children handle it
 * SIGCHLD: Clean up zombie child processes
 */
void setup_signal_handlers(void) {
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, SIG_DFL);  /* Use default handler for SIGCHLD */
}
