#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<signal.h>
#include	<sys/wait.h>
#include	<string.h>
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
void	setup_env(int ac, char **av, FILE **source, char **prompt);
void	do_smsh(char *prompt, FILE *source);

int main(int ac, char **av)
{
	FILE *source = stdin;
	char *prompt = DFL_PROMPT;
	
	setup();

	setup_env(ac, av, &source, &prompt);

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

/**
 * purpose: set up the environment for a script file by setting the source
 *          and storing script variables if provided
 * returns: nothing. exits if the script file cannot be opened 
 *          or if script var cannot be stored
 */
void setup_script_env(int ac, char **av, FILE **source, char **prompt)
{
	*prompt = "";
	*source = fopen(av[1], "r");
	if (*source == NULL) {
		fprintf(stderr, "Error: Cannot open script file: %s\n", av[1]);
		exit(1);
	}

	/* store script variables if provided */
	if (ac > 2) {
		char buf[3];
		for (int i = 2; i < ac; i++) {
			if(i > 99)
			{
				fprintf(stderr, "Error: Too many script variables, max 99\n");
				exit(1);
			}
			/* script variable index starts from 1 */
			snprintf(buf, sizeof(buf), "%d", (i-1) % 99);

			if (VLstoreScriptVar(buf, av[i]) != 0) {
				fprintf(stderr, 
					"Error: Cannot store script variable: %s\n", av[i]);
				exit(1);
			}
		}
	}
}

/**
 * purpose: set up the environment for the shell
 *          if a script file is provided, open it and set the prompt to empty
 * returns: nothing. exits if the script file cannot be opened
 */
void setup_env(int ac, char **av, FILE **source, char **prompt) 
{
	/* check if arguments are supplied */
	if (ac > 1) 
	{
		if(strcmp(av[1], "-c") == 0 ) 			/* handle the -c option */
		{
			FILE *fp = tmpfile(); /* create a temporary file for command */
			if (fp == NULL) {
				fprintf(stderr, "Error: Cannot create temporary file\n");
				exit(1);
			}

			/* write the command arguments to the temporary file */
			for(int i = 2; i < ac; i++)
			{
				fwrite(av[i], 1, strlen(av[i]), fp);
				fwrite(" ", 1, 1, fp);
			}

			/* write a newline to the temporary file and rewind it*/
			fwrite("\n", 1, 1, fp);
			rewind(fp);

			*source = fp;
		} else { 		/* handle the case where a script file is provided */
			setup_script_env(ac, av, source, prompt);
		}
	}
}
