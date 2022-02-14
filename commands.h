/*
 * Author: Donato Quartuccia
 * Description: Header for public functions of commands.c
 *              Last Modified: 2/3/2022
 */

#ifndef SMALLSH_COMMANDS_H
#define SMALLSH_COMMANDS_H

#include <stdbool.h>

void builtin_exit();
void builtin_cd(char** argv);
void builtin_status();
void run_command(char** argv, char* input_file, char* output_file, bool in_background);

#endif //SMALLSH_COMMANDS_H
