${CC:=clang} -std=c99 -g -O0 src/sdl_img.c -o sdl_img.exe `pkg-config sdl2 --libs --cflags` `pkg-config libcurl --libs --cflags` -lm -Wall
ldd sdl_img.exe | grep mingw64 | cut -d' ' -f3 | xargs -I{} cp {} .
