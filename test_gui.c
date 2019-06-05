
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

// TODO sin, cos, sqrt etc.
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
//#define NK_INCLUDE_FONT_BAKING
//#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl.h"



#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define STRBUF_SZ 1024

int n_imgs = 1;
SDL_Window* win;
SDL_Renderer* ren;
int running;
int slideshow = nk_false;
int show_about = nk_false;

float x_scale;
float y_scale;

char license[STRBUF_SZ*4] =
"The MIT License (MIT)\n"
"\n"
"Copyright (c) 2017-2019 Robert Winkler\n"
"\n"
"Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated\n"
"documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation\n"
"the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and\n"
"to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n"
"\n"
"The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n"
"\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED\n"
"TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL\n"
"THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF\n"
"CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS\n"
"IN THE SOFTWARE.";


void draw_gui(struct nk_context* ctx);
void draw_simple_gui(struct nk_context* ctx);

int main(void)
{
	struct nk_color background;
	struct nk_colorf bg;
	int win_width, win_height;
	struct nk_context *ctx;
	/* float bg[4]; */

	/* SDL setup */
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		printf( "Can't init SDL:  %s\n", SDL_GetError( ) );
		return 1;
	}

	running = 1;

	int win_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;

	win = SDL_CreateWindow("sdl_img gui",
	                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                       WINDOW_WIDTH, WINDOW_HEIGHT,
	                       win_flags);

	if (!win) {
		printf("Can't create window: %s\n", SDL_GetError());
		return 1;
	}
	/* try VSYNC and ACCELERATED */
	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
	if (!ren) {
		printf("Can't create ren: %s\n", SDL_GetError());
		return 1;
	}

	float hdpi, vdpi, ddpi;
	SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);
	printf("DPIs: %.2f %.2f %.2f\n", ddpi, hdpi, vdpi);

	SDL_Rect r;
	SDL_GetDisplayBounds(0, &r);
	printf("display bounds: %d %d %d %d\n", r.x, r.y, r.w, r.h);

	x_scale = hdpi/72;  // adjust for dpi, then go from 8pt font to 12pt
	y_scale = vdpi/72;

	SDL_RenderSetScale(ren, x_scale, y_scale);

	/*
	if (SDL_RenderSetLogicalSize(ren, WINDOW_WIDTH*x_scale, WINDOW_HEIGHT*y_scale)) {
		printf("logical size failure: %s\n", SDL_GetError());
		return 1;
	}
	*/

	if (!(ctx = nk_sdl_init(win, ren, x_scale, y_scale))) {
		printf("nk_sdl_init() failed!");
		return 1;
	}


	background = nk_rgb(28,48,62);
	bg = nk_color_cf(background);
	while (running)
	{
		SDL_RenderSetScale(ren, x_scale, y_scale);
		nk_sdl_scale(x_scale, y_scale);
		/* Input */
		SDL_Event evt;
		nk_input_begin(ctx);
		while (SDL_PollEvent(&evt)) {
			if (evt.type == SDL_QUIT)
				goto cleanup;

			if (evt.type == SDL_KEYUP && evt.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				if (show_about)
					show_about = nk_false;
				else if (slideshow)
					slideshow = 0;
				else
					goto cleanup;
			} else if (evt.type == SDL_WINDOWEVENT) {
				if (evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					//SDL_RenderSetLogicalSize(ren, evt.window.data1*x_scale, evt.window.data2*y_scale);
				}
			}
			nk_sdl_handle_event(&evt);
		}
		nk_input_end(ctx);

		/* GUI */
		draw_gui(ctx);
		draw_simple_gui(ctx);


		SDL_Delay(50);
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
		SDL_RenderSetClipRect(ren, NULL);
		SDL_RenderClear(ren);
		nk_sdl_render(NULL, 0);
		SDL_RenderPresent(ren);

	}

cleanup:
	nk_sdl_shutdown();
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}



void draw_simple_gui(struct nk_context* ctx)
{
	int gui_flags = NK_WINDOW_NO_SCROLLBAR; //NK_WINDOW_BORDER|NK_WINDOW_TITLE;

	int win_w, win_h;
	int scr_w, scr_h;
	int out_w, out_h;
	SDL_GetWindowSize(win, &win_w, &win_h);
	SDL_RenderGetLogicalSize(ren, &scr_w, &scr_h);
	SDL_GetRendererOutputSize(ren, &out_w, &out_h);

	float cur_x_scale, cur_y_scale;
	SDL_RenderGetScale(ren, &cur_x_scale, &cur_y_scale);
	//printf("scale = %.2f x %.2f\n", cur_x_scale, cur_y_scale);

	int fill = 0, slideshow = 0;

	if (nk_begin(ctx, "demo", nk_rect(100/cur_x_scale, 100/cur_y_scale, 200, 400), gui_flags)) {
		nk_layout_row_static(ctx, 0, 80, 2);
		if (nk_button_label(ctx, "hello")) {
			puts("down");
			x_scale -= 0.25;
			y_scale -= 0.25;
		}
		if (nk_button_label(ctx, "goodbye")) {
			puts("up");
			x_scale += 0.25;
			y_scale += 0.25;
		}
		nk_checkbox_label(ctx, "Best Fit", &fill);
		nk_checkbox_label(ctx, "Slideshow", &slideshow);
	}
	nk_end(ctx);

	/*
	if (nk_begin(ctx, "demo2", nk_rect(400/cur_x_scale, 100/cur_y_scale, 200, 400), gui_flags)) {
		nk_layout_row_static(ctx, 0, 80, 2);
		if (nk_button_label(ctx, "hello")) {
			;
		}
		if (nk_button_label(ctx, "goodbye")) {
			;
		}
		nk_checkbox_label(ctx, "Best Fit", &fill);
		nk_checkbox_label(ctx, "Slideshow", &slideshow);
	}
	nk_end(ctx);
	*/

}



