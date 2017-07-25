
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



typedef struct img_state
{
	unsigned char* pixels;
	int w;
	int h;

	

	int index;

	int frame_i;
	int delay; // for now just use the same delay for every frame
	int frames;
	int frame_capacity;
	int frame_timer;
	
	SDL_Texture** tex;

	SDL_Rect scr_rect;  // rect describing available space
	SDL_Rect disp_rect; // rect image is actually rendered to
} img_state;

typedef struct global_state
{
	SDL_Window* win;
	SDL_Renderer* ren;

	int scr_w;
	int scr_h;

	img_state* img_focus;
	int n_imgs;
	img_state img[8];


	cvector_str files;

	int fullscreen;
	int mouse_timer;
	int mouse_state;

} global_state;

global_state gs = { 0 };


void setup(const char* img_name);
void cleanup(int ret);
int load_image(const char* path, img_state* img);
int handle_events();

//TODO combine/reorg
void adjust_rect(img_state* img, int w, int h);
void set_rect_bestfit(img_state* img, int fill_screen);
void set_rect_zoom(img_state* img, int zoom);







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


	if (!load_image(path, &gs.img[0])) {
		cleanup(0);
	}

	gs.n_imgs = 1;
	

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

		// if it's a regular file and an image stb_image recognizes
		if (S_ISREG(file_stat.st_mode) && stbi_info(fullpath, NULL, NULL, NULL)) {
			cvec_push_str(&gs.files, fullpath);
		}
	}
	closedir(dir);

	qsort(gs.files.a, gs.files.size, sizeof(char*), cmp_string_lt);

	for (int i=0; i<gs.files.size; ++i) {
		// find starting image index
		if (!strcmp(gs.files.a[i], path))
			gs.img[0].index = i;
	}



	set_rect_bestfit(&gs.img[0], 0);

	int status;
	int ticks;
	while (1) {
		status = handle_events();
		if (status == QUIT) {
			break;
		}

		ticks = SDL_GetTicks();

		if (gs.mouse_state && ticks - gs.mouse_timer > 5000) {
			SDL_ShowCursor(SDL_DISABLE);
			gs.mouse_state = 0;
		}


		// for each image
		for (int i=0; i<gs.n_imgs; ++i) {
			if (gs.img[i].frames > 1 && ticks - gs.img[i].frame_timer > gs.img[i].delay) {
				gs.img[i].frame_i = (gs.img[i].frame_i + 1) % gs.img[i].frames;
				gs.img[i].frame_timer = ticks; // should be set after present ...
			//	printf("frame %d %d\n", gs.img[i].frame_i, gs.img[i].delay);
				status = REDRAW;
			}
		}

		if (status == REDRAW) {
			// clear whole screen
			SDL_RenderSetClipRect(gs.ren, NULL);
			SDL_RenderClear(gs.ren);
			// for each img
			for (int i=0; i<gs.n_imgs; ++i) {
				SDL_RenderSetClipRect(gs.ren, &gs.img[i].scr_rect);
				SDL_RenderCopy(gs.ren, gs.img[i].tex[gs.img[i].frame_i], NULL, &gs.img[i].disp_rect);
			}
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
void adjust_rect(img_state* img, int w, int h)
{
	img->disp_rect.x = img->scr_rect.x + (img->scr_rect.w-w)/2;
	img->disp_rect.y = img->scr_rect.y + (img->scr_rect.h-h)/2;
	img->disp_rect.w = w;
	img->disp_rect.h = h;

}


void set_rect_bestfit(img_state* img, int fill_screen)
{
	float aspect = img->w/(float)img->h;
	int h, w;
	
	// TODO macro evaluates division twice
	int tmp = MIN(img->scr_rect.h, img->scr_rect.w/aspect);
	if (fill_screen)
		h = tmp;
	else
		h = MIN(tmp, img->h); //show actual size if smaller than viewport

	w = h * aspect;

	adjust_rect(img, w, h);
}


void set_rect_zoom(img_state* img, int zoom)
{
	float aspect = img->w/(float)img->h;
	int h, w;
	
	h = img->disp_rect.h * (1.0 + zoom*0.05);
	w = h * aspect;

	adjust_rect(img, w, h);
}



int load_image(const char* path, img_state* img)
{
	int frames, n;

	//clean up previous image if any
	stbi_image_free(img->pixels);
	for (int i=0; i<img->frames; ++i) {
		SDL_DestroyTexture(img->tex[i]);
	}

	//img = stbi_load(path, &img->w, &img->h, &n, 4);
	img->pixels = stbi_xload(path, &img->w, &img->h, &n, 4, &frames);
	if (!img->pixels) {
		//printf("failed to load %s: %s\n", path, stbi_failure_reason());
		return 0;
	}

	if (frames > img->frame_capacity) {
		img->tex = realloc(img->tex, frames*sizeof(SDL_Texture*));
		img->frame_capacity = frames;
	}

	size_t size = img->w * img->h * 4;

	for (int i=0; i<frames; ++i) {
		img->tex[i] = SDL_CreateTexture(gs.ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, img->w, img->h);
		if (!img->tex[i]) {
			printf("Error: %s\n", SDL_GetError());
			return 0;
		}
		if (SDL_UpdateTexture(img->tex[i], NULL, img->pixels+(size+2)*i, img->w*4)) {
			printf("Error updating texture: %s\n", SDL_GetError());
			return 0;
		}
	}

	//gif delay is in 100ths, ticks are 1000ths
	//assume that the delay is the same for all frames (never seen anything else anyway)
	//and if delay is 0, default to 10 fps
	if (frames > 1) {
		img->delay = *(short*)(&img->pixels[size]) * 10;
		if (!img->delay)
			img->delay = 100;
	}

	img->frame_i = 0;
	img->frames = frames;


	return 1;
}

void setup(const char* img_name)
{
	gs.win = NULL;
	gs.ren = NULL;

	memset(&gs.img[0], 0, sizeof(img_state));

	gs.img[0].tex = malloc(100*sizeof(SDL_Texture*));
	gs.img[0].frame_capacity = 100;

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
	int tmp;
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


			case SDL_SCANCODE_0:
				gs.img_focus = NULL;
				SDL_SetWindowTitle(gs.win, basename(gs.files.a[gs.img[0].index], title_buf));
				break;
			case SDL_SCANCODE_1:
				gs.img_focus = &gs.img[0];
				SDL_SetWindowTitle(gs.win, basename(gs.files.a[gs.img_focus->index], title_buf));
				break;
			case SDL_SCANCODE_2:
				status = REDRAW;
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					if (!gs.img[1].pixels) {
						gs.img[1].index = gs.img[0].index;
						do {
							gs.img[1].index = (gs.img[1].index + 1) % gs.files.size;
						} while (!(ret = load_image(gs.files.a[gs.img[1].index], &gs.img[1])));
					}
					gs.img[0].scr_rect.x = 0;
					gs.img[0].scr_rect.y = 0;
					gs.img[0].scr_rect.w = gs.scr_w/2;
					gs.img[0].scr_rect.h = gs.scr_h;
					gs.img[1].scr_rect.x = gs.scr_w/2;
					gs.img[1].scr_rect.y = 0;
					gs.img[1].scr_rect.w = gs.scr_w/2;
					gs.img[1].scr_rect.h = gs.scr_h;

					
					set_rect_bestfit(&gs.img[0], gs.fullscreen);
					set_rect_bestfit(&gs.img[1], gs.fullscreen);

					gs.n_imgs = 2;
					gs.img_focus = NULL;

				} else if (gs.n_imgs >= 2) {
					gs.img_focus = &gs.img[1];
					SDL_SetWindowTitle(gs.win, basename(gs.files.a[gs.img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_4:
			case SDL_SCANCODE_8:
				break;

			case SDL_SCANCODE_A:
				status = REDRAW;
				// set image to actual size
				if (!gs.img_focus) {
					for (int i=0; i<gs.n_imgs; ++i) {
						adjust_rect(&gs.img[i], gs.img[i].w, gs.img[i].h);
					}
				} else {
					adjust_rect(gs.img_focus, gs.img_focus->w, gs.img_focus->h);
				}
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
					if (!gs.img_focus) {
						for (int i=0; i<gs.n_imgs; ++i)
							set_rect_bestfit(&gs.img[i], 1);
					} else {
						set_rect_bestfit(gs.img_focus, 1);
					}
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

			// TODO add moving image around when larger than screen
			// and zooming in/out on non-center, recentering if it gets smaller
			// than the display area again
			case SDL_SCANCODE_RIGHT:
			case SDL_SCANCODE_DOWN:
				status = REDRAW;

				if (!gs.img_focus) {
					for (int i=0; i<gs.n_imgs; ++i) {
						do {
							gs.img[i].index = (gs.img[i].index + gs.n_imgs) % gs.files.size;
						} while (!(ret = load_image(gs.files.a[gs.img[i].index], &gs.img[i])));
						set_rect_bestfit(&gs.img[i], gs.fullscreen);
					}
					// just set title to upper left image when !img_focus
					SDL_SetWindowTitle(gs.win, basename(gs.files.a[gs.img[0].index], title_buf));

				} else {
					do {
						gs.img_focus->index = (gs.img_focus->index + 1) % gs.files.size;
					} while (!(ret = load_image(gs.files.a[gs.img_focus->index], gs.img_focus)));
					set_rect_bestfit(gs.img_focus, gs.fullscreen);
					SDL_SetWindowTitle(gs.win, basename(gs.files.a[gs.img_focus->index], title_buf));
				}

				break;

			case SDL_SCANCODE_LEFT:
			case SDL_SCANCODE_UP:
				status = REDRAW;

				// TODO reuse imgs
				if (!gs.img_focus) {
					for (int i=0; i<gs.n_imgs; ++i) {
						do {
							tmp = gs.img[i].index - gs.n_imgs;
							gs.img[i].index = (tmp < 0) ? gs.files.size+tmp : tmp;
						} while (!(ret = load_image(gs.files.a[gs.img[i].index], &gs.img[i])));

						set_rect_bestfit(&gs.img[i], gs.fullscreen);
					}

					// just set title to upper left image when !img_focus
					SDL_SetWindowTitle(gs.win, basename(gs.files.a[gs.img[0].index], title_buf));
				} else {
					do {
						gs.img_focus->index = (gs.img_focus->index-1 < 0) ? gs.files.size-1 : gs.img_focus->index-1;
					} while (!(ret = load_image(gs.files.a[gs.img_focus->index], gs.img_focus)));
					set_rect_bestfit(gs.img_focus, gs.fullscreen);

					SDL_SetWindowTitle(gs.win, basename(gs.files.a[gs.img_focus->index], title_buf));
				}


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
			if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL) {
				if (!gs.img_focus) {
					for (int i=0; i<gs.n_imgs; ++i)
						set_rect_zoom(&gs.img[i], e.wheel.y);
				} else {
					set_rect_zoom(gs.img_focus, e.wheel.y);
				}
			} else {
				if (!gs.img_focus) {
					for (int i=0; i<gs.n_imgs; ++i)
						set_rect_zoom(&gs.img[i], -e.wheel.y);
				} else {
				set_rect_zoom(gs.img_focus, -e.wheel.y);
				}
			}
			break;

		case SDL_WINDOWEVENT:
			status = REDRAW;
			switch (e.window.event) {
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				//printf("resized %d x %d\n", e.window.data1, e.window.data2);
				gs.scr_w = e.window.data1;
				gs.scr_h = e.window.data2;

				// TODO how/where to reset all the "subscreens" rects
				if (gs.n_imgs == 1) {
					gs.img[0].scr_rect.x = 0;
					gs.img[0].scr_rect.y = 0;
					gs.img[0].scr_rect.w = gs.scr_w;
					gs.img[0].scr_rect.h = gs.scr_h;
					set_rect_bestfit(&gs.img[0], gs.fullscreen);
				} else if (gs.n_imgs == 2) {
					gs.img[0].scr_rect.x = 0;
					gs.img[0].scr_rect.y = 0;
					gs.img[0].scr_rect.w = gs.scr_w/2;
					gs.img[0].scr_rect.h = gs.scr_h;
					gs.img[1].scr_rect.x = gs.scr_w/2;
					gs.img[1].scr_rect.y = 0;
					gs.img[1].scr_rect.w = gs.scr_w/2;
					gs.img[1].scr_rect.h = gs.scr_h;

					
					set_rect_bestfit(&gs.img[0], gs.fullscreen);
					set_rect_bestfit(&gs.img[1], gs.fullscreen);
				}


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
	for (int i=0; i<gs.n_imgs; ++i) {
		stbi_image_free(gs.img[i].pixels);

		for (int j=0; j<gs.img[i].frames; ++j)
			SDL_DestroyTexture(gs.img[i].tex[j]);

		free(gs.img[i].tex);
	}

	cvec_free_str(&gs.files);

	SDL_DestroyRenderer(gs.ren);
	SDL_DestroyWindow(gs.win);

	SDL_Quit();

	exit(ret);
}
