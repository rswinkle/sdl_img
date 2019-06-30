${CC:=clang} -std=c99 -O2 src/sdl_img.c -o sdl_img.exe -DNDEBUG `sdl2-config --libs --cflags` `curl-config --libs --cflags` -lSDL2_gfx -lm
ldd sdl_img.exe | grep mingw64 | cut -d' ' -f3 | xargs -I{} cp {} .

