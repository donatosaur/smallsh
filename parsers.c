/*
 * Author: Donato Quartuccia
 * Description: Contains functions that parse user input into a command struct.
 *              Last Modified: 2/8/2022
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "config.h"
#include "error_handlers.h"
#include "parsers.h"


/** ------------------------------------------ command struct definitions ----------------------------------------- */

/**
 * Creates a command struct with the following initial values:
 *  command: NULL;
 *  args: NULL;
 *  background: false;
 *  i_stream: NULL;
 *  o_stream: NULL;
 *
 * @return a pointer to the command struct on success, or NULL on failure
 */
struct command *create_command_struct() {
    struct command *created_command = malloc(sizeof(struct command));
    if (created_command == NULL) {
        return NULL;
    }
    created_command->argv = NULL;
    created_command->background = false;
    created_command->i_stream = NULL;
    created_command->o_stream = NULL;
    return created_command;
}

void delete_command_struct(struct command *command_struct) {
    // free all the arg strings in argv[]
    int i = 0;
    while (command_struct->argv[i] != NULL) {
        free(command_struct->argv[i]);
        i++;
    }
    // free everything else
    free(command_struct->argv);
    free(command_struct->i_stream);
    free(command_struct->o_stream);
    free(command_struct);
}

/**
 * Prints a command struct to stdout. For debugging only.
 *
 * @param parsed_command the command struct to print
 */
void print_command_struct(struct command *parsed_command) {
    int i = 0;
    printf("| ");
    fflush(stdout);
    while (parsed_command->argv[i] != NULL) {
        printf("%s ", parsed_command->argv[i]);
        i++;
    }
    fflush(stdout);
    printf("BG: %d, ", parsed_command->background);
    fflush(stdout);
    printf("I: %s, O: %s\n", parsed_command->i_stream, parsed_command->o_stream);
    fflush(stdout);

}


/** ----------------------------------------------- command parser ------------------------------------------------ */

/**
 * Trims whitespace from the end of the input string and returns the new length of the string.
 *
 * @param input_string pointer to the string to be trimmed
 * @param len the length of the string
 * @return the new length of the string
 */
int truncate_right(char* input_string, int len) {
    while (len > 0 && (input_string[len - 1] == ' ' || input_string[len - 1] == '\n')) {
        input_string[len - 1] = 0;  // truncate
        len--;
    }
    return len;
}


/**
 * Parses the passed string into an argv[] array of char pointers. Assumes the args are space-delimited.
 *
 * @param command_struct the struct whose argv property is to be populated
 * @param argv_string a space-delimited string of args
 */
void parse_args(struct command *command_struct, char* argv_string) {
    // allocate space for the argv[] array, which needs to be terminated by a null pointer for use with exec()
    command_struct->argv = calloc(MAX_INPUT_CHARS_SIZE, sizeof(char*));
    if (command_struct->argv == NULL) {
        handle_memory_error();
    }

    char *DELIMITER = " ";
    char *arg_token;
    char *save_ptr;

    // get the first token (argv[0] = command) and continue parsing until there are no more args
    int i = 0;
    arg_token = strtok_r(argv_string, DELIMITER, &save_ptr);
    do {
        // copy the arg
        command_struct->argv[i] = calloc(strlen(arg_token) + 1, sizeof(char));
        if (command_struct->argv[i] == NULL) {
            handle_memory_error();
        }
        strcpy(command_struct->argv[i], arg_token);

        // parse the next arg
        arg_token = strtok_r(NULL, DELIMITER, &save_ptr);
        i++;
    } while (arg_token != NULL && i < MAX_ARGV_SIZE);

    // ensure the argv array is null-terminated and place it in the command struct
    command_struct->argv[i] = NULL;
}



/**
 * Processes and expands the input string, overwriting the output string buffer with the new contents as follows:
 *   - Any instances `$$` are replaced by this program's process id
 *   - Any instance of multiple spaces are replaced by exactly one space
 *   - Any *leading* spaces are trimmed
 *
 * @param input_string pointer to the string to be parsed
 * @param output_string pointer to a buffer to be overwritten with the new string
 * @return the new length of the input string (after expansion)
 */
int expand(const char* input_string, char* output_string) {
    // get the process id as a string
    char pid_as_str[12];  // signed int + null term
    int pid_len = sprintf(pid_as_str, "%d", getpid());

    int i = 0;  // input_string
    int j = 0;  // output_string

    // copy space-separated words
    while (input_string[i] != 0) {
        // skip whitespace (this will also skip any leading whitespace)
        while (input_string[i] == ' ') {
            i++;
        }

        // copy one char at a time until we reach '$', whitespace, or the end of the string
        while (input_string[i] != ' ' && input_string[i] != '$' && input_string[i] != 0) {
            output_string[j] = input_string[i];
            i++;
            j++;
        }

        // replace '$$' if we've found it; otherwise copy either the '$' or ' '
        if (input_string[i] == '$' && input_string[i + 1] == '$') {
            strncpy(&output_string[j], pid_as_str, pid_len);
            j += pid_len;  // account for the characters we wrote
            i += 2;        // skip second '$'
        } else if (input_string[i] == '$') {
            output_string[j] = input_string[i];
            i++;
            j++;
        }

        // we may reach whitespace either directly (after we exit the first loop that copies chars) or indirectly
        // if it comes directly after '$$' (since we incremented i)
        if (input_string[i] == ' ') {
            output_string[j] = ' ';
            i++;
            j++;
        }
    }

    // write the null terminator
    output_string[j] = 0;

    return j;  // we wrote j chars to the output string
}


