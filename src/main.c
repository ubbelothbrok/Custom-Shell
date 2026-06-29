#include "shell.h"

/*
 * COMPLETE UNIX-LIKE SHELL IMPLEMENTATION
 * ========================================
 *
 * PHASE 1: Basic REPL with external command execution
 * PHASE 2: Built-in commands (cd, exit, pwd)
 * PHASE 3: Input/output redirection (<, >, >>)
 * PHASE 4: Piping (|) with multiple commands
 * PHASE 5: Signal handling (ignore Ctrl+C, background jobs with &)
 *
 * Architecture:
 * - Modular design with separate functions for each feature
 * - POSIX APIs only (fork, exec vp, pipe, dup2, chdir, signal)
 * - Manual memory management (malloc/free)
 * - Comprehensive error handling
 */

/*
 * Function: main
 * ---------------
 * The main REPL loop supporting all 5 phases.
 * 
 * Flow:
 * 1. Setup signal handlers (PHASE 5)
 * 2. Read input from stdin
 * 3. Parse into pipeline structure (handles phases 1-4)
 * 4. Execute (built-ins or external with piping/redirection)
 * 5. Free memory
 * 6. Repeat or exit
 */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    char input[MAX_INPUT_SIZE];

    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║           Custom Shell - All Phases Complete           ║\n");
    printf("║                                                        ║\n");
    printf("║ Features:                                              ║\n");
    printf("║  • External commands: ls, pwd, echo, date, etc.        ║\n");
    printf("║  • Built-ins: cd, pwd, exit                            ║\n");
    printf("║  • Redirection: <, >, >>                               ║\n");
    printf("║  • Piping: cmd1 | cmd2 | cmd3                          ║\n");
    printf("║  • Background: cmd &  (doesn't exit on Ctrl+C)         ║\n");
    printf("║                                                        ║\n");
    printf("║ Examples:                                              ║\n");
    printf("║  shell> ls -la                                         ║\n");
    printf("║  shell> cat file.txt | grep pattern | sort             ║\n");
    printf("║  shell> cat < input.txt > output.txt                   ║\n");
    printf("║  shell> cd /tmp                                        ║\n");
    printf("║  shell> exit                                           ║\n");
    printf("║                                                        ║\n");
    printf("║ Press Ctrl+D to exit gracefully                        ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");

    /* Setup signal handlers (PHASE 5) */
    setup_signal_handlers();

    /* 
     * MAIN SHELL LOOP
     * Supports all phases (1-5)
     */
    while (1) {
        /* Display prompt */
        printf("shell> ");
        fflush(stdout);

        /* 
         * READ: Get user input
         */
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            /* EOF (Ctrl+D) - exit gracefully */
            printf("\nExit\n");
            break;
        }

        /* 
         * Remove trailing newline
         */
        input[strcspn(input, "\n")] = '\0';

        /* 
         * Skip empty lines
         */
        if (strlen(input) == 0) {
            continue;
        }

        /* 
         * PARSE: Convert input into pipeline structure
         * 
         * This handles:
         * - PHASE 1: Single command parsing
         * - PHASE 2: Command recognition
         * - PHASE 3: Redirection operators (< > >>)
         * - PHASE 4: Pipe operators (|)
         * - PHASE 5: Background operator (&)
         */
        Pipeline pipe = parse_pipeline(input);

        if (pipe.num_cmds > 0) {
            /* 
             * Check if first command is built-in (PHASE 2)
             */
            if (is_builtin(pipe.cmds[0].cmd.args[0])) {
                /* Execute built-in command (cd, exit, pwd) */
                execute_builtin(pipe.cmds[0].cmd);
            } else {
                /* 
                 * Execute pipeline with possible redirection and pipes
                 * (PHASES 1, 3, 4, 5)
                 */
                execute_pipeline(pipe);
            }
        }

        /* 
         * FREE: Clean up all allocated memory
         */
        free_pipeline(pipe);
    }

    return EXIT_SUCCESS;
}
