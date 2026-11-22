#ifndef	BUILTIN_H
#define	BUILTIN_H

int is_builtin(char **args, int *resultp);
int is_assign_var(char *cmd, int *resultp);
int is_list_vars(char *cmd, int *resultp);
int is_cd(char **args, int *resultp);
int is_exit(char **args);
int is_read(char **args, int *resultp);
int do_read(char **args, int *resultp);
int is_comment(char *cmd);
void do_exit(char **args);
void varsub(char *line);
int assign(char *);
int cd(char **);
int okname(char *);
int is_export(char **, int *);
void insert_var(char **target, char *varNameStart, int var_length);
void splice_str(char **target, char *strToInsert, char *splicePos, int spliceLength);
void execute_inline_commands(char **line);

#endif
