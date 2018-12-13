
//#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

//POSIX works with MinGW64
#include <sys/stat.h>
#include <dirent.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>



#define MAX(a, b)  ((a) > (b)) ? (a) : (b)
#define MIN(a, b)  ((a) < (b)) ? (a) : (b)

typedef uint8_t u8;
typedef uint32_t u32;
typedef int16_t i16;
typedef int32_t i32;



int rotdegs;
int changed;



void rotate(char* image_name, int deg)
{
	int w, h, n, frames;

	u8* img = stbi_xload(image_name, &w, &h, &n, 4, &frames);

	int w2 = w/2;
	int h2 = h/2;
	int x, y, xout, yout;

	float rads = deg * (3.14159265f/180.0f);

	u8* outimg = calloc(w*h*4, 1);

	for (int i=0; i<h; ++i) {
		y = i - h2;
		for (int j=0; j<w; ++j) {
			x = j - w2;
			/*
			 * taking pixels of original image and rotating creates
			 * holes because of aliasing, straight lines become diagonal
			 * and thus require more pixels to fill
			xout = x * cos(rads) - y * sin(rads) + w2;
			yout = x * sin(rads) + y * cos(rads) + h2;

			if (xout >= 0 && xout < w && yout >= 0 && yout < h)
				memcpy(&outimg[(yout*w + xout)*4], &img[(i*w+j)*4], 4);
			*/

			// Going the other way, rotating backward to find which pixel
			// in the original image (if any) ends up there has no holes
			// because some pixels will be the source for multiple in the
			// rotated image
			xout = x * cos(-rads) - y * sin(-rads) + w2;
			yout = x * sin(-rads) + y * cos(-rads) + h2;

			if (xout >= 0 && xout < w && yout >= 0 && yout < h)
				memcpy(&outimg[(i*w + j)*4], &img[(yout*w+xout)*4], 4);

		}
	}


	char full_img_path[4096] = { 0 };
	strcpy(full_img_path, image_name);
	char* ext = strrchr(full_img_path, '.');
	*ext = 0;
	sprintf(ext, "_rotated_%d_degs.jpg", deg);

	stbi_write_jpg(full_img_path, w, h, 4, outimg, 100);

	free(outimg);
}

void rotate_scale_to_fit(char* image_name, int deg)
{
	int w, h, n, frames;

	u8* img = stbi_xload(image_name, &w, &h, &n, 4, &frames);

	int x, y, xout, yout;

	float rads = deg * (3.14159265f/180.0f);

	int w1 = w-1;
	int h1 = h-1;

	int x1 = w1 * cos(rads); // top right
	int y1 = w1 * sin(rads);

	int x2 = -h1 * sin(rads);   // bottom left
	int y2 = h1 * cos(rads);

	int x3 = w1 * cos(rads) - h1 * sin(rads);
	int y3 = w1 * sin(rads) + h1 * cos(rads);

	int xmin = MIN(0, MIN(x1, MIN(x2, x3)));
	int ymin = MIN(0, MIN(y1, MIN(y2, y3)));

	int xmax = MAX(0, MAX(x1, MAX(x2, x3)));
	int ymax = MAX(0, MAX(y1, MAX(y2, y3)));

	int roth = ymax-ymin;
	int rotw = xmax-xmin;

	u8* outimg = calloc(rotw*roth*4, 1);
	int h2 = roth/2;
	int w2 = rotw/2;

	int h2old = h/2;
	int w2old = w/2;

	for (int i=0; i<roth; ++i) {
		y = i - h2;
		for (int j=0; j<rotw; ++j) {
			x = j - w2;
			xout = x * cos(-rads) - y * sin(-rads) + w2old;
			yout = x * sin(-rads) + y * cos(-rads) + h2old;

			if (xout >= 0 && xout < w && yout >= 0 && yout < h)
				memcpy(&outimg[(i*rotw + j)*4], &img[(yout*w + xout)*4], 4);

		}
	}


	char full_img_path[4096] = { 0 };
	strcpy(full_img_path, image_name);
	char* ext = strrchr(full_img_path, '.');
	*ext = 0;
	sprintf(ext, "_rotated_%d_degs_scaled.jpg", deg);

	stbi_write_jpg(full_img_path, rotw, roth, 4, outimg, 100);

	free(outimg);
}

