

# default to linux
PLAT=linux


# windows: msys2 mingw64 environment
#
# cross_win: msys2 clang/ucrt64 cross compile env using
# using https://github.com/HolyBlackCat/quasi-msys2

PLATS=linux cross_win msys2

#CC=tcc

# for some reason the sanitizers aren't working in my cross compile environment
# but it's really only for creating a windows release/package anyway
ifeq ($(PLAT), cross_win)
TARGET=sdl_img.exe
ifeq ($(config), release)
	OPTS=-std=gnu99 -msse -O3 -DNDEBUG
	#OPTS=-std=gnu99 -msse -O3 -DNDEBUG -DSDL_DISABLE_IMMINTRIN_H
else
	-fsanitize=undefined -std=gnu99 -g -O0 -Wall
	#OPTS=-fsanitize=address -fsanitize=undefined -std=gnu99 -g -O0 -Wall -DSDL_DISABLE_IMMINTRIN_H
endif
else
TARGET=sdl_img
endif

ifeq ($(PLAT), linux)
ifeq ($(config), release)
	#OPTS=-std=gnu99 -msse -O3 -DNDEBUG
	OPTS=-std=gnu99 -msse -O3 -g -DNDEBUG
	#OPTS=-std=gnu99 -msse -O3 -DNDEBUG -DSDL_DISABLE_IMMINTRIN_H
else
	OPTS=-fsanitize=address -fsanitize=undefined -std=gnu99 -g -O0 -Wall

	#for valgrind or gdb
	#OPTS=-std=gnu99 -g -O0 -Wall

	#OPTS=-fsanitize=address -fsanitize=undefined -std=gnu99 -g -O0 -Wall -DSDL_DISABLE_IMMINTRIN_H
endif
endif

ifeq ($(PLAT), msys2)
ifeq ($(config), release)
	OPTS=-std=gnu99 -msse -O3 -DNDEBUG
	#OPTS=-std=gnu99 -msse -O3 -DNDEBUG -DSDL_DISABLE_IMMINTRIN_H
else
	OPTS=-std=gnu99 -g -O0 -Wall
endif
endif



#CFLAGS=`pkg-config sdl2 libcurl --cflags` -Ilua-5.4.7/src
#LIBS=`pkg-config sdl2 libcurl --libs` -lm -Llua-5.4.7/src -llua

CFLAGS=`pkg-config sdl2 libcurl --cflags` -Ilua-5.4.7/src
LIBS=`pkg-config sdl2 libcurl --libs` -lm

DESTDIR=/usr/local

PKGDIR=package_linux
PKG_DIR=$(PKGDIR)$(DESTDIR)

SRCS=src/sdl_img.c src/events.c src/gui.c src/rendering.c src/lua_config.c \
	 src/thumbs.c src/curl_stuff.c src/playlists.c src/sorting.c src/controls_str.c \
	 src/sdl_img.h src/thumbs.h src/curl_stuff.h \
	 src/compile_constants.h src/config_constants.h \
	 src/clnk.h src/file_browser.h src/lua_helper.h


all: $(TARGET)

sdl_img: $(SRCS) nuklear.o minilua.o
	$(CC) $(OPTS) src/sdl_img.c minilua.o nuklear.o -o $@ $(CFLAGS) $(LIBS)

sdl_img.exe: $(SRCS) nuklear.o minilua.o
	$(CC) $(OPTS) src/sdl_img.c nuklear.o minilua.o -o $@ $(CFLAGS) $(LIBS)

nuklear.o: src/nuklear.h src/nuklear_sdl_renderer.h
	$(CC) $(OPTS) -c src/nuklear.c `pkg-config sdl2 --cflags`

minilua.o: src/minilua.c
	$(CC) $(OPTS) -c src/minilua.c -lm