void draw_gui(struct nk_context* ctx)
{
	char info_buf[STRBUF_SZ];
	int len;
	int gui_flags = NK_WINDOW_NO_SCROLLBAR; //NK_WINDOW_BORDER|NK_WINDOW_TITLE;
	SDL_Event event = { 0 };

	int win_w, win_h;
	int scr_w, scr_h;
	int out_w, out_h;
	SDL_GetWindowSize(win, &win_w, &win_h);
	//SDL_RenderGetLogicalSize(ren, &scr_w, &scr_h);
	SDL_GetRendererOutputSize(ren, &out_w, &out_h);

	float scale_x, scale_y;
	SDL_RenderGetScale(ren, &scale_x, &scale_y);
	//scale_y = 2;
	printf("scale = %.2f x %.2f\n", scale_x, scale_y);

	scr_w = out_w/scale_x;
	scr_h = out_h/scale_y;

	printf("win %d x %d\nlog %d x %d\nout %d x %d\n", win_w, win_h, scr_w, scr_h, out_w, out_h);

	struct nk_rect bounds;
	const struct nk_input* in = &ctx->input;

	static int fill = nk_false;
	static int hovering = 0;
	static int what_hover = 0;

	int total_width = 0;
	int do_zoom = 0, do_toggles = 0, do_rotates = 0;
	what_hover = 0;
	if (nk_begin(ctx, "Menu", nk_rect(0, 0, scr_w, 30), gui_flags)) {

		//g->gui_rect = nk_window_get_bounds(ctx);
		//printf("gui %f %f %f %f\n", g->gui_rect.x, g->gui_rect.y, g->gui_rect.w, g->gui_rect.h);

		//nk_layout_row_static(ctx, 0, 80, 16);
		//nk_layout_row_dynamic(ctx, 0, 16);
		
		nk_layout_row_template_begin(ctx, 0);

		// menu
		nk_layout_row_template_push_static(ctx, 50);

		// prev next
		nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 80);

		total_width += 50 + 80 + 80;

		// zoom, -, +
		//nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);

		total_width += 80;
		if (total_width < scr_w)
			do_zoom = 1;

		// best fit, slideshow, and actual size
		nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 80);

		total_width += 240;
		if (total_width < scr_w)
			do_toggles = 1;

		// Rotate left and right
		nk_layout_row_template_push_static(ctx, 90);
		nk_layout_row_template_push_static(ctx, 90);
		total_width += 180;
		if (total_width < scr_w)
			do_rotates = 1;

		// Mode 1 2 4 8
		/*
		nk_layout_row_template_push_static(ctx, 80);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);
		nk_layout_row_template_push_static(ctx, 40);
		*/
		nk_layout_row_template_end(ctx);

		if (nk_menu_begin_label(ctx, "MENU", NK_TEXT_LEFT, nk_vec2(200, 400))) {
			nk_layout_row_dynamic(ctx, 0, 1);
			if (nk_menu_item_label(ctx, "About", NK_TEXT_LEFT)) {
				show_about = nk_true;
			}

			nk_label(ctx, "Image Actions", NK_TEXT_LEFT);
				nk_menu_item_label(ctx, "Delete", NK_TEXT_RIGHT);
				nk_menu_item_label(ctx, "Rotate Left", NK_TEXT_RIGHT);
				nk_menu_item_label(ctx, "Rotate Right", NK_TEXT_RIGHT);

			nk_label(ctx, "Viewing Mode", NK_TEXT_LEFT);
				nk_menu_item_label(ctx, "1 image", NK_TEXT_RIGHT);
				nk_menu_item_label(ctx, "2 images", NK_TEXT_RIGHT);
				nk_menu_item_label(ctx, "4 images", NK_TEXT_RIGHT);
				nk_menu_item_label(ctx, "8 images", NK_TEXT_RIGHT);

			if (nk_menu_item_label(ctx, "Exit", NK_TEXT_LEFT)) {
				running = 0;
			}

			nk_menu_end(ctx);
		}

		nk_button_set_behavior(ctx, NK_BUTTON_REPEATER);
		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_LEFT, "prev", NK_TEXT_RIGHT)) {
			;
		}
		if (nk_button_symbol_label(ctx, NK_SYMBOL_TRIANGLE_RIGHT, "next", NK_TEXT_LEFT)) {
			;
		}

		if (!do_zoom)
			goto end_main_gui;

		//nk_label(ctx, "zoom:", NK_TEXT_RIGHT);
		bounds = nk_widget_bounds(ctx);
		if (nk_input_is_mouse_hovering_rect(in, bounds)) {
			hovering++;
			if (hovering > 20)
				nk_tooltip(ctx, "Shrink the image");
			what_hover = 1;
		}
		if (nk_button_symbol(ctx, NK_SYMBOL_MINUS)) {
			;
		}
		bounds = nk_widget_bounds(ctx);
		if (nk_input_is_mouse_hovering_rect(in, bounds)) {
			hovering++;
			if (hovering > 20)
				nk_tooltip(ctx, "Enlarge the image");
			what_hover = 2;
		}
		if (nk_button_symbol(ctx, NK_SYMBOL_PLUS)) {
			;
		}
		nk_button_set_behavior(ctx, NK_BUTTON_DEFAULT);

		if (!do_toggles)
			goto end_main_gui;

		//nk_select_symbol_label(ctx, NK_SYMBOL_RECT_OUTLINE, "Best fit", NK_TEXT_RIGHT, fill);
		//nk_selectable_symbol_label(ctx, NK_SYMBOL_RECT_OUTLINE, "Best fit", NK_TEXT_RIGHT, &fill);
		bounds = nk_widget_bounds(ctx);
		if (nk_input_is_mouse_hovering_rect(in, bounds)) {
			hovering++;
			if (hovering > 20)
				nk_tooltip(ctx, "Toggle fit image to window");
			what_hover = 3;
		}
		nk_selectable_label(ctx, "Best fit", NK_TEXT_RIGHT, &fill);
		bounds = nk_widget_bounds(ctx);
		if (nk_input_is_mouse_hovering_rect(in, bounds)) {
			hovering++;
			if (hovering > 20)
				nk_tooltip(ctx, "Toggle start a slideshow");
			what_hover = 4;
		}
		nk_selectable_label(ctx, "Slideshow", NK_TEXT_RIGHT, &slideshow);


		if (nk_button_label(ctx, "Actual")) {
			;
		}

		if (!do_rotates)
			goto end_main_gui;

		if (nk_button_label(ctx, "Rot Left")) {
			;
		}
		if (nk_button_label(ctx, "Rot Right")) {
			;
		}

		/*
		nk_label(ctx, "mode:", NK_TEXT_RIGHT);
		if (nk_button_label(ctx, "1")) {
			n_imgs = 1;
		}
		if (nk_button_label(ctx, "2")) {
			n_imgs = 2;
		}
		if (nk_button_label(ctx, "4")) {
			n_imgs = 4;
		}
		if (nk_button_label(ctx, "8")) {
			n_imgs = 8;
		}
		*/

