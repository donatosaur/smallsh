/*
 * Author: Donato Quartuccia
 * Description: A small linux shell with support for running executables from the working directory or PATH, i/o
 *              redirection, variable expansion of '$$' into the shell's pid, and management of foreground and
 *              background processes. Works with space-delimited input strings with the following format:
 *                (#|command) [arg1 arg2 ...] [(>|<) file] [(>|<) file] [&]
 *
 *              The following built-in commands and signals are supported:
 *                * cd      changes the directory (to the shell's location by default)
 *                * status  prints the exit status of the most recent foreground process
 *                * exit    terminates any child processes and exits the shell
 *                * ^C      immediately terminates any foreground processes being run by the shell
 *                * ^Z      toggles foreground only mode
 *
 *              Last Modified: 2/8/2022
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "config.h"
#include "parsers.h"
#include "commands.h"
#include "signal_handlers.h"


/**
 * Set signal handlers and create the process in a new session (if it's not already the session leader)
 */
void setup() {
    set_initial_signal_handlers();
    setsid();
}

/**
 * Handles shell control flow.
 */
int main() {
    setup();

    //
    char input_buffer[MAX_INPUT_CHARS_SIZE];
    bool exit_triggered = false;
    sigset_t sigchld_set;
    sigemptyset(&sigchld_set);


    sigaddset(&sigchld_set, SIGCHLD);

    while (!exit_triggered) {
        // block sigchld until we
        sigprocmask(SIG_BLOCK, &sigchld_set, NULL);

        // prompt for input and parse; if the input is a comment or blank line, the result of the parse will be NULL
        printf(": ");
        fflush(stdout);
        if (fgets(input_buffer, MAX_INPUT_CHARS_SIZE, stdin) == NULL) {
            clearerr(stdin);
        }
        struct command *parsed_command = parse_command(input_buffer);

        if (parsed_command != NULL) {
            // check whether to exit
            if (strcmp(parsed_command->argv[0], "exit") == 0) {
                exit_triggered = true;
            // check whether to call a built-in
            } else if (strcmp(parsed_command->argv[0], "cd") == 0) {
                builtin_cd(parsed_command->argv);
            } else if (strcmp(parsed_command->argv[0], "status") == 0) {
                builtin_status();
            // otherwise check whether we should run the command in the foreground or background
            } else {
                run_command(
                  parsed_command->argv,
                  parsed_command->i_stream,
                  parsed_command->o_stream,
                  parsed_command->background && get_foreground_flag() != 1
                );
            }

            delete_command_struct(parsed_command);
        }

        // clean up zombie processes and wait a few milliseconds so that any immediately-terminating processes
        // output a message right away (e.g. for something like "sleep not_an_int &"; 5ms should be enough
        sigprocmask(SIG_UNBLOCK, &sigchld_set, NULL);
        usleep(5000);
    }

    builtin_exit();
    return 0;
}
