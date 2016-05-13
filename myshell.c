#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>				// errno
#include <fcntl.h>
#include <glob.h>				// les meta caracteres et les motifs
#include <readline/history.h> 	// gestion de l'historique
#include "myshell.h"

////  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~  GLOBALES  ~~~~~~~~~~~~~~~~~~~~~~~~~~~  ////
extern char *internCmd[];

int numcmd;							// compteur numero de commande
int numcmd_seq;						// compteur numero de sequence de commandes
int numcmd_pipe;					// compteur numero de pipe
int numcmd_par;						// compteur numero de commande parallele
char ligne[MAXCMD];					// LDC
char ligne_glb[MAXCMD];				// ligne globale
char input[MAXCMD];					// 
char Prompt[MAXCMD];				// message du prompt
char cmds[MAXELEMS][MAXCMD];		// 
char cmds_pipe[MAXELEMS][MAXCMD];	// 
char cmds_seq[MAXELEMS][MAXCMD];	// 
char cmds_par[MAXELEMS][MAXCMD];	// 
static char *dir;					// 

char* elems[MAXELEMS]; /* pointeurs sur les mots de la ligne (voir decoupe_cmd) */
char* elems1[MAXELEMS]; /* pointeurs sur les mots de la commande1 (voir decoupe_cmd) */
char* elems2[MAXELEMS]; /* pointeurs sur les mots de la commande2 (voir decoupe_cmd) */

int seq = 0;
int next = 0;
volatile int fg_pid = -1;

//booleans pour les types de commandes
int piped = 0;
int par = 0;
int redirected = 0;
int double_redirection = 0;
int rd_t;
char *rd_fl;
int use_glob = 0;
int en_fond = 0;
int Exit = 0;

////  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~  FONCTIONS  ~~~~~~~~~~~~~~~~~~~~~~~~~~  ////
// initialise les variables globals //
void init(void){
	int i;
	char null[55];
	null[0] = '\0';
	for(i = 0; i < MAXELEMS; i++)
		strcpy(cmds[i], null);
	for(i = 0; i < MAXELEMS; i++)
	{
		elems[i] = NULL;
		elems1[i] = NULL;
		elems2[i] = NULL;
	}
	*ligne = '\0';
	en_fond = 0;

	numcmd = 0;
	piped = 0;
	next = 0;
	redirected = 0;
	double_redirection = 0;
	use_glob = 0;
	rd_fl = (char *)malloc(sizeof(char));

	*ligne_glb = '\0';
}

//############################################################################//
//cherche la derniere ligne de l'historique
char *last_hist_entry(void){
	int i = 0;
	HIST_ENTRY **histlist;
	histlist = history_list();
	if(histlist)
		for(i = 0; histlist[i]; i++);	// i: num de la derniere entree de l'hist
	return histlist[i-1]->line;
}

//############################################################################//
// recherche le dernier caractere de la commande //
char last_char(char *mot){
	int i = 0;
	while(mot[i] != '\0') i++;
	return mot[i-1];
}

//############################################################################//
//affichage du prompte
void prompt(void){
	char *user, *host, *at = "@", *mode = "$";
	
	dir = getcwd(NULL, 0);
	user = getenv("USER");
	if(user == NULL) user = getenv("USERNAME");
	if(user == NULL) user = "me";
	host = getenv("HOSTNAME");
	if(host == NULL) host = "lsh" ;
	sprintf(Prompt, "%s%s%s:%s%s: ",user, at, host, dir, mode);
	fflush(stdout);
}

//############################################################################//
// netoyer le nom du fichier
char *clean_filename(char *filename){
	char *newfilename, *first = filename, *i = filename;
	
	while(*first && isspace(*first)) first++;
	//if(!*first) ;
	newfilename = first;
	i = first;
	while(*i && (isalnum(*i) || *i == '.' || *i == '/'))
		i++;
	i = '\0';
	return newfilename;
}

//############################################################################//
// retire les \t et \n d'une chaine
char *strip(const char *s){
    char *pt = malloc(strlen(s) + 1), *pt2 = NULL;
    if(pt)
    {
    	pt2 = pt;
    	while(*s != '\0')
    	{
    		if(*s != '\t' && *s != '\n') *pt2++ = *s++;
    		else ++s;
    	}
    	*pt2 = '\0';
    }
    return pt;
}

//############################################################################//
// lire une ligne des commandes a partire du clavier
void lire(void){
	int lastHistEntryCmp = 0;
	char *newligne = NULL;
	*ligne_glb = '\0';
	newligne = readline(Prompt);
	sprintf(ligne_glb, "%s\n", newligne);
	sprintf(input, "%s", newligne);
		
	lastHistEntryCmp = strcmp(last_hist_entry(), ligne_glb);
	if(ligne_glb && *ligne_glb && lastHistEntryCmp != -10)
	{									// si le mot n'est pas deja le dernier
		char *l = strip(ligne_glb);		// enleve les \n et \t
		add_history(l);					// ajoute le mot à l'hist
		free(l);						// libere l'espace
	}
}

