////  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~  INCLUDE  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~  ////
#include <stdio.h>
#include <stdlib.h>
#include "myshell.h"

extern int numcmd_seq;						// compteur numero de sequence de commandes
extern char cmds_seq[MAXELEMS][MAXCMD];	// 

const char *hist_file = "History";	// 

//booleans pour les types de commandes
extern int piped;
extern int redirected;
extern int rd_t;
extern char *rd_fl;
extern int Exit;

////  ~~~~~~~~~~~~~~~~~~~~~~~~~~~  PROGRAMME  ~~~~~~~~~~~~~~~~~~~~~~~~~~~  ////
int main(void){
	int i = 0;
	char *elem[MAXELEMS];
	struct sigaction sig = {{0}};					// initialise le handler
	sig.sa_handler = SIG_IGN;						// ignorer le signal
	sigaction(SIGINT, &sig, NULL);
	initialize_readline();							// pour l'autocompletion
	using_history();								// utilise l'hist precedent
	read_history(hist_file);						// lit l'historique

	while(!Exit)
	{												// tant qu'on ne quitte pas:
		init();										// initialise les variables
		prompt();									// affiche le prompt
		lire();										// lit une ldc
		decoupe_ligne_seq();						// decoupe les seq de cmds
		for(i = 0; i<numcmd_seq; i++)
		{											// pour chaque seq de cmds:
			decoupe_ligne_pipe(cmds_seq[i]);		// decoupe les pipelines
			if(piped) runPipe();					// s'il y en a: exec en pipe
			else
			{										// sinon:
				decoupe_cmd(cmds_seq[i], elem);		// decoupe la commande
				if(redirected)						// si tu as trouve < ou >:
					run_rd_cmd(elem, rd_fl, rd_t);	// exec et redirige la cmd
				else								// sinon: cmd simple
					execute(elem);					// exec la
			}
		}
	}
	write_history(hist_file);						// ajoute les LDC à l'hist
	return EXIT_SUCCESS;							// c'est fini
}

