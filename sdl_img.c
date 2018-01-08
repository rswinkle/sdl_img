
#define CVECTOR_IMPLEMENTATION
#include "cvector.h"

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//#include "c_utils.h"
#include "tinycthread.h"

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
	int looped;
	
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
	img_state* img;

	img_state img1[8];
	img_state img2[8];

	int status;

	char* dirpath;

	cvector_str files;

	int fullscreen;
	int mouse_timer;
	int mouse_state;

	int slideshow;
	int slide_timer;

	int loading;
	int done_loading;

} global_state;

global_state gs = { 0 };


void setup(const char* img_name);
void cleanup(int ret);
int load_image(const char* path, img_state* img);
int handle_events();
int load_image_names(DIR* dir, char* path, int milliseconds);

//TODO combine/reorg
void adjust_rect(img_state* img, int w, int h);
void set_rect_bestfit(img_state* img, int fill_screen);
void set_rect_zoom(img_state* img, int zoom);


void print_img_state(img_state* img);



#define PATH_SEPARATOR '/'


// works same as SUSv2 libgen.h dirname except that
// dirpath is user provided output buffer, assumed large
// enough, return value is dirpath
char* mydirname(const char* path, char* dirpath)
{
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
char* mybasename(const char* path, char* base)
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

	printf("path separator is %c\n", PATH_SEPARATOR);

	char* path = argv[1];
	char dirpath[4096] = { 0 };
	//char fullpath[4096] = { 0 };
	char img_name[1024] = { 0 };

	//normalize path (stupid windows)
	for (int i=0; i<strlen(path); ++i) {
		if (path[i] == '\\')
			path[i] = '/';
	}
	//mydirname
	mydirname(path, dirpath);
	mybasename(path, img_name);

	gs.dirpath = dirpath;
	gs.n_imgs = 1;

	setup(img_name);

	if (!load_image(img_name, &gs.img[0])) {
		cleanup(0);
	}

	set_rect_bestfit(&gs.img[0], 0);
	SDL_RenderSetClipRect(gs.ren, &gs.img[0].scr_rect);
	SDL_RenderCopy(gs.ren, gs.img[0].tex[gs.img[0].frame_i], NULL, &gs.img[0].disp_rect);
	SDL_RenderPresent(gs.ren);
	

	DIR* dir = opendir(dirpath);
	if (!dir) {
		perror("opendir");
		cleanup(1);
	}
	cvec_str(&gs.files, 0, 100);
	printf("reading file names\n");

	int done_loading = load_image_names(dir, img_name, 2000);

	printf("done with setup\n");

	int ticks;
	while (1) {
		if (handle_events())
			break;

		ticks = SDL_GetTicks();

		if (gs.mouse_state && ticks - gs.mouse_timer > 5000) {
			SDL_ShowCursor(SDL_DISABLE);
			gs.mouse_state = 0;
		}

		if (!done_loading) {
			done_loading = load_image_names(dir, NULL, 350);
		}


		// for each image
		for (int i=0; i<gs.n_imgs; ++i) {
			if (gs.img[i].frames > 1 && ticks - gs.img[i].frame_timer > gs.img[i].delay) {
				gs.img[i].frame_i = (gs.img[i].frame_i + 1) % gs.img[i].frames;
				if (gs.img[i].frame_i == 0)
					gs.img[i].looped = 1;
				gs.img[i].frame_timer = ticks; // should be set after present ...
				gs.status = REDRAW;
			}
		}

		if (gs.status == REDRAW) {
			// clear whole screen
			SDL_RenderSetClipRect(gs.ren, NULL);
			SDL_RenderClear(gs.ren);
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
	int x, y;
	SDL_GetMouseState(&x, &y);
	
	int l = img->scr_rect.x;
	int r = l + img->scr_rect.w;
	int t = img->scr_rect.y;
	int b = t + img->scr_rect.h;
	
	x = MAX(x, l);
	x = MIN(x, r);
	y = MAX(y, t);
	y = MIN(y, b);

	float px = (x-img->disp_rect.x)/(float)img->disp_rect.w;
	float py = (y-img->disp_rect.y)/(float)img->disp_rect.h;

	// There might be a slightly better way to organize/calculate this but this works
	int final_x, final_y;
	if ((gs.n_imgs == 1 || gs.img_focus == img) && w > img->scr_rect.w) {
		final_x = x-px*w;
		
		if (final_x + w < r)
			final_x += r-(final_x+w);
		if (final_x > l)
			final_x -= final_x-l;
	} else {
		final_x = img->scr_rect.x + (img->scr_rect.w-w)/2;
	}

	if ((gs.n_imgs == 1 || gs.img_focus == img) && h > img->scr_rect.h) {
		final_y = y-py*h;

		if (final_y + h < b)
			final_y += b-(final_y+h);
		if (final_y > t)
			final_y -= final_y-t;
	} else {
		final_y = img->scr_rect.y + (img->scr_rect.h-h)/2;
	}
	img->disp_rect.x = final_x;
	img->disp_rect.y = final_y;
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

// don't waste space even when drag/panning
void fix_rect(img_state* img)
{
	int w = img->disp_rect.w;
	int h = img->disp_rect.h;
	int scr_right = img->scr_rect.x + img->scr_rect.w;
	int scr_bottom = img->scr_rect.y + img->scr_rect.h;
	if (w > img->scr_rect.w) {
		if (img->disp_rect.x > img->scr_rect.x) {
			img->disp_rect.x = img->scr_rect.x;
		} else if (img->disp_rect.x + w < scr_right) {
			img->disp_rect.x += scr_right - (img->disp_rect.x+w);
		}
	}
	if (h > img->scr_rect.h) {
		if (img->disp_rect.y > img->scr_rect.y) {
			img->disp_rect.y = img->scr_rect.y;
		} else if (img->disp_rect.y + h < scr_bottom) {
			img->disp_rect.y += scr_bottom - (img->disp_rect.y+h);
		}
	}
}


int load_image(const char* img_name, img_state* img)
{
	int frames, n;

	//clean up previous image if any
	for (int i=0; i<img->frames; ++i) {
		SDL_DestroyTexture(img->tex[i]);
	}

	char fullpath[4096];

	int ret = snprintf(fullpath, 4096, "%s/%s", gs.dirpath, img_name);
	if (ret >= 4096) {
		printf("path too long\n");
		cleanup(0);
	}

	printf("loading %s\n", fullpath);
	printf("%p\n", img);

	//img = stbi_load(path, &img->w, &img->h, &n, 4);
	img->pixels = stbi_xload(fullpath, &img->w, &img->h, &n, 4, &frames);
	if (!img->pixels) {
		printf("failed to load %s: %s\n", fullpath, stbi_failure_reason());
		return 0;
	}

	if (frames > img->frame_capacity) {
		// img->tex is either NULL or previously alloced
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
	
	img->looped = 1;
	if (frames > 1) {
		img->looped = 0;
		img->delay = *(short*)(&img->pixels[size]) * 10;
		if (!img->delay)
			img->delay = 100;
		printf("%d frames %d delay\n", frames, img->delay);
	}

	img->frame_i = 0;
	img->frames = frames;

	// don't need the pixels anymore (don't plan on adding editting features)
	stbi_image_free(img->pixels);
	img->pixels = NULL;

	puts("in load_image()");
	print_img_state(img);

	return 1;
}


int load_image_names(DIR* dir, char* path, int milliseconds)
{
	struct dirent* entry;
	struct stat file_stat;
	char fullpath[4096] = { 0 };
	int ret;

	char* names[8] = { 0 };
	if (path) {
		names[0] = path;
	} else {
		for (int i=0; i<gs.n_imgs; ++i) {
			names[i] = gs.files.a[gs.img[i].index];
			printf("names[%d] = %s\n", i, names[i]);
		}
	}

	int ticks, start;
	ticks = start = SDL_GetTicks();

	while (ticks - start < milliseconds && (entry = readdir(dir))) {
		ret = snprintf(fullpath, 4096, "%s/%s", gs.dirpath, entry->d_name);
		if (ret >= 4096) {
			printf("path too long\n");
			cleanup(0);
		}
		if (stat(fullpath, &file_stat)) {
			perror("stat");
			continue;
		}

		// if it's a regular file and an image stb_image recognizes
		if (S_ISREG(file_stat.st_mode) && stbi_info(fullpath, NULL, NULL, NULL) && strcmp(entry->d_name, names[0])) {
			cvec_push_str(&gs.files, entry->d_name);
		}

		ticks = SDL_GetTicks();
	}

	// could do it manually since I maintain sorted
	//printf("sorting images\n");
	qsort(gs.files.a, gs.files.size, sizeof(char*), cmp_string_lt);

	printf("finding current images\n");
	int img;
	for (int i=0; i<gs.n_imgs; ++i) {
		for (img=0; img<gs.files.size; ++img) {
			// find starting image index
			if ((ret = strcmp(gs.files.a[img], names[i])) >= 0) {
				if (ret > 0) {
					puts("pushing current image");
					cvec_insert_str(&gs.files, img, names[i]);
				}
				gs.img[i].index = img;
				break;
			}
		}
	}

	if (!entry) {
		printf("Loaded all %zd filenames\n", gs.files.size);
		closedir(dir);
		return 1;
	} else {
		printf("loaded %zd filenames\n", gs.files.size);
	}


	return 0;
}


void setup(const char* img_name)
{
	gs.win = NULL;
	gs.ren = NULL;

	char error_str[1024] = { 0 };

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	//not sure if necessary
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		snprintf(error_str, 1024, "Couldn't initialize SDL: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, gs.win);
		puts(error_str);
		exit(1);
	}

	gs.scr_w = 640;
	gs.scr_h = 480;
	
	gs.win = SDL_CreateWindow(img_name, 100, 100, gs.scr_w, gs.scr_h, SDL_WINDOW_RESIZABLE);
	if (!gs.win) {
		snprintf(error_str, 1024, "Couldn't create window: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, gs.win);
		puts(error_str);
		exit(1);
	}

	/*
	gs.ren = SDL_CreateRenderer(gs.win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!gs.ren) {
		snprintf(error_str, 1024, "%s, falling back to software renderer.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning: No HW Acceleration", error_str, gs.win);
		puts(error_str);
	}
	*/

	gs.ren = SDL_CreateRenderer(gs.win, -1, SDL_RENDERER_SOFTWARE);
	if (!gs.ren) {
		snprintf(error_str, 1024, "Software rendering failed: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, gs.win);
		puts(error_str);
		cleanup(1);
	}
	SDL_SetRenderDrawColor(gs.ren, 0, 0, 0, 255);
	// get black screen while loading (big gif could take a few seconds)
	SDL_RenderClear(gs.ren);
	SDL_RenderPresent(gs.ren);

	gs.img = gs.img1;
	// all of gs is initialized to 0 on startup

	gs.img[0].tex = malloc(100*sizeof(SDL_Texture*));
	gs.img[0].frame_capacity = 100;

	gs.img[0].scr_rect.x = 0;
	gs.img[0].scr_rect.y = 0;
	gs.img[0].scr_rect.w = gs.scr_w;
	gs.img[0].scr_rect.h = gs.scr_h;

	gs.fullscreen = 0;

}



void clear_img(img_state* img)
{
	//stbi_image_free(img->pixels);
	for (int i=0; i<img->frames; ++i) {
		SDL_DestroyTexture(img->tex[i]);
	}

	//could clear everything else but these are the important
	//ones logic is based on
	//img->pixels = NULL;
	img->frames = 0;
}



int load_new_images(void* right_or_down)
{
	int is_right = *(int*)right_or_down;
	int tmp;

	char title_buf[1024];
	int ret;

	img_state* img;
	if (gs.img == gs.img1)
		img = gs.img2;
	else
		img = gs.img1;

	for (int i=0; i<gs.n_imgs; ++i)
		img[i].scr_rect = gs.img[i].scr_rect;

	if (!gs.img_focus) {
		for (int i=0; i<gs.n_imgs; ++i) {
			do {
				if (is_right) {
					img[i].index = (gs.img[i].index + gs.n_imgs) % gs.files.size;
				} else {
					tmp = gs.img[i].index - gs.n_imgs;
					img[i].index = (tmp < 0) ? gs.files.size+tmp : tmp;
				}
			} while (!(ret = load_image(gs.files.a[img[i].index], &img[i])));
			set_rect_bestfit(&img[i], gs.fullscreen);
		}
		// just set title to upper left image when !img_focus
		SDL_SetWindowTitle(gs.win, mybasename(gs.files.a[img[0].index], title_buf));

	} else {
		// TODO how to handle?
		do {
			if (is_right)
				img[0].index = (gs.img_focus->index + 1) % gs.files.size;
			else
				img[0].index = (gs.img_focus->index-1 < 0) ? gs.files.size-1 : gs.img_focus->index-1;
		} while (!(ret = load_image(gs.files.a[img[0].index], &img[0])));
		img[0].scr_rect = gs.img_focus->scr_rect;
		set_rect_bestfit(&img[0], gs.fullscreen);
		SDL_SetWindowTitle(gs.win, mybasename(gs.files.a[img[0].index], title_buf));
	}

	gs.loading = 0;
	gs.done_loading = 1;
	return 0;
}



void toggle_fullscreen()
{
	gs.status = REDRAW;
	if (gs.fullscreen) {
		SDL_SetWindowFullscreen(gs.win, 0);
		gs.fullscreen = 0;
	} else {
		SDL_SetWindowFullscreen(gs.win, SDL_WINDOW_FULLSCREEN_DESKTOP);
		gs.fullscreen = 1;
	}
}

int handle_events()
{
	SDL_Event e;
	int sc;
	int ret;
	int tmp;
	int panned;
	int right_or_down;
	char title_buf[1024];
	img_state tmp_img = { 0 };
	img_state* img;

	thrd_t loading_thrd;
	SDL_Texture** tmptex;

	gs.status = NOCHANGE;

	SDL_Keymod mod_state = SDL_GetModState();

	SDL_Event right;
	right.type = SDL_KEYDOWN;
	right.key.keysym.scancode = SDL_SCANCODE_RIGHT;

	if (gs.done_loading) {
		img = (gs.img == gs.img1) ? gs.img2 : gs.img1;
		if (gs.img_focus) {
			clear_img(gs.img_focus);
			tmptex = gs.img_focus->tex;
			tmp = gs.img_focus->frame_capacity;
			memcpy(gs.img_focus, &img[0], sizeof(img_state));
			img[0].tex = tmptex;
			img[0].frame_capacity = tmp;
			img[0].frames = 0;
		} else {
			for (int i=0; i<gs.n_imgs; ++i)
				clear_img(&gs.img[i]);
			gs.img = img;
		}
		gs.done_loading = 0;
		gs.status = REDRAW;
	}

	int ticks = SDL_GetTicks();
	int set_slide_timer = 0;

	if (!gs.loading && gs.slideshow && ticks - gs.slide_timer > gs.slideshow) {
		int i;
		// make sure all current gifs have gotten to the end
		// at least once
		for (i=0; i<gs.n_imgs; ++i) {
			if (!gs.img[i].looped)
				break;
		}
		if (i == gs.n_imgs) {
			SDL_PushEvent(&right);
			set_slide_timer = 1;
		}
	}

	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			return 1;
		case SDL_KEYUP:
			sc = e.key.keysym.scancode;
			switch (sc) {
			case SDL_SCANCODE_ESCAPE:
				if (!gs.fullscreen && !gs.slideshow) {
					return 1;
				} else {
					if (gs.slideshow) {
						gs.slideshow = 0;
					} else if (gs.fullscreen) {
						gs.status = REDRAW;
						SDL_SetWindowFullscreen(gs.win, 0);
						gs.fullscreen = 0;
					}
				}
				break;

			// CAPSLOCK comes right before F1 and F1-F12 are contiguous
			case SDL_SCANCODE_F1:
			case SDL_SCANCODE_F2:
			case SDL_SCANCODE_F3:
			case SDL_SCANCODE_F4:
			case SDL_SCANCODE_F5:
			case SDL_SCANCODE_F6:
			case SDL_SCANCODE_F7:
			case SDL_SCANCODE_F8:
			case SDL_SCANCODE_F9:
			case SDL_SCANCODE_F10:
				gs.slideshow = (sc - SDL_SCANCODE_CAPSLOCK)*1000;
				set_slide_timer = 1;
				break;

			case SDL_SCANCODE_F11:
				toggle_fullscreen();
				break;

			case SDL_SCANCODE_0:
				gs.img_focus = NULL;
				SDL_SetWindowTitle(gs.win, mybasename(gs.files.a[gs.img[0].index], title_buf));
				break;
			case SDL_SCANCODE_1:
				if (gs.n_imgs != 1 && mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					gs.status = REDRAW;
					set_slide_timer = 1;
					// TODO refactor into function?  don't free everything everytime
					// make load_image smarter
					if (gs.img_focus && gs.img_focus != &gs.img[0]) {
						for (int i=0; i<gs.n_imgs; ++i) {
							if (gs.img_focus == &gs.img[i]) {
								continue;
							}

							clear_img(&gs.img[i]);
						}
						gs.img[0] = *gs.img_focus;
						memset(gs.img_focus, 0, sizeof(img_state));

					} else {
						for (int i=1; i<gs.n_imgs; ++i) {
							clear_img(&gs.img[i]);
						}
					}

					gs.img[0].scr_rect.x = 0;
					gs.img[0].scr_rect.y = 0;
					gs.img[0].scr_rect.w = gs.scr_w;
					gs.img[0].scr_rect.h = gs.scr_h;
					
					set_rect_bestfit(&gs.img[0], gs.fullscreen);

					gs.n_imgs = 1;
					gs.img_focus = NULL;

				} else if (gs.n_imgs >= 2) {
					gs.img_focus = &gs.img[0];
					SDL_SetWindowTitle(gs.win, mybasename(gs.files.a[gs.img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_2:
				gs.status = REDRAW;
				set_slide_timer = 1;
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					if (gs.n_imgs != 2 && gs.files.size >= 2) {

						// TODO hmm
						if (gs.n_imgs == 1) {
							gs.img[1].index = gs.img[0].index;
							do {
								gs.img[1].index = (gs.img[1].index + 1) % gs.files.size;
							} while (!(ret = load_image(gs.files.a[gs.img[1].index], &gs.img[1])));
						} else {
							for (int i=gs.n_imgs-1; i>1; --i)
								clear_img(&gs.img[i]);
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
					}

				} else if (gs.n_imgs >= 2) {
					gs.img_focus = &gs.img[1];
					SDL_SetWindowTitle(gs.win, mybasename(gs.files.a[gs.img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_3:
				gs.status = REDRAW;
				if (gs.n_imgs >= 3) {
					gs.img_focus = &gs.img[2];
					SDL_SetWindowTitle(gs.win, mybasename(gs.files.a[gs.img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_4:
				gs.status = REDRAW;
				set_slide_timer = 1;
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					if (gs.n_imgs != 4 && gs.files.size >= 4) {
						
						for (int i=gs.n_imgs; i<4; ++i) {
							gs.img[i].index = gs.img[i-1].index;
							do {
								gs.img[i].index = (gs.img[i].index + 1) % gs.files.size;
							} while (!(ret = load_image(gs.files.a[gs.img[i].index], &gs.img[i])));
						}
						for (int i=gs.n_imgs-1; i>3; --i) {
							clear_img(&gs.img[i]);
						}

						for (int i=0; i<4; ++i) {
							gs.img[i].scr_rect.x = (i%2)*gs.scr_w/2;
							gs.img[i].scr_rect.y = (i/2)*gs.scr_h/2;
							gs.img[i].scr_rect.w = gs.scr_w/2;
							gs.img[i].scr_rect.h = gs.scr_h/2;
							set_rect_bestfit(&gs.img[i], gs.fullscreen);
						}

						gs.n_imgs = 4;
						gs.img_focus = NULL;
					}

				} else if (gs.n_imgs >= 4) {
					gs.img_focus = &gs.img[3];
					SDL_SetWindowTitle(gs.win, mybasename(gs.files.a[gs.img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_5:
				gs.status = REDRAW;
				if (gs.n_imgs >= 5) {
					gs.img_focus = &gs.img[4];
					SDL_SetWindowTitle(gs.win, mybasename(gs.files.a[gs.img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_6:
				gs.status = REDRAW;
				if (gs.n_imgs >= 6) {
					gs.img_focus = &gs.img[5];
					SDL_SetWindowTitle(gs.win, mybasename(gs.files.a[gs.img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_7:
				gs.status = REDRAW;
				if (gs.n_imgs >= 7) {
					gs.img_focus = &gs.img[6];
					SDL_SetWindowTitle(gs.win, mybasename(gs.files.a[gs.img_focus->index], title_buf));
				}
				break;
			case SDL_SCANCODE_8:
				gs.status = REDRAW;
				set_slide_timer = 1;
				if (mod_state & (KMOD_LCTRL | KMOD_RCTRL)) {
					if (gs.n_imgs != 8 && gs.files.size >= 8) {
						
						for (int i=gs.n_imgs; i<8; ++i) {
							gs.img[i].index = gs.img[i-1].index;
							do {
								gs.img[i].index = (gs.img[i].index + 1) % gs.files.size;
							} while (!(ret = load_image(gs.files.a[gs.img[i].index], &gs.img[i])));
						}
						// This loop will never run unless I add a higher number somehow like 12 or 16
						for (int i=gs.n_imgs-1; i>7; --i) {
							clear_img(&gs.img[i]);
						}

						for (int i=0; i<8; ++i) {
							gs.img[i].scr_rect.x = (i%4)*gs.scr_w/4;
							gs.img[i].scr_rect.y = (i/4)*gs.scr_h/2;
							gs.img[i].scr_rect.w = gs.scr_w/4;
							gs.img[i].scr_rect.h = gs.scr_h/2;
							set_rect_bestfit(&gs.img[i], gs.fullscreen);
						}

						gs.n_imgs = 8;
						gs.img_focus = NULL;
					}

				} else if (gs.n_imgs >= 8) {
					gs.img_focus = &gs.img[7];
					SDL_SetWindowTitle(gs.win, mybasename(gs.files.a[gs.img_focus->index], title_buf));
				}
				break;

			case SDL_SCANCODE_A:
				gs.status = REDRAW;
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
				gs.status = REDRAW;
				if (mod_state & (KMOD_LALT | KMOD_RALT)) {
					toggle_fullscreen();
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
		}
			break;  //end SDL_KEYUP

		case SDL_KEYDOWN:
			// TODO use symcodes?
			sc = e.key.keysym.scancode;
			switch (sc) {

			case SDL_SCANCODE_RIGHT:
				panned = 0;
				gs.status = REDRAW;
				if (gs.loading || !(mod_state & (KMOD_LALT | KMOD_RALT))) {
					if (!gs.img_focus) {
						for (int i=0; i<gs.n_imgs; ++i) {
							img = &gs.img[i];
							if (img->disp_rect.w > img->scr_rect.w) {
								img->disp_rect.x -= 0.05 * img->disp_rect.w;
								fix_rect(img);
								panned = 1;
							}
						}
					} else {
						img = gs.img_focus;
						if (img->disp_rect.w > img->scr_rect.w) {
							img->disp_rect.x -= 0.05 * img->disp_rect.w;
							fix_rect(img);
							panned = 1;
						}
					}
				}
				if (!gs.loading && !panned) {
					set_slide_timer = 1;
					right_or_down = 1;

					gs.loading = 1;
					gs.done_loading = 0;
					if (thrd_success != thrd_create(&loading_thrd, load_new_images, &right_or_down)) {
						puts("couldn't create thread");
					}
					//load_new_images(&right_or_down);
				}
				break;
			case SDL_SCANCODE_DOWN:
				panned = 0;
				gs.status = REDRAW;
				if (gs.loading || !(mod_state & (KMOD_LALT | KMOD_RALT))) {
					if (!gs.img_focus) {
						for (int i=0; i<gs.n_imgs; ++i) {
							img = &gs.img[i];
							if (img->disp_rect.h > img->scr_rect.h) {
								img->disp_rect.y -= 0.05 * img->disp_rect.h;
								fix_rect(img);
								panned = 1;
							}
						}
					} else {
						img = gs.img_focus;
						if (img->disp_rect.h > img->scr_rect.h) {
							img->disp_rect.y -= 0.05 * img->disp_rect.h;
							fix_rect(img);
							panned = 1;
						}
					}
				}
				if (!gs.loading && !panned) {
					set_slide_timer = 1;
					right_or_down = 1;
					gs.loading = 1;
					gs.done_loading = 0;
					if (thrd_success != thrd_create(&loading_thrd, load_new_images, &right_or_down)) {
						puts("couldn't create thread");
					}
					//load_new_images(&right_or_down);
				}
				break;

			case SDL_SCANCODE_LEFT:
				panned = 0;
				gs.status = REDRAW;
				if (gs.loading || !(mod_state & (KMOD_LALT | KMOD_RALT))) {
					if (!gs.img_focus) {
						for (int i=0; i<gs.n_imgs; ++i) {
							img = &gs.img[i];
							if (img->disp_rect.w > img->scr_rect.w) {
								img->disp_rect.x += 0.05 * img->disp_rect.w;
								fix_rect(img);
								panned = 1;
							}
						}
					} else {
						img = gs.img_focus;
						if (img->disp_rect.w > img->scr_rect.w) {
							img->disp_rect.x += 0.05 * img->disp_rect.w;
							fix_rect(img);
							panned = 1;
						}
					}
				}
				if (!gs.loading && !panned) {
					set_slide_timer = 1;
					right_or_down = 0;
					gs.loading = 1;
					gs.done_loading = 0;
					if (thrd_success != thrd_create(&loading_thrd, load_new_images, &right_or_down)) {
						puts("couldn't create thread");
					}
					//load_new_images(&right_or_down);
				}
				break;
			case SDL_SCANCODE_UP:
				panned = 0;
				gs.status = REDRAW;
				if (gs.loading || !(mod_state & (KMOD_LALT | KMOD_RALT))) {
					if (!gs.img_focus) {
						for (int i=0; i<gs.n_imgs; ++i) {
							img = &gs.img[i];
							if (img->disp_rect.h > img->scr_rect.h) {
								img->disp_rect.y += 0.05 * img->disp_rect.h;
								fix_rect(img);
								panned = 1;
							}
						}
					} else {
						img = gs.img_focus;
						if (img->disp_rect.h > img->scr_rect.h) {
							img->disp_rect.y += 0.05 * img->disp_rect.h;
							fix_rect(img);
							panned = 1;
						}
					}
				}
				if (!gs.loading && !panned) {
					set_slide_timer = 1;
					right_or_down = 0;
					gs.loading = 1;
					gs.done_loading = 0;
					if (thrd_success != thrd_create(&loading_thrd, load_new_images, &right_or_down)) {
						puts("couldn't create thread");
					}
					//load_new_images(&right_or_down);
				}
				break;

			case SDL_SCANCODE_MINUS:
				gs.status = REDRAW;
				if (!gs.img_focus) {
					for (int i=0; i<gs.n_imgs; ++i)
						set_rect_zoom(&gs.img[i], -1);
				} else {
					set_rect_zoom(gs.img_focus, -1);
				}
				break;
			case SDL_SCANCODE_EQUALS:
				gs.status = REDRAW;
				if (!gs.img_focus) {
					for (int i=0; i<gs.n_imgs; ++i)
						set_rect_zoom(&gs.img[i], 1);
				} else {
					set_rect_zoom(gs.img_focus, 1);
				}
				break;



			default:
				;
			}

			break;

		case SDL_MOUSEMOTION:
			if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
				img = NULL;
				if (!gs.img_focus) {
					for (int i=0; i<gs.n_imgs; ++i) {
						img = &gs.img[i];
						if (e.motion.xrel != 0 && img->disp_rect.w > img->scr_rect.w) {
							img->disp_rect.x += e.motion.xrel;
						}
						if (e.motion.yrel != 0 && img->disp_rect.h > img->scr_rect.h) {
							img->disp_rect.y += e.motion.yrel;
						}
						fix_rect(img);
					}
				} else {
					img = gs.img_focus;
					if (e.motion.xrel != 0 && img->disp_rect.w > img->scr_rect.w) {
						img->disp_rect.x += e.motion.xrel;
					}
					if (e.motion.yrel != 0 && img->disp_rect.h > img->scr_rect.h) {
						img->disp_rect.y += e.motion.yrel;
					}
					fix_rect(img);
				}
				gs.status = REDRAW;
			}
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			SDL_ShowCursor(SDL_ENABLE);
			gs.mouse_timer = SDL_GetTicks();
			gs.mouse_state = 1;
			break;

		case SDL_MOUSEWHEEL:
			gs.status = REDRAW;
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
			gs.status = REDRAW;
			switch (e.window.event) {
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				printf("resized %d x %d\n", e.window.data1, e.window.data2);
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
				} else if (gs.n_imgs == 4) {
					for (int i=0; i<4; ++i) {
						gs.img[i].scr_rect.x = (i%2)*gs.scr_w/2;
						gs.img[i].scr_rect.y = (i/2)*gs.scr_h/2;
						gs.img[i].scr_rect.w = gs.scr_w/2;
						gs.img[i].scr_rect.h = gs.scr_h/2;
						set_rect_bestfit(&gs.img[i], gs.fullscreen);
					}
				} else if (gs.n_imgs == 8) {
					for (int i=0; i<8; ++i) {
						gs.img[i].scr_rect.x = (i%4)*gs.scr_w/4;
						gs.img[i].scr_rect.y = (i/4)*gs.scr_h/2;
						gs.img[i].scr_rect.w = gs.scr_w/4;
						gs.img[i].scr_rect.h = gs.scr_h/2;
						set_rect_bestfit(&gs.img[i], gs.fullscreen);
					}
				}


				break;
			case SDL_WINDOWEVENT_EXPOSED: {
				int x, y;
				SDL_GetWindowSize(gs.win, &x, &y);
				printf("windowed %d %d %d %d\n", gs.scr_w, gs.scr_h, x, y);
				puts("exposed event");
			}
				break;
			default:
				;
			}
			break;
		}
	}

	if (set_slide_timer) {
		gs.slide_timer =  SDL_GetTicks();
	}

	return 0;
}


void print_img_state(img_state* img)
{
	printf("{\nimg = %p\n", img);
	printf("pixels = %p\n", img->pixels);
	printf("WxH = %dx%d\n", img->w, img->h);
	printf("index = %d\n", img->index);
	printf("frame_i = %d\ndelay = %d\nframes = %d\nframe_cap = %d\n", img->frame_i, img->delay, img->frames, img->frame_capacity);
	printf("frame_timer = %d\nlooped = %d\n", img->frame_timer, img->looped);

	printf("tex = %p\n", img->tex);
	for (int i=0; i<img->frames; ++i) {
		printf("tex[i] = %p\n", img->tex[i]);
	}

	printf("scr_rect = %d %d %d %d\n", img->scr_rect.x, img->scr_rect.y, img->scr_rect.w, img->scr_rect.h);
	printf("disp_rect = %d %d %d %d\n}\n", img->disp_rect.x, img->disp_rect.y, img->disp_rect.w, img->disp_rect.h);
}


void cleanup(int ret)
{
	for (int i=0; i<gs.n_imgs; ++i) {
		//stbi_image_free(gs.img[i].pixels);

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
