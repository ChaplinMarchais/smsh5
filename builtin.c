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
 * purpose: remove a character at a given position in a string
 * args:
 *   pos - pointer to the character to remove
 */
void remove_char_at(char *pos)
{
	/* shift the rest of the string left by one character */
	memmove(pos, pos + 1, strlen(pos));
}

/*
 * purpose: check if a variable is a special variable ($$ or $?) if a special
			var is found replace it with its value
 * args:
 *   arg - the argument string containing the variable
 *   cp - pointer to the '$' character in the argument string
 * returns: 1 if it is a special variable, 0 otherwise
 */
int is_special_var(char *arg, char *cp)
{
	char sp[32];

	switch (cp[1]) { /* check the character after $ */
		case '$':
			pid_t pid = getpid();
			snprintf(sp, 32, "%d", pid);
			splice_str(&arg, sp, cp, 2);
			return 1;
		case '?':
			snprintf(sp, 32, "%d", result);
			splice_str(&arg, sp, cp, 2);
			return 1;
		default:
			return 0;
	}
}

/*
 * purpose: process an argument to identify variable references and their 
 *          positions
 * args:
 *   arg - the argument string to process
 *   varNameStart - pointer to the start of the variable name (output)
 *   is_var - flag indicating if a variable was found (output)
 *   var_length - length of the variable name (output)
 */
void process_argument(char *arg, char **varNameStart, int *is_var, 
						int *var_length)
{
	char *cp;

	for (cp = arg; *cp; cp++)
	{
		if (*cp == '\\') {   /* handle escaped character */
			remove_char_at(cp);
			continue;
		} else if (*cp == '$') { /* var has been found */
			if (is_special_var(arg, cp)) /* $$ and $? */
			{
				cp++;
				continue; /* skip special variable */
			}
			*is_var = 1;
			*varNameStart = cp + 1; /* start of variable name */
		} else if (*is_var) {					/* processing variable name */
			if (isalnum(*cp) || *cp == '_') {
				(*var_length)++;
			} else 
				break; /* exit the loop after replacing the variable */
		}
	}
}

/* 
 * purpose: substitute variable values for arguments starting with $
 *          and remove comments starting with #
 */
void varsub(char **args)
{
	int	i;

	for ( i = 0 ; args[i] != NULL ; i++ )
	{
		if (i != 0 && args[i][0] == '#') { /* handle comments */
			/* free the comment argument and terminate the argument list */
			free(args[i]);
			args[i] = NULL;
			break;
		} else {
			char *varNameStart = NULL;
			int is_var = 0, var_length = 0;

			/* process the argument to handle variable substitution and 
		       escaped chars */
			process_argument(args[i], &varNameStart, &is_var, &var_length);

			if (is_var) {
				insert_var(&args[i], varNameStart, var_length);
			}
		}
	}
}

/*
 * purpose: insert the value of a variable into a target string
 * args:
 *   target - the original string containing the variable
 *   varNameStart - pointer to the start of the variable name
 *   var_length - length of the variable name
 */
void insert_var(char **target, char *varNameStart, int var_length)
{
	char *newstr;

	/* extract the variable name */
	char varName[var_length + 1];
	strncpy(varName, varNameStart, var_length);
	varName[var_length] = '\0';

	/* look up the value of the variable */
	newstr = VLlookup(varName);
	if (newstr == NULL)
		newstr = "";

	/* splice the variable value into the target string account for the $ sign*/
	splice_str(target, newstr, varNameStart - 1, var_length + 1);
}

/*
 * purpose: splice a string into the target at the specified position
 * args:
 *   target - the original string
 *   strToInsert - the string to insert
 *   splicePos - the position in the target to splice at
 *   spliceLength - the length of the portion to replace
 */
void splice_str(char **target, char *strToInsert, char *splicePos, 
				int spliceLength)
{
	char *varNameEnd = splicePos + spliceLength;
	/* replace the variable in the original string */
	*splicePos = '\0';
	char *tmp = malloc(strlen(*target) + strlen(strToInsert) 
						+ strlen(varNameEnd) + 1);
	sprintf(tmp, "%s%s%s", *target, strToInsert, varNameEnd);

	/* copy the new string back to the target */
	free(*target);
	*target = strdup(tmp);
	free(tmp);
}
