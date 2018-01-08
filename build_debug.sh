gcc -std=c99 -g -O0 sdl_img.c tinycthread.c -o sdl_img `sdl2-config --libs` -lm -Wall -pthread
