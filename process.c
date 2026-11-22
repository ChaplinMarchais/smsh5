#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<signal.h>
#include	<sys/wait.h>
#include	<string.h>
#include	"varlib.h"
#include	"controlflow.h"
#include	"process.h"


/* process.c
 * command processing layer: handles layers of processing
 * 
 * The process(char **arglist) function is called by the main loop
 * It sits in front of the do_command function which sits 
 * in front of the execute() function.  This layer handles
 * two main classes of processing:
 *	a) process - checks for flow control (if, while, for ...)
 * 	b) do_command - does the command by 
 *		         1. Is command built-in? (exit, set, read, cd, ...)
 *                       2. If not builtin, run the program (fork, exec...)
 *                    - also does variable substitution (should be earlier)
 */


int process(char *args[])
/*
 * purpose: process user command: this level handles flow control
 * returns: result of processing command
 *  errors: arise from subroutines, handled there
 */
{
	int		rv = 0;

	if ( args[0] == NULL )
		rv = 0;		// note: $? is unchanged by empty commands
	else if ( is_control_command(args[0]) )
		rv = do_control_command(args);
	else if ( ok_to_execute() )
		rv = do_command(args);
	return rv;
}

/*
 * do_command
 *   purpose: do a command - either builtin or external
 *   returns: result of the command
 *    errors: returned by the builtin command or from exec,fork,wait
 *      note: this version does variable substitution: not where sh does it
 *
 */
int do_command(char **args)
{
	int  is_builtin(char **, int *);
	int  rv;

	if ( !is_builtin(args, &rv) )		/* if not done by shell	*/
		rv = execute(args, STDOUT_FILENO);		/* fork and exec it	*/
	return rv;				/* return exit status	*/
}

int execute(char *argv[], int output_fd)
/*
 * purpose: run a program passing it arguments
 * returns: status returned via wait, or -1 on error
 *  errors: -1 on fork() or wait() errors
 *    NOTE: this does not return the exit(n) status from child
 *	    Instead it returns the status word wait() receives
 *	    To get the exit(n) status, you must add some code
 */
{
	extern char **environ;		/* note: declared in <unistd.h>	*/
	int	pid ;
	int	child_info = -1;

	if ( argv[0] == NULL )		/* nothing succeeds		*/
		return 0;

	fflush(stdout);			/* flush stdout buffer before fork  */

	if ( (pid = fork())  == -1 )
		perror("fork");
	else if ( pid == 0 ){
		environ = VLtable2environ();
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);

		if(output_fd != STDOUT_FILENO) /* redirect output if needed */
			dup2(output_fd, STDOUT_FILENO);

		execvp(argv[0], argv);
		perror("cannot execute command");
		free(environ);		/* see comment for VLtable2environ	*/
		_exit(1);
	}
	else {
		if ( wait(&child_info) == -1 )
			perror("wait");
	}
	return child_info;
}

/*
 * purpose: execute a command inline and capture its output
 * returns: output of the command as a string, or NULL on error
 *    note: caller is responsible for freeing the returned string
 */
char *execute_inline_command(char **argv)
{
	char output[1024];
	char template[] = "/tmp/smshXXXXXX";

	/* create a temporary file to capture the command output */
	int tmp_fd = mkstemp(template);
	if (tmp_fd == -1) {
		perror("mkstemp");
		return NULL;
	}

	unlink(template); /* remove the temporary file on close*/

	if (argv != NULL)
		execute(argv, tmp_fd); /* execute the command and capture output */

	lseek(tmp_fd, 0, SEEK_SET);
	int n = read(tmp_fd, output, sizeof(output) - 1); /* read command output */
	if (n >= 0)
		output[n] = '\0'; 
	else 
		output[0] = '\0';

	close(tmp_fd); /* close the temporary file */

	for (char *p = output; *p; p++) /* replace newlines with spaces */
		if (*p == '\n')
			*p = ' ';
	
	return strdup(output);
}
