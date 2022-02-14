/*
 * Author: Donato Quartuccia
 * Description: Contains program-wide macro definitions.
 *              Last Modified: 2/8/2022
 */

#ifndef SMALLSH_CONFIG_H
#define SMALLSH_CONFIG_H

#ifndef MAX_INPUT_CHARS_SIZE
#define MAX_INPUT_CHARS_SIZE 2050  // 2048 chars + '\n' + null term
#endif //MAX_INPUT_CHARS_SIZE

#ifndef MAX_ARGV_SIZE
#define MAX_ARGV_SIZE 514          // 1 command + 512 args + null pointer
#endif //MAX_ARGV_SIZE

#endif //SMALLSH_CONFIG_H
