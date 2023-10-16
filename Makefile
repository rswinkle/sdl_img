
ifeq ($(OS), Windows_NT)
	CC=win-clang
	DBG_OPTS=-std=gnu99 -g -O0 -Wall
else
	CC=clang
	DBG_OPTS=-fsanitize=address -fsanitize=undefined -std=gnu99 -g -O0 -Wall
endif

CFLAGS=`pkg-config sdl2 libcurl --cflags` -Ilua-5.4.6/src
LIBS=`pkg-config sdl2 libcurl --libs` -lm -Llua-5.4.6/src -llua
REL_OPTS=-std=gnu99 -msse -O3 -DNDEBUG

all:
	echo $(CFLAGS)
	echo $(LIBS)
	echo $(OPTS)

debug: src/sdl_img.c src/events.c src/gui.c src/sorting.c nuklear.o lua
	$(CC) $(DBG_OPTS) src/sdl_img.c nuklear.o -o sdl_img $(CFLAGS) $(LIBS)

nuklear.o: src/nuklear.h src/nuklear_sdl_renderer.h
	$(CC) $(DBUG_OPTS) -c src/nuklear.c `sdl2-config --cflags`

release: src/sdl_img.c src/events.c src/gui.c src/sorting.c nuklear_release.o lua
	$(CC) $(REL_OPTS) src/sdl_img.c nuklear.o -o sdl_img $(CFLAGS) $(LIBS)

nuklear_release.o: src/nuklear.h src/nuklear_sdl_renderer.h
	$(CC) $(REL_OPTS) -c src/nuklear.c `pkg-config sdl2 --cflags`

win_debug: nuklear.o lua
	$(CC) $(DBG_OPTS) src/sdl_img.c nuklear.o -o sdl_img.exe $(CFLAGS) $(LIBS)

win_release: nuklear.o lua
	$(CC) $(REL_OPTS) src/sdl_img.c nuklear.o -o sdl_img.exe $(CFLAGS) $(LIBS)

lua:
	$(MAKE) -C lua-5.4.6/

win_package:
	#cat mingw_dll_list.txt | xargs -I{} cp {} package/
	win-ldd sdl_img.exe | grep mingw64 | awk '{print $$3}' | xargs -I{} cp {} package/
	#cp ./*.dll package/
	cp LICENSE.txt package/
	cp LICENSE package/
	cp README.md package/
	unix2dos package/README.md package/LICENSE*
	cp sdl_img.exe package/

clean:
	rm -f sdl_img *.o *.exe
	$(MAKE) -C lua-5.4.6/ clean

