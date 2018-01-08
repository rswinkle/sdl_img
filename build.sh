gcc -std=c99 -O2 sdl_img.c tinycthread.c -o sdl_img `sdl2-config --libs` -lm -pthread
