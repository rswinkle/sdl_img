${CC:=gcc} -std=c99 -g -O0 test_gui.c -o gui_test -I../src `sdl2-config --libs --cflags` -lm -Wall