/**
 * Gets input from the specified stream and parses it to a command. Assumes the input has the following
 * format: (#|command) [arg1 arg2 ...] [(>|<) file] [(>|<) file] [&]. Any instances of `$$` are expanded
 * to the program's process id. The input string is mutated in the process.
 *
 * The calling function is responsible for freeing memory allocated for the returned command struct.
 *
 * @param command_string pointer to the string to be parsed
 * @return pointer to a command struct representing the command on success or NULL on failure
 */
struct command *parse_command(char* input_string) {
    // expand any instances of '$$' to pid, remove duplicate whitespace, and get the new length of the string
    char command_string[MAX_INPUT_CHARS_SIZE];

    int len = expand(input_string, command_string);  // expand '$$' and trim left
    len = truncate_right(command_string, len);        // trim right

    // check whether anything was entered aside from whitespace
    if (len == 0) {
        return NULL;
    }

    /**
     * Parse the input string. At this point, the string has at least one meaningful character in it and we have
     * already expanded any instances of '$$'. The argv portion of the command string is its most variable, but
     * we know it'll end either before one of the other operators ('>', '<' or '&') or at the end of the input
     * string. We also have already checked whether only whitespace was entered. So, we can proceed in this order:
     *   1. Parse from the left and check for a leading '#' or the beginning of the command
     *   2. Parse from the right to check for the '&' operator
     *   3. Parse argv (The command and any args), stopping at '>', '<' or the end of the string
     *   4. Parse i/o redirection ('>' and/or '<' in any order)
     */
    int left = 0;
    int right = len - 1;


    // (1) check whether the input is a comment (the string was already left-trimmed)
    if (command_string[left] == '#') {
        return NULL;
    }

    // create the command struct; we have at least one meaningful char
    struct command *parsed_command = create_command_struct();
    if (parsed_command == NULL) {
        handle_memory_error();
    }

    // (2) check whether this should be run as a background task; if '&' is present then it must occur at the end
    //     however if it's the only thing that appears in the input string then we'll need to treat it as part of
    //     the argv string (i.e. as a command), otherwise we risk breaking anything that relies on the assumption
    //     that there is at least one entry in the command string
    if (command_string[right] == '&' && right > left + 1 && command_string[right - 1] == ' ') {
        parsed_command->background = true;
        right--;                    // move to whitespace
        command_string[right] = 0;  // truncate so we don't parse '&' again
        right--;                    // move over the null char
    }

    // (3) find argv (parsing and stopping if we reach an i/o redirection operator)
    int argv_right = left;
    while ( argv_right < right
            && strncmp(&command_string[argv_right], " > ", 3) != 0
            && strncmp(&command_string[argv_right], " < ", 3) != 0
          ) {
        argv_right++;
    }
    // tokenize (truncate) the argv string if we found either '>' or '<'
    if (command_string[argv_right] == ' ') {
        command_string[argv_right] = 0;
        argv_right++;
    }

    parse_args(parsed_command, &command_string[left]);
    left = argv_right;  // move to the start of the potential i/o operator string

    // (4) check for the i/o redirect operators in any order; at this point, if left == right, then we didn't find
    //     either of the operator, so only proceed if left < right; using a while loop here will allow this to work
    //     even for cases where two of the same stream are passed (defaulting to the rightmost one)
    while (left < right) {
        bool redirect_input = command_string[left] == '<' ? true : false;

        // skip initial whitespace
        left ++;
        while (command_string[left] == ' ' && left < right) {
            left++;
        }
        // find the end of the filename string
        int filename_end = left;
        while ( filename_end < right
                && strncmp(&command_string[filename_end], " > ", 3) != 0
                && strncmp(&command_string[filename_end], " < ", 3) != 0
              ) {
            filename_end++;
        }
        // make sure there's actually something to parse aside from whitespace (for cases like " >  < filename") and
        // trim any trailing whitespace if it exists; the filename is the input string from left to filename_end
        if (truncate_right(&command_string[left], filename_end - left + 1) > 0) {
            if (redirect_input) {
                parsed_command->i_stream = calloc(filename_end - left + 1, sizeof(char));
                strcpy(parsed_command->i_stream, &command_string[left]);
            } else {
                parsed_command->o_stream = calloc(filename_end - left + 1, sizeof(char));
                strcpy(parsed_command->o_stream, &command_string[left]);
            }
        }
        left = filename_end + 1;  // check for any other operators
    }

    return parsed_command;
}
