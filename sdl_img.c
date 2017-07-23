
#define CVECTOR_IMPLEMENTATION
#include "cvector.h"

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//#include "c_utils.h"

#include <stdio.h>

//POSIX works with MinGW64
#include <sys/stat.h>
#include <dirent.h>


#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>



void setup(const char* img_name);
void cleanup(int ret);
int load_image(const char* path);
int handle_events();

//TODO combine/reorg
void adjust_rect();
void set_rect_actual();
void set_rect_bestfit();
void set_rect_zoom();


typedef struct global_state
{
	SDL_Window* win;
	SDL_Renderer* ren;
	SDL_Texture* tex;

	unsigned char* img;

	int scr_w;
	int scr_h;

	int img_w;
	int img_h;
	int img_index;

	SDL_Rect rect;

	cvector_str files;

	int fullscreen;
	int mouse_timer;
	int mouse_state;

} global_state;

global_state gs;






// works same as SUSv2 libgen.h dirname except that
// dirpath is user provided output buffer, assumed large
// enough, return value is dirpath
char* dirname(const char* path, char* dirpath)
{

#define PATH_SEPARATOR '/'

	if (!path || !path[0]) {
		dirpath[0] = '.';
		dirpath[1] = 0;
		return dirpath;
	}

	char* last_slash = strrchr(path, PATH_SEPARATOR);
	if (last_slash) {
		strncpy(dirpath, path, last_slash-path);
		puts("1");
	} else {
		dirpath[0] = '.';
		dirpath[1] = 0;
		puts("2");
	}

	return dirpath;
}

// same as SUSv2 basename in libgen.h except base is output

// buffer
char* basename(const char* path, char* base)
{
	if (!path || !path[0]) {
		base[0] = '.';
		base[1] = 0;
		return base;
	}

	int end = strlen(path) - 1;

	if (path[end] == PATH_SEPARATOR)
		end--;

	int start = end;
	while (path[start] != PATH_SEPARATOR && start != 0)
		start--;
	if (path[start] == PATH_SEPARATOR)
		start++;

	memcpy(base, &path[start], end-start+1);
	base[end-start+1] = 0;

	return base;
}


int cmp_string_lt(const void* a, const void* b)
{
	return strcmp(*(const char**)a, *(const char**)b);
}

enum { QUIT, REDRAW, NOCHANGE };



int main(int argc, char** argv)
{
	if (argc != 2) {
		printf("usage: %s image_name\n", argv[0]);
		exit(0);
	}

	char* path = argv[1];
	char dirpath[4096] = { 0 };
	char fullpath[4096] = { 0 };
	char img_name[1024] = { 0 };

	//dirname
	dirname(path, dirpath);
	basename(path, img_name);

	setup(img_name);


	if (!load_image(path)) {
		cleanup(0);
	}
	

	DIR* dir = opendir(dirpath);
	if (!dir) {
		perror("opendir");
		cleanup(1);
	}

	struct dirent* entry;
	struct stat file_stat;

	cvec_str(&gs.files, 0, 20);

	int ret;
	while ((entry = readdir(dir))) {
		ret = snprintf(fullpath, 4096, "%s/%s", dirpath, entry->d_name);
		if (ret >= 4096) {
			printf("path too long\n");
			cleanup(0);
		}
		if (stat(fullpath, &file_stat)) {
			perror("stat");
			continue;
		}
		if (S_ISREG(file_stat.st_mode)) {
			cvec_push_str(&gs.files, fullpath);
		}
	}
	closedir(dir);

	qsort(gs.files.a, gs.files.size, sizeof(char*), cmp_string_lt);

	for (int i=0; i<gs.files.size; ++i) {
		if (!strcmp(gs.files.a[i], path))
			gs.img_index = i;
	}

	set_rect_bestfit();

	int status;
	while (1) {
		status = handle_events();
		if (status == QUIT) {
			break;
		}

		if (gs.mouse_state && SDL_GetTicks() - gs.mouse_timer > 5000) {
			SDL_ShowCursor(SDL_DISABLE);
			gs.mouse_state = 0;
		}

		if (status == REDRAW) {
			SDL_RenderClear(gs.ren);
			SDL_RenderCopy(gs.ren, gs.tex, NULL, &gs.rect);
			SDL_RenderPresent(gs.ren);
		}
	}


	cleanup(0);

	//never get here
	return 0;
}

