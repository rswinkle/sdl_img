${CC:=gcc} -fsanitize=address -fsanitize=undefined -std=gnu99 -g -O0 src/sdl_img.c -o sdl_img `sdl2-config --libs --cflags` `curl-config --libs --cflags` -lSDL2_gfx -lm -Wall
