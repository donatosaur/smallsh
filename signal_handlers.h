/*
 * Author: Donato Quartuccia
 * Description: Header for public functions of signal_handlers.c
 *              Last Modified: 2/3/2022
 */

#ifndef SMALLSH_SIGNAL_HANDLERS_H
#define SMALLSH_SIGNAL_HANDLERS_H

#include <stdbool.h>
#include <signal.h>

void set_initial_signal_handlers();
void set_child_signal_handlers(bool background_mode);
void set_cleanup_signal_handlers();
sig_atomic_t get_foreground_flag();


#endif //SMALLSH_SIGNAL_HANDLERS_H
