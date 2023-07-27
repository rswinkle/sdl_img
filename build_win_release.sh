${CC:=clang} -std=c99 -O3 src/sdl_img.c -o sdl_img.exe -DNDEBUG `pkg-config sdl2 --libs --cflags` `pkg-config libcurl --libs --cflags` -lm
ldd sdl_img.exe | grep mingw64 | cut -d' ' -f3 | xargs -I{} cp {} .

