${CC:=gcc} -std=gnu99 -O2 src/sdl_img.c -o sdl_img -DNDEBUG `sdl2-config --libs --cflags` `curl-config --libs --cflags` -lm
