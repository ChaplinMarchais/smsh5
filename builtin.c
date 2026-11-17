/* builtin.c
 * contains the switch and the functions for builtin commands
 */

#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	"smsh.h"
#include	"varlib.h"
#include	"builtin.h"

int is_builtin(char **args, int *resultp)
/*
 * purpose: run a builtin command 
 * returns: 1 if args[0] is builtin, 0 if not
 * details: test args[0] against all known builtins.  Call functions
 */
{
	if ( is_assign_var(args[0], resultp) )	/* X=123 ?  */
		return 1;
	if ( is_list_vars(args[0], resultp) )	/* set   ?  */
		return 1;
	if ( is_export(args, resultp) )		/* export X */
		return 1;
	if ( is_cd(args, resultp) )				/* cd PATH */
		return 1;
	if ( is_exit(args, resultp) )			/* exit */
		return 1;
	return 0;
}

/* checks if a legal assignment cmd
 * if so, does it and returns 1
 * else return 0
 */
int is_assign_var(char *cmd, int *resultp)
{
	if ( strchr(cmd, '=') != NULL ){
		*resultp = assign(cmd);
		if ( *resultp == 0 )
			return 1;
	}
	return 0;
}

/* checks if command is "set" : if so list vars */
int is_list_vars(char *cmd, int *resultp)
{
	if ( strcmp(cmd,"set") == 0 ){	     /* 'set' command? */
		VLlist();
		fflush(stdout);		/* for case buffered stdout, eg pipe */
		*resultp = 0;
		return 1;
	}
	return 0;
}

/*
 * checks if command is "cd" : if so, change directory
 */
int is_cd(char **args, int *resultp)
{
	if ( strcmp(args[0], "cd") == 0 ){
		*resultp = cd(args);
		return 1;
	}
	return 0;
}

/*
 * checks if command is "exit" : if so, exit
 */
int is_exit(char **args, int *resultp)
{
	if ( strcmp(args[0], "exit") == 0 ){
		do_exit(args);
		return 1;
	}
	return 0;
}

/*
 * if an export command, then export it and ret 1
 * else ret 0
 * note: the opengroup says
 *  "When no arguments are given, the results are unspecified."
 */
int is_export(char **args, int *resultp)
{
	if ( strcmp(args[0], "export") == 0 ){
		if ( args[1] != NULL && okname(args[1]) )
			*resultp = VLexport(args[1]);
		else
			*resultp = 1;
		return 1;
	}
	return 0;
}

/*
 * purpose: execute name=val AND ensure that name is legal
 * returns: -1 for illegal lval, or result of VLstore 
 * warning: modifies the string, but retores it to normal
 */
int assign(char *str)

{
	char	*cp;
	int	rv ;

	cp = strchr(str,'=');
	*cp = '\0';
	rv = ( okname(str) ? VLstore(str,cp+1) : -1 );
	*cp = '=';
	return rv;
}

/*
 * purpose: change the current working directory
 * returns: result of chdir()
 */
int cd(char **args)
{
	if ( args[1] == NULL ) /* no argument */
		return chdir(getenv("HOME"));
	if ( args[2] ) /* too many arguments */
	{
		fprintf(stderr, "cd: too many arguments\n");
		return -1;	
	}

	return chdir(args[1]);
}

/*
 * purpose: exit the shell with the given status or the last result
 */
void do_exit(char **args)
{
	if ( args[1] != NULL ){
		exit(atoi(args[1]));
	}

	exit(result);
}

/*
 * purpose: determines if a string is a legal variable name
 * returns: 0 for no, 1 for yes
 * rule: valid is any alpha or _ followed by any string of alnum or _
 *   eg: _xy234  or x__3  z
 */
int okname(char *str)
{
	char	*cp;

	for ( cp = str; *cp; cp++ ){             // !  modifies an || cond
		if ( (isdigit(*cp) && cp==str) || !(isalnum(*cp) || *cp=='_' ))
			return 0;
	}
	return ( cp != str );	/* no empty strings, either */
}

/*
 * step through args.  REPLACE any arg with leading $ with the
 * value for that variable or "" if no match.
 * note: this is *** NOT *** how regular sh works
 */
void varsub(char **args)
{
	int	i;
	char	*newstr;

	for ( i = 0 ; args[i] != NULL ; i++ )
		if ( args[i][0] == '$' ){
			newstr = VLlookup( args[i]+1 );
			if ( newstr == NULL )
				newstr = "";
			free(args[i]);
			args[i] = strdup(newstr);
		}
}