end_main_gui:
		// still don't know if this has to be inside the if or just
		// before the nk_end()
		if (show_about)
		{
			static int license_len;;
			license_len = strlen(license);
			int w = 400/scale_x, h = 400/scale_y;
			struct nk_rect s;
			s.x = scr_w/2-w/2;
			s.y = scr_h/2-h/2;
			s.w = w;
			s.h = h;
			if (nk_popup_begin(ctx, NK_POPUP_STATIC, "About sdl_img", NK_WINDOW_CLOSABLE, s))
			{
				nk_layout_row_dynamic(ctx, 20/scale_y, 1);
				nk_label(ctx, "sdl_img 1.0", NK_TEXT_CENTERED);
				nk_label(ctx, "By Robert Winkler", NK_TEXT_LEFT);
				nk_label(ctx, "robertwinkler.com", NK_TEXT_LEFT);  //TODO project website
				nk_label(ctx, "sdl_img is licensed under the MIT License.",  NK_TEXT_LEFT);

				// credits
				// Sean T Barret (sp?) single header libraries
				// stb_image, stb_image_write
				//
				// nuklear (which also uses stb libs)
				//
				// My own cvector lib

				//nk_layout_row_dynamic(ctx, 200, 1);
				//nk_label_wrap(ctx, license);
				//nk_edit_string(ctx, NK_EDIT_BOX, license, &license_len, 1024, nk_filter_default);
				nk_popup_end(ctx);
			} else show_about = nk_false;
		}
	}

	nk_end(ctx);



	if (nk_begin(ctx, "Controls", nk_rect(0, scr_h-30, scr_w, 30), gui_flags))
	{
		if (n_imgs == 1) {
			len = snprintf(info_buf, STRBUF_SZ, "1000 x 600 pixels   55 %%");
			if (len >= STRBUF_SZ) {
				puts("info path too long");
				exit(1);
			}
			nk_layout_row_static(ctx, 0, scr_w, 1);
			nk_label(ctx, info_buf, NK_TEXT_LEFT);
		}
	}
	nk_end(ctx);

	if (!what_hover)
		hovering = 0;
}




