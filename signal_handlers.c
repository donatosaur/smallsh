/*
 * Author: Donato Quartuccia
 * Description: Contains signal handlers and setters.
 *              Last Modified: 2/8/2022
 */

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <errno.h>
#include "signal_handlers.h"


// these prototypes need to be declared so we can swap between handlers
void SIGTSTP_handler_normal_mode(int signal_number);
void SIGTSTP_handler_foreground_only_mode(int signal_number);

/** ---------------------------------------------------- Flags ----------------------------------------------------- */

static volatile sig_atomic_t foreground_flag = 0;   // set if foreground only mode is active

/**
 * Returns the value of foreground_flag.
 */
sig_atomic_t get_foreground_flag() {
    return foreground_flag;
}

/** --------------------------------------------------- SIGCHLD ---------------------------------------------------- */

/**
 * Helper function for SIGCHLD_handler. Prints an integer to stdout using only async-safe write calls.
 *
 * @param value the integer to be printed
 */
void write_int(int value) {
    char buffer[10];  // we'll be printing these one at a time, so the null terminator is unnecessary; digits only
    int i = 9;

    // handle edge case (we still want to print zero)
    if (value == 0) {
        buffer[0] = '0';
        write(STDOUT_FILENO, buffer, 1);
        return;
    }

    // print sign and flip the value; it *seems* C truncates toward zero, but this is here for safety
    if (value < 0) {
        buffer[0] = '-';
        write(STDOUT_FILENO, buffer, 1);
        value = -value;
    }

    // get digits (right-to-left) and write them in reverse order
    while (value != 0) {
        buffer[i] = (char) ('0' + value % 10);
        value /= 10;
        i--;
    }
    i++; // reverse last decrement so we can start our buffer at the correct location
    write(STDOUT_FILENO, &buffer[i], 10 - i);  // we wrote 10 - i chars
}

/**
 * Handler for SIGCHLD.
 */
void SIGCHLD_handler(__attribute__((unused)) int signal_number) {
    int save_err = errno;

    char* output_1 = "Background PID ";        // 15 chars
    char* output_2 = " is done: ";             // 10 chars
    char* exited = "exit value ";              // 11 chars
    char* signaled = "terminated by signal ";  // 21 chars
    char* newline = "\n";                      // 1 char
    int child_pid;
    int child_exit_status;

    // wait for all terminating children
    child_pid = waitpid(-1, &child_exit_status, WNOHANG);
    while (child_pid > 0) {
        // write the background PID of the terminating process
        write(STDOUT_FILENO, output_1, 15);
        write_int(child_pid);
        write(STDOUT_FILENO, output_2, 10);

        // write the exit value or signal number of the terminating process
        if (WIFEXITED(child_exit_status)) {
            // terminated normally
            write(STDOUT_FILENO, exited, 11);
            write_int(WEXITSTATUS(child_exit_status));
        } else if (WIFSIGNALED(child_exit_status)) {
            // terminated by a signal; WIFSIGNALED & WIFEXTIED are mutually exclusive, but there
            // are other possible states here (continued or stopped), so an else if seems safer
            write(STDOUT_FILENO, signaled, 21);
            write_int(WTERMSIG(child_exit_status));
        }
        write(STDOUT_FILENO, newline, 1);

        // keep going until we're out of terminating child processes
        child_pid = waitpid(-1, &child_exit_status, WNOHANG);
    }

    errno = save_err;
}

/**
 * Handler for SIGCHLD with *no* output
 */
void SIGCHLD_handler_no_output(__attribute__((unused)) int signal_number) {
    int save_err = errno;

    int child_pid = waitpid(-1, NULL, WNOHANG);
    while (child_pid > 0) {
        child_pid = waitpid(-1, NULL, WNOHANG);
    }

    errno = save_err;
}


/** --------------------------------------------------- SIGTSTP ---------------------------------------------------- */

/**
 * Creates and registers a signal handler for SIGTSTP using only async-safe calls.
 *
 * @param mode 0 for foreground mode off\n
 *             1 for foreground mode on
 */
