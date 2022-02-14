/*
 * Author: Donato Quartuccia
 * Description: Header for public functions of error_handlers.c
 *              Last Modified: 2/4/2022
 */

#ifndef SMALLSH_ERROR_HANDLERS_H
#define SMALLSH_ERROR_HANDLERS_H

#include <stdbool.h>

void handle_memory_error();
void handle_fork_error();
void handle_exec_error(char* command);
void handle_path_error();
void handle_file_error(char* file_name, bool for_input);

#endif //SMALLSH_ERROR_HANDLERS_H
