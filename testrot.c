
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

//#define SDL_MAIN_HANDLED
//#include <SDL.h>



#define MAX(a, b)  ((a) > (b)) ? (a) : (b)
#define MIN(a, b)  ((a) < (b)) ? (a) : (b)

typedef uint8_t u8;
typedef uint32_t u32;
typedef int16_t i16;
typedef int32_t i32;






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


int main(int argc, char** argv)
{

	if (argc != 3) {
		printf("usage:\n    %s image degree_to_rot\n", argv[0]);
		return 0;
	}

	int deg = atoi(argv[2]);

	rotate(argv[1], deg);
	rotate_scale_to_fit(argv[1], deg);



	return 0;
}