int handle_events()
{
	SDL_Event e;
	int sc;

	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			return 1;
		case SDL_KEYUP:
			sc = e.key.keysym.scancode;
			switch (sc) {
			case SDL_SCANCODE_ESCAPE:
				return 1;
				break;
			}
			break;
		case SDL_KEYDOWN:
			sc = e.key.keysym.scancode;
			switch (sc) {
			case SDL_SCANCODE_RIGHT:
				rotdegs++;
				changed = 1;
				break;
			case SDL_SCANCODE_LEFT:
				rotdegs--;
				changed = 1;
				break;
			}
			break;
		}
	}

	return 0;
}




#define STRBUF_SZ 4096

int main(int argc, char** argv)
{
	char error_str[STRBUF_SZ] = {0};

	if (argc != 3) {
		printf("usage:\n    %s image degree_to_rot\n", argv[0]);
		return 0;
	}

	rotdegs = atoi(argv[2]);

	int w, h, n, frames;

	u8* img = stbi_xload(argv[1], &w, &h, &n, 4, &frames);

	int dim = sqrt(w*w + h*h);

	u8* rotimg = malloc(dim*dim*4);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		snprintf(error_str, STRBUF_SZ, "Couldn't initialize SDL: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, NULL);
		puts(error_str);
		exit(1);
	}

	SDL_Window* win = SDL_CreateWindow("Image Rotation", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_RESIZABLE);
	if (!win) {
		snprintf(error_str, STRBUF_SZ, "Couldn't create window: %s; exiting.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_str, NULL);
		puts(error_str);
		exit(1);
	}

	SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!ren) {
		snprintf(error_str, STRBUF_SZ, "%s, falling back to software renderer.", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning: No HW Acceleration", error_str, win);
		puts(error_str);
		ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
	}


	int dim2 = dim/2;
	int h2old = h/2;
	int w2old = w/2;
	float rads;
	int x, y, xout, yout;


	SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, dim, dim);

	changed = 1;

	unsigned int old_time = 0, new_time=0, counter = 0;
	unsigned int last_frame = 0;
	float frame_time = 0;

	u32* outu32 = (u32*)rotimg;
	u32* inu32 = (u32*)img;

	u8* pixels;
	int pitch;

	while (1) {
		if (handle_events())
			break;

		new_time = SDL_GetTicks();
		frame_time = (new_time - last_frame)/1000.0f;
		last_frame = new_time;

		counter++;
		if (!(counter % 300)) {
			printf("%d  %f FPS\n", new_time-old_time, 300000/((float)(new_time-old_time)));
			old_time = new_time;
			counter = 0;
		}

		// No apparent speed benefit to using lock/unlock vs UpdateTexture
		SDL_LockTexture(tex, NULL, (void**)&outu32, &pitch);

		if (changed) {
			memset(outu32, 0, dim*dim*4);

			rads = rotdegs * (3.14159265f/180.0f);

			for (int i=0; i<dim; ++i) {
				y = i - dim2;
				for (int j=0; j<dim; ++j) {
					x = j - dim2;
					xout = x * cos(-rads) - y * sin(-rads) + w2old;
					yout = x * sin(-rads) + y * cos(-rads) + h2old;

					if (xout >= 0 && xout < w && yout >= 0 && yout < h) {
						//memcpy(&rotimg[(i*dim + j)*4], &img[(yout*w + xout)*4], 4);
						//

						outu32[i*dim + j] = inu32[yout*w + xout];
					}


				}
			}

			SDL_UnlockTexture(tex);
			//SDL_UpdateTexture(tex, NULL, rotimg, dim*4);
		}

		SDL_RenderClear(ren);
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);

		changed = 0;
	}


	//rotate(argv[1], deg);
	//rotate_scale_to_fit(argv[1], deg);



	return 0;
}
