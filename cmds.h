#ifndef CMDS_H_
#define CMDS_H_

#include "comod.h"

////  ~~~~~~~~~~~~~~~~~~~~~~~~~~  PROTOTYPES  ~~~~~~~~~~~~~~~~~~~~~~~~~~~  ////
String 	rpl_wd		(String str, String word, String rep);
bool 	cmd_cd		(String *elem);
bool 	cmd_history	(void);
bool 	cmd_export	(String *elem, String *elems);
void 	cmd_help	(void);

#endif /*CMDS_H_*/