linux_package: sdl_img
	mkdir -p $(PKG_DIR)/bin
	mkdir -p $(PKG_DIR)/share/man/man1
	mkdir -p $(PKG_DIR)/share/applications
	mkdir -p $(PKG_DIR)/share/icons/hicolor/48x48/apps
	cp ./sdl_img $(PKG_DIR)/bin
	cp sdl_img.1 $(PKG_DIR)/share/man/man1
	cp sdl_img.desktop $(PKG_DIR)/share/applications
	cp ./package/sdl_img.png $(PKG_DIR)/share/icons/hicolor/48x48/apps
	fpm -s dir -t deb -v 1.0.0-beta -n sdl_img -C $(PKGDIR) \
	--log info --verbose \
	-d "libsdl2-2.0-0 >= 2.0.20" -d "libcurl4" \
	-m "Robert Winkler <rob121618@gmail.com>" \
	--description "A simple image viewer based on SDL2 and stb_image" \
	--license MIT \
	--url "https://github.com/rswinkle/sdl_img"
	fpm -s dir -t tar -v 1.0.0-beta -n sdl_img_1.0.0-beta -C package_linux \
	--log info --verbose \
	-d "libsdl2-2.0-0 >= 2.0.20" -d "libcurl4" \
	-m "Robert Winkler <rob121618@gmail.com>" \
	--description "A simple image viewer based on SDL2 and stb_image" \
	--license MIT \
	--url "https://github.com/rswinkle/sdl_img"

cross_win_package: cross_win
	#cat mingw_dll_list.txt | xargs -I{} cp {} package/
	#win-ldd sdl_img.exe | grep mingw64 | awk '{print $$3}' | xargs -I{} cp {} package/
	win-ldd sdl_img.exe | grep ucrt64 | awk '{print $$3}' | xargs -I{} cp {} package/
	cp LICENSE.txt package/
	cp LICENSE package/
	cp README.md package/
	unix2dos package/README.md package/LICENSE*
	cp sdl_img.exe package/
	makensis make_installer.nsi


install: sdl_img
	mkdir -p $(DESTDIR)/bin
	mkdir -p $(DESTDIR)/share/man/man1
	mkdir -p $(DESTDIR)/share/applications
	mkdir -p $(DESTDIR)/share/icons/hicolor/48x48/apps
	cp ./sdl_img $(DESTDIR)/bin
	cp sdl_img.1 $(DESTDIR)/share/man/man1
	cp sdl_img.desktop $(DESTDIR)/share/applications
	cp ./package/sdl_img.png $(DESTDIR)/share/icons/hicolor/48x48/apps

# Only do this on older distro like Ubuntu 22.04 for maximum
# compatibility
#
# Prereq, download the latest version of the linuxdeploy appimage
# here: https://github.com/linuxdeploy/linuxdeploy/releases 
# put it in ~/Applications and make it executable with chmod +x  
appimage:
	make install DESTDIR=AppDir/usr
	~/Applications/linuxdeploy-x86_64.AppImage --appdir AppDir/ -dAppDir/usr/share/applications/sdl_img.desktop -iAppDir/usr/share/icons/hicolor/48x48/apps/sdl_img.png -eAppDir/usr/bin/sdl_img --output appimage

clean:
	rm -f sdl_img *.o *.exe
	$(MAKE) -C lua-5.4.7/ clean
	rm package/*.dll


# Below here are no longer used... unless/until I update them
windows: nuklear.o minilua.o
	$(CC) $(OPTS) src/sdl_img.c nuklear.o minilua.o -o sdl_img.exe $(CFLAGS) $(LIBS)

windows_package: windows
	ldd sdl_img.exe | grep mingw64 | cut -d' ' -f3 | xargs -I{} cp {} package/
	cp LICENSE.txt package/
	cp LICENSE package/
	cp README.md package/
	unix2dos package/README.md package/LICENSE*
	cp sdl_img.exe package/
	makensis.exe make_installer.nsi



lua:
	$(MAKE) -C lua-5.4.7/

lua_win:
	cd lua-5.4.7/src && $(MAKE) PLAT=mingw

# These are using https://github.com/HolyBlackCat/quasi-msys2
lua_cross_win:
	cd lua-5.4.7/src && $(MAKE) CC=win-clang PLAT=generic

