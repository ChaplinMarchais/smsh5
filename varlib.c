/* varlib.c
 *
 * a simple storage system to store name=value pairs
 * with facility to mark items as part of the environment
 *
 * interface:
 *     VLstore( name, value )    returns 0 for 0k, 1 for no
 *     VLlookup( name )          returns string or NULL if not there
 *     VLlist()			 prints out current table
 *
 * environment-related functions
 *     VLexport( name )		 adds name to list of env vars
 *     VLtable2environ()	 copy from table to environ
 *     VLenviron2table()         copy from environ to table
 *
 * details:
 *	the table is stored as an array of structs that
 *	contain a flag for `is_env' and a single string of
 *	the form name=value.  This allows easy addition to the
 *	environment.  It makes searching pretty easy, as
 *	long as you search for "name=" 
 *
 * hist: 2023-04-23 BW fixed new_string-to handle val==NULL correctly
 * hist: 2021-04-20 VLlookup now returns NULL on not found (not "" as before)
 * hist: 2020-03-24 changed to strdup and changed flag to is_env
 * hist: 2015-05-14 VLstore now handles NULL cases safely (10q mk)
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	"varlib.h"
#include	<string.h>

#define	MAXVARS	200		/* a linked list would be nicer */

struct var {
		char *str;		/* name=val string	*/
		int  is_env;		/* a boolean		*/
	};

static struct var tab[MAXVARS];			/* the table	*/

static char *new_string( char *, char *);	/* private methods	*/
static struct var *find_item(char *, int);

void VLinit()
/*
 * initialize variable storage system
 */
{
	int	i;

	for ( i = 0 ; i < MAXVARS ; i++ ){
		tab[i].str = NULL;
		tab[i].is_env = 0;
	}
}

int VLstore( char *name, char *val )
/*
 * traverse list, if found, replace it, else add at end
 * since there is no delete, a blank one is a free one
 * return 1 if trouble, 0 if ok (like a command)
 */
{
	struct var *itemp;
	char	*s;
	int	rv = 1;				/* assume failure	*/

	/* 1. find spot to put it */
	itemp = find_item( name, 1 );
	if ( itemp != NULL )
	{
		/* found a spot, 2. make a string to store in element */
		if ( ( s = new_string( name, val ) ) != NULL )
		{
			if ( itemp->str )		/* has a val?	*/
				free(itemp->str);	/* y: remove it	*/
			itemp->str = s;			/* 3. store it	*/
			rv = 0;				/* ok! */
		}
	}
	return rv;
}

char * new_string( char *name, char *val )
/*
 * returns new string of form name=value or NULL on error
 */
{
	char	*retval;

	if ( name == NULL )
		retval = NULL;
	else if ( val == NULL )
		// BW: fixed bug here. was +1 so not enough space for '='
		retval = malloc(strlen(name)+2);
	else
		retval = malloc( strlen(name) + strlen(val) + 2 );

	if ( retval != NULL )
		sprintf(retval, "%s=%s", name, (val==NULL ? "" : val) );
	return retval;
}

char * VLlookup( char *name )
/*
 * returns value of var or empty string if not there
 */
{
	struct var *itemp;

	if ( (itemp = find_item(name,0)) != NULL )
		return itemp->str + 1 + strlen(name);
	return NULL;

}

int VLexport( char *name )
/*
 * marks a var for export, adds it if not there
 * returns 1 for no, 0 for ok
 */
{
	struct var *itemp;
	int	rv = 1;

	if ( (itemp = find_item(name,0)) != NULL ){
		itemp->is_env = 1;
		rv = 0;
	}
	else if ( VLstore(name, "") == 0 )	/* bug fix 31 mar 08 */
		rv = VLexport(name);
	return rv;
}

static struct var * find_item( char *name , int first_blank )
/*
 * searches table for an item
 * returns ptr to struct or NULL if not found
 * OR if (first_blank) then ptr to first blank one
 */
{
	int	i;
	int	len ;
	char	*s;

	if ( name == NULL )
		return NULL;

	len = strlen(name);

	for( i = 0 ; i<MAXVARS && tab[i].str != NULL ; i++ )
	{
		s = tab[i].str;
		if ( strncmp(s,name,len) == 0 && s[len] == '=' ){
			return &tab[i];
		}
	}
	if ( i < MAXVARS && first_blank )
		return &tab[i];
	return NULL;
}


void VLlist()
/*
 * performs the shell's  `set'  command
 * Lists the contents of the variable table, marking each
 * exported variable with the symbol  '*' 
 */
{
	int	i;
	for(i = 0 ; i<MAXVARS && tab[i].str != NULL ; i++ )
	{
		if ( tab[i].is_env )
			printf("  * %s\n", tab[i].str);
		else
			printf("    %s\n", tab[i].str);
	}
}

int VLenviron2table(char *env[])
/*
 * initialize the variable table by loading array of strings
 * return 1 for ok, 0 for not ok
 */
{
	int     i;
	char	*newstring;

	for(i = 0 ; env[i] != NULL ; i++ )
	{
		if ( i == MAXVARS )
			return 0;
		newstring = strdup(env[i]);
		if ( newstring == NULL )
			return 0;
		tab[i].str = newstring;
		tab[i].is_env = 1;
	}
	while( i < MAXVARS ){
		tab[i].str = NULL ;
		tab[i++].is_env = 0;
	}
	return 1;
}

char ** VLtable2environ()
/*
 * build an array of pointers suitable for making a new environment
 * note, you need to free() this when done to avoid memory leaks
 */
{
	int	i,			/* index			*/
		j,			/* another index		*/
		n = 0;			/* counter			*/
	char	**envtab;		/* array of pointers		*/

	/*
	 * first, count the number of env variables
	 */

	for( i = 0 ; i<MAXVARS && tab[i].str != NULL ; i++ )
		if ( tab[i].is_env == 1 )
			n++;

	/* then, allocate space for that many variables	*/
	envtab = (char **) malloc( (n+1) * sizeof(char *) );
	if ( envtab == NULL )
		return NULL;

	/* then, load the array with pointers		*/
	for(i = 0, j = 0 ; i<MAXVARS && tab[i].str != NULL ; i++ )
		if ( tab[i].is_env == 1 )
			envtab[j++] = tab[i].str;
	envtab[j] = NULL;
	return envtab;
}
