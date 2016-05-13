#include <readline/readline.h>	// lecture avancer des cmds et autocompletion
#include <readline/history.h> 	// gestion de l'historique
#include "cmds.h"

String internCmd[] = {"cd", "cwd", "history", "export", "help", NULL};

////  ~~~~~~~~~~~~~~~~~~~~~~~~~~~  FONCTIONS  ~~~~~~~~~~~~~~~~~~~~~~~~~~~  ////
String rpl_wd(String str, String word, String rep){
  static char buff[2048];
  String p;

  if(!(p = strstr(str, word))) return str;
  strncpy(buff, str, p-str);
  buff[p-str] = '\0';
  sprintf(buff+(p-str), "%s%s", rep, p+strlen(word));
  return buff;
}

////  ~~~~~~~~~~~~~~~~~~~~~~~~~  CMD INTERNES  ~~~~~~~~~~~~~~~~~~~~~~~~~~  ////
// cd //
bool cmd_cd(String *elem){
	String dir;
	if(strncmp(elem[1],"~", 1) == 0) dir = rpl_wd(elem[1], "~", getenv("HOME"));
	else dir = elem[1];
	if(chdir(dir) == 0)
	{
		dir = getcwd(NULL, 0);
		return TRUE;
	}
	printf("lsh: cd: impossible de changer de répertoire\n");
	return FALSE;
}

// history //
bool cmd_history(void){
	HIST_ENTRY **histlist;
	int i = 0;
	histlist =  history_list();
	if(histlist)
	{
		for(i = 0; histlist[i]; i++)
			printf("%d: %s\n", i, histlist[i]->line);
		return TRUE;
	}
	printf("lsh: history: impossible de charger l'historique\n");
	return FALSE;
}

// export //
bool cmd_export(String *elem, String *elems){
	char env[128];
	sprintf(env, "%s=%s", elem[1], elem[2]);
	if(putenv(env) == 0)
	{
		printf("[%s]\n", env);
		return TRUE;
	}
	printf("lsh: export: impossible");
	return FALSE;
}

// help //
void cmd_help(void){
	printf("##############################################################\n");
	printf("############# lilshell v1.0 par Mathieu FOURCROY #############\n");
	printf("#############    un petit shell très modeste     #############\n");
	printf("##############################################################\n");
	printf("# ~~~~~~~~~~~~~~~~~~~ commandes internes ~~~~~~~~~~~~~~~~~~~ #\n");
	printf("# cd: cd <path>                                              #\n");
	printf("#    => permet de se placer dans le repertoire <path>        #\n");
	printf("# cwd:                                                       #\n");
	printf("#    => permet d'afficher le repertoire courant              #\n");
	printf("# history:                                                   #\n");
	printf("#    => permet de visualiser l'historique entier             #\n");
	printf("# export: export <name> <value>                              #\n");
	printf("#    => permet de lier variable <name> à la valeur <value>   #\n");
	printf("# exit ou quit:                                              #\n");
	printf("#    => permet de quitter le shell                           #\n");
	printf("# ~~~~~~~~~~~~~~~~~~~ commandes externes ~~~~~~~~~~~~~~~~~~~ #\n");
	printf("# lilshell peut executer de nombreuses commandes externes    #\n");
	printf("# ~~~~~~~~~~~~~~~~ historique des versions ~~~~~~~~~~~~~~~~~ #\n");
	printf("# pour consulter l'historique des versions utilisez la       #\n");
	printf("# commande \"cat /tmp/changelog\"                              #\n");
	printf("##############################################################\n");
}

