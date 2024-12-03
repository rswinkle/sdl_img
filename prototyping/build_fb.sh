OPTS=-fsanitize=address -fsanitize=undefined -std=gnu99 -g -O0 -Wall


${CC:=gcc} -std=gnu99 -g -O0 filebrowser.c c_utils.c -o fb_test -I../src
${CC:=gcc} ${OPTS} nuklear_filebrowser.c c_utils.c -o nuklear_fb_test -I../src `sdl2-config --cflags --libs` -lm
