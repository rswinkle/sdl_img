${CC:=gcc} -std=c99 -g -O0 test_gui.c -o gui_test -Isrc `sdl2-config --libs --cflags` -lSDL2_gfx -lm -Wall
