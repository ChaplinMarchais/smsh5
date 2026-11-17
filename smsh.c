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
void	do_smsh(char *prompt, FILE *source);

int main(int ac, char **av)
{
	FILE *source = stdin;
	char *prompt = DFL_PROMPT;
	
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

	do_smsh(prompt, source);

	/* close the source file if it is not stdin */
	if (source != stdin) {
		fclose(source);
	}

	return result;
}

/*
 * purpose: initialize shell
 * returns: nothing. calls fatal() if trouble
 */
void setup()
{
	extern char **environ;

	if ( VLenviron2table(environ) == 0 )
		fatal("Cannot copy environ", "", 2);
	if ( signal(SIGINT,  SIG_IGN) == SIG_ERR 
			|| signal(SIGQUIT, SIG_IGN) == SIG_ERR )
		fatal("Cannot ignore sigint and/or sigquit", "", 2);
}

/**
 * purpose: main loop of the shell
 * returns: nothing
 */
void do_smsh(char *prompt, FILE *source)
{
	char *cmdline, **arglist;

	while ( (cmdline = next_cmd(prompt, source)) != NULL )
	{
		if ( (arglist = splitline(cmdline)) != NULL  )
		{
			result = process(arglist);
			freelist(arglist);
		}
		free(cmdline);
	}
}

void fatal(char *s1, char *s2, int n)
{
	fprintf(stderr,"Error: %s,%s\n", s1, s2);
	exit(n);
}

