#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<signal.h>
#include	<sys/wait.h>
#include	"smsh.h"
#include	"splitline2.h"
#include	"varlib.h"
#include	"process.h"

#define	DFL_PROMPT	"> "

/**
 * global variable to store the result of the last command
 */
int result = 0;

void	setup();

int main(int ac, char **av)
{
	FILE *source = stdin;
	char	*cmdline, *prompt, **arglist;

	prompt = DFL_PROMPT;
	setup();

	/* check if a script file is provided as an argument */
	if (ac > 1) {
		prompt = "";
		source = fopen(av[1], "r");
		if (!source) {
			perror(av[1]);
			exit(1);
		}
	}

	while ( (cmdline = next_cmd(prompt, source)) != NULL )
	{
		if ( (arglist = splitline(cmdline)) != NULL  )
		{
			result = process(arglist);
			freelist(arglist);
		}
		free(cmdline);
	}

	/* close the source file if it is not stdin */
	if (source != stdin) {
		fclose(source);
	}

	return result;
}

void setup()
/*
 * purpose: initialize shell
 * returns: nothing. calls fatal() if trouble
 */
{
	extern char **environ;

	if ( VLenviron2table(environ) == 0 )
		fatal("Cannot copy environ", "", 2);
	if ( signal(SIGINT,  SIG_IGN) == SIG_ERR 
			|| signal(SIGQUIT, SIG_IGN) == SIG_ERR )
		fatal("Cannot ignore sigint and/or sigquit", "", 2);
}

void fatal(char *s1, char *s2, int n)
{
	fprintf(stderr,"Error: %s,%s\n", s1, s2);
	exit(n);
}