//############################################################################//
//decouper la ligne et extraire les commandes sequentielles
void decoupe_ligne_seq(void){
	if(*ligne_glb == '\0') return;	// pas de cmd: economise un peu de mem
	numcmd_seq = 0; next = 0;
	char temp[55];
	int i = 0;

	if(strchr(ligne_glb, '|'))								// pipeline
		piped = 1;
	if((strchr(ligne_glb, '>')) || (strchr(ligne_glb, '>')))// cmd redirigee
		redirected = 1;
	else
		if(strchr(ligne_glb, ';')) seq = 1;					// +ieurs cmds
	do
	{
		i=0;
		while(ligne_glb[next] == ';') next++;
		for(; ((ligne_glb[next] != ';') && (ligne_glb[next] != '\n')); next++)
		{
			temp[i] = ligne_glb[next];
			i++;
		}
		if(i != 0) temp[i] = '\0';
		else break;											// cmd vide
		strcpy(cmds_seq[numcmd_seq], temp);
		numcmd_seq++;
	} while(ligne_glb[next] != '\n');
}

//############################################################################//
//decouper la ligne et extraire les commandes en pipes
void decoupe_ligne_pipe(char input[]){
	if(!input) return;					// economise un peu de mem
	numcmd_pipe = 0;
	int i = 0, cpt = 0;
	char temp[55];

	if(strchr(input, '|')) piped = 1;	// il y a un pipeline
	do
	{
		i = 0;
		while(input[cpt] == '|') cpt++;	// passe le |
		for(; ((input[cpt] != '|') && (input[cpt] != '\0')); cpt++)
		{
			temp[i] = input[cpt];
			i++;
		}
		if(i != 0) temp[i] = '\0';
		else break;						// cmd vide: sort !
		strcpy(cmds_pipe[numcmd_pipe], temp);
		numcmd_pipe++;
	} while(input[cpt] != '\n');		// jusqu'à la fin de la ldc
}

//############################################################################//
//decouper la ligne et extraire les commandes en parallele
void decoupe_ligne_par(char input[]){
	if(!input) return;					// economise un peu de mem
	numcmd_par = 0;
	int i = 0, cpt = 0;
	char temp[55];

	if(strchr(input, '&')) par = 1;		// il y a un process a exec en bg
	do
	{
		i = 0;
		while(input[cpt] == '&') cpt++; // passe le &
		for(; ((input[cpt] != '&') && (input[cpt] != '\0')); cpt++)
		{
			temp[i] = input[cpt];
			i++;
		}
		if(i != 0) temp[i] = '\0';		// fin de la cmd
		else break;						// cmd vide : sort !
		strcpy(cmds_pipe[numcmd_par], temp);
		numcmd_par++;
	} while(input[cpt] != '\n');		// jusqu'à la fin de la ldc
}

//############################################################################//
/* decoupe la commande en mots  et les mettre dans elem
   elem doit se termine par NULL pour execvp */
void decoupe_cmd(char *line, char **elem){
    if(!line) exit;							// inutile: economise un peu de mem
    char* first = line, *cpt = line;
    int i = 0;

    if(strchr(line, '&')) en_fond = 1;		// proc à gerer en bg
    if(strchr(line, '*')) use_glob = 1;		// meta caractere de remplacement
    if(strchr(line, '>'))					// sortie redirigee
    {
        redirected = 1;
        rd_t = STDOUT;
    }
    if(strchr(line, '<'))					// entrée redirigee
    {
        redirected = 1;
        rd_t = STDIN;
    }
    if(strchr(line, '>') == (strrchr(line, '>') -1)) // double redirection
    {
        double_redirection = 1;
        fflush(stdout);
    }
    if(redirected)							// supprime les < et > de la LDC
    {
        if(rd_t == STDOUT) first = strtok(line, ">");
        else first = strtok(line, "<");
    }
    if((redirected && rd_t == STDIN))
    {
        redirected = 1;
        if(rd_fl != NULL)
        {
            char *_rd_fl;
            _rd_fl = strtok( NULL, "<" );
            rd_fl = clean_filename(_rd_fl);
            exit;
        }
    }
    if(redirected && rd_t == STDOUT)
    {
        redirected = 1;
        if(rd_fl!= NULL )
        {
            char *_rd_fl;
            _rd_fl = strtok( NULL, ">" );
            rd_fl = clean_filename(_rd_fl);
            exit;
        }
    }
    for (i=0;i<MAXELEMS-1; i++) {
        /* saute les espaces */
        while (*first && (isspace(*first) || (*first == '&')))
            first++;
        /* fin de ligne ? */
        if(!*first) break;
        /* on se souvient du début de ce mot */
        elem[i] = first;
        cpt = first;
        /* cherche la fin du mot */
        while(*cpt && !isspace(*cpt) )
            cpt++; /* saute le mot */
        /* termine le mot par un \0 et passe au suivant */
        if(*cpt)
        {
            *cpt = 0;
            cpt++;
        }
        first = cpt;
    }

    elem[i] = NULL; // le dernier doit etre NULL pour execvp

    if((i != 0) && (((strncmp(elem[0], "ls", 2)) == 0) ||
    		((strncmp(elem[0], "grep", 4)) == 0)))
    { //Ajouter la colorisation pour ls et grep
        int fin = 0;
        char* temp;
        do fin++; while(elem[fin] != NULL);
        int i;
        for(i = fin; i > 1; i--)
            elem[i] = elem[i-1];
        elem[1] = "--color=auto";
        elem[fin+1] = NULL;
    }
}

