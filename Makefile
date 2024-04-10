CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lrt -pthread

EXECUTABLES = 	game barbarian wizard rogue

all: $(EXECUTABLES)

game: game.c dungeon_info.h dungeon_settings.h
	$(CC) $(CFLAGS) game.c -o game $(LDFLAGS)

barbarian: barbarian.c dungeon_info.h dungeon_settings.h
	$(CC) $(CFLAGS) barbarian.c -o barbarian $(LDFLAGS)

wizard: wizard.c dungeon_info.h dungeon_settings.h
	$(CC) $(CFLAGS) wizard.c -o wizard $(LDFLAGS)

rogue: rogue.c dungeon_info.h dungeon_settings.h
	$(CC) $(CFLAGS) rogue.c -o rogue $(LDFLAGS)

clean:
	rm -f $(EXECUTABLES)
