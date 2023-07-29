
CC=clang
CFLAGS=`pkg-config sdl2 libcurl --cflags`
LIBS=`pkg-config sdl2 libcurl --libs` -lm
DBG_OPTS=-fsanitize=address -fsanitize=undefined -std=gnu99 -g -O0 -Wall
REL_OPTS=-std=gnu99 -msse -O3 -DNDEBUG

all:
	echo $(CFLAGS)
	echo $(LIBS)
	echo $(OPTS)

debug: src/sdl_img.c src/events.c src/gui.c src/sorting.c nuklear.o
	$(CC) $(DBG_OPTS) src/sdl_img.c nuklear.o -o sdl_img $(CFLAGS) $(LIBS)

nuklear.o: src/nuklear.h src/nuklear_sdl_renderer.h
	$(CC) $(DBUG_OPTS) -c src/nuklear.c `sdl2-config --cflags` -lm

release: src/sdl_img.c src/events.c src/gui.c src/sorting.c nuklear_release.o
	$(CC) $(REL_OPTS) src/sdl_img.c nuklear.o -o sdl_img $(CFLAGS) $(LIBS)

nuklear_release.o: src/nuklear.h src/nuklear_sdl_renderer.h
	$(CC) $(REL_OPTS) -c src/nuklear.c `sdl2-config --cflags` -lm

win_debug:
	echo "win_debug"

win_release:
	echo "win_release"

clean:
	rm sdl_img *.o *.exe