//############################################################################//
void child_signal(int signal){
	/* un signal peut correspondre à plusieurs fils finis, donc on boucle */
	while(1)
	{
		printf(" wait ");
		int status;
		pid_t pid = waitpid(-1, &status, WNOHANG);
		if(pid<0)
		{
			if(errno == EINTR) continue; /* interrompu => on recommence */
			if(errno == ECHILD) break;   /* plus de fils termine => on quitte */
			printf("erreur de wait: (%s)\n", strerror(errno));
			break;
		}

		if(pid == 0) break; /* plus de fils termine => on quitte */
		/*  signale à execute que fg_pid s'est termine */
		if(pid == fg_pid) fg_pid = -1;
		if(WIFEXITED(status))
			printf("terminaison normale, pid=%i, status %i\n",
					pid,WEXITSTATUS(status));
		if(WIFSIGNALED(status))
			printf("terminaison par signal %i, pid = %i\n",
					WTERMSIG(status),pid);
	}
}

//############################################################################//
// verifie si l'utilisateur quitte //
bool check_exit(word){
	if(((strncmp(word, "exit", 4)) == 0) || ((strncmp(word, "quit", 4)) == 0))
		return TRUE;
	return FALSE;
}

//############################################################################//
// execute l'instruction de la commande interne //
bool exec_intern_cmd(char **elem, char **elems, int cmdid, char *cwd){
	switch(cmdid)							// execute le code de la cmd
	{
		case 0:		return cmd_cd(elem);			break;	// cd
		case 1:		puts(cwd);						break;	// cwd
		case 2:		return cmd_history();			break;	// history
		case 3:		return cmd_export(elem, elems);	break;	// export
		case 4:		cmd_help();						break;	// help
		default:	return FALSE;					break;	// sans insctruction
	}
	return TRUE;
}

//############################################################################//
// verifie s'il faut executer une commande interne //
bool check_in_cmd(char **elem, char **elems, char *cwd){
	int i = 0;
	while(internCmd[i] != NULL)				// parcoure les cmd internes
	{
		if(strcmp(*elem, internCmd[i]) == 0)// si on en appelle une
		{
			exec_intern_cmd(elem, elems, i, cwd);	// execute la
			return TRUE;					// sors avec T
		}
		i++;
	}
	return FALSE;							// sinon: sors avec F
}

//  redirection de l'entree standard sur /dev/null //
void redirStdin(char *elem){
	printf("[%s] %d\n",elem, getpid());
	fflush(stdout);
	int devnull = open("/dev/null", O_RDONLY);
	if(devnull != -1)
	{
		close(0);
		dup2(devnull, 0);
	}
}

// reactivation de SIGINT & debloque SIGCLHD Controle+C  //
void unlock(sigset_t sigset){
	struct sigaction sig;
	sig.sa_flags = 0;
	sig.sa_handler = SIG_DFL;
	sigemptyset(&sig.sa_mask);
	sigaction(SIGINT, &sig, NULL);
	sigprocmask(SIG_UNBLOCK, &sigset, NULL);
}

