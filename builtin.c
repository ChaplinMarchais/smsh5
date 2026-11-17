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
	if ( is_export(args, resultp) )			/* export X */
		return 1;
	if ( is_cd(args, resultp) )				/* cd PATH */
		return 1;
	if ( is_exit(args) )					/* exit */
		return 1;
	if ( is_read(args, resultp) )			/* read VAR */
		return 1;
	if ( is_comment(args[0]) )				/* # comment */
		return 1;
	return 0;
}

/*
 * checks if command is "read" : if so, read variable
 * returns: 1 if command is "read", 0 otherwise
 */
int is_read(char **args, int *resultp)
{
	if ( strcmp(args[0], "read") == 0 ){
		*resultp = do_read(args, resultp);
		if ( *resultp == 0 )
			return 1;
	}
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
int is_exit(char **args)
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
 * purpose: read a value for a variable from standard input
 * returns: 0 on success, -1 on failure
 */
int do_read(char **args, int *resultp)
{
	/* check for missing variable name */
	if ( args[1] == NULL ){
		fprintf(stderr, "read: missing variable name\n");
		return *resultp = -1;
	}

	char buf[512];
	
	/* read a line from stdin */
	if ( fgets(buf, sizeof(buf), stdin) != NULL ){
		buf[strcspn(buf, "\n")] = '\0';  	/* remove newline */
		*resultp = VLstore(args[1], buf);
		return *resultp;
	}

	return *resultp = -1;
}

/*
 * purpose: check if a command is a comment
 * returns: 1 if it is a comment, 0 otherwise
 */
int is_comment(char *cmd)
{
	return (cmd[0] == '#');
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
 * purpose: substitute variable values for arguments starting with $
 *          and remove comments starting with #
 */
void varsub(char **args)
{
	int	i;
	char	*newstr;

	for ( i = 0 ; args[i] != NULL ; i++ )
		if ( args[i][0] == '$' ){ /* variable substitution */
			newstr = VLlookup( args[i]+1 );
			if ( newstr == NULL )
				newstr = "";
			free(args[i]);
			args[i] = strdup(newstr);
		} else if (i != 0 && args[i][0] == '#') { /* comment*/
			/* free the comment argument and terminate the argument list */
			free(args[i]);
			args[i] = NULL;
			break;
		}
}
