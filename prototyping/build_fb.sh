${CC:=gcc} -std=gnu99 -g -O0 filebrowser.c -o fb_test -I../src `sdl2-config --libs --cflags` -lm -Wall