// gere les motifs (les meta caracteres) //
void manageMetaChar(char **elem){
	glob_t globbuf;
	int i = 0, num = 0, cptm = 0, cptg = 1;
	for(i = 1; elem[i] != NULL; i++)
		if(strchr(elem[i], '*') != NULL)
			num++;
	globbuf.gl_offs = num;
	fflush(stdout);
	for(i = 1; elem[i] != NULL; i++)
		if(strchr(elem[i], '*'))
		{
			cptm++;
			if(cptm == 1)
				glob(elem[i], GLOB_DOOFFS, NULL, &globbuf);
			else
				glob(elem[i], GLOB_DOOFFS|GLOB_APPEND, NULL, &globbuf);
		}
	globbuf.gl_pathv[0] = elem[0];
	for(i = 1; elem[i] != NULL; i++)
		if(strchr(elem[i], '*') == NULL)
		{
			globbuf.gl_pathv[cptg] = elem[i];
			cptg++;
		}
	execvp(elem[0], &globbuf.gl_pathv[0]);
}

//############################################################################//
// execute une commande simple //
void execute(char** elem){
	sigset_t sigset;
	pid_t pid;
	if(!elem[0]) return;
	if(!en_fond)
	{
		/* bloque child_signal jusqu'à ce que le pere ait place le pid du
		   fils dans fg_pid
		   sinon on risque de manquer la notification de fin du fils */
		sigemptyset(&sigset);
		sigaddset(&sigset, child_signal);
		sigprocmask(SIG_BLOCK, &sigset, NULL);
	}

	if(!elem[0]) return;						// ligne vide: sors!
	if(check_exit(elem[0])) {Exit = 1; return;}	// l'utilisateur quite
	if(check_in_cmd(elem, elems, dir)) return;	// si cmd interne: exec et sors!

	pid = fork();
	if(pid < 0)									// parent: erreur
	{
		printf("erreur fork (%s)\n", strerror(errno));
		return;
	}
	if(pid == 0)								// enfant: OK
	{
		if(en_fond)
			redirStdin(elem[0]);				// redirige l'entree standard
		else
			unlock(sigset);						// reactive Ctrl+C
		if(use_glob)
			manageMetaChar(elem);				// gere les meta caracteres
		else
			execvp(elem[0], elem);				// exec le programme
		printf("lsh: exec: \"%s\" (%s)\n", elem[0], strerror(errno));
		exit(1);								// il y a erreur: quitte
	}
	else
	{
		if(!en_fond)
		{
			fflush(stdout); fflush(stdout);
			waitpid(pid, NULL, 0);
			while(fg_pid != -1)					// attend la fin du proc en fg
				fg_pid = pid;
		}
	}
}

//############################################################################//
// execute les pipes recursivement //
void recursive_pipe(int i){
	int fd[2], pid = 0, Pipe = pipe(fd);
	char line[55], *elem[MAXELEMS];
	
	strcpy(line, cmds_pipe[i]);
	decoupe_cmd(line, elem);
	fflush(stdout);
	if(Pipe == -1)
	{
		printf("Erreur: Pipe failed.n");
		return;
	}
	if(i != (numcmd_pipe-1))
	{
		pid = fork();
		if(pid < 0)
		{
			printf("Erreur: Fork failed.n");
			return;
		}

		if(pid == 0) //enfant
		{
			close(fd[1]);
			dup2(fd[0], STDIN_FILENO);
			close(fd[0]);
			recursive_pipe(i+1);
			printf("Erreur: execvp failed.n");
			return;
		}
		else		// parent
		{
			close(fd[0]);
			dup2(fd[1], 1);
			close(fd[1]);
			int erreur = execvp(elem[0], elem);
			printf("Erreur: execvp failed : (%s)\n", strerror(erreur));
		}
	}
	else
	{
		close(fd[0]);
		execvp( elem[0], elem);
		printf("Erreur: execvp failed.n");
	}
}

//############################################################################//
// executer les commandes en pipes
void runPipe(void){
	if(numcmd_pipe > 0)
	{
		int pid = fork();
		if(pid < 0)				// erreur fork
		{
			printf("Erreur: Fork failed.n");
			return;
		}
		if(pid == 0)			// enfant
			recursive_pipe(0);
		else					// parent
			waitpid(pid, NULL, 0);
	}
}

//############################################################################//
/* executer une commande qui contient une redirection fileid
(STDOUT ou STDIN) vers file */
int run_rd_cmd(char **items, char *file, int fileid){
    int pid = fork(), fid = 0;
    if(pid < 0)
    {
        printf("Erreur: Fork failed.n");
        return 1;
    }
    if(pid == 0)						// enfant
    {
        if(fileid == STDIN)
        {
            fid = open(file, O_RDONLY, 0666);
            fflush(stdout);
        }
        else if(fileid == STDOUT)
        {
            if(double_redirection == 0)
                fid = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0666);
            else
                fid = open(file, O_APPEND| O_WRONLY, 0666);
            fflush(stdout);
        }
        dup2(fid, fileid);
        close(fid);
        fflush(stdout);
        execvp(items[0], items);
        printf("Erreur: execvp failed.n");
        return 1;
    }
}

