gcc -std=c99 -g -O0 sdl_img.c -o sdl_img `sdl2-config --libs` -lm -Wall
ldd sdl_img.exe | grep mingw64 | cut -d' ' -f3 | xargs -I{} cp {} .
