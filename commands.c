/*
 * Author: Donato Quartuccia
 * Description: Contains smallsh's built-in commands: exit, cd & status, as well as logic for running other commands
 *              Last Modified 2/8/2022
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "commands.h"
#include "signal_handlers.h"
#include "error_handlers.h"


// exit status of last foreground process
static int exit_status = 0;
static bool by_signal = false;


/**
 * Terminates all processes started by program, then exits the shell
 */
void builtin_exit() {
    set_cleanup_signal_handlers();

    // send SIGTERM to every child process and give SIGCHLD time to do cleanup
    kill(0, SIGTERM);
    usleep(5000);
    exit(0);
}

/**
 * Changes the current working directory of the process to the path reconstructed from argv[], or to $HOME if
 * no args are passed (i.e., if argv[1] is NULL)
 *
 * @param argv the parsed argv[] array (including the command)
 */
void builtin_cd(char** argv) {
    if (chdir(argv[1] == NULL ? getenv("HOME") : argv[1]) == -1) {
        handle_path_error();
    }
}

/**
 * Prints the exit status of the most recently terminated child process.
 */
void builtin_status() {
    printf(
        "Last foreground process status: %s %d\n",
        by_signal ? "terminated by signal" : "exit value",
        exit_status
    );
    fflush(stdout);
}


/**
 * Runs the command specified by argv[0] in either foreground or background mode.\n\n
 *
 * If run in foreground mode, the shell waits for the command to have finished executing before printing its
 * exit status and returning control. Input and output are not redirected unless specified.\n\n
 *
 * If run in background mode, the shell immediately returns terminal control. Input and output are discarded
 * unless specified.
 *
 * @param argv the parsed argv[] array (including the command)
 * @param input_file a string representing the path to the input file (or NULL for default behavior)
 * @param output_file a string representing the path to the output file (or NULL for default behavior)
 * @param in_background true if the command should run in the background, false otherwise
 */
void run_command(char** argv, char* input_file, char* output_file, bool in_background) {
    // determine input stream
    int input_fd = input_file == NULL
        ? (in_background ? open("/dev/null", O_RDONLY) : STDIN_FILENO)
        : open(input_file, O_RDONLY);
    if (input_fd == -1) {
        // we never attempt to open STDIN; it's conceivable that we could run into an fd limit for /dev/null;
        handle_file_error(input_file == NULL ? "/dev/null" : input_file, true);
        exit_status = 1;
        return;
    }

    // determine output stream
    int output_fd = output_file == NULL
        ? (in_background ? open("/dev/null", O_WRONLY) : STDOUT_FILENO)
        : open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (output_fd == -1) {
        // if we opened a file, close it
        if (input_fd != STDIN_FILENO) {
            close(input_fd);
        }
        // we never attempt to open STDOUT
        handle_file_error(output_file == NULL ? "/dev/null" : output_file, false);
        exit_status = 1;
        return;
    }

    // at this point, we can attempt to fork the process
    pid_t pid = fork();
    if (pid == -1) {
        // fork failed
        handle_fork_error();
    } else if (pid == 0) {
        // child process
        set_child_signal_handlers(in_background);

        dup2(input_fd, STDIN_FILENO);
        dup2(output_fd, STDOUT_FILENO);
        execvp(argv[0], argv);

        // if we get here, it means exec failed
        handle_exec_error(argv[0]);
        exit(1);
    } else {
        if (in_background) {
            // don't wait for the process
            printf("Background PID %d\n", pid);
            fflush(stdout);
        } else {
            // wait for the child process to finish and record the exit status
            waitpid(pid, &exit_status, 0);
            if WIFEXITED(exit_status) {
                by_signal = false;
                exit_status = WEXITSTATUS(exit_status);
            } else {
                by_signal = true;
                exit_status = WTERMSIG(exit_status);
                // immediately print the exit status if the child was terminated
                putchar('\n');
                builtin_status();
                fflush(stdout);
            }
        }
    }
}
