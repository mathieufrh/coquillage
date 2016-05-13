#ifndef MYSHELL_H_
#define MYSHELL_H_

#include <signal.h>			// traitement des signaux
#include "comod.h"

////  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~  DEFINES  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~  ////
#define MAXELEMS 32			// nb max d'elts
#define STDIN 0				// STDIN: flux 0 dans exec
#define STDOUT 1			// STDOUT: flux 1 dans exec
#define STDERR 2			// STDERR: flux 2 dans exec
#define MAXCMD 55			// nb max de mot sur la LDC

////  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~  FONCTIONS  ~~~~~~~~~~~~~~~~~~~~~~~~~~  ////
void 	init				(void);
char *	last_hist_entry		(void);
char 	last_char			(char *mot);
void 	prompt				(void);
char *	clean_filename		(char *filename);
char *	strip				(const char *s);
void 	lire				(void);
void 	decoupe_ligne_seq	(void);
void 	decoupe_ligne_pipe	(char input[]);
void 	decoupe_ligne_par	(char input[]);
void 	decoupe_cmd			(char *line, char **elem);
void 	child_signal		(int signal);
bool 	check_exit			(word);
bool 	exec_intern_cmd		(char **elem, char **elems, int cmdid, char *cwd);
bool 	check_in_cmd		(char **elem, char **elems, char *cwd);
void 	redirStdin			(char *elem);
void 	unlock				(sigset_t sigset);
void 	manageMetaChar		(char **elem);
void 	execute				(char** elem);
void 	recursive_pipe		(int i);
void 	runPipe				(void);
int 	run_rd_cmd			(char **items, char *file, int fileid);

#endif /*MYSHELL_H_*/

