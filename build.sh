gcc -std=c99 -g -O0 src/sdl_img.c -o sdl_img `sdl2-config --libs --cflags` `curl-config --libs` -lm -Wall
