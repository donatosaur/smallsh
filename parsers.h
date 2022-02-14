/*
 * Author: Donato Quartuccia
 * Description: Header for public functions of parsers.c
 *              Last Modified: 1/29/2022
 */

#ifndef SMALLSH_PARSERS_H
#define SMALLSH_PARSERS_H

/**
 * A struct that holds command info. Should be initialized with create_command_struct()
 *
 * @property argv: pointer to an array of strings; argv[0] = command
 * @property background: true if the command should be run as a background task, false otherwise
 * @property i_stream: string containing the input stream; NULL => unset
 * @property o_stream: string containing the output stream name; NULL => unset
 */
struct command {
    char **argv;
    bool background;
    char *i_stream;
    char *o_stream;
};

struct command *parse_command(char* command_string);
void delete_command_struct(struct command *command_struct);
void print_command_struct(struct command *command_struct);


#endif //SMALLSH_PARSERS_H