#define MAX(X, Y) ((X > Y) ? X : Y)
#define MIN(X, Y) ((X < Y) ? X : Y)



//need to think about best way to organize following 4 functions' functionality
void adjust_rect()
{
	gs.rect.x = (gs.scr_w-gs.rect.w)/2;
	gs.rect.y = (gs.scr_h-gs.rect.h)/2;
}


void set_rect_bestfit(int fill_screen)
{
	float aspect = gs.img_w/(float)gs.img_h;
	int h, w;
	
	int tmp = MIN(gs.scr_h, gs.scr_w/aspect);
	if (fill_screen)
		h = tmp;
	else
		h = MIN(tmp, gs.img_h); //show actual size if smaller than viewport

	w = h * aspect;

	gs.rect.x = (gs.scr_w-w)/2;
	gs.rect.y = (gs.scr_h-h)/2;
	gs.rect.w = w;
	gs.rect.h = h;
}

void set_rect_actual()
{
	gs.rect.x = (gs.scr_w-gs.img_w)/2;
	gs.rect.y = (gs.scr_h-gs.img_h)/2;
	gs.rect.w = gs.img_w;
	gs.rect.h = gs.img_h;
}

void set_rect_zoom(int zoom)
{
	float aspect = gs.img_w/(float)gs.img_h;
	int h, w;
	
	h = gs.rect.h * (1.0 + zoom*0.05);
	w = h * aspect;

	gs.rect.x = (gs.scr_w-w)/2;
	gs.rect.y = (gs.scr_h-h)/2;
	gs.rect.w = w;
	gs.rect.h = h;
}



int load_image(const char* path)
{
	int n;
	gs.img = stbi_load(path, &gs.img_w, &gs.img_h, &n, 4);
	if (!gs.img) {
		//printf("failed to load %s: %s\n", path, stbi_failure_reason());
		return 0;
	}

	gs.tex = SDL_CreateTexture(gs.ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, gs.img_w, gs.img_h);
	if (!gs.tex) {
		printf("Error: %s\n", SDL_GetError());
		return 0;
	}

	if (SDL_UpdateTexture(gs.tex, NULL, gs.img, gs.img_w*4)) {
		printf("Error updating texture: %s\n", SDL_GetError());
		return 0;
	}

	return 1;
}

void setup(const char* img_name)
{
	gs.win = NULL;
	gs.ren = NULL;
	gs.tex = NULL;

	gs.img = NULL;

	gs.fullscreen = 0;

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	//not sure if necessary
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		exit(1);
	}

	gs.scr_w = 640;
	gs.scr_h = 480;

	gs.win = SDL_CreateWindow(img_name, 100, 100, gs.scr_w, gs.scr_h, SDL_WINDOW_RESIZABLE);
	if (!gs.win) {
		printf("Error: %s\n", SDL_GetError());
		exit(1);
	}

#ifndef _WIN32
	gs.ren = SDL_CreateRenderer(gs.win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#else
	//windows seems to have trouble with RENDERER_ACCELERATED
	gs.ren = SDL_CreateRenderer(gs.win, -1, SDL_RENDERER_SOFTWARE);
#endif

	if (!gs.ren) {
		printf("Error: %s\n", SDL_GetError());
		cleanup(1);
	}
	SDL_SetRenderDrawColor(gs.ren, 0, 0, 0, 255);
}


