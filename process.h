#ifndef	PROCESS_H
#define	PROCESS_H

int process(char **args);
int do_command(char **args);
int execute(char **args, int output_fd);
char *execute_inline_command(char **argv);

#endif
