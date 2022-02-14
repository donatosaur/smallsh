# smallsh
### Description
A small unix shell that runs on linux

### Disclaimer
This shell is for demonstration only. It is not intended to be used in production and must be modified to take
security considerations in mind before it is used.

### Build Instructions
#### Using `gcc`:
`gcc --std=gnu99 -o smallsh main.c parsers.c error_handlers.c signal_handlers.c commands.c`