int handle_events()
{
	SDL_Event e;
	int sc;
	int ret;
	char title_buf[1024];

	int status = NOCHANGE;

	SDL_Keymod mod_state = SDL_GetModState();

	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			return QUIT;
		case SDL_KEYDOWN:
			// TODO use symcodes?
			sc = e.key.keysym.scancode;
			switch (sc) {
			case SDL_SCANCODE_ESCAPE:
				if (gs.fullscreen) {
					status = REDRAW;
					SDL_SetWindowFullscreen(gs.win, 0);
					gs.fullscreen = 0;
				} else {
					return QUIT;
				}

			case SDL_SCANCODE_1:
				status = REDRAW;
				set_rect_actual();
				break;

			case SDL_SCANCODE_F: {
				status = REDRAW;
				if (mod_state & (KMOD_LALT | KMOD_RALT)) {
					if (gs.fullscreen) {
						SDL_SetWindowFullscreen(gs.win, 0);
						gs.fullscreen = 0;
					} else {
						SDL_SetWindowFullscreen(gs.win, SDL_WINDOW_FULLSCREEN_DESKTOP);
						gs.fullscreen = 1;
					}
				} else {
					set_rect_bestfit(1);
				}
			}
				break;

			case SDL_SCANCODE_F11:
				status = REDRAW;
				if (gs.fullscreen) {
					SDL_SetWindowFullscreen(gs.win, 0);
					gs.fullscreen = 0;
				} else {
					SDL_SetWindowFullscreen(gs.win, SDL_WINDOW_FULLSCREEN_DESKTOP);
					gs.fullscreen = 1;
				}
				break;

			case SDL_SCANCODE_RIGHT:
			case SDL_SCANCODE_DOWN:
				status = REDRAW;
				stbi_image_free(gs.img);
				SDL_DestroyTexture(gs.tex);

				do {
					gs.img_index = (gs.img_index + 1) % gs.files.size;
				} while (!(ret = load_image(gs.files.a[gs.img_index])));

				SDL_SetWindowTitle(gs.win, basename(gs.files.a[gs.img_index], title_buf));

				set_rect_bestfit(gs.fullscreen);
				break;

			case SDL_SCANCODE_LEFT:
			case SDL_SCANCODE_UP:
				status = REDRAW;
				stbi_image_free(gs.img);
				SDL_DestroyTexture(gs.tex);

				do {
					gs.img_index = (gs.img_index-1 < 0) ? gs.files.size-1 : gs.img_index-1;
				} while (!(ret = load_image(gs.files.a[gs.img_index])));

				SDL_SetWindowTitle(gs.win, basename(gs.files.a[gs.img_index], title_buf));

				set_rect_bestfit(gs.fullscreen);
				break;

			default:
				;
			}

			break;

		//TODO implement drag for when picture is zoomed in past edges
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION:
			SDL_ShowCursor(SDL_ENABLE);
			gs.mouse_timer = SDL_GetTicks();
			gs.mouse_state = 1;
			break;

		case SDL_MOUSEWHEEL:
			status = REDRAW;
			if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL)
				set_rect_zoom(e.wheel.y);
			else
				set_rect_zoom(-e.wheel.y);
			break;

		case SDL_WINDOWEVENT:
			switch (e.window.event) {
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				//printf("resized %d x %d\n", e.window.data1, e.window.data2);
				gs.scr_w = e.window.data1;
				gs.scr_h = e.window.data2;

				set_rect_bestfit(0);
				//adjust_rect();
				status = REDRAW;

				break;
			case SDL_WINDOWEVENT_EXPOSED:
				//puts("exposed event");
				break;
			default:
				;
			}
			break;
		}
	}
	return status;
}

void cleanup(int ret)
{
	stbi_image_free(gs.img);
	SDL_DestroyTexture(gs.tex);
	SDL_DestroyRenderer(gs.ren);
	SDL_DestroyWindow(gs.win);

	SDL_Quit();

	exit(ret);
}
