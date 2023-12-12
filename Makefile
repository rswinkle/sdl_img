
# TODO if using github.com/Alexpux/MSYS2-Cross
#@ifeq ($(OS), Windows_NT)
#@	CC=win-clang
#@	DBG_OPTS=-std=gnu99 -g -O0 -Wall
#@else
#@	CC=clang
#@	DBG_OPTS=-fsanitize=address -fsanitize=undefined -std=gnu99 -g -O0 -Wall
#@endif

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


win_debug: nuklear.o lua_win
	$(CC) $(DBG_OPTS) src/sdl_img.c nuklear.o -o sdl_img.exe $(CFLAGS) $(LIBS)

win_release: nuklear_release.o lua_win
	$(CC) $(REL_OPTS) src/sdl_img.c nuklear.o -o sdl_img.exe $(CFLAGS) $(LIBS)

win_package: win_release
	ldd sdl_img.exe | grep mingw64 | cut -d' ' -f3 | xargs -I{} cp {} package/
	cp LICENSE.txt package/
	cp LICENSE package/
	cp README.md package/
	unix2dos package/README.md package/LICENSE*
	cp sdl_img.exe package/
	makensis.exe make_installer.nsi

lua:
	$(MAKE) -C lua-5.4.6/

lua_win:
	cd lua-5.4.6/src && $(MAKE) PLAT=mingw

cross_win_package:
	#cat mingw_dll_list.txt | xargs -I{} cp {} package/
	win-ldd sdl_img.exe | grep mingw64 | awk '{print $$3}' | xargs -I{} cp {} package/
	cp LICENSE.txt package/
	cp LICENSE package/
	cp README.md package/
	unix2dos package/README.md package/LICENSE*
	cp sdl_img.exe package/

install: release
	cp ./sdl_img /usr/local/bin
	cp sdl_img.1 /usr/local/share/man/man1
	cp sdl_img.desktop /usr/share/applications
	cp ./package/sdl_img.png /usr/share/icons/hicolor/48x48/apps

clean:
	rm -f sdl_img *.o *.exe
	rm package/*.dll
	$(MAKE) -C lua-5.4.6/ clean

