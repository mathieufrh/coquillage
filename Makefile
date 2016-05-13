CC=gcc
EXEC=lsh
LDFLAGS= /usr/lib/x86_64-linux-gnu/libreadline.so* /usr/lib/x86_64-linux-gnu/libhistory.so*
CFLAGS= -Wall
SRC= rline.c cmds.c myshell.c main.c
OBJ=$(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	@$(CC) -o $@ $^ $(LDFLAGS)

cmds.o: cmds.c cmds.h comod.h
	@$(CC) -o $@ -c $< $(CFLAGS)

myshell.o: myshell.c myshell.h comod.h
	@$(CC) -o $@ -c $< $(CFLAGS)

#~ main.o: main.c myshell.h comod.h
	#~ @$(CC) -o $@ -c $< $(CFLAGS)

# pour forcer clean/cleanall même s'il existe un fichier de même nom
.PHONY: clean cleanall

clean:
	@rm -rf *.o

cleanall: clean				
	@rm -rf $(EXEC)
