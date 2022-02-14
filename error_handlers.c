/*
 * Author: Donato Quartuccia
 * Description: Contains error handlers.
 *              Last Modified: 2/8/2022
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "error_handlers.h"

/**
 * Prints an error message to stderr.
 */
void handle_memory_error() {
    perror("Error. Memory allocation failed.\n");
    fflush(stderr);
    exit(1);
}

/**
 * Prints an error message to stderr
 *
 * @param command a string containing the command that failed to execute
 */
void handle_exec_error(char* command) {
    fprintf(stderr, "Error. Command %s not found. ", command);
    perror("");
    fflush(stderr);
}

/**
 * Prints an error message to stderr.
 */
void handle_fork_error() {
    perror("Error. fork failed");
    fflush(stderr);
}

/**
 * Prints an error message to stderr.
 */
void handle_path_error() {
    fprintf(stderr, "Error. Path not found\n");
    fflush(stderr);
}

/**
 * Prints an error message to stderr
 *
 * @param file_name a string containing the file name
 * @param for_input true if the file was being opened for input, false for output
*/
void handle_file_error(char* file_name, bool for_input) {
    fprintf(stderr, "Error. Could not open file %s ", file_name);
    perror(for_input ? "for input" : "for output");
    fflush(stderr);
}
