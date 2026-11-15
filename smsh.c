#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<signal.h>
#include	<sys/wait.h>
#include	"smsh.h"
#include	"splitline2.h"
#include	"varlib.h"
#include	"process.h"

/**
 **	small-shell version 5
 **		first really useful version after prompting shell
 **		this one parses the command line into strings
 **		uses fork, exec, wait, and ignores signals
 **
 **     hist: 2017-04-12: changed comment to say "version 5"
 **/

#define	DFL_PROMPT	"> "

void	setup();

int main()
{
	char	*cmdline, *prompt, **arglist;
	int	result;

	prompt = DFL_PROMPT ;
	setup();

	while ( (cmdline = next_cmd(prompt, stdin)) != NULL )
	{
		if ( (arglist = splitline(cmdline)) != NULL  )
		{
			result = process(arglist);
			freelist(arglist);
		}
		free(cmdline);
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