void set_SIGTSTP_handler(int mode) {
    struct sigaction SIGTSTP_action = {0};
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    SIGTSTP_action.sa_handler = mode == 0
        ? SIGTSTP_handler_normal_mode
        : SIGTSTP_handler_foreground_only_mode;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}

/**
 * Initial handler for SIGTSTP
 */
void SIGTSTP_handler_normal_mode(__attribute__((unused)) int signal_number) {
    int save_err = errno;

    char* enter_message = "\nEntering foreground-only mode (& is now ignored)\n: ";  // 50 + 2 '\n' = 52 chars
    write(STDOUT_FILENO, enter_message, 52);

    // enter foreground only mode
    foreground_flag = 1;
    set_SIGTSTP_handler(1);

    errno = save_err;
}

/**
 * Foreground-only-mode handler for SIGTSTP (so we print the correct message)
 */
void SIGTSTP_handler_foreground_only_mode(__attribute__((unused)) int signal_number) {
    int save_err = errno;

    char* exit_message = "\nExiting foreground-only mode\n: ";  // 30 + 2 '\n' = 32 chars
    write(STDOUT_FILENO, exit_message, 32);

    // exit foreground only mode
    foreground_flag = 0;
    set_SIGTSTP_handler(0);

    errno = save_err;
}


/** --------------------------------------------------- Setters ---------------------------------------------------- */

/**
 * Sets baseline signal handlers for the shell:
 *   - SIGCHLD: performs cleanup for terminated background processes
 *   - SIGTSTP: toggles foreground only mode
 *   - SIGINT: ignored
 */
void set_initial_signal_handlers() {
    set_SIGTSTP_handler(0);

    // set SIGCHLD to write output about terminating background processes
    struct sigaction SIGCHLD_action = {0};
    SIGCHLD_action.sa_flags = SA_RESTART;
    sigfillset(&SIGCHLD_action.sa_mask);
    SIGCHLD_action.sa_handler = SIGCHLD_handler;
    sigaction(SIGCHLD, &SIGCHLD_action, NULL);

    // ignore SIGINT
    struct sigaction SIGINT_action = {0};
    SIGINT_action.sa_flags = SA_RESTART;
    SIGINT_action.sa_handler = SIG_IGN;
    sigaction(SIGINT, &SIGINT_action, NULL);
}

/**
 * Registers signal handlers such that they are in the appropriate state in a forked process (before the use of exec):
 *   - SIGTSTP: ignored
 *   - SIGINT: ignored if background_mode is true, otherwise restored to default
 *
 * @param background_mode true to set signal handlers for background processes (false for foreground processes)
 */
void set_child_signal_handlers(bool background_mode) {
    // child processes should ignore SIGTSTP since we're using it to toggle a flag
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_flags = SA_RESTART;
    SIGTSTP_action.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    // only background processes should ignore SIGINT
    struct sigaction SIGINT_action = {0};
    SIGINT_action.sa_flags = SA_RESTART;
    SIGINT_action.sa_handler = background_mode ? SIG_IGN : SIG_DFL;
    sigaction(SIGINT, &SIGINT_action, NULL);

    struct sigaction SIGCHLD_action = {0};
    SIGCHLD_action.sa_flags = SA_RESTART;
    SIGCHLD_action.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &SIGCHLD_action, NULL);
}

/**
 * Sets signal handlers for cleanup mode:
 *   - SIGCHLD: performs cleanup for terminated background processes **with output suppressed**
 *   - SIGTERM: ignored
 */
void set_cleanup_signal_handlers() {
    struct sigaction SIGCHLD_action = {0};
    SIGCHLD_action.sa_flags = SA_RESTART;
    sigfillset(&SIGCHLD_action.sa_mask);
    SIGCHLD_action.sa_handler = SIGCHLD_handler_no_output;
    sigaction(SIGCHLD, &SIGCHLD_action, NULL);

    struct sigaction SIGTERM_action = {0};
    sigfillset(&SIGTERM_action.sa_mask);
    SIGTERM_action.sa_handler = SIG_IGN;
    sigaction(SIGTERM, &SIGTERM_action, NULL);
}
