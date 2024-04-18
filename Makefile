CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lrt -pthread

# Executables to be built
EXECUTABLES = game barbarian wizard rogue

# Default target
all: $(EXECUTABLES)

# Specific rules for each executable
game: game.c dungeon.o
	$(CC) $(CFLAGS) -o game game.c dungeon.o $(LDFLAGS)

barbarian: barbarian.c
	$(CC) $(CFLAGS) -o barbarian barbarian.c $(LDFLAGS)

wizard: wizard.c
	$(CC) $(CFLAGS) -o wizard wizard.c $(LDFLAGS)

rogue: rogue.c
	$(CC) $(CFLAGS) -o rogue rogue.c $(LDFLAGS)

clean:
	rm -f $(EXECUTABLES) *.o

.PHONY: all clean